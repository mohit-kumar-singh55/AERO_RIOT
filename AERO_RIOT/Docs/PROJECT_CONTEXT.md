# AERO_RIOT — Project Context

Last updated: 2026-09-15

Compact handoff for continuing AERO_RIOT in a fresh chat. Inspect latest `master` before assuming this file is perfectly current.

## Project / learning workflow

AERO_RIOT is a DirectXTK/C++ 3D fighter-aircraft dogfight project used to learn C++, DirectX/DirectXTK, engine architecture, custom physics/aerodynamics, HLSL/shaders, rendering, lighting/shadows, VFX, animation, cameras, AI, and optimization.

Rule: the game drives engine development. Do not build a whole engine upfront.

The user manually writes code for learning. Preferred flow: UNDERSTAND -> DESIGN -> IMPLEMENT -> REVIEW -> IMPROVE. Do not give full copy-paste implementations unless explicitly requested, stuck, or the task is mechanical. For hard physics/math/rendering/HLSL/memory topics, teach theory first. After each meaningful completed step, update this file rather than turning it into a raw chat log.

## Core engine architecture

Ownership/lifecycle:

Game -> SceneManager -> Scene -> GameObjectManager -> GameObject -> Component

Important decisions:
- GameObject is final; composition is intentional.
- GameObject owns Transform + Components.
- GameObjectManager owns GameObjects with `unique_ptr`.
- Scene owns scene-local systems and GameObjectManager.
- GameObject stores non-owning `GameObjectManager*`; manager stores non-owning owning `Scene*`.
- Component reaches Scene through GameObject -> GameObjectManager -> Scene.
- New objects are pending until BeginFrame; destruction/removal is deferred to safe EndFrame processing.
- Scene member order keeps Kinetics alive while GameObjects/components unregister during destruction.

Fixed simulation order:
1. Scene OnFixedUpdate()
2. GameObjectManager FixedUpdate() — gameplay/components submit forces
3. Kinetics Integrate(FixedDeltaTime) — physics consumes forces

Current fixed step: 1/60 s.

## Transform / coordinate conventions

- canonical local Forward = (0,0,-1)
- Right = (1,0,0)
- Up = (0,1,0)
- `Transform::GetForward/Right/Up()` return world-space aircraft basis directions.
- local visual roll delta uses canonical `Vector3::Forward`; world thrust uses `Transform::GetForward()`.

## Kinetics / KineticBody

Kinetics is scene-local and holds non-owning `KineticBody*` references.

KineticBody currently owns/stores:
- mass + inverse mass
- linear velocity
- linear acceleration
- accumulated force
- Transform remains current position source of truth

Semi-implicit Euler:
- a = F * inverseMass
- v += a * fixedDt
- x += v * fixedDt
- clear forces

Physics experiments already validated one-shot force, continuous acceleration, linear drag equilibrium, and quadratic drag equilibrium.

## Aircraft architecture

Current hierarchy:

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

Root is simulation/gameplay orientation. Body is presentation-only for fake visual effects such as the 360-degree evade spin.

## Aircraft linear aerodynamics — current

Old scalar-speed/direct-position movement is removed. Aircraft currently owns the first physical translation/aerodynamics implementation and resolves sibling KineticBody.

Current tuning values:
- `m_maxThrust = 20`
- `m_forwardDrag = 1`
- `m_sideDrag = 3`
- `m_verticalDrag = 2`
- `m_airBrakePower = 4`
- temporary `m_rotationSpeed`

Current force flow in `Aircraft::OnFixedUpdate()`:
- TEMP direct kinematic orientation using `FixedDeltaTime`
- thrust along aircraft world Forward
- read KineticBody world velocity
- project velocity onto aircraft Forward/Right/Up to get signed forward/side/vertical speeds
- calculate separate quadratic drag per axis: `-axis * coefficient * speed * abs(speed)`
- sum drag; TEMP air-brake multiplier scales total drag
- submit forces to KineticBody

