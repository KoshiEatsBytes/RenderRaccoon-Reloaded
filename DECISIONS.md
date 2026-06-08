# RenderRaccoon-Reloaded — Architecture Decisions

A living record of the engine's key design decisions: *what* was chosen, *why*, and the
gotchas/limitations attached to each. Intended as a reference for future work. 
The code is the source of truth; this captures the reasoning the code can't.

---

## 1. Layering overview

```
Engine (singleton)            platform + subsystems + main loop
  └─ ApplicationManager       scene lifecycle + cross-scene SubSystems  (the "middle layer")
       └─ Scene (abstract)    one self-contained level: GameObjects + its OWN physics world
            └─ GameObject     transform + components + children
                 └─ Component behaviour
```

- **Engine** owns process-global things only: window, GL context, input, renderer, timing,
  and the `ApplicationManager`. It is **game-agnostic** — it never references concrete game
  types. The application chooses its first scene in `main.cpp`
  (`engine.GetAppManager().RequestSceneLoad<Game>()`), not the engine.
- **ApplicationManager** is the middle layer: it owns the active scene, the deferred scene-load
  request, and the `ISubSystem` list. Scene swaps and cross-scene systems live here.
- **Scene** is where a level lives, fully self-contained (its objects *and* its physics world).

---

## 2. Scene system

- `Scene` is an **abstract base**; games subclass it and override `Init/PreUpdate/Update/
  LateUpdate/Destroy`. Container logic (object iteration, queues) lives in the private
  `*Internal` methods the `ApplicationManager` drives; the virtual hooks are for game code.
- **Deferred spawn/destroy queues.** Structural changes never happen mid-iteration. `MarkForDestroy`
  and runtime `CreateObject` enqueue; the queues flush once per frame in `LateUpdateInternal`.
  - `ProcessDestroyQueue` computes **destruction roots first** (objects with no queued ancestor),
    then erases only those — so cascading a parent never frees a sibling entry mid-loop (avoids UAF).
  - Both queues drain via `std::exchange(queue, {})` so re-entrant spawns/destroys during the
    flush land in a fresh queue and process next frame (no iterator invalidation).
- **`m_sceneStarted`** gates creation: during `Init` (false) objects are created immediately;
  at runtime (true) they're deferred. **Deferred `CreateObject` sets `obj->m_scene` immediately**
  (before enqueue) so that `AddComponent` — which runs a component's `Init` synchronously — can
  reach the scene (e.g. `PhysicsComponent` needs `m_owner->m_scene->GetPhysicsManager()`).
  *Caveat:* the parent isn't wired until flush, so a runtime spawn **with a parent** computes
  `GetWorldPosition()` from local space — a parented runtime physics body lands at the wrong
  spot. Root-level runtime spawns are correct.
- **Scene switching** is deferred to the end of `LateUpdate` (`LoadPendingScene`): the new scene
  is installed and `OnLoad`'d, then the old one is `OnDestroy`'d (rollback-on-failure supported).

---

## 3. ApplicationManager & SubSystems

- Owns `unique_ptr<Scene> m_activeScene` and a `std::function<unique_ptr<Scene>()> m_pendingScene`
  (a deferred scene factory). `RequestSceneLoad<T>()` just stores the lambda; the swap happens at
  the frame boundary.
- **SubSystems** (`ISubSystem`) are persistent, cross-scene systems (e.g. an EventManager). They
  must be added **before** `Engine::Init` (which sets `m_pastInitialization = true` and locks the
  list). They're retrieved by type via the custom RTTI (see §5).
- `ISubSystem` lifecycle hooks (`Init/PreUpdate/Update/LateUpdate/Destroy`) have **default empty
  bodies** (not pure virtual), so a subsystem overrides only what it needs; the base stays
  non-instantiable via a protected constructor.

---

## 4. GameObject & Components

- Components are owned by `unique_ptr` in `GameObject::m_components`; destroyed automatically with
  the object. Sorted by `GetExecutionOrder()` on add (PhysicsComponent runs first at -100).
- **`AddComponent` is only safe before the first tick** (`m_ticked` guard) — adding a component at
  runtime would `Init` it immediately and can reallocate the component vector mid-iteration.
