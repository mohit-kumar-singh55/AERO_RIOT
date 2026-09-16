# AERO_RIOT — Project Context

Last updated: 2026-09-16

Compact handoff for continuing AERO_RIOT in a fresh chat. Inspect latest `master` before assuming this file is perfectly current.

## Learning workflow
AERO_RIOT is a DirectXTK/C++ 3D fighter-aircraft dogfight project used to learn engine architecture, custom physics/aerodynamics, rendering/HLSL, VFX, cameras, AI, and optimization.

The user manually writes code for learning. Preferred flow: UNDERSTAND -> DESIGN -> IMPLEMENT -> REVIEW -> IMPROVE. Do not provide full copy-paste implementations unless explicitly requested, stuck, or the task is mechanical. Teach hard physics/math/rendering concepts before implementation. The game drives engine development; do not build systems without a concrete need.

## Core engine / lifecycle
Ownership: Game -> SceneManager -> Scene -> GameObjectManager -> GameObject -> Component.

Fixed simulation order:
1. Scene OnFixedUpdate()
2. GameObjectManager FixedUpdate() — gameplay/components submit forces
3. Kinetics Integrate(FixedDeltaTime) — world physics consumes forces

Current fixed step: 1/60 s.

Important lifecycle rule: GameObject starts each component immediately before that component's own update. Cross-component dependencies needed during first FixedUpdate should be resolved in OnInitialize or explicitly guaranteed.

Transform conventions:
- local Forward = (0,0,-1), Right = (1,0,0), Up = (0,1,0)
- Transform::GetForward/Right/Up return world-space directions
- Transform normalizes stored rotations
- current angular velocity/torque convention is WORLD-space

## Kinetics / KineticBody
Kinetics is scene-local and stores non-owning KineticBody pointers.

KineticBody currently has:
- mass / inverse mass
- linear velocity, linear acceleration, accumulated force
- useGravity + gravityScale
- scalar moment of inertia / inverse scalar inertia
- angular velocity, angular acceleration, accumulated torque
- Transform remains position/orientation source of truth

Linear integration:
- a = F * inverseMass
- v += a * dt
- x += v * dt

Angular integration:
- alpha = accumulatedTorque * inverseMomentOfInertia
- omega += alpha * dt
- orientation integrates from omega using axis-angle quaternion for omega*dt

After integration, accumulated force and accumulated torque are cleared. Linear/angular velocities persist.

Gravity is COMPLETE:
- Kinetics owns world gravity {0,-9.81,0}
- before integration it applies mass * gravity * gravityScale to gravity-enabled bodies
- mass-independent free fall was tested successfully

## Aircraft architecture
Current flow:
AircraftController -> Aircraft -> AircraftKinetics -> KineticBody

Aircraft still performs temporary direct pitch/turn/bank first, then explicitly calls AircraftKinetics::Apply(controlInput). AircraftKinetics owns thrust, directional drag, airflow decomposition, AoA, lift, and stall calculations.

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

## Aerodynamics — COMPLETE for current milestone
Directional drag uses signed projections onto aircraft Forward/Right/Up and per-axis quadratic drag:
`-axis * coefficient * speed * abs(speed)`

Current tuning:
- maxThrust = 20
- forwardDrag = 1
- sideDrag = 3
- verticalDrag = 2
- airBrakePower = 4 (TEMP multiplier over total drag)

AoA:
`angleOfAttack = -atan2(verticalSpeed, forwardSpeed)`
- nose above flight path -> positive AoA
- nose below flight path -> negative AoA
- raw AoA is not clamped

Lift:
- pitchSpeedSquared = forwardSpeed^2 + verticalSpeed^2
- Lift = 0.5 * airDensity * pitchSpeedSquared * wingArea * Cl
- liftDirection = normalize(Right x pitchVelocity)

Current tuning:
- airDensity = 1.225
- wingArea = 2.0
- liftSlope = 4.0 / rad
- stallAngle = 15 deg

Stall curve is validated:
- |AoA| <= 15 deg: Cl = liftSlope * AoA
- 15 < |AoA| < 90 deg: peak Cl = liftSlope * stallAngle, linearly decays toward zero, sign restored from AoA
- |AoA| >= 90 deg: Cl = 0

Observed behavior is plausible: no-throttle spawn mostly falls; powered flight followed by throttle release glides/descends temporarily before losing energy.

## Angular velocity integration — COMPLETE
Commit `f5596a0b27d4dc6d9e18c1e51b5ccdc6dc8e5a16` added world-space angular velocity integration.

Validation:
- debug cube rotates continuously at expected rate
- pre-rotated cube + omega around global Y rotates around GLOBAL Y
- angular zero threshold reduced to 0.001^2

Debug-test state intentionally retained:
- rotating test cube remains until torque/inertia testing is finished
- `m_kb->SetUseGravity(false)` on the aircraft is intentional during rotational debugging

## Torque + scalar inertia — IMPLEMENTED, final scalar-inertia check pending
Commit `3081c8e09ddc6ab9328dea9539a19e535c9e2f5b` added:
- scalar `m_momentOfInertia` + inverse value
- `m_angularAcceleration`
- `m_accumulatedTorque`
- `AddTorque()` accumulator
- `alpha = torque / I`
- `omega += alpha * dt`
- accumulated torque clear at end of Integrate

The implementation structure matches the linear force system. Multiple AddTorque calls in one fixed step intentionally sum before integration; the current three debug calls sum to one equivalent world-space torque `(10,12,6)` for that frame.

One-frame torque behavior is understood and observed correctly: torque changes angular velocity once, then with zero subsequent torque the body keeps rotating at that angular velocity because there is no angular damping yet.

Before marking scalar inertia fully validated, perform one deterministic comparison:
- same one-frame torque with I=2
- repeat from same initial state with I=4
- delta omega for I=4 should be exactly half of I=2

## NEXT IMMEDIATE STEP
After the I=2 vs I=4 comparison passes, mark torque + scalar inertia complete.

Then move to per-axis / diagonal inertia, because an aircraft should resist roll, pitch, and yaw differently. Teach the coordinate-space issue before implementation: inertia belongs naturally to body/local principal axes, while current torque and angular velocity are world-space. Do not jump directly to a full arbitrary inertia tensor or aircraft control torques without understanding the local<->world conversion.

Later progression:
- diagonal/body-space inertia
- angular damping / aerodynamic rotational damping as needed
- aircraft control torques
- stabilization / bank behavior
- then revisit camera Up behavior

## Temporary technical debt
- normal aircraft pitch/turn/bank still directly mutates Transform
- evade root displacement bypasses KineticBody
- evade Body spin is presentation-only
- Aircraft Body lookup assumes child index 0
- direct Aircraft -> AircraftKinetics Apply bypasses normal Component enabled dispatch
- air brake is not a separate aerodynamic contribution
- no induced drag
- sideslip/beta not modeled
- no render interpolation
- camera still uses world Up; revisit after physical bank/stabilization

## Repository
GitHub: https://github.com/mohit-kumar-singh55/AERO_RIOT
Default branch: `master`
