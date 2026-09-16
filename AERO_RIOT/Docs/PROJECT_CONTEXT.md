# AERO_RIOT — Project Context

Last updated: 2026-09-15

Compact handoff for continuing AERO_RIOT in a fresh chat. Inspect latest `master` before assuming this file is perfectly current.

## Project / learning workflow

AERO_RIOT is a DirectXTK/C++ 3D fighter-aircraft dogfight project used to learn C++, DirectX/DirectXTK, engine architecture, custom physics/aerodynamics, HLSL/shaders, rendering, lighting/shadows, VFX, animation, cameras, AI, and optimization.

Rule: the game drives engine development. The user manually writes code for learning. Preferred flow: UNDERSTAND -> DESIGN -> IMPLEMENT -> REVIEW -> IMPROVE. Do not give full copy-paste implementations unless explicitly requested, stuck, or the task is mechanical. For hard physics/math/rendering/HLSL/memory topics, teach theory first.

## Core engine architecture

Ownership/lifecycle:

Game -> SceneManager -> Scene -> GameObjectManager -> GameObject -> Component

Fixed simulation order:
1. Scene OnFixedUpdate()
2. GameObjectManager FixedUpdate() — gameplay/components submit forces
3. Kinetics Integrate(FixedDeltaTime) — world physics consumes forces

Current fixed step: 1/60 s.

Important lifecycle rule: GameObject FixedUpdate starts each component immediately before that component's own OnFixedUpdate. Cross-component dependencies needed by first FixedUpdate should be resolved in OnInitialize or otherwise explicitly guaranteed.

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

Implemented in `aaa67bdcbeb737344583043f6c930986baaa46a1`.

- Kinetics owns world gravity `{0,-9.81,0}`.
- KineticBody has `useGravity` default true and `gravityScale` default 1.
- Before integration, Kinetics applies `mass * gravity * gravityScale`.
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

Aircraft performs temporary direct kinematic orientation first, then explicitly calls AircraftKinetics::Apply(controlInput), preserving ordering without relying on sibling FixedUpdate order.

### AircraftKinetics refactor — COMPLETE

AircraftKinetics owns:
- KineticBody dependency resolved in OnInitialize
- max thrust
- directional drag coefficients
- temporary air-brake drag multiplier
- thrust
- aircraft-basis velocity decomposition
- AoA
- lift / stall calculations

Apply receives `const AircraftControlInput&`.

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

Raw formula:

```cpp
angleOfAttack = -atan2(verticalSpeed, forwardSpeed);
```

Sign convention:
- nose above flight path -> positive AoA
- nose below flight path -> negative AoA

Do not clamp raw AoA. Keep radians for physics; convert to degrees only for debug/UI. Current air velocity is assumed zero. Sideslip/beta not implemented.

## Lift + stall — COMPLETE / validated

Basic lift implemented in `8bf52d823fdfb9b630849843ad5207ae50acd26e`.

Current lift model:
- pitchSpeedSquared = forwardSpeed^2 + verticalSpeed^2
- low-speed guard ~0.01 m/s pitch-plane speed
- Lift = 0.5 * airDensity * pitchSpeedSquared * wingArea * Cl
- pitch-plane velocity = Forward * forwardSpeed + Up * verticalSpeed
- lift direction = normalize(Right x pitchVelocity)
- signed Cl reverses lift for negative AoA

Current tuning:
- airDensity = 1.225
- wingArea = 2.0
- liftSlope = 4.0 per radian
- stallAngle = 15 degrees

Unlimited linear `Cl = liftSlope * AoA` first demonstrated the need for stall: near-vertical fall produced unrealistically huge Cl and forward lift.

Stall curve added in `dd45b14bba0c7950b14694e298c1e0503c8cd839`, then corrected in `eb7eb76925e97970eb2d122f6360797d4bea2e12` so peak lift is deterministic rather than previous-frame state.

Current `CalculateLiftCoefficient` behavior:
- |AoA| <= 15 deg: `Cl = liftSlope * AoA`
- 15 deg < |AoA| < 90 deg: peak `Cl = liftSlope * stallAngle`, then linear falloff toward zero while preserving AoA sign
- |AoA| >= 90 deg: `Cl = 0`

Expected values with current tuning:
- 0 deg -> Cl 0
- 5 deg -> ~0.349
- 10 deg -> ~0.698
- 15 deg -> ~1.047 peak
- 30 deg -> ~0.838
- 60 deg -> ~0.419
- 90 deg -> 0
- negative angles mirror sign

Behavior validation passed qualitatively:
- from rest with zero throttle, aircraft mostly falls and no longer gains unrealistic forward acceleration from huge deep-stall lift
- after building forward speed and releasing throttle, inertia/lift produce temporary glide/descent before energy is lost

## NEXT IMMEDIATE STEP — angular dynamics foundation

Translation/aerodynamics are now physical enough that the largest remaining mismatch is rotation: normal pitch/turn/bank still directly mutates Transform.

Next progression:
1. teach angular velocity and angular acceleration
2. teach torque as rotational analogue of force
3. introduce inertia, starting with a deliberately simple representation before full tensor complexity
4. extend KineticBody/Kinetics with angular state and torque accumulation
5. integrate orientation using quaternions during the fixed physics step
6. validate with isolated rotational experiments
7. only then replace temporary direct aircraft rotation with physical control torque / stabilization

Do not jump directly to full aircraft control torques before generic angular integration is understood and tested.

## Temporary technical debt

- normal pitch/turn/bank directly mutates Transform; next major physics target
- evade root displacement bypasses KineticBody
- evade Body spin is presentation-only
- Aircraft Body lookup assumes child index 0
- direct Aircraft -> AircraftKinetics Apply bypasses normal enabled/update dispatch; revisit when useful
- air brake is not yet a separate aerodynamic surface/force
- no induced drag yet
- sideslip/beta not modeled yet
- no render interpolation

## Camera

Separate chase camera with right-stick spherical freelook + delayed recenter. Camera currently uses world Up. After physical bank/stabilization, revisit bank readability and world-Up vs target-Up vs blended/stabilized camera Up.

## Build / app infrastructure

PCH complete and active. AppConfig complete for current needs: title, width, height, VSync, fullscreen flag; fullscreen behavior itself is not wired yet.

## Repository

GitHub: https://github.com/mohit-kumar-singh55/AERO_RIOT
Default branch: `master`