Directional aerodynamic resistance milestone is complete. A previous bug used `GetForward()` for all three axes; predicted terminal speed exposed the error. This reinforced validating physics with expected numeric behavior.

## Angle of Attack — COMPLETE / validated

AoA experiment was implemented in `Aircraft::OnFixedUpdate()` in commit `b9c149406aeb32ffb51c074678da7affb9477544`.

Current formula:

```cpp
angleOfAttack = -atan2(verticalSpeed, forwardSpeed);
```

The experiment converts the result to degrees for observation. Sign convention:
- nose above flight path -> positive AoA
- nose below flight path -> negative AoA

Sanity checks passed:
- straight flight -> AoA ~0
- quick pitch up -> positive AoA
- quick dive -> negative AoA
- aggressive orientation changes can produce |AoA| > 90 degrees when `forwardSpeed < 0`; this is valid raw geometry, not something to clamp. Stall/lift response will later determine aerodynamic behavior at high AoA.

No lift force has been added yet.

Important conceptual distinction:
- aircraft-relative air velocity = aircraft velocity - air velocity
- current air velocity is assumed zero, so relative velocity numerically equals world velocity
- relative wind points opposite relative velocity
- AoA is measured in the aircraft Forward-Up plane; sideslip/beta is separate and not implemented yet

## NEXT IMMEDIATE STEP — extract AircraftPhysics

The aerodynamic subsystem is now large enough to justify a separate `AircraftPhysics : Component` before adding lift.

Target responsibility split:
- `AircraftController`: hardware/player input -> semantic AircraftControlInput
- `Aircraft`: aircraft/gameplay state, temporary direct orientation handling, evade/presentation behavior; should stop owning aerodynamic force equations
- `AircraftPhysics`: KineticBody dependency, thrust, relative-air calculations, Forward/Right/Up projections, directional drag, AoA, and future lift/stall/aerodynamic forces
- `KineticBody`: generic force accumulation and integration only

Need deliberately design how `AircraftPhysics` receives throttle/airBrake/control state from `Aircraft` without coupling it directly to input hardware. Preserve the flow Controller -> Aircraft -> AircraftPhysics -> KineticBody.

Do the extraction as a behavior-preserving refactor first. Do not add lift in the same step.

## Temporary aircraft technical debt

- Normal pitch/turn/bank directly mutates Transform; later replace with angular velocity/torque/inertia.
- Evade visual Body spin remains in LateUpdate.
- Evade root lateral displacement bypasses KineticBody; temporary.
- Aircraft Body lookup assumes child index 0; later explicit wiring/factory/prefab.
- Air brake currently multiplies all directional drag; later make it a separate aerodynamic drag contribution.

## Camera

Separate chase camera root targets ThirdPersonCameraAnchor. Right-stick spherical freelook + delayed recenter. Camera currently uses world Up for horizon stability.

Pending after physical bank/stabilization:
- bank/roll readability and flight assist
- choose final world Up vs target Up vs stabilized/blended Up
- resolve creation-order LateUpdate dependency only when justified

## Build / application infrastructure

PCH is complete and active: project uses `/Yu pch.h`; `pch.cpp` uses `/Yc`. PCH contains stable Windows/DirectX/STL headers only; keep SNX/game dependencies explicit.

`Configs/AppConfig.h` is complete for current needs: title, width, height, VSync, fullscreen flag. `wWinMain` creates the config, window creation and Game receive it by `const AppConfig&`, Game stores an owned value, and Present receives `useVSync`. Fullscreen behavior is not wired yet.

## Planned physics progression

After AircraftPhysics extraction:
- lift
- stall behavior / lift coefficient curve
- sideslip when useful
- angular velocity
- torque
- inertia / inertia tensor
- physical controls + flight stabilization
- physical/impulse evade behavior when appropriate
- gravity when useful
- collision detection/response later

Other major targets: custom HLSL/shaders, real lighting/shadows, particles/trails/explosions/VFX, animation, aircraft AI/missiles, render interpolation, optimization.

## Repository

GitHub: https://github.com/mohit-kumar-singh55/AERO_RIOT
Default branch: `master`
