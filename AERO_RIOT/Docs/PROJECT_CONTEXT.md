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

Component lifecycle detail: GameObject FixedUpdate iterates components in insertion order and calls `EnsureComponentStarted(component)` immediately before that same component's `OnFixedUpdate()`. Do not assume all sibling components have run OnStart before another component's FixedUpdate. Cross-component dependencies needed during first FixedUpdate should be resolved in OnInitialize or otherwise made explicit.

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

Current gravity status: not implemented yet. Next small physics step is to add scene/world gravity before lift. Preferred architecture: Kinetics owns world gravity acceleration; KineticBody owns per-body `useGravity` (and optionally gravityScale later). During Kinetics integration, gravity can be applied as `F_g = mass * gravity` before `Integrate`, preserving the existing force-accumulator model and ensuring all masses receive the same gravitational acceleration.

## Aircraft architecture

Current hierarchy:

```text
AircraftRoot              <- gameplay/simulation transform
|- Aircraft
|- AircraftController
|- KineticBody
|- AircraftKinetics
|- ThirdPersonCameraAnchor
`- Body                   <- visual/presentation transform
   |- Base
   `- Wing
```

`AircraftControlInput` and `EvadeRoll` were extracted into `AircraftControlInput.h`.

AircraftController produces semantic `AircraftControlInput`:
- pitch [-1,+1]
- turn [-1,+1]
- throttle [0,1]
- airBrake [0,1]
- evadeRoll None/Left/Right

Root is simulation/gameplay orientation. Body is presentation-only for fake visual effects such as the 360-degree evade spin.

## Aircraft physics refactor — IMPLEMENTED, one lifecycle cleanup pending

Commit `bb4d10445b311901e395e8bf88659b5f2114f01b` extracted thrust, directional drag, and AoA calculation from `Aircraft` into `AircraftKinetics : Component`.

Current flow:

AircraftController -> Aircraft -> AircraftKinetics -> KineticBody

`Aircraft::OnFixedUpdate()` still performs TEMP direct kinematic orientation first, then explicitly calls `m_aircraftKinetics->Apply(m_controlInput)`. This explicit call intentionally preserves ordering so aerodynamic calculations use the freshly updated aircraft orientation and do not depend on sibling FixedUpdate order.

`AircraftKinetics` now owns:
- KineticBody dependency
- `m_maxThrust = 20`
- `m_forwardDrag = 1`
- `m_sideDrag = 3`
- `m_verticalDrag = 2`
- `m_airBrakePower = 4`
- thrust calculation
- world velocity projection onto aircraft Forward/Right/Up
- directional quadratic drag
- current AoA calculation

Behavior appears preserved after the refactor.

Important pending fix before building more physics: `AircraftKinetics` currently resolves its `KineticBody*` in OnStart, but `Aircraft::OnFixedUpdate()` may call `AircraftKinetics::Apply()` before AircraftKinetics itself has reached OnStart because component starts are sequential. Move the mandatory KineticBody lookup to OnInitialize (or otherwise guarantee readiness before Apply). Also change `Apply(AircraftControlInput&)` to `Apply(const AircraftControlInput&)` because it does not modify the input.

Naming note: `AircraftKinetics` is workable, but `AircraftPhysics` would be semantically broader/clearer as lift, stall, angular dynamics, torque and other aircraft-specific physics grow. Renaming is optional; avoid churn unless desired.

## Directional drag — COMPLETE

Aircraft physics uses signed projections:
- forwardSpeed = dot(velocity, aircraftForward)
- sideSpeed = dot(velocity, aircraftRight)
- verticalSpeed = dot(velocity, aircraftUp)

Per-axis quadratic resistance:
`-axis * coefficient * speed * abs(speed)`

Directional resistance milestone is complete. A previous bug used GetForward for all three axes; predicted terminal speed exposed the error and reinforced validating physics with expected numbers.

Air brake still temporarily multiplies all directional drag. Later model it as a separate aerodynamic contribution.

## Angle of Attack — COMPLETE / validated

Current formula:

```cpp
angleOfAttack = -atan2(verticalSpeed, forwardSpeed);
```

Sign convention:
- nose above flight path -> positive AoA
- nose below flight path -> negative AoA

Sanity checks passed:
- straight flight -> AoA ~0
- quick pitch up -> positive AoA
- quick dive -> negative AoA
- aggressive orientation changes can produce |AoA| > 90 degrees when forwardSpeed < 0; valid raw geometry, not something to clamp

Current refactor still converts AoA to degrees inside AircraftKinetics and discards it. Before lift, keep AoA internally in radians and convert to degrees only for debug/UI.

Conceptual distinction:
- aircraft-relative air velocity = aircraft velocity - air velocity
- current air velocity is assumed zero, so relative velocity numerically equals world velocity
- relative wind points opposite relative velocity
- AoA is measured in the aircraft Forward-Up plane; sideslip/beta is separate and not implemented yet

## Temporary aircraft technical debt

- Normal pitch/turn/bank directly mutates Transform; later replace with angular velocity/torque/inertia.
- Evade visual Body spin remains in LateUpdate.
- Evade root lateral displacement bypasses KineticBody; temporary.
- Aircraft Body lookup assumes child index 0; later explicit wiring/factory/prefab.
- Air brake currently multiplies all directional drag; later separate contribution.
- Direct call to AircraftKinetics::Apply bypasses normal Component enabled/update dispatch; decide later whether Apply should internally respect enabled/remove state or Aircraft should check it.

## Camera

Separate chase camera root targets ThirdPersonCameraAnchor. Right-stick spherical freelook + delayed recenter. Camera currently uses world Up for horizon stability.

Pending after physical bank/stabilization:
- bank/roll readability and flight assist
- choose final world Up vs target Up vs stabilized/blended Up
- resolve creation-order LateUpdate dependency only when justified

## Build / application infrastructure

PCH is complete and active: project uses `/Yu pch.h`; `pch.cpp` uses `/Yc`. PCH contains stable Windows/DirectX/STL headers only; keep SNX/game dependencies explicit.

`Configs/AppConfig.h` is complete for current needs: title, width, height, VSync, fullscreen flag. `wWinMain` creates the config, window creation and Game receive it by `const AppConfig&`, Game stores an owned value, and Present receives `useVSync`. Fullscreen behavior is not wired yet.

## NEXT IMMEDIATE STEP

1. Fix AircraftKinetics initialization safety (`KineticBody` lookup in OnInitialize) and make Apply take `const AircraftControlInput&`.
2. Add simple world gravity before lift.
3. Validate gravity independently with a bare KineticBody and confirm mass-independent acceleration.
4. Then return to lift using AoA and relative airflow.

Preferred gravity architecture:
- Kinetics owns world gravity vector, initially `{0,-9.81,0}` if 1 world unit = 1 meter.
- KineticBody owns `useGravity` (default choice can be deliberate; dynamic-body-style default true is reasonable).
- Kinetics applies `mass * gravity` to each gravity-enabled body before calling Integrate.
- Gravity is world-down, never aircraft-local Down.

## Planned physics progression

After gravity:
- lift
- stall behavior / lift coefficient curve
- sideslip when useful
- angular velocity
- torque
- inertia / inertia tensor
- physical controls + flight stabilization
- physical/impulse evade behavior when appropriate
- collision detection/response later

Other major targets: custom HLSL/shaders, real lighting/shadows, particles/trails/explosions/VFX, animation, aircraft AI/missiles, render interpolation, optimization.

## Repository

GitHub: https://github.com/mohit-kumar-singh55/AERO_RIOT
Default branch: `master`