- **Enable/disable:** `Component::m_enabled` + the update loops skip disabled components.
  `SetEnabled` is non-virtual and does **change-detection**, dispatching to virtual `OnEnable/
  OnDisable` hooks (so derived classes can't forget the "only fire on transition" guard).
  `PhysicsComponent`/`KinematicCharacterController` override these to remove/re-add their body or
  ghost from the world when toggled.
- **Find API:** `FindObjectByName/Type` + `FindObjectsByName/Type`, both **self-inclusive**
  ("find this object or a descendant"), with a `searchDescendants` flag. Plural versions delegate
  to a private `(vector&)` accumulator helper.

---

## 5. Custom RTTI (`TypeInfo`)

- **Pointer-identity type system** replacing `dynamic_cast` for GameObject/SubSystem queries.
  Each class has a `static TypeInfo` whose address is its unique id, linked to its base's
  `TypeInfo`. `IsA` walks the base chain comparing pointers — O(depth) with a tiny constant,
  much cheaper than RTTI.
- Wired via the `GAMEOBJECT(Class, Base)` / `SUBSYSTEM(Class, Base)` macros (provide `StaticType`
  + a virtual `GetType` override). **Discipline cost:** every subclass *must* use the macro or it
  silently reports its base's type (the compiler can't catch the omission, because the base must
  stay instantiable so `GetType` can't be pure-virtual).
- **Single-inheritance only** — the `static_cast` after `IsA` is valid only for a linear hierarchy
  from `GameObject`/`ISubSystem`. No multiple inheritance / cross-casts.
- **Components deliberately use a different scheme:** exact `StaticTypeID<T>()` integer match (no
  is-a). Their hierarchy is flat and lookups are hot-path (the physics walker), so exact-match is
  both correct and faster. GameObjects use is-a because gameplay queries want "this type and its
  subclasses."

---

## 6. Physics — architecture (per-scene worlds)

- **Each `Scene` owns its own `PhysicsManager`** (a Bullet world wrapper), declared as a
  `unique_ptr<PhysicsManager>` **before `m_objects`**. This is the load-bearing invariant:
  reverse-order member destruction means GameObjects (and their bodies) die **first**, the world
  **last** — so `removeRigidBody` always runs against a live world. Get this order wrong and every
  scene unload is a use-after-free.
- Going per-scene (vs one global world) is what makes scene switching correct: unloading a scene
  destroys its world *and* its step-callbacks together, so the character-controller callback dangle
  that plagued the global-world design simply can't happen (the callbacks die with the world that
  holds them, and that world is never stepped again).
- The active scene's world is stepped between `PreUpdate` and `Update`
  (`Engine::Launch → ApplicationManager → Scene::UpdatePhysicsInternal → PhysicsManager::Update`).
- **Fixed timestep** `1/60` with an accumulator; `maxStepsPerFrame = 4` caps the catch-up so a
  slow frame degrades to slow-motion instead of a spiral of death. Visual smoothness comes from
  recording pre/post-step positions and interpolating by `GetInterpolationAlpha()`.

---

## 7. Physics — RigidBody & body lifecycle

- **`RigidBody` holds a `PhysicsManager&`** captured at construction = the body's *own* scene's
  world. It removes itself from **that** world in its destructor. This fixed the bug where removal
  went through the Engine's "active scene" PM — wrong during a scene swap, when the active scene is
  already the *new* one while old bodies are being torn down.
- Add/remove go through `m_owner->m_scene->GetPhysicsManager()` in `PhysicsComponent`
  (Init / Rebuild / OnEnable / OnDisable). The `IsAddedToWorld` flag guards against double-remove.
- `GetRigidBody()` returns a `weak_ptr` (not `shared_ptr`) so external code can't extend a body's
  lifetime past its scene — which would dangle the `PhysicsManager&`.
- The `btDefaultMotionState`, compound shape, and colliders are owned by the `RigidBody`
  (`unique_ptr`/`vector<shared_ptr<Collider>>`), declared so the body dies before the shapes it
  references.

---

## 8. Physics — KinematicCharacterController

