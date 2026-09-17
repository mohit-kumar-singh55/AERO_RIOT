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

## Aircraft aerodynamics — ACTIVE
Implemented and currently part of flight behavior:
- thrust
- directional quadratic drag on body Forward/Right/Up
- gravity
- AoA = -atan2(verticalSpeed, forwardSpeed)
- lift = 0.5 * rho * Vpitch^2 * S * Cl
- stall curve linear up to 15 deg AoA, then lift falls toward zero by 90 deg

The stall model is not dead code: `CalculateLiftCoefficient(angleOfAttack)` directly feeds the lift-force calculation. It should remain a soft aerodynamic consequence, not be replaced by hard orientation clamps unless gameplay later proves that necessary.

## Physical aircraft controls — WORKING FOUNDATION
Control axis mapping:
- pitch -> local X
- yaw -> local Y
- roll -> local Z (Forward is -Z, so turn-right uses negative Z for right bank)

Input semantics:
- `controlInput.pitch +1 = nose up`
- raw gamepad Y is inverted in AircraftController
- a small per-axis stick threshold was added in commit `9e8cfba4cd337e3d6ef7ab4d2461ed76eda522b8` to suppress unintended pitch/yaw cross-talk; testing says it feels okay

Pitch:
- torque-based local X control
- pitchTorque=20, pitchDamping=1 test values
- aerodynamic damping uses local omega.x * dynamic pressure

Yaw:
- torque-based local Y control: `-turn * yawTorque`
- yawTorque=10, yawDamping=1 test values
- previous apparent roll during yaw was confirmed to come from tiny simultaneous pitch input, not yaw physics

Roll:
- commit `4ab2714dffdfb9fc60be9601ba4b94f124ebf70b` added physical roll torque and damping
- local Z control: `-turn * rollTorque`
- current test values rollTorque=20, rollDamping=1
- damping is now compact per-axis vector math: `-Vector3(pitchDamping,yawDamping,rollDamping) * localAngularVelocity * dynamicPressure`
- testing reports roll behavior seems structurally okay

Dynamic pressure currently uses translational speed only:
`q = 0.5 * airDensity * linearVelocity.LengthSquared()`.
This is sufficient for the current game model. Do not deepen into detailed rotational-flow simulation unless gameplay justifies it.

## NEXT DESIGN MILESTONE — FLIGHT ASSIST / STABILIZATION
Do NOT hard-clamp aircraft orientation as the primary solution. Pitch/yaw/roll should remain physical enough to allow loops, barrel rolls, evasive maneuvers, etc.

Game-oriented control intent:
- keep stall/lift as aerodynamic behavior rather than replacing it with max-angle clamps
- normal left-stick X should produce a coordinated turn: yaw + bank
- instead of endlessly accumulating raw roll while turn is held, move toward a target bank angle for normal turning
- when horizontal turn input returns to zero, aircraft should automatically level its bank so the player can focus on dogfighting rather than manually correcting attitude
- prefer torque-based stabilization (PD-style controller: angle error + roll-rate damping) over directly snapping/clamping Transform rotation
- start with ROLL auto-level / target-bank assist only; do not automatically force pitch to horizon yet because that can fight intentional climbs/dives/loops
- pitch release should currently just stop pitch rate via damping and preserve attitude; revisit weak pitch assist later only if playtesting wants it
- yaw does not need an orientation clamp
- special maneuvers (evade roll / future barrel-roll actions) can temporarily override normal bank-assist rules

Suggested next lesson: design how to calculate signed bank angle relative to world horizon and how a target-bank controller should generate corrective roll torque.

Camera note: AircraftCameraController still uses WORLD Up in LookAt, so the camera does not roll with the aircraft. Revisit after bank-assist feel is established.

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
- debug rotating cube remains until rotational tests are no longer useful

Possible future fidelity upgrades only if gameplay needs them: airspeed-based control-authority curves, rotational-flow damping from omega x r, gyroscopic coupling, arbitrary inertia tensor, detailed aerodynamic surfaces.

## Repository
GitHub: https://github.com/mohit-kumar-singh55/AERO_RIOT
Default branch: `master`
