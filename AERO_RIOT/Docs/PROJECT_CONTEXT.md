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
- current linear/angular velocities and accumulated force/torque are WORLD-space

## Kinetics / KineticBody
Current linear state:
- mass / inverse mass
- world-space linear velocity and acceleration
- world-space accumulated force
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
- optional generic angular damping using body-space coefficients

## Rotational physics milestones
Angular velocity integration — COMPLETE / validated:
- omega in rad/s, world-space
- orientation integrates by axis-angle quaternion from `|omega| * dt`
- pre-rotated cube confirmed global-Y omega rotates around GLOBAL Y
- near-zero threshold is 0.001^2

Torque + scalar inertia — COMPLETE / validated:
- AddTorque accumulates until integration
- alpha = torque / I
- omega += alpha * dt
- accumulated torque clears once per fixed step
- one-frame torque changes omega once and rotation persists without damping
- I=2 vs I=4 produced expected 2:1 angular-acceleration response

Body-space diagonal inertia — COMPLETE / validated:
- `m_momentOfInertia` and inverse are Vector3 in BODY/LOCAL axes
- path: world torque -> local torque -> component-wise inverse inertia -> local angular acceleration -> world angular acceleration -> world angular velocity
- validated with pre-rotated cube and I=(1,2,4); local Z rotates much more slowly under comparable torque
- full gyroscopic coupling / arbitrary inertia tensor is deliberately postponed

Generic angular damping — COMPLETE / validated:
- initially added in `b67e9f1bf4a029fb1e6be42c2530f58a43ee18b7`
- fixed in `9ea75dc95b386d679c28508551cb597f841065af` to damp angular VELOCITY rather than angular acceleration
- body-space damping coefficients
- conceptual model: local damping torque = `-localAngularVelocity * angularDamping`
- damping torque is transformed back to world space and added to accumulated torque, so inertia still governs the response
- OFF: one-frame torque leaves persistent angular velocity
- ON: angular velocity decays toward zero
- recommended generic default is no damping / opt-in; aircraft will disable generic angular damping and later use airflow-dependent aerodynamic rotational damping in AircraftKinetics

## Generic linear damping vs aerodynamic drag — NEXT DESIGN STEP
There should be a linear-motion counterpart in KineticBody, but call it generic `linear damping` rather than aerodynamic drag.

Generic KineticBody linear damping:
- engine/gameplay convenience for arbitrary rigid bodies
- should oppose current linear velocity
- simplest force model: `F_damping = -k * v`
- should feed through AddForce / accumulated force so mass still governs acceleration
- default should be zero / opt-in
- may be isotropic scalar for simplest generic behavior, or body-space Vector3 if per-axis damping is intentionally desired

Aircraft aerodynamic drag is different and already belongs in AircraftKinetics:
- current aircraft model uses body-axis signed speed projections and quadratic directional drag
- depends on aircraft orientation and velocity, and eventually relative airflow / atmospheric properties
- generic KineticBody linear damping should be disabled for the aircraft once used, to avoid double-counting resistance

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
- rotating/torque debug cube remains for rotational-physics tests
- `m_kb->SetUseGravity(false)` on the aircraft is intentional during rotational debugging

## NEXT IMMEDIATE STEP
Decide and implement generic KineticBody linear damping, analogous to generic angular damping but acting on linear velocity through a force contribution.

Then:
- disable generic linear/angular damping for the aircraft
- replace temporary direct aircraft pitch/turn/bank with physical control torques
- choose body-axis torque mapping for pitch/yaw/roll
- tune per-axis inertia/control authority
- add aircraft-specific aerodynamic angular damping in AircraftKinetics when useful
- add stabilization / bank behavior
- revisit camera Up behavior after physical bank

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
