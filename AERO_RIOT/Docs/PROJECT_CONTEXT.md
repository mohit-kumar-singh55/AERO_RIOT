# AERO_RIOT — Project Context

Last updated: 2026-09-17

Compact handoff for continuing AERO_RIOT in a fresh chat. Inspect latest `master` before assuming this file is perfectly current.

## Learning workflow
AERO_RIOT is a DirectXTK/C++ 3D fighter-aircraft dogfight project used to learn engine architecture, custom physics/aerodynamics, rendering/HLSL, VFX, cameras, AI, and optimization.

The user manually writes code for learning. Preferred flow: UNDERSTAND -> DESIGN -> IMPLEMENT -> REVIEW -> IMPROVE. Do not provide full copy-paste implementations unless explicitly requested, stuck, or the task is mechanical. Teach hard physics/math/rendering concepts before implementation. The game drives engine development.

## Core engine / lifecycle
Ownership: Game -> SceneManager -> Scene -> GameObjectManager -> GameObject -> Component.

Fixed simulation order:
1. Scene OnFixedUpdate()
2. GameObjectManager FixedUpdate() — gameplay/components submit forces/torques
3. Kinetics Integrate(FixedDeltaTime) — world physics consumes them

Current fixed step: 1/60 s.

Transform conventions:
- local Forward = (0,0,-1), Right = (1,0,0), Up = (0,1,0)
- Transform::GetForward/Right/Up return world-space directions
- Transform normalizes stored rotations
- current angular velocity and accumulated torque convention is WORLD-space

## Kinetics / KineticBody
Current linear state:
- mass / inverse mass
- linear velocity, linear acceleration, accumulated force
- useGravity + gravityScale

Linear integration:
- a = F * inverseMass
- v += a * dt
- x += v * dt

Current angular state:
- per-axis moment of inertia Vector3 in BODY/LOCAL principal axes
- inverse per-axis inertia Vector3
- world-space angular velocity
- world-space angular acceleration
- world-space accumulated torque

Angular velocity integration is COMPLETE:
- omega is rad/s, world-space
- angleThisStep = |omega| * dt
- axis = normalize(omega)
- orientation integrates via axis-angle quaternion
- pre-rotated cube confirmed global-Y omega rotates around GLOBAL Y
- near-zero threshold is 0.001^2

Torque + scalar inertia milestone is COMPLETE / validated:
- AddTorque accumulates until integration
- alpha = torque / I
- omega += alpha * dt
- accumulated torque clears once per fixed step
- one-frame torque changes omega once and rotation persists without damping
- I=2 vs I=4 produced the expected 2:1 angular-acceleration response

## Body-space diagonal inertia — COMPLETE / validated
Commit `146b4026da29dba4a355e068bc6d6309151fc545` changed scalar inertia to Vector3 and added body-space inertia handling. Commit `0c483d3c3b5d00e1b42c450015265e6ae57a9b46` fixed the local angular-acceleration calculation to use inverse inertia.

Current path:
`world torque -> inverse world rotation -> local torque -> component-wise inverse inertia -> local angular acceleration -> world rotation -> world angular acceleration -> world angular velocity`

Key properties:
- `m_momentOfInertia` is BODY/LOCAL-space principal-axis inertia
- `m_inverseMomentOfInertia` is computed component-wise
- accumulated torque remains WORLD-space
- local torque is obtained using inverse body/world rotation
- local angular acceleration uses `torque * inverseInertia`
- local angular acceleration is transformed back to WORLD-space before updating world angular velocity

Validated with a pre-rotated debug cube and non-uniform inertia `I=(1,2,4)`. Rotation around local Z is much slower than X/Y under comparable torque, matching the expected larger Z inertia. Body-axis behavior remains tied to the object's local axes rather than global XYZ.

This is still a simplified diagonal-inertia model. Full gyroscopic coupling / arbitrary inertia tensor is deliberately postponed. The complete rigid-body relation `tau = I*alpha + omega x (I*omega)` is not yet implemented.

## Aircraft architecture / aerodynamics
Current flow:
AircraftController -> Aircraft -> AircraftKinetics -> KineticBody

Aircraft still temporarily mutates Transform directly for normal pitch/turn/bank, then calls AircraftKinetics::Apply(controlInput). This is the major behavior to replace after generic rotational physics is solid.

Aerodynamics milestone is COMPLETE for now:
- directional quadratic drag on Forward/Right/Up projections
- gravity
- AoA = -atan2(verticalSpeed, forwardSpeed)
- lift = 0.5 * rho * Vpitch^2 * S * Cl
- stall curve: linear to 15 deg, then falloff to zero by 90 deg
- observed powered-flight/glide behavior is plausible

Debug-test state intentionally retained:
- rotating/torque debug cube remains for the next rotational-physics tests
- `m_kb->SetUseGravity(false)` on the aircraft is intentional during rotational debugging

## NEXT IMMEDIATE STEP
Add a simple angular damping / rotational drag layer before moving aircraft controls onto torque.

Reason: current angular velocity persists forever after torque stops, which is correct for an ideal isolated rigid body but not useful for an aircraft in air. The next lesson should distinguish:
- generic numerical/gameplay angular damping
- physically motivated aerodynamic rotational damping

Start with a simple controlled damping model and validate it on the debug cube, then use the same rotational state for aircraft control torques.

After damping:
- replace temporary direct aircraft pitch/turn/bank with control torques
- choose body-axis torque mapping for pitch/yaw/roll
- tune per-axis inertia/control authority
- add stabilization / bank behavior
- revisit camera Up behavior after physical bank

Full arbitrary inertia tensor / gyroscopic term remains postponed until justified.

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
- camera still uses world Up

## Repository
GitHub: https://github.com/mohit-kumar-singh55/AERO_RIOT
Default branch: `master`
