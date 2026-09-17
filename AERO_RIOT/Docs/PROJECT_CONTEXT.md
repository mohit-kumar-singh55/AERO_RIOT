# AERO_RIOT — Project Context

Last updated: 2026-09-17

Compact handoff for continuing AERO_RIOT in a fresh chat. Inspect latest `master` before assuming this file is perfectly current.

## Learning workflow
AERO_RIOT is a DirectXTK/C++ 3D fighter-aircraft dogfight project used to learn engine architecture, custom physics/aerodynamics, rendering/HLSL, VFX, cameras, AI, and optimization.

The user manually writes code for learning. Preferred flow: UNDERSTAND -> DESIGN -> IMPLEMENT -> REVIEW -> IMPROVE. Do not provide full copy-paste implementations unless explicitly requested, stuck, or the task is mechanical.

## Core engine / lifecycle
Ownership: Game -> SceneManager -> Scene -> GameObjectManager -> GameObject -> Component.
Fixed simulation: gameplay/components submit forces/torques, then Kinetics integrates at fixed 1/60 s.

Transform conventions:
- Forward = (0,0,-1), Right = (1,0,0), Up = (0,1,0)
- Transform GetForward/Right/Up return world-space directions
- linear/angular velocities and accumulated force/torque are WORLD-space

## KineticBody status
Current simplified rigid-body model supports:
- mass/inverse mass, force accumulation, gravity
- linear velocity/acceleration/integration
- body-space diagonal moment of inertia Vector3 + inverse
- world angular velocity/acceleration/torque accumulation
- axis-angle quaternion orientation integration
- generic scalar linear damping and body-space angular damping
- generic bodies use damping by default
- aircraft disables both generic dampings
- full gyroscopic coupling / arbitrary inertia tensor postponed

Generic linear damping: `F = -v * mass * linearDamping`, giving mass-independent `a = -v * linearDamping`.
Generic angular damping: local torque `-omegaLocal * angularDamping` transformed back to world.

## Aircraft aerodynamics already implemented
- thrust
- directional quadratic drag on body Forward/Right/Up
- gravity
- AoA = -atan2(verticalSpeed, forwardSpeed)
- lift = 0.5 * rho * Vpitch^2 * S * Cl
- stall curve linear to 15 deg then falloff to zero by 90 deg

## Physical aircraft control torques — IN PROGRESS
Commit `ce88a7242077c68b169b189f54d158d3a77cfb17` replaced direct normal pitch rotation with body-space pitch torque in AircraftKinetics.

Control axis mapping:
- pitch -> local X
- yaw -> local Y
- roll -> local Z, sign handled carefully because Forward=-Z

Input semantics:
- `controlInput.pitch +1 = nose up`
- commit `d4a497f4c5a5dd133ae3fee66ba818219b1b34c4` inverted raw gamepad Y in AircraftController so pulling stick back/down gives positive pitch-up input

Pitch control torque path is structurally correct:
`pitch input -> local X torque -> aircraft rotation -> world torque -> KineticBody::AddTorque`.

Current tuning after commit `54208bb0be84aa153b5bd8442efd1af7dbce9466`:
- pitchTorque = 20
- pitchDamping = 1
These are test/tuning values, not final aircraft data.

## Aircraft aerodynamic pitch damping — STRUCTURALLY CORRECT
Commit `54208bb0be84aa153b5bd8442efd1af7dbce9466` fixed pitch damping to use actual local pitch angular velocity.

Current path:
`world angular velocity -> inverse aircraft rotation -> local angular velocity -> omegaLocal.x -> damping torque = -omegaLocal.x * pitchDamping * dynamicPressure -> transform to world -> AddTorque`.

Dynamic pressure currently uses translational speed only:
`q = 0.5 * airDensity * linearVelocity.LengthSquared()`.
This is appropriate for the current simple no-wind model and works while the aircraft is moving.

Important limitation exposed by testing:
- at zero linear velocity, q = 0, so this aerodynamic pitch damping becomes zero
- the aircraft can still rotate in the current test because pitch control torque is currently available even at zero airspeed
- therefore a stationary aircraft can acquire angular velocity and keep rotating forever in this temporary model
- this is not a bug in the corrected damping code; it is a mismatch between simplified aerodynamic damping and speed-independent control torque
- real rotating geometry in still air would also experience some rotational aerodynamic resistance because different parts sweep through the air, but that requires a more detailed rotational-airflow model than the current translational-q approximation

Do not patch this by re-enabling generic angular damping on the aircraft unless intentionally choosing an arcade baseline. For current learning progression, keep generic aircraft damping disabled so aerodynamic behavior stays visible.

## NEXT IMMEDIATE STEP
Decide how aircraft control authority should behave with airspeed before expanding all axes.
Recommended progression:
1. Keep current pitch damping model as the moving-aircraft aerodynamic damping foundation.
2. Make aerodynamic control torque itself depend on airflow/dynamic pressure (or a controlled gameplay curve), so at zero airspeed normal aerodynamic pitch authority becomes weak/zero rather than allowing free in-place rotation.
3. Validate pitch response across low/medium/high speed.
4. Then add yaw and roll control torques with per-axis aerodynamic damping.
5. Add stabilization/bank behavior and revisit camera Up after physical bank.

Possible future fidelity upgrade, only if justified: rotational-flow damping based on local surface velocity from angular motion, rather than relying only on center-of-mass translational airspeed.

## Temporary technical debt
- evade root displacement bypasses KineticBody
- evade Body spin is presentation-only
- Aircraft Body lookup assumes child index 0
- direct Aircraft -> AircraftKinetics Apply bypasses normal Component enabled dispatch
- air brake is not a separate aerodynamic contribution
- no induced drag
- sideslip/beta not modeled
- no render interpolation
- camera still uses world Up
- debug rotating cube remains until no longer useful
- aircraft gravity is intentionally disabled during current rotational debugging

## Repository
GitHub: https://github.com/mohit-kumar-singh55/AERO_RIOT
Default branch: `master`
