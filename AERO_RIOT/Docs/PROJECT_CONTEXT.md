# AERO_RIOT — Project Context

Last updated: 2026-09-17

Compact handoff for continuing AERO_RIOT in a fresh chat. Inspect latest `master` before assuming this file is perfectly current.

## Learning workflow
AERO_RIOT is a DirectXTK/C++ 3D fighter-aircraft dogfight project used to learn engine architecture, custom physics/aerodynamics, rendering/HLSL, VFX, cameras, AI, and optimization.

The user manually writes code for learning. Preferred flow: UNDERSTAND -> DESIGN -> IMPLEMENT -> REVIEW -> IMPROVE. Do not provide full copy-paste implementations unless explicitly requested, stuck, or the task is mechanical.

### Game-first design rule
AERO_RIOT is a GAME, not a full flight simulator. Physics should be believable enough to create satisfying, readable, exciting flight, but realism is not a goal by itself.

At meaningful complexity points, explicitly ask: **"Are we going deeper than the game needs?"** Only add extra physical fidelity if it improves gameplay feel, control, readability, tuning, debugging, or worthwhile learning value. Prefer tuned arcade/physical behavior over simulation detail that adds stress or complexity without clear payoff.

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
Control axis mapping:
- pitch -> local X
- yaw -> local Y
- roll -> local Z, sign handled carefully because Forward=-Z

Input semantics:
- `controlInput.pitch +1 = nose up`
- raw gamepad Y is inverted in AircraftController so pulling stick back/down gives positive pitch-up input

Pitch:
- commit `ce88a7242077c68b169b189f54d158d3a77cfb17` replaced direct pitch Transform rotation with body-space torque
- current test tuning after `54208bb0be84aa153b5bd8442efd1af7dbce9466`: pitchTorque=20, pitchDamping=1
- pitch damping uses local angular velocity X and dynamic pressure; structurally correct and working while moving

Yaw:
- commit `c1e25aca50b5d3e963a9d929e90c7f76c136d142` added body-space yaw torque and yaw aerodynamic damping
- yaw control torque is `-controlInput.turn * yawTorque` on local Y
- yaw damping is `-localAngularVelocity.y * yawDamping * dynamicPressure`
- current test values: yawTorque=10, yawDamping=1
- gravity is enabled again for aircraft testing
- yaw implementation is structurally correct; no intentional local Z/roll torque is added

## Yaw cross-axis diagnostic — CONFIRMED
Observed: holding yaw for a while seemed to develop roll/bank.

Diagnostic result:
- setting pitch torque to zero makes the apparent roll disappear
- therefore yaw torque itself is not the source
- the left stick feeds both pitch and yaw; DirectXTK circular dead zone removes center drift but still allows a small perpendicular Y component while the stick is held mostly sideways
- with pitchTorque=20 and yawTorque=10, even a small unintended pitch input can accumulate noticeably over time
- simultaneous pitch+yaw produces a combined rotation/orientation that can visually resemble bank even without explicit local-Z torque

Recommended game-oriented input fix before adding roll:
- keep circular dead zone for normal stick feel, but add a small per-axis/axial dead zone (or equivalent input shaping) in AircraftController so tiny perpendicular components are zeroed
- do not snap all diagonal input away; intentional diagonal pitch+yaw should still work
- tune the axial threshold by feel rather than realism, likely small enough to suppress cross-talk without making the stick feel notchy

Camera note: AircraftCameraController still uses WORLD Up in LookAt, so the camera does not roll with the aircraft. This can make attitude/bank more visually obvious but does not create physical roll.

## Aircraft aerodynamic pitch/yaw damping
Dynamic pressure currently uses translational speed only:
`q = 0.5 * airDensity * linearVelocity.LengthSquared()`.
This is sufficient for the current game model.

With gravity disabled, a stationary aircraft could receive control torque while q=0 and retain angular velocity. With gravity enabled, the aircraft falls, gains speed, and damping starts working naturally. This is acceptable; do not deepen into rotational surface-airflow simulation unless gameplay justifies it.

## NEXT IMMEDIATE STEP
1. Add small per-axis input dead-zone/shaping for left-stick pitch/yaw cross-talk.
2. Re-test pure yaw with pitch torque restored.
3. Once yaw remains clean, add physical roll torque on local Z.
4. Add roll aerodynamic damping.
5. Tune pitch/yaw/roll authority and damping for fun, responsive dogfighting rather than strict realism.
6. Add stabilization / coordinated bank behavior only where it improves control feel.
7. Revisit camera Up behavior after physical bank.

Possible future fidelity upgrades (only if justified by gameplay): airspeed-based control authority curves, rotational-flow damping from omega x r, gyroscopic coupling, arbitrary inertia tensor, detailed aerodynamic surfaces.

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
