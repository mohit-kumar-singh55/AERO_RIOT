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
- optional generic linear damping being finalized

Linear integration:
- optional damping contributes a force first
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
- fixed in `9ea75dc95b386d679c28508551cb597f841065af` to damp angular VELOCITY rather than angular acceleration
- body-space damping coefficients
- local damping torque = `-localAngularVelocity * angularDamping`
- damping torque is transformed back to world space and added to accumulated torque, so inertia still governs response
- OFF: one-frame torque leaves persistent angular velocity
- ON: angular velocity decays toward zero
- generic damping is intended as opt-in; aircraft disables it and will later use airflow-dependent aerodynamic rotational damping in AircraftKinetics

## Generic linear damping — IMPLEMENTED, API/default cleanup pending
Commit `015edae0b7b948f45396a2407d301e9474bec15a` added:
- `m_useLinearDamping`
- scalar `m_linearDamping`
- damping force contribution inside KineticBody::Integrate
- aircraft explicitly disables both generic linear and angular damping in AircraftKinetics::OnInitialize

Current damping force is correct for mass-independent decay behavior:
`F_damping = -linearVelocity * mass * linearDamping`
Because normal integration later multiplies by inverseMass, this gives damping acceleration `a_damping = -linearVelocity * linearDamping`.

The implementation correctly feeds damping through `AddForce()` rather than modifying velocity directly.

Review cleanup before marking COMPLETE:
- add public GetLinearDamping / SetLinearDamping API; current coefficient is private and effectively fixed at 1.0 unless source is edited
- clamp linear damping coefficient to >= 0 so negative damping cannot inject energy
- current defaults are `useLinearDamping=true`, `linearDamping=1`, `useAngularDamping=true`, `angularDamping=Vector3::One`; preferred engine baseline is opt-in/no artificial damping by default: linear OFF/0 and angular OFF/Zero
- optionally clamp angular damping components to >= 0 as well

Aircraft-specific resistance remains separate:
- AircraftKinetics already owns directional quadratic aerodynamic drag
- generic linear/angular damping should remain disabled for aircraft to avoid double-counting
- aircraft aerodynamic angular damping will later depend on airflow / dynamic pressure

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
Finish generic damping API/default cleanup:
- expose scalar linear damping getter/setter and clamp >= 0
- make generic linear/angular damping opt-in by default
- optionally clamp angular damping coefficients >= 0

Then move to physical aircraft control torques:
- replace temporary direct aircraft pitch/turn/bank
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
