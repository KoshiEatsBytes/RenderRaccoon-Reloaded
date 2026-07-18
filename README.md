# RenderRaccoon-Reloaded

Voxel games such as Minecraft render vast, player-editable worlds whose streaming and drawing costs constrain the view distance consumer hardware can sustain. This project measures the extent to which five rendering and scheduling optimisations improve player quality of experience (QoE).

The instrument is a purpose-built C++20 / OpenGL 3.3 voxel engine with a procedurally generated world, each optimisation toggleable, evaluated through a deterministic in-engine benchmark, counting a total of 306 recorded runs across six machines spanning four consumer hardware tiers.

## Report

[Rendering and Scheduling Optimisations for Player Quality of Experience in Voxel Engines Across Consumer Hardware Tiers](https://drive.google.com/file/d/14MjanEGP219FQwzRvzyAe0F5Udo8HG8Z/view?usp=sharing)

[Windows executable](https://drive.google.com/file/d/1vdMIvenUFeU_uzQBPJFeEwLCARDrDh6M/view?usp=sharing)

[Benchmark Campaign](https://drive.google.com/file/d/1LkNoO5ykTdFBXyx4meUXYXoR1SB0752a/view?usp=sharing)

## Features

- Voxel Engine
  
    - Procedurally generated worlds from a single seed — biomes, rivers with karst tunnels, terraced mesas
    - Chunk streaming with a shared geometry arena and batched draw submission
    - Deterministic, thread-safe world generation (bit-identical across machines)

- Optimisation Techniques (independently toggleable)
  
    - Level of Detail — concentric ring system with stepped surface meshing
    - Chunk Aggregation — distant chunks merged into single-draw nodes
    - Greedy Meshing — coplanar face merging within the tile mesher
    - Multi-Threading — topology-aware worker pool with staleness guards
    - Adaptive Budgeting — workload-sensitive per-frame upload controller

- Benchmark Suite
  
    - Deterministic 17-configuration ladder over a fixed seed and scripted camera path
    - Self-describing per-frame CSV logging, warm-up watchdogs, abort-writes-nothing semantics
    - Median-of-three summary export with QoE-oriented metrics (percentile lows, 95% floor, gaps, interrupt magnitude, world coverage)

- Analysis Interface
  
    - In-engine ImGui/ImPlot analyzer with benchmark, analyze and compare views
    - Frame-time graphs, coverage tracking, and scores from published QoE models

## Video

[](https://youtu.be/QSWlw0ZV4aQ?si=918eewJ9k0S7npy9)

## Screenshots





## Scenes

### Main Menu

Launches benchmark runs and hosts the analysis interface — completed runs can be inspected and compared without leaving the engine.

### Benchmark

Executes the deterministic ladder: fixed seed, scripted flight, automatic run advancement. Each run writes a self-describing CSV beside the executable; the summary export aggregates three passes per configuration by median.

### Free Roam

Fly the world freely. TAB opens the rendering panel — wireframe view, the LOD showcase ladder, a live greedy-meshing toggle, and streaming lock for inspecting coarse LOD tiles up close.

## Credits

Block textures from the [Faithful 32x](https://faithfulpack.net/) resource pack, used with attribution — licence bundled unmodified at `Assets/Textures/Blocks/LICENSE.txt`. This work is based on ["Sten Gun(Machine Carbine)"](https://sketchfab.com/3d-models/sten-gunmachine-carbine-f61b01edd0e74cd8bd6217a3b2e6afb4) by [BadassCreeper](https://sketchfab.com/badasscreeper), licensed under [CC-BY-4.0](http://creativecommons.org/licenses/by/4.0/). All third-party libraries are listed with their licences in the dissertation's Appendix B; each licence text is retained in this repository.
