# AERO_RIOT — Project Context

Last updated: 2026-09-11

Compact handoff for continuing AERO_RIOT in a fresh chat. Inspect the latest `master` before assuming this file is perfectly current.

## Project / learning goal

AERO_RIOT is a DirectXTK/C++ 3D fighter-aircraft dogfight project used primarily to learn C++, DirectX/DirectXTK, engine architecture, custom physics/aerodynamics, HLSL/shaders, rendering, lighting/shadows, VFX, animation, cameras, AI, and optimization.

Rule: the game drives engine development. Do not build a whole engine upfront; add systems when the game creates a real need.

## Collaboration workflow

The user manually writes code for learning. Do not give copy-paste final implementations unless explicitly requested, stuck, or the task is mechanical.

Preferred flow:
1. Decide next practical goal.
2. User proposes design/architecture where appropriate.
3. Review/fix/improve it and explain why.
4. Give concepts, constraints, pitfalls, and hints rather than full implementation.
5. User implements manually and pushes.
6. Inspect latest GitHub state.
7. Review bugs/architecture/performance/safety.
8. User fixes where practical.
9. Move on once solid.

For hard topics (physics/math/rendering/HLSL/memory/ownership/algorithms), teach theory first.

Priority: UNDERSTAND -> DESIGN -> IMPLEMENT -> REVIEW -> IMPROVE.

After each meaningful completed step, update/overwrite this file with the latest state rather than turning it into a raw chat log.

## Core engine architecture

Ownership/lifecycle:

Game -> SceneManager -> Scene -> GameObjectManager -> GameObject -> Component

Important decisions:
- GameObject is final; composition is intentional.
- GameObject owns Transform + Components.
- GameObjectManager owns GameObjects with `unique_ptr`.
- Scene owns scene-local systems and GameObjectManager.
- GameObject stores non-owning `GameObjectManager*`.
- GameObjectManager stores non-owning owning `Scene*`.
- Component can reach Scene through GameObject -> GameObjectManager -> Scene.
- Scene exposes intentional scene-level capabilities (e.g. RequestSceneLoad, GetKinetics); gameplay should not tunnel directly into SceneManager.
- New objects are pending until BeginFrame; component/object destruction is deferred to safe EndFrame processing.

Scene member order intentionally keeps Kinetics alive while objects/components are destroyed:

```cpp
Kinetics m_kinetics;
GameObjectManager m_gameObjects;
```

Members destroy in reverse declaration order, so GameObjects unregister before Kinetics dies.

## Transform / coordinate conventions

- local Forward = (0,0,-1)
- Right = (1,0,0)
- Up = (0,1,0)
- `Transform::GetForward/Right/Up()` return world-space directions.
- `Vector3::Forward` is only the canonical axis; meaning comes from the coordinate frame.

Rule already learned:
- local visual/body roll delta -> canonical `Vector3::Forward`
- world thrust direction -> `Transform::GetForward()`

## Update / fixed simulation phases

Time uses a fixed accumulator. Current fixed step: 1/60 s.

`Scene::FixedUpdate()`:
1. Scene OnFixedUpdate()
2. GameObjectManager FixedUpdate() — gameplay/components submit forces
3. Kinetics Integrate(FixedDeltaTime) — physics consumes forces

This phase split avoids hidden component insertion-order dependencies.

## Kinetics / KineticBody

Kinetics is scene-local and holds non-owning `KineticBody*` references.

KineticBody currently has:
- mass + inverse mass
- linear velocity
- derived/stored linear acceleration
- accumulated force
- Transform as current position source of truth

Semi-implicit Euler:
- a = F * inverseMass
- v += a * fixedDt
- x += v * fixedDt
- clear accumulated forces

KineticBody registers/unregisters with Kinetics through lifecycle callbacks. Kinetics skips disabled/remove-requested/inactive/destroy-requested bodies.

No duplicate simulation-position state yet. Static/infinite-mass semantics will later use inverseMass = 0 when needed.

## Physics experiments completed

Debug cube validated:
- one force once -> velocity changes once, then remains constant with zero net force
- continuous force -> continuous acceleration
- linear drag `F = -k*v` -> velocity approaches zero
- continuous diagonal force (5 Forward + 5 Right) + k=0.5 linear drag -> predicted/observed terminal speed 14.142
- simplified quadratic drag `F = -k*|v|*v` with same force/k -> predicted/observed terminal speed ~3.76

Test cube has been removed.

## Aircraft architecture

Conceptual hierarchy:

```text
AircraftRoot              <- gameplay/simulation transform
|- Aircraft
|- AircraftController
|- KineticBody
|- ThirdPersonCameraAnchor
`- Body                   <- visual/presentation transform
   |- Base
   `- Wing
```

AircraftController produces semantic `AircraftControlInput`:
- pitch [-1,+1]
- turn [-1,+1]
- throttle [0,1]
- airBrake [0,1]
- evadeRoll None/Left/Right

