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

## 12. Build notes

- **Per-scene physics**, not a global Engine `PhysicsManager`. Components reach physics via their
  scene.
- Asset copy uses an **absolute source path** (`${CMAKE_SOURCE_DIR}/Assets/`), decoupled from the
  runtime `ASSETS_ROOT` (which is relative in Release) — earlier the two were conflated and broke
  the Release build.
- **`glfwSwapInterval(0)`** disables vsync — required for meaningful FPS benchmarking.
- Always benchmark **Release** (`-O2`): Debug Bullet is ~20–40× slower (un-inlined math).

---

## 13. Benchmark findings (1000 dynamic boxes, Ryzen 7 7800X3D)

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

## 14. Known limitations & future work

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
