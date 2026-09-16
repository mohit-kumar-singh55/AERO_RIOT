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
- Transform::RotateEulerDegrees uses different quaternion composition order for Local vs World rotation; this matters for angular velocity integration

## Kinetics / KineticBody
Kinetics is scene-local and owns non-owning KineticBody pointers.

KineticBody currently has:
- mass / inverse mass
- linear velocity, linear acceleration, accumulated force
- useGravity + gravityScale
- angular velocity (new, experimental)
- Transform remains position/orientation source of truth

Linear integration uses semi-implicit Euler:
- a = F * inverseMass
- v += a * dt
- x += v * dt
- clear accumulated force

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

Lift model:
- pitchSpeedSquared = forwardSpeed^2 + verticalSpeed^2
- Lift = 0.5 * airDensity * pitchSpeedSquared * wingArea * Cl
- pitchVelocity = Forward * forwardSpeed + Up * verticalSpeed
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

## Angular dynamics — CURRENT MILESTONE
Commit `f5596a0b27d4dc6d9e18c1e51b5ccdc6dc8e5a16` added the first angular-velocity-only experiment.

Current implementation:
- `Vector3 m_angularVelocity`, units rad/s
- magnitude from dot(omega,omega)
- if above threshold, compute angularSpeed = |omega|
- axis = omega / angularSpeed
- angleThisStep = angularSpeed * fixedDt
- delta quaternion from axis-angle
- compose delta with current world rotation and write through Transform::SetRotation

A debug cube with omega = (0, pi/2, 0) rotates continuously and visually confirms basic angular integration/timing.

### Review findings before marking angular velocity complete
1. `m_angularVelocity` was defined conceptually as WORLD-space, but the current quaternion composition order in KineticBody matches Transform's LOCAL rotation composition convention. Identity-orientation Y rotation cannot reveal this because local/world Y initially coincide. Use Transform's world-space composition convention and verify with a pre-rotated cube where local Y differs from global Y.
2. Current angular zero check uses 0.01 rad/s, which is a gameplay-sized deadzone (~0.57 deg/s), not a numerical epsilon. Replace it with a much smaller numerical threshold so legitimate slow angular velocity does not freeze.
3. MainScene currently calls `m_kb->SetUseGravity(false)` on the aircraft for the angular debug experiment. Remove/restore it after the test or later flight behavior is invalid.
4. Remove the temporary rotating debug cube once this isolated test is complete.

Transform already normalizes rotations, so KineticBody does not need duplicate explicit quaternion normalization unless that architecture changes later.

## NEXT IMMEDIATE STEP
Fix and validate angular velocity space/composition:
- keep angular velocity WORLD-space for this first implementation
- use the same quaternion composition convention as Transform's world-space rotation path
- start the debug cube with a non-identity orientation, then set omega around global Y and confirm it rotates around global Y rather than its tilted local Y
- reduce the zero-speed check to a true numerical epsilon
- restore aircraft gravity/remove temporary test state

After this passes, angular velocity integration is COMPLETE.

Then teach/implement the next rotational layer:
- accumulated torque
- angular acceleration
- simple scalar inertia first
- alpha = torque / I
- omega += alpha * dt
- clear accumulated torque
- validate with isolated rotational experiments before full inertia tensor or aircraft control torques

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
