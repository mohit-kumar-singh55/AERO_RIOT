# AERO_RIOT — Development Roadmap

Created: 2026-09-29
Purpose: Preserve project-level goals, schedule, scope boundaries, and the planned DirectX 12 / DXR continuation independently of the implementation details in `PROJECT_CONTEXT.md`.

This is a *working plan*, not a promise that every optional feature will be finished on a specific day. Revise dates or scope based on learning, school/work availability, and actual gameplay progress.

## Guiding principles

- **Game first, simulation second:** AERO_RIOT should be an exciting, playable fighter-dogfight game, not a complete aircraft simulator or general-purpose engine.
- **Learn through writing:** User proposes architecture and manually implements C++/HLSL, then we review latest GitHub commit and iterate. Do not substitute copy-paste solutions for understanding.
- **Do not rush away shader learning to meet the deadline:** HLSL is a core learning objective; mastery continues beyond this game.
- **Freeze completed foundations unless gameplay requires changes:** Avoid expanding collision/physics systems for hypothetical future use cases.
- **A completed, polished vertical slice beats a broad unfinished feature list.**

## AERO_RIOT — DX11 target

**Feature-complete / playable target: 2026-10-31** (end of October; allow pragmatic scope cuts to optional features).

Finish AERO_RIOT in **C++ / DirectX 11 / DirectXTK**. Do not migrate it to DX12 during October. Its portfolio role is a self-built, playable dogfighting game showing engine architecture, controls/physics, collision/CCD, custom HLSL, and integrated gameplay/VFX.

### Planned October sequence

| Window | Main focus | Desired proof of progress |
|---|---|---|
| Sep 29 – Oct 5 | Custom HLSL foundation | Own VS/PS, unlit material, input layout, constant buffer, first custom-rendered object, animated/emissive shader |
| Oct 6 – Oct 12 | Shader experimentation and VFX | Engine exhaust / pulsing effects; trails; simple particle-system foundation; blend/transparency and rendering integration |
| Oct 13 – Oct 19 | Playable dogfight loop | Basic enemy aircraft AI, health/damage, bullet hit behavior, combat feedback, simple objective/win/lose loop |
| Oct 20 – Oct 26 | Visual identity and polish | Hit sparks/explosions, stylized colors, UI/HUD, camera feedback, menu/game-over, optional audio |
| Oct 27 – Oct 31 | Stabilize and publish | Bug testing/tuning, Release Windows build, README, brief gameplay footage, portfolio explanation; avoid new major engine features |

Dates may overlap or shift. Prioritize *a full basic playable loop* over optional feature count. Protect meaningful time for HLSL learning.

### Minimum definition of done

- Can launch a packaged Windows Release build.
- Player can fly and shoot with stable controls.
- There is at least one encounter/opponent and a clear gameplay objective, success/failure, and restart path.
- Collision and fast-bullet detection function reliably enough for the intended encounters.
- The visuals demonstrate **our own HLSL code** and at least a few recognizable game effects (rather than all effects using stock DirectXTK).
- Document and show the actual technical contribution accurately in GitHub README/portfolio video.

### October non-goals / possible scope cuts

- No full DX12 conversion during development of this game.
- No generalized rigid-body contact solver, Box–Box SAT, broadphase, or simulator-level aerodynamics unless a real gameplay requirement demands it.
- No full commercial-level content volume or general-purpose graphics/material framework.
- Optional extra AI types, elaborate menus, audio polish, advanced particles/postprocessing, etc. can be cut if the core loop or shader learning needs the time.
- Small gameplay integrations (e.g. bullet destruction on valid hit, ownership/self-hit rules) still matter and should be done when building the dogfight loop.

## After AERO_RIOT — graphics and SNX DX12

**November 2026: tentative start of a new DirectX 12 learning/porting project, tentatively `SNX_DX12`.** No artificial requirement to complete DXR mastery by the end of November.

Technical motivation: go deeper in graphics programming and demonstrate it in future game-programmer applications. The major personal learning target within DX12 is **DXR (DirectX Raytracing)**.

### Migration strategy

1. Finish AERO_RIOT in DX11 first.
2. Extract/reuse **concepts and appropriately decoupled core code** from SNX: GameObject/Component, Scene lifecycle, Transform/Camera math, Time, relevant simulation/collision and gameplay architecture.
3. Build a new DX12 rendering backend incrementally instead of merely renaming DX11 API calls. Existing RenderContext, PrimitiveRenderer, IPrimitiveMaterial, BasicEffect/DirectXTK integrations are DX11-specific and will need redesign; do **not** prematurely burden October's DX11 code with generic cross-API abstractions.
4. Use **DirectXTK12** where useful. It is the DX12 variant, not a drop-in switch from DX11 DirectXTK.
5. Preserve shader/math learning: HLSL concepts (VS/PS, coordinate spaces, lighting, procedural effects) transfer; DX12 changes resource and pipeline binding.

### Tentative learning milestones

1. DX12 basics: device, swap chain, render target, command allocator/list/queue, frame lifecycle, fences.
2. First triangle/cube using own HLSL; root signature, PSO, constant buffers, descriptor heaps, uploads and resource-state transitions.
3. Bring back a small selection of reusable SNX scene, camera, transform and gameplay systems.
4. **DXR foundation:** feature-support check, raytracing pipeline/state object and shaders, acceleration structures (BLAS/TLAS), ray generation/miss/closest-hit shaders, shader binding table and dispatch.
5. Build a small **DXR showcase** such as ray-traced shadows/reflections in a familiar AERO_RIOT test scene; learn performance/debugging and fallbacks, rather than promising to convert the entire game to ray tracing.
6. Keep learning HLSL beyond October: lighting, procedural noise, shader animation, trails/particles, postprocessing and later raytracing shaders.

The goal is to understand explicit DX12/DXR resource and rendering concepts, **not merely to port the application mechanically**.

## Career / portfolio timing

- October: aim to finish and document the DX11 game as a playable portfolio project.
- November: start DX12/DXR exploration and prepare documentation/showcase of both gameplay and rendering-engine learning.
- **December 2026:** user intends to resume searching/applying for **2028 new-graduate (28新卒)** game-programmer opportunities.
- Distinct story for technical interviews: `AERO_RIOT` demonstrates shipping a game using C++/DX11, custom physics/collision and HLSL/VFX; `SNX_DX12` demonstrates deliberate graphics-API/engine learning with DXR as a planned focus.
- Do not present planned DX12/DXR milestones as completed portfolio achievements until actually implemented and verified.

## Related document

`Docs/PROJECT_CONTEXT.md` — technical source of truth for current implementation, specific design decisions, issues and immediate next coding step. This roadmap preserves the long-term intentions, scope and milestones.
