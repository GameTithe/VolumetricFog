# AGENTS.md

## Repo Shape
- This repo is the plugin at `Plugins/VolumetricFog`, not the whole Unreal project. The host project is `..\..\FluidFog.uproject`, and its targets are `FluidFogEditor` and `FluidFog`.
- The host project is pinned to Unreal Engine 5.7 (`EngineAssociation` in `..\..\FluidFog.uproject`).

## Verify Changes
- There are no repo-local tests, CI workflows, or task-runner files in this plugin repo.
- Build against the host project, not the plugin directory. Example editor build from this folder: `"<UE_5.7>\Engine\Build\BatchFiles\Build.bat" FluidFogEditor Win64 Development -Project="..\..\FluidFog.uproject" -WaitMutex`
- Use `FluidFog` for runtime-only checks and `FluidFogEditor` when changing module wiring or editor-facing code.
- Smoke-test rendering changes in the host project. `..\..\Config\DefaultEngine.ini` forces DX12 + SM6 and enables ray tracing and Substrate; reproduce rendering issues under that config.

## Module Boundaries
- `Source/VolumetricFog` is the real runtime module. It loads at `PostConfigInit` so `Source/VolumetricFog/Private/VolumetricFlowFog.cpp` can map `/VolumetricFog` to `Shaders/Private` before shader compilation.
- If you rename or move `.usf` / `.ush` files, update the matching `IMPLEMENT_GLOBAL_SHADER` paths in `Source/VolumetricFog/Private/*.cpp`.
- `Source/VolumetricFogEditor` is a thin editor module; nearly all behavior lives in the runtime module.
- `Source/VolumetricFog/VolumetricFog.Build.cs` depends on engine renderer `Private` and `Internal` headers. Engine upgrades can break includes here first.

## Runtime Wiring
- `AVolumetricFluidFog` is the main placed actor. It owns the `UBoxComponent` bounds and the `UFluidSimulationComponent`.
- `UFluidSimulationComponent` allocates ping-pong RHI textures in `BeginPlay`, enqueues the fluid simulation each tick, and pushes density/velocity plus fog parameters into `FFogSceneViewExtension`.
- `FFogSceneViewExtension` injects the fullscreen fog pass through the renderer's post-opaque delegate. The main fog shader is `Shaders/Private/Rendering/FogRayMarch.usf`.

## Gotchas
- `OutputRT` is optional debug output only. The fog render path still works without it; the RT is only used for copying the density field.
- Bounds come from `AVolumetricFluidFog::BoundsComponent` when the component is attached to that actor; otherwise the code falls back to `GetActorBounds()`. The `SimulationWorldSize` property is currently not used by the active path.
- `CurlNoiseTexture` must be assigned or imported explicitly. The loose PNGs under `CurlNoises/` are sample assets, not auto-loaded plugin content.
- Ignore `Intermediate/` and `Binaries/` when reading the repo. They are generated outputs and can pollute searches with stale UBT/UHT code.

## Collaboration Preference
- Default behavior is explanation-only.
- Unless the user explicitly asks for code changes, do not modify code or files.
- Do not use `apply_patch`, create files, delete files, run formatting that changes files, or make commits unless explicitly requested by the user.
- By default, stay in read-only mode: inspect the codebase and explain the solution in detail.
- Prefer giving step-by-step implementation guidance, tradeoffs, and exact locations to change instead of making the change directly.
- If the user's request is ambiguous, ask whether they want explanation only or code changes.
