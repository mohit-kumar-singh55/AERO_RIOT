# AERO_RIOT — Project Context

Last updated: 2026-09-17

Compact handoff for continuing AERO_RIOT in a fresh chat. Inspect latest `master` before assuming this file is perfectly current.

## Learning workflow
AERO_RIOT is a DirectXTK/C++ 3D fighter-aircraft dogfight project used to learn engine architecture, custom physics/aerodynamics, rendering/HLSL, VFX, cameras, AI, and optimization.

The user manually writes code for learning. Preferred flow: UNDERSTAND -> DESIGN -> IMPLEMENT -> REVIEW -> IMPROVE. Do not provide full copy-paste implementations unless explicitly requested, stuck, or the task is mechanical.

### Game-first design rule
AERO_RIOT is a GAME, not a full flight simulator. Physics should be believable enough to create satisfying, readable, exciting flight, but realism is not a goal by itself.

At meaningful complexity points, explicitly ask: **"Are we going deeper than the game needs?"** Only add extra physical fidelity if it improves gameplay feel, control, readability, tuning, debugging, or learning value that is worth the complexity. Prefer tuned arcade/physical behavior over simulation detail that adds player stress or implementation burden without clear gameplay payoff.

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

## Aircraft aerodynamic pitch damping — WORKING
Commit `54208bb0be84aa153b5bd8442efd1af7dbce9466` fixed pitch damping to use actual local pitch angular velocity.

Current path:
`world angular velocity -> inverse aircraft rotation -> local angular velocity -> omegaLocal.x -> damping torque = -omegaLocal.x * pitchDamping * dynamicPressure -> transform to world -> AddTorque`.

Dynamic pressure currently uses translational speed only:
`q = 0.5 * airDensity * linearVelocity.LengthSquared()`.
This works while the aircraft is moving.

Testing note:
- with gravity disabled, a stationary test aircraft can still receive pitch torque while q=0, so it can keep rotating because aero damping is zero
- once gravity is enabled, the aircraft falls, gains linear velocity, and the current damping starts working naturally even without throttle
- this behavior is acceptable for the current game model; do NOT add a more detailed rotational-airflow model unless gameplay later justifies it
- similarly, do not automatically make all control authority fully airspeed-dependent just for realism; only do so if needed for good flight feel

## NEXT IMMEDIATE STEP
Continue the game-oriented control model rather than deepening the simulator model:
1. Keep current pitch torque + aerodynamic pitch damping foundation.
2. Add yaw and roll control torques one axis at a time.
3. Add corresponding per-axis aerodynamic angular damping.
4. Tune pitch/yaw/roll authority and damping for fun, responsive dogfighting rather than strict realism.
5. Add stabilization / bank behavior where it improves control feel.
6. Revisit camera Up behavior after physical bank.

Possible future fidelity upgrades (only if justified by gameplay): airspeed-based control authority curves, rotational-flow damping from omega x r, gyroscopic coupling, arbitrary inertia tensor, more detailed aerodynamic surfaces.

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

## Repository
GitHub: https://github.com/mohit-kumar-singh55/AERO_RIOT
Default branch: `master`