Root is simulation/gameplay orientation. Body is presentation-only for fake visual motion such as 360° evade spin.

## Aircraft linear physics — current

Old scalar-speed/direct-position system removed:
- no m_maxSpeed
- no m_speedRate
- no m_currentSpeed
- no m_speedDrag

Aircraft resolves sibling KineticBody in OnStart().

Current config values:
- m_maxThrust
- m_airDrag
- m_airBrakePower
- temporary m_rotationSpeed

`Aircraft::OnFixedUpdate()` currently:
- TEMP direct kinematic pitch/turn/bank using `FixedDeltaTime`
- thrust = world Forward * maxThrust * throttle
- AddForce(thrust)
- read velocity + speed from KineticBody
- simplified quadratic drag
- air brake increases drag as additional drag
- AddForce(drag)

No artificial speed clamp; terminal speed emerges from thrust vs drag.

Expected behavior: orientation and velocity are now different. Turning the nose does not instantly rotate velocity. That is intentional and motivates local airflow, directional resistance, lift, AoA, stall, etc.

## Temporary aircraft technical debt

- Normal rotation directly mutates Transform; later replace with torque/angular velocity/inertia.
- Evade visual Body spin remains in LateUpdate.
- Evade root lateral displacement still bypasses KineticBody; temporary.
- Aircraft visual Body lookup still assumes child index 0; later make wiring explicit when prefab/factory/config work justifies it.

## Camera

Separate chase camera root targets ThirdPersonCameraAnchor.

Camera supports right-stick spherical freelook + delayed recenter. Position follows target basis; LookAt currently uses world Up for horizon stability.

Pending after real bank/flight stabilization:
- decide world Up vs target Up vs stabilized/blended Up
- prevent normal bank/roll from becoming disorienting/effectively inverted

Known technical debt: camera/aircraft LateUpdate can depend on creation order. Add update priorities/PostLateUpdate/camera phase only when justified.

## Rendering interpolation

Not implemented yet. `RenderContext` carries `FixedInterpolationAlpha`, but primitive rendering currently uses the current Transform directly.

Aircraft can still look visually smooth because chase camera moves with it and displacement per 60 Hz step is small.

## Build system / PCH — COMPLETE

Precompiled headers are now enabled and working (commit `81943c8558ba91dd8faf74149eb0965055e7fefe`, followed by merge commit `3dc1d13fc689f2053640233fce2b371a1fa5f72d`).

Project-root files:
- `pch.h`
- `pch.cpp`

Project default uses `/Yu pch.h`; `pch.cpp` overrides to `/Yc pch.h` for Debug/Release and Win32/x64. All normal `.cpp` files include `pch.h` first.

Current PCH contains stable/common external headers only:
- `WIN32_LEAN_AND_MEAN`, `NOMINMAX`
- Windows.h, d3d11.h
- DirectXMath.h, SimpleMath.h
- algorithm, cstdint, memory, stdexcept, string, string_view, utility, vector

Do not put SNX gameplay/engine systems into the PCH just because they are common. Keep engine dependencies explicit. Headers should remain self-contained even if a type is also available through PCH.

Important distinction:
- PCH = compiler/build optimization
- common/umbrella header = source dependency convenience (not created yet; avoid dumping ground)
- AppConfig = runtime/startup settings
- Aircraft/AI/Weapon definitions = gameplay data/config, separate from AppConfig

## NEXT IMMEDIATE STEP — AppConfig

Create a deliberately small application/startup configuration layer for genuinely global settings already used by the program, such as:
- initial window width/height
- VSync preference if the current rendering path supports/configures it
- FPS cap only if/when an actual limiter exists
- possibly title/startup constants if appropriate

Do not mix gameplay tuning, aircraft stats, AI tuning, debug build macros, or every engine constant into AppConfig.

Prefer typed modern C++ (`struct`, `constexpr`, enum class where useful) over global preprocessor macros. Decide first whether values are compile-time defaults only or runtime mutable settings; do not build JSON/INI loading until there is a real need.

After the small AppConfig step, return to aircraft physics.

## Planned physics progression

Next physics topics:
- aircraft-local airflow / directional aerodynamic resistance
- lift
- angle of attack
- stall behavior
- angular velocity
- torque
- inertia / inertia tensor
- physical controls + flight stabilization
- physical/impulse evade behavior when appropriate
- gravity when useful
- collision detection/response later

Proceed progressively and validate each concept with observable tests.

## Major future learning targets

- custom HLSL/shaders
- real shadows/lighting
- particles/trails/explosions/VFX
- animation
- aircraft AI/missiles
- physics/collision expansion
- render interpolation
- optimization

## Repository

GitHub: https://github.com/mohit-kumar-singh55/AERO_RIOT
Default branch: `master`
