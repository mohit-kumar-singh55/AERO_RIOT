# AERO_RIOT — Project Context

Last updated: 2026-09-14

Compact handoff for continuing AERO_RIOT in a fresh chat. Inspect latest `master` before assuming this file is perfectly current.

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

After each meaningful completed step, update/overwrite this file with latest state rather than turning it into a raw chat log.

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
- Component reaches Scene through GameObject -> GameObjectManager -> Scene.
- Scene exposes intentional scene-level capabilities such as RequestSceneLoad and GetKinetics; gameplay should not tunnel directly into SceneManager.
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

Important rules:
- local visual/body roll delta -> canonical `Vector3::Forward`
- world thrust direction -> `Transform::GetForward()`
- variables holding `GetForward/Right/Up()` are world-space aircraft basis directions; avoid misleading `localForward` naming.

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

Old scalar-speed/direct-position system is removed. Aircraft resolves sibling KineticBody in OnStart().

Current configurable values:
- `m_maxThrust = 20`
- `m_forwardDrag = 1`
- `m_sideDrag = 3`
- `m_verticalDrag = 2`
- `m_airBrakePower = 4`
- temporary `m_rotationSpeed`

`Aircraft::OnFixedUpdate()` currently:
- TEMP direct kinematic pitch/turn/bank using FixedDeltaTime
- thrust = aircraft world Forward * maxThrust * throttle
- reads KineticBody world velocity
- projects velocity onto aircraft world Forward/Right/Up with dot products to get signed forward/side/vertical speeds
- computes separate quadratic drag per axis with `-axis * coefficient * speed * abs(speed)`
- sums directional drag contributions
- TEMP air-brake multiplier scales total directional drag
- submits thrust + drag with AddForce

No artificial speed clamp; terminal speed emerges from thrust vs drag.

Directional aerodynamic resistance milestone is COMPLETE as of commit `94fba25f8fbeb015c95c28a15187f004195c21a8`.

Important bug/lesson from this step: first implementation accidentally used `GetForward()` for Forward, Right, and Up. This made all three drag coefficients act on forward motion, producing straight terminal speed ~2.108 because effective k became 1+2+1.5=4.5, matching `sqrt(20/4.5)`. After turning, drag nearly vanished because all projections were onto the new Forward. Fixing Right/Up to `GetRight()` / `GetUp()` restored intended behavior. This was a useful example of using predicted physics values to diagnose coordinate/basis bugs.

Expected behavior now: orientation and velocity remain distinct. A sharp nose turn creates side velocity relative to the aircraft; strong side drag then progressively removes sideslip while thrust builds velocity along the new Forward, curving the flight path rather than snapping velocity instantly.

## Temporary aircraft technical debt

- Normal rotation directly mutates Transform; later replace with torque/angular velocity/inertia.
- Evade visual Body spin remains in LateUpdate.
- Evade root lateral displacement still bypasses KineticBody; temporary.
- Aircraft visual Body lookup still assumes child index 0; later make wiring explicit when prefab/factory/config work justifies it.
- Air brake currently multiplies all directional drag; later model it as a separate drag contribution opposite relative airflow/velocity.

## Camera

Separate chase camera root targets ThirdPersonCameraAnchor.

Camera supports right-stick spherical freelook + delayed recenter. Position follows target basis; LookAt currently uses world Up for horizon stability.

Pending after real bank/flight stabilization:
- decide world Up vs target Up vs stabilized/blended Up
- prevent normal bank/roll from becoming disorienting/effectively inverted

Known technical debt: camera/aircraft LateUpdate can depend on creation order. Add update priorities/PostLateUpdate/camera phase only when justified.

## Rendering interpolation

Not implemented yet. `RenderContext` carries `FixedInterpolationAlpha`, but primitive rendering currently uses current Transform directly.

Aircraft can still look visually smooth because chase camera moves with it and displacement per 60 Hz step is small.

## Build system / PCH — COMPLETE

Precompiled headers are enabled and working.

Project-root files:
- `pch.h`
- `pch.cpp`

Project default uses `/Yu pch.h`; `pch.cpp` overrides to `/Yc pch.h` for Debug/Release and Win32/x64. All normal `.cpp` files include `pch.h` first.

Current PCH contains stable/common external headers only:
- `WIN32_LEAN_AND_MEAN`, `NOMINMAX`
- Windows.h, d3d11.h
- DirectXMath.h, SimpleMath.h
- algorithm, cstdint, memory, stdexcept, string, string_view, utility, vector

Do not put SNX gameplay/engine systems into PCH just because they are common. Keep engine dependencies explicit. Headers should remain self-contained even if a type is also available through PCH.

## AppConfig — COMPLETE

`Configs/AppConfig.h` contains:
- `std::wstring title = L"AERO RIOT"`
- `windowWidth = 1280`
- `windowHeight = 720`
- `useVSync = true`
- `isFullScreen = false`

Flow:
- `wWinMain` creates one `const AppConfig appConfig{}`.
- `CreateGameWindow(const AppConfig&)` reads width/height/title.
- `Game::Initialize(..., const AppConfig&)` receives it and deliberately stores an owned `AppConfig m_appConfig` copy.
- `Game::Render` calls `m_deviceResources.Present(m_appConfig.useVSync)`.

Forward-declaration lesson: `struct AppConfig;` is sufficient for pointer/reference declarations, but `Game` stores `AppConfig m_appConfig` by value, so `Game.h` needs the complete type and includes `AppConfig.h`.

`isFullScreen` exists as config data but fullscreen behavior is not wired yet. Do not build JSON/INI loading until there is a real need.

## NEXT IMMEDIATE STEP — relative airflow + angle of attack

Directional resistance works. Next teach and implement the aerodynamic quantities needed before lift:
- aircraft velocity relative to air
- relative wind as the opposite direction of relative velocity
- pitch-plane airflow
- angle of attack (AoA): angle between aircraft chord/Forward and relative airflow/velocity direction in the Forward-Up plane
- signed AoA, so nose-above-flow and nose-below-flow are distinguishable

Do not add a full lift/stall model until AoA is understood and observable/debuggable.

## Planned physics progression

- relative airflow + angle of attack
- lift
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
