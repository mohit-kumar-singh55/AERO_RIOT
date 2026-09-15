# AERO_RIOT — Project Context

Last updated: 2026-09-15

Compact handoff for continuing AERO_RIOT in a fresh chat. Inspect latest `master` before assuming this file is perfectly current.

## Project / learning workflow

AERO_RIOT is a DirectXTK/C++ 3D fighter-aircraft dogfight project used to learn C++, DirectX/DirectXTK, engine architecture, custom physics/aerodynamics, HLSL/shaders, rendering, lighting/shadows, VFX, animation, cameras, AI, and optimization.

Rule: the game drives engine development. Do not build a whole engine upfront.

The user manually writes code for learning. Preferred flow: UNDERSTAND -> DESIGN -> IMPLEMENT -> REVIEW -> IMPROVE. Do not give full copy-paste implementations unless explicitly requested, stuck, or the task is mechanical. For hard physics/math/rendering/HLSL/memory topics, teach theory first.

## Core engine architecture

Ownership/lifecycle:

Game -> SceneManager -> Scene -> GameObjectManager -> GameObject -> Component

Fixed simulation order:
1. Scene OnFixedUpdate()
2. GameObjectManager FixedUpdate() — gameplay/components submit forces
3. Kinetics Integrate(FixedDeltaTime) — world physics consumes forces

Current fixed step: 1/60 s.

Important lifecycle rule: GameObject FixedUpdate starts each component immediately before that component's own OnFixedUpdate. Do not assume all sibling OnStart calls happened first. Cross-component dependencies needed by first FixedUpdate should be resolved in OnInitialize or otherwise explicitly guaranteed.

## Transform conventions

- canonical local Forward = (0,0,-1)
- Right = (1,0,0)
- Up = (0,1,0)
- Transform::GetForward/Right/Up return world-space basis directions.
- local visual roll delta uses canonical Vector3::Forward; world thrust uses Transform::GetForward().

## Kinetics / KineticBody

Kinetics is scene-local and holds non-owning KineticBody pointers. KineticBody owns mass/inverse mass, linear velocity, linear acceleration, accumulated force, gravity flags, and uses Transform as position source of truth.

Semi-implicit Euler:
- a = F * inverseMass
- v += a * fixedDt
- x += v * fixedDt
- clear accumulated force

Validated experiments: one-shot force, continuous force, linear drag, quadratic drag, and mass-independent gravity.

### Gravity — COMPLETE

Implemented in commit `aaa67bdcbeb737344583043f6c930986baaa46a1`.

- Kinetics owns world gravity `{0,-9.81,0}`.
- KineticBody has `useGravity` (default true) and `gravityScale` (default 1).
- Before integration, Kinetics applies `mass * gravity * gravityScale` to gravity-enabled bodies.
- Because Integrate divides force by mass, gravitational acceleration is mass-independent.
- Gravity is world-space down, not aircraft-local down.
- User tested independent free fall and aircraft sinking successfully.

## Aircraft architecture

Current hierarchy:

```text
AircraftRoot
|- Aircraft
|- AircraftController
|- KineticBody
|- AircraftKinetics
|- ThirdPersonCameraAnchor
`- Body
   |- Base
   `- Wing
```

AircraftControlInput and EvadeRoll live in `AircraftControlInput.h`.

Current flow:

AircraftController -> Aircraft -> AircraftKinetics -> KineticBody

Aircraft performs temporary direct kinematic orientation first, then explicitly calls AircraftKinetics::Apply(controlInput). This explicit orchestration avoids sibling FixedUpdate ordering dependencies and ensures aerodynamic forces use the updated orientation.

### AircraftKinetics refactor — COMPLETE

AircraftKinetics owns:
- KineticBody dependency
- max thrust
- forward/side/vertical drag coefficients
- temporary air-brake drag multiplier
- thrust calculation
- aircraft-basis velocity decomposition
- directional quadratic drag
- AoA calculation
- future lift/stall calculations

Mandatory KineticBody lookup now occurs in OnInitialize. Apply receives `const AircraftControlInput&`.

Minor cleanup still pending: AircraftKinetics::OnInitialize exception text still says `OnStart`.

## Directional drag — COMPLETE

Signed velocity components:
- forwardSpeed = dot(relativeVelocity, aircraftForward)
- sideSpeed = dot(relativeVelocity, aircraftRight)
- verticalSpeed = dot(relativeVelocity, aircraftUp)

Current quadratic axis resistance:
`-axis * coefficient * speed * abs(speed)`

Current tuning:
- maxThrust = 20
- forwardDrag = 1
- sideDrag = 3
- verticalDrag = 2
- airBrakePower = 4

Air brake still temporarily multiplies all directional drag; later make it a separate contribution.

## Angle of Attack — COMPLETE / validated

Current raw formula:

```cpp
angleOfAttack = -atan2(verticalSpeed, forwardSpeed);
```

Sign convention:
- nose above flight path -> positive AoA
- nose below flight path -> negative AoA

Sanity checks passed, including |AoA| > 90 degrees when forwardSpeed becomes negative during extreme orientation changes. Do not clamp raw AoA. Keep AoA in radians for physics; convert to degrees only for debug/UI.

Current air velocity is assumed zero, so aircraft-relative air velocity numerically equals KineticBody world velocity. Relative wind points opposite relative velocity. Sideslip/beta is separate and not implemented yet.

## NEXT IMMEDIATE STEP — lift

Gravity now gives lift a meaningful opposing force. Next teach and implement a first lift model inside AircraftKinetics while preserving the current learning progression.

Start with the standard structure:

`Lift = 0.5 * rho * V^2 * S * Cl(alpha)`

Need to decide deliberately:
- which airspeed magnitude is used for the first model
- air density / scaling for game units
- wing reference area
- first simple lift-coefficient curve vs AoA
- lift direction in world space
- low-speed guard

Do not jump straight to a complex stall model. First make lift understandable, observable, and able to oppose gravity at plausible forward speed; then add stall behavior as the following step.

## Temporary technical debt

- normal pitch/turn/bank directly mutates Transform; later angular velocity/torque/inertia
- evade root displacement bypasses KineticBody
- evade Body spin is presentation-only
- Aircraft Body lookup assumes child index 0
- direct Aircraft -> AircraftKinetics Apply bypasses normal enabled/update dispatch; revisit when useful
- air brake is not yet a separate aerodynamic surface/force
- no render interpolation

## Camera

Separate chase camera with right-stick spherical freelook + delayed recenter. Camera currently uses world Up. After physical bank/stabilization, revisit bank readability and world-Up vs target-Up vs blended/stabilized camera Up.

## Build / app infrastructure

PCH complete and active. AppConfig complete for current needs: title, width, height, VSync, fullscreen flag; fullscreen behavior itself is not wired yet.

## Planned physics progression

- lift
- stall / lift coefficient curve
- sideslip when useful
- angular velocity
- torque
- inertia / inertia tensor
- physical controls + stabilization
- physical evade behavior
- collision detection/response later

Other targets: HLSL/shaders, lighting/shadows, particles/trails/explosions/VFX, animation, AI/missiles, render interpolation, optimization.

## Repository

GitHub: https://github.com/mohit-kumar-singh55/AERO_RIOT
Default branch: `master`