- Ghost-object based (`btPairCachingGhostObject` + `btKinematicCharacterController`), driven by a
  `PhysicsManager&` passed in (its scene's world). Not a RigidBody.
- **Movement uses `setWalkDirection(velocity * FixedTimeStep)`**, *not*
  `setVelocityForTimeInterval`. The latter's internal early-out skips gravity when the interval
  lapses, which froze the character mid-air when keys were released. `setWalkDirection` keeps
  gravity/stepDown running every step; the fixed timestep makes it frame-rate independent.
- **Step callbacks** record ghost position pre/post step for interpolation. Registered in the
  **constructor** (so `Resize` doesn't re-subscribe) and **unregistered in the destructor** via a
  **handle system** — `std::function` has no `operator==`, so registration returns a `size_t`
  handle and unregister erases by handle. This closes the mid-game single-object-destroy dangle.
- Walk direction is **yaw-only** (FPS-style) so looking up doesn't walk into the sky.
- The owning camera GO is tagged `PhysicsOwnership::CHARACTER`; the controller drives the GO
  transform via the bypass setters.

---

## 9. Physics — PhysicsOwnership guards

- `GameObject::m_physicsOwnership` (NONE / DYNAMIC / KINEMATIC / STATIC / CHARACTER / INHERITED)
  records who owns the object's transform. Public `SetPosition/Rotation/Scale` check it and
  **warn + discard** writes that would silently de-sync from physics, pointing the caller at the
  right API (`PhysicsComponent::Teleport`, `PlayerControllerComponent::SetLookRotation`, etc.).
- Physics/engine code writes through **bypass `*Internal` setters** that skip the guard and do the
  world→local conversion.
- **`INHERITED`** is set on every descendant baked into a parent's compound at Init (the walker
  tags them) and cleared on `Rebuild`/destroy. So an object *inside* a physics body's hierarchy is
  protected the same way the root is. The check is O(1) via a single helper.

---

## 10. Physics — compound colliders & scale

- A `PhysicsComponent` assembles a **`btCompoundShape`** by walking its GameObject's subtree at
  Init, collecting every `ColliderComponent` (multiple per GO supported) at its accumulated local
  transform. **Stops at nested `PhysicsComponent`s** (sub-bodies are independent).
- The shape layout is **baked at Init** and frozen. Changing the hierarchy/transforms afterward
  needs `PhysicsComponent::Rebuild()` (expensive: destroy + rebuild + re-add).
- **World scale** is baked into each child shape via `setLocalScaling` (the walker seeds with the
  root's `GetWorldLossyScale()`). Shear (non-uniform scale + rotation) is rejected via a skew check
  in the matrix decompose. `GameObject::SetScale` is guarded for physics-owned objects; use
  `PhysicsComponent::SetScale` (which rebuilds).

---

## 11. Physics — multithreading

- Bullet built with `BT_THREADSAFE` (+ OpenMP define, though the **native "ThreadSupport"
  scheduler** is what actually got picked — fine, no OpenMP runtime dependency). Enabled via
  `target_compile_definitions(... PUBLIC)` on `LinearMath`/`BulletCollision`/`BulletDynamics` so
  the defines stay ABI-consistent between Bullet and the engine.
- One **global task scheduler**, created + `btSetTaskScheduler`'d once in `Engine::Init`
  (the per-scene `Mt` worlds all share it; only one scene steps at a time, so no contention).
- Each `PhysicsManager` builds the `Mt` stack: `btCollisionDispatcherMt`,
  `btConstraintSolverPoolMt(BT_MAX_THREAD_COUNT)`, `btSequentialImpulseConstraintSolverMt`,
  `btDiscreteDynamicsWorldMt`. Pre/post-step callbacks run on the **main thread** (outside Bullet's
  parallel regions) so they stay safe.

---

## 12. Audio

- **Library: miniaudio** (single-header). Picked over OpenAL Soft / SoLoud / FMOD for: zero MinGW
  build friction (single header, like `cgltf`/`stb_image`), permissive license, built-in WAV/FLAC/MP3
  decoders, and — decisive for an internals-focused project — a dual high-level/low-level API (the
  mixer can be profiled or replaced later). Format support is identical whether you use the engine or
  decode by hand (same decoders).
- **Build:** header-only, so *not* an `add_subdirectory` library. `#define MINIAUDIO_IMPLEMENTATION`
  lives in **exactly one** TU, wrapped in a small `STATIC` `miniaudio` target that also carries the
  platform link deps (`winmm`/`ole32`; Linux `dl`/`pthread`/`m`; macOS Core Audio frameworks).
  Windows needs none by default (backends are runtime-loaded) — they're insurance / for
  `MA_NO_RUNTIME_LINKING`.
  - *Gotcha — one target only:* listing the impl `.c` in both the `miniaudio` lib *and*
    `PROJECT_SOURCE_FILES` double-compiles it and risks duplicate symbols.
  - *Gotcha — forward-declaring miniaudio types in `Types.h`:* `ma_engine`/`ma_sound`/`ma_decoder`
    are named-tag structs → `struct X;` works. `ma_result` (an **enum**) and `ma_audio_buffer`/`_ref`
    (**anonymous** typedefs) **cannot** be `struct`-forward-declared ("using typedef-name after
    struct") — anonymous ones are wrapped in our own named struct (`AudioBufferRef`) to keep
    `miniaudio.h` out of engine headers. `ma_sound_group` *is* `ma_sound` (`maSoundGroup = ma_sound`).
- **Placement:** `AudioManager` is **Engine-owned** (like `RenderQueue`/`InputManager`), reached via
  `Engine::GetInstance().GetAudioManager()` — *not* a SubSystem. Audio is core infrastructure, not a
  game opt-in. It's ticked from the Engine loop (its `Update` reaps finished voices); an Engine-owned
  manager can still be driven per-frame, so "needs an Update" didn't force the SubSystem path.

### Clip flyweight vs voices (the core split)
- **`AudioClip` is the flyweight** — immutable decoded PCM (`vector<float>` + channels/rate), loaded
  **once** and shared. A **voice** (one `ma_sound`) is per-playback. Fusing them (the original `Audio`
  class) was the root mistake: a `ma_sound` is a *single* voice, so a shared object couldn't overlap
  with itself or be played by two emitters. The split is what enables polyphony and sharing.
- Decode once to PCM at the **engine sample rate** but **native channel count** (mono stays mono —
  miniaudio only spatializes mono sources). Each voice wraps its **own** `ma_audio_buffer_ref`
  (independent read cursor) over the clip's shared PCM — you **cannot** share a `ma_decoder`/data
  source across simultaneous voices (single cursor).
- Cache is `unordered_map<key, weak_ptr<AudioClip>>` → clips free when the last referencing voice
  dies (≈ per-scene lifetime for free; no per-scene cache needed). Decoding is lazy (first `GetClip`);
  the manifest scan only catalogs.
- **Voices:** `AudioVoice` (base: play/stop/pause/resume, fades, volume/pitch, group routing,
  `IsFinished`/`IsPaused`) → `SpatialAudio` (3D + **mono gate**: a non-mono clip warns and degrades to
  2D) and `StaticAudio` (2D + pan). miniaudio handles are pimpl'd so headers stay miniaudio-free; the
  dtor **manually `*_uninit`s** in dependency order (sound → buffer_ref → clip) because the mixer
  reads the buffer on its own thread — a `unique_ptr` only frees memory, it doesn't `uninit`.

### Asset registry (keys, not paths)
- Audio is addressed by **stable camelCase keys**, built at `Init` by scanning `Assets/Audio`
  (`FileSystem::ListAssetFiles`, the reusable enumeration primitive) into a `key → path` manifest;
  `GetClip(key)` resolves + lazily decodes + caches.
- The key is the **full relative path under `Audio/`, camelCased** (`Audio/Gun/shoot.wav` →
  `gunShoot`; split on `/ _ - space`). Using the *full* path (not just the filename) makes collisions
  nearly impossible by construction — so the original "append 1/2/3 on duplicate" idea was dropped:
  it was scan-order-dependent → **unstable keys**. Genuine collisions (same name+folder, different
  extension) **warn** instead.

### Channels = `ma_sound_group` buses
- A **channel is a `ma_sound_group`**, not a hand-rolled mixer. Channel volume/pitch/pan are **group**
  properties the mixer applies (`ma_sound_group_set_*`); per-voice values stay independent. The
  earlier hand-rolled version multiplied `track × channel` and stored the result on the voice — which
  was **lossy** (changing channel volume clobbered per-track volumes) and broke the menu-slider use
  case. Groups fix it for free and enable master/music/SFX volume buses.
- Voices join a channel via **`AudioVoice::AttachToGroup`** (`ma_node_attach_output_bus`, runtime
  re-routing) — so voice creation stays group-agnostic and "bind" just assigns the bus. An
  **un-grouped voice routes to the engine endpoint** → affected by master volume only.
- **Channels are plain `uInt` indices**, created up front by `Init(channelCount, fallbackChannel)` — the
  *game* supplies its own `enum : int` mapping, so the manager carries no hard-coded channel set (the
  earlier in-manager `AudioChannelID` enum was dropped for exactly this reason). A `key → channel`
  **binding** (`m_keyToChannel`, set **explicitly** by `BindTrack`) lets the by-key `Play`/`Stop`/`Get*`
  calls resolve a channel without one being passed every time; unbound keys fall back to
  `fallbackChannel`. Voice *creation* never writes the binding — `CreateSpatial` takes its channel as an
  argument and deliberately leaves the global map alone, so two emitters of the same key on different
  channels can't stomp each other. Indices are 0-based — valid is `< channelCount`.
- A channel exposes **bus controls + collection lifecycle** (add / stop / reap) but **no per-track
  tuning** — individual-voice control is the Tracker's job (reaching into one sound "through" a bus is
  out of place for what a bus does).
- **Reap** (per-frame `Update`): **only one-shots**, on `IsFinished()`. Managed/prototype tracks are
  **never reaped** — they persist for the channel's lifetime (see *Voice lifecycle*) so their settings
  stick and they can be replayed or cloned without rebuilding. Note `ma_sound_is_playing` reports node
  *state* (started/stopped), **not** "reached the end" — a finished one-shot is still "started", which
  is why reaping keys off `ma_sound_at_end` (`IsFinished`).

### Tracker — non-owning voice handle
- **`Tracker<T>`** (`T` = `StaticAudio`/`SpatialAudio`) is a copyable, lifecycle-neutral handle for
  controlling a specific voice without the channel proxying every method. Holds `weak_ptr<T>` + a
  `std::function` **revive callback** + a sticky-config cache.
- **Two access tiers.** The tracker's own methods are *smart*: `.Play()` **revives** a reaped voice
  via the callback (rebuild from the cached clip → re-add to the channel; flyweight intact), sticky
  config (volume/pitch/pan) is **cached and replayed on revive**, and all no-op safely if the voice
  is gone. **`operator->` goes straight to the raw voice** for the long tail (`SetCone`,
  `SetMinMaxDistance`…) — the fast path, assumes alive (guard with `if (tracker)`).
- **Type-safe surface** via C++20 `requires`: `SetPan` exists only on `Tracker<StaticAudio>`,
  `SetPosition`/etc. only on `Tracker<SpatialAudio>`. The getter's *context* fixes `T` (manager
  getters → static, component getters → spatial), so `auto track = ...` is never ambiguous.
- **Ownership:** the channel owns the voice (`shared_ptr<AudioVoice>`), the Tracker observes
  (`weak_ptr<T>`) — both from one `make_shared<Derived>` so they share a control block. Since prototypes
  now persist (only one-shots reap), the `weak_ptr` normally stays valid for the channel's lifetime, so
  the revive callback is a safety net rather than the common path. A Tracker never extends a voice's
  lifetime.
- **Two convenience wrappers** sit on `Tracker<T>`, mirroring each other: **`ManagerAudioTracker`**
  (wraps `Tracker<StaticAudio>` + the `AudioManager`) and **`ComponentAudioTracker`** (wraps
  `Tracker<SpatialAudio>` + its owning `AudioSourceComponent`). Both add **`PlayOneShot()`** (delegates
  to the owner, which clones the cached voice) and **`operator->`** (raw voice access for config), so a
  caller can **bind once and fire/configure without reaching back to the singleton/component** each time.
  `AudioManager::GetStatic(key)` returns a `ManagerAudioTracker`; `AudioSourceComponent::BindTrack` /
  `GetTrack` return a `ComponentAudioTracker`.

### Voice lifecycle — cached prototypes & cloned one-shots
- The **manager mirrors `AudioSourceComponent`**: a key's voice is **created once and cached**
  (`GetOrCreateVoice` → it lives in its channel's `m_tracks`) and **never reaped**. That cached voice is
  the key's **prototype** — the single place its volume/pitch/pan live, so settings applied through a
  handle *stick* across plays.
- **One-shots clone the prototype.** `PlayOneShot(key)` get-or-creates the prototype, builds a fresh
  voice (`CreateVoiceShared`), copies the prototype's settings onto it (`StaticAudio::CloneSettings`),
  routes it into the bus, plays it non-looping, and the channel reaps **the clone** at end. This resolves
  the earlier "ephemeral vs keyed" tension: the **prototype is keyed-and-reused** (settings stay
  consistent and tweakable) while the **clones are ephemeral and overlapping** (polyphony — rapid
  gunfire/footsteps still layer). `PlayOneShot`'s `_vol` is a **scale** on the prototype's volume
  (default `1.0` = inherit unchanged), so a per-shot override never clobbers the sticky setting.
- **Managed / music** playback drives the **prototype directly** (`PlayManaged`, or
  `GetStatic(key)->Play`) — keyed, single-instance, controllable (stop/fade; `CrossFade` = two concurrent
  fades; multiple concurrent keys → layering). Re-playing a key reuses the same cached voice instead of
  orphaning it.
- Because the prototype must exist before settings can stick, **`GetTrack`/`GetStatic` create eagerly**
  (get-or-create) so `GetStatic(key)->SetVolume(..)` always has a live voice. *Cost:* one cached
  `ma_sound` per distinct key per channel, alive for the channel's lifetime — **bounded by the sound
  set** (like the component's `m_voices`), not a leak.

### 2D vs 3D
- **2D (`StaticAudio`)** voices are **manager-owned** — the cached prototypes above, living in channels.
- **3D (`SpatialAudio`)** voices are **component-owned**, because the same key can play from many emitters
  at once (three torches, all `torchLoop`) and each voice must die with its **entity** — neither fits the
  manager's keyed/owned model. The manager only offers a **factory**, `CreateSpatial(key, channel)`, which
  builds the voice, routes it into the channel's group (volume bus), and returns the *owning* `shared_ptr`
  (channel passed explicitly; it does **not** touch the global binding). `AudioSourceComponent` owns those
  voices (keyed by clip), drives position/direction/velocity each `LateUpdate`, reaps **only** its finished
  one-shot clones, and hands out a `ComponentAudioTracker`. Its `PlayOneShot` mirrors the manager's:
  **clone the cached voice** (`SpatialAudio::CloneSettings`), position the clone at the emitter's **world**
  position, play, reap at end. A per-source **3D profile** (min/max distance, rolloff, attenuation model,
  doppler factor, cone) is stamped onto each voice at creation and re-applied when changed; per-clip
  exceptions go through `operator->`.
- **Listener** is global/singular, driven by an `AudioListenerComponent` on the camera — it pushes
  position + direction + world-up **+ velocity** each frame (see Velocity & Doppler).

### Scene unload (`UnloadAll`)
- Component-owned 3D voices die with their entities on scene unload, but **manager-owned 2D voices and
  decoded clips would otherwise persist across scenes.** `AudioManager::UnloadAll()` flushes them:
  `AudioChannel::Clear()` every bus (destroy voices, **keep the group**) + clear the clip cache (PCM frees
  once the removed voices release it). The **asset registry (`key → path`) and channel bindings are kept**
  — they're scene-independent (paths and routing, not loaded data), and rebuilding the registry would mean
  a filesystem rescan.
- Driven from `ApplicationManager` at both teardown points: `UnloadCurrentScene` (explicit + shutdown via
  `Destroy`) and `LoadPendingScene` — in the latter **before** the incoming scene's `OnLoad`, so it can't
  wipe sounds the new scene just set up. *Trade-offs:* on the rare scene-load **failure + rollback** the
  old scene's 2D music has already been flushed (its 3D component voices are untouched); and mid-transition
  a clip still held by the outgoing scene's not-yet-destroyed voices may briefly re-decode. Both are
  transient and self-correcting.

### Velocity & Doppler
- Doppler is **relative**, so it needs velocity on *both* the source and the listener — miss the
  listener half and a moving player won't Doppler world sounds. Both sides push velocity each frame;
  `dopplerFactor` (per voice, default 1) scales the effect.
- **Velocity is finite-differenced from world position** (`(pos − lastPos)/dt`, with a teleport guard
  and a smoothing lerp), *not* read from the physics body — and that's the *more* correct choice, not a
  shortcut. A rigid body's `GetLinearVelocity()` is its **center-of-mass** velocity, but a point on the
  body moves at `v_com + ω × r`. Our composite bodies bake child colliders at an offset, so an audio
  source on a *child* of a rotating body (a speaker on a spinning platform) has real tangential
  velocity that `GetLinearVelocity()` misses entirely (it reports ~0 for a platform spinning in place).
  Finite-differencing the child's world position captures exactly that — the same value Bullet's
  `getVelocityInLocalPoint` would give, with no physics coupling — and it matches the
  *interpolated/rendered* transform, so Doppler tracks what's on screen.

---

## 13. Build notes

- **Per-scene physics**, not a global Engine `PhysicsManager`. Components reach physics via their
  scene.
- Asset copy uses an **absolute source path** (`${CMAKE_SOURCE_DIR}/Assets/`), decoupled from the
  runtime `ASSETS_ROOT` (which is relative in Release) — earlier the two were conflated and broke
  the Release build.
- **`glfwSwapInterval(0)`** disables vsync — required for meaningful FPS benchmarking.
- Always benchmark **Release** (`-O2`): Debug Bullet is ~20–40× slower (un-inlined math).

---

## 14. Benchmark findings (1000 dynamic boxes, Ryzen 7 7800X3D)

- **Debug → Release single-threaded:** ~20–40× (the earlier "5–7 FPS" was a Debug artifact; real
  single-thread baseline is ~6 ms/frame).
- **Multithreading speedup:** ~3.3×, **plateauing at 8 physical cores** — SMT (16 threads) gives
  nothing and is marginally worse. Bandwidth/serial-fraction bound.
- **Island count doesn't matter** (1 vs 4 vs 9 vs 16 pits ≈ flat). Bullet's `Mt` solver
  parallelizes *within* islands (contact graph-coloring), so a single pile was already parallel —
  the multi-pit hypothesis was refuted, and explained.
- **Frame-time benchmarks of a subsystem are contaminated.** At 8 threads physics is ~0.8 ms but
  unculled rendering of 1000 boxes is a fixed ~1 ms floor that *masks* physics differences. To
  benchmark physics, instrument `stepSimulation` directly (`GetLastStepMs` — TODO).

---

## 15. Known limitations & future work

- **No frustum culling** — every object draws every frame (the next obvious optimization; the
  benchmark scenes already demonstrate the cost).
- **`GetLastStepMs` physics-isolation timing** — to be added for clean physics benchmarks.
- **Mesh colliders** (triangle-mesh / convex hull from `Mesh` CPU data) — designed, deferred.
- **No collision events / triggers / raycasts** yet.
- **GLTF animation** — being dropped; physics scale-animation interaction is therefore moot.
- **Runtime parented physics spawn** lands at the wrong position (parent not wired until flush).
- **Component removal at runtime** not implemented (enable/disable covers most needs).
- **Reparenting** a physics-hierarchy GameObject has stale-ownership edge cases (documented, not
  guarded).
- **MT determinism** — multithreaded solving isn't bit-exact run-to-run (acceptable for gameplay).
- **Audio — no voice pool / voice limiting** yet (the optimization to reach for if one-shot *clone*
  spawning ever profiles hot — pool/reuse the clone voices; the per-key prototype already stays single,
  only the overlapping clones would need pooling).
- **Audio — OGG needs `stb_vorbis` wired in** (WAV/MP3/FLAC are built into miniaudio).
- **Audio — no streaming;** clips fully decode to PCM (fine for SFX/short music; large music files
  would benefit from a per-voice streaming decoder).
