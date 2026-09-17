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

## Aircraft aerodynamic angular damping — CURRENT REVIEW
Commit `d4a497f4c5a5dd133ae3fee66ba818219b1b34c4` added dynamic-pressure-based pitch damping attempt.

Correct part:
- `dynamicPressure = 0.5 * airDensity * linearVelocity.LengthSquared()` is appropriate for the current no-wind model
- damping belongs in AircraftKinetics, not generic KineticBody
- damping torque should be authored in body space then transformed to world before AddTorque

Bug found:
- current damping uses `localTorque.x` (the player control torque) instead of current local pitch angular velocity
- therefore damping only exists while input torque exists; releasing the stick makes damping zero and the aircraft keeps rotating
- at speed, the damping term can also directly overpower/reverse the control torque because it is proportional to control torque rather than rotation rate

Correct conceptual path:
`world angular velocity -> inverse aircraft rotation -> local angular velocity -> local pitch rate omega.x -> pitch damping torque = -omega.x * pitchDamping * dynamicPressure -> world torque -> AddTorque`.

No deltaTime should be multiplied into the damping torque; KineticBody integration applies dt later.

Current `m_pitchDamping = 5.0f` may be strong depending on speed/inertia. Tune only after the formula is corrected. With explicit Euler integration, very large damping relative to inertia and fixed dt can overshoot/reverse angular velocity.

## NEXT IMMEDIATE STEP
1. Fix aerodynamic pitch damping to use local angular velocity X, not local control torque X.
2. Test while moving: release pitch stick and verify pitch rate decays toward zero.
3. Test near zero airspeed: aerodynamic damping should become weak/nearly zero.
4. Tune pitch damping coefficient only after behavior is structurally correct.
5. Then add yaw and roll control torques and corresponding aerodynamic angular damping.
6. Later make control authority itself speed-dependent, because current torque-based control still works at zero airspeed.
7. Add stabilization/bank behavior and revisit camera Up after physical bank.

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
