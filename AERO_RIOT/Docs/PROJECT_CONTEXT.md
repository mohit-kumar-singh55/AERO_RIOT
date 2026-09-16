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
- Transform::RotateEulerDegrees uses different quaternion composition order for Local vs World rotation; keep angular-velocity space/composition consistent

## Kinetics / KineticBody
Kinetics is scene-local and owns non-owning KineticBody pointers.

KineticBody currently has:
- mass / inverse mass
- linear velocity, linear acceleration, accumulated force
- useGravity + gravityScale
- angular velocity
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

## Angular velocity integration — COMPLETE for current milestone
Commit `f5596a0b27d4dc6d9e18c1e51b5ccdc6dc8e5a16` added the first angular-velocity-only experiment.

Current conceptual implementation:
- `Vector3 m_angularVelocity`, units rad/s
- world-space angular velocity for this first implementation
- magnitude from dot(omega,omega)
- if above numerical threshold, compute angularSpeed = |omega|
- axis = omega / angularSpeed
- angleThisStep = angularSpeed * fixedDt
- delta quaternion from axis-angle
- compose with current world rotation and write through Transform::SetRotation
- Transform normalizes the stored quaternion

Validation:
- debug cube rotates continuously at the expected rate
- a pre-rotated cube was tested with omega around global Y and confirmed to rotate around GLOBAL Y, validating the chosen world-space quaternion composition
- zero-speed threshold was reduced from 0.01^2 to 0.001^2 so slow angular motion is not cut off too aggressively

Debug-test state intentionally retained for now:
- rotating test cube remains in MainScene until torque/inertia work is also tested
- `m_kb->SetUseGravity(false)` on the aircraft is intentional during the current rotational debug phase; do NOT treat this as an accidental regression or remove it unless the user decides to restore flight testing

The latest user adjustments above may be local if not yet pushed; inspect latest `master` before assuming the repository already contains every small validation tweak.

## NEXT IMMEDIATE STEP — torque + scalar inertia
Add the rotational equivalent of the existing linear force system, deliberately using a simple scalar moment of inertia first before a tensor.

Conceptual rotational integration:
- accumulated torque `tau`
- scalar moment of inertia `I`
- angular acceleration `alpha = tau / I`
- angular velocity `omega += alpha * dt`
- orientation integrates from omega using the already validated quaternion path
- accumulated torque clears once per fixed step

Keep torque/world-space conventions consistent with the current world-space angular velocity experiment.

Validate with the existing debug cube before touching aircraft controls:
- one torque impulse for one fixed step -> angular velocity changes once, then remains constant
- same continuous torque every fixed step -> angular speed increases steadily
- larger scalar inertia under same torque -> slower angular acceleration

Only after scalar torque/inertia is understood and validated should we move to per-axis/diagonal inertia or a full inertia tensor, then replace direct aircraft pitch/turn/bank with physical control torques and stabilization.

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
