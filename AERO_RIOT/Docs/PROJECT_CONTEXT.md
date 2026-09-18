# AERO_RIOT — Project Context

Last updated: 2026-09-18

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

The stall model is active: `CalculateLiftCoefficient(angleOfAttack)` directly feeds lift force. Keep it as a soft aerodynamic consequence rather than replacing it with hard orientation clamps unless gameplay later proves that necessary.

## Physical aircraft controls — WORKING FOUNDATION
Control axis mapping:
- pitch -> local X
- yaw -> local Y
- roll -> local Z (Forward is -Z, so turn-right uses negative Z for right bank)

Input semantics:
- `controlInput.pitch +1 = nose up`
- raw gamepad Y is inverted in AircraftController
- small per-axis stick threshold added in commit `9e8cfba4cd337e3d6ef7ab4d2461ed76eda522b8` suppresses unintended pitch/yaw cross-talk

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
- damping uses compact per-axis vector math: `-Vector3(pitchDamping,yawDamping,rollDamping) * localAngularVelocity * dynamicPressure`

Dynamic pressure currently uses translational speed only:
`q = 0.5 * airDensity * linearVelocity.LengthSquared()`.
This is sufficient for the current game model.

## Flight assist / stabilization — CURRENT MILESTONE
Do NOT hard-clamp aircraft orientation as the primary solution. Pitch/yaw/roll should remain capable of loops, barrel rolls, evasive maneuvers, etc.

Game-oriented intent:
- normal left-stick X produces coordinated yaw + bank
- instead of endlessly accumulating raw roll, use a target bank angle for normal turning
- when turn input returns to zero, target bank becomes zero and aircraft auto-levels
- use torque-based stabilization, not direct Transform snapping/clamping
- start with roll auto-level / target-bank assist only
- do not force pitch back to horizon yet; preserve intentional climbs/dives/loops
- special maneuvers can later override normal bank assist

### Signed bank angle — IMPLEMENTED
Commit `09e7f1d098a43ea4f9256089941e027f134e2996` added `AircraftKinetics::CalculateBankAngle()`.

Current calculation:
- get aircraft world Forward and Up
- project world Up onto plane perpendicular to Forward:
  `levelUp = WorldUp - Forward * dot(Forward, WorldUp)`
- normalize levelUp
- signed bank:
  `atan2(Forward dot (levelUp cross Up), levelUp dot Up)`

Observed wrap during full rotation:
- value reaches just under +pi (~3.1395)
- then wraps to just over -pi (~-3.1383)
- this is expected because atan2 returns the principal angle in [-pi, +pi]
- continuing the same physical roll then moves from -pi back toward 0

Important remaining guard:
- when Forward is nearly parallel to WorldUp, projected `levelUp` becomes near-zero and bank is not uniquely defined
- before Normalize(), check `levelUp.LengthSquared()` against a small threshold
- if too small, skip bank assist / report no valid bank for that frame rather than normalizing an almost-zero vector
- do not over-engineer vertical-flight bank handling unless gameplay later needs it

## NEXT IMMEDIATE STEP
1. Add the near-vertical guard to `CalculateBankAngle()`.
2. Then build roll target-bank / auto-level assistance as a PD-style torque controller:
   - target bank from turn input (0 input -> 0 bank, full turn -> tuned max bank)
   - bank error = shortest signed angular difference target-current
   - proportional term pushes toward target angle
   - derivative term opposes current local roll rate
   - output is a roll-assist torque added through KineticBody
3. Tune for dogfight feel, not simulation accuracy.
4. Revisit camera Up behavior after bank assist feels good.

Camera note: AircraftCameraController still uses WORLD Up in LookAt, so the camera does not roll with the aircraft.

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

Possible future fidelity upgrades only if gameplay needs them: airspeed-based control-authority curves, rotational-flow damping from omega x r, gyroscopic coupling, arbitrary inertia tensor, detailed aerodynamic surfaces.

## Repository
GitHub: https://github.com/mohit-kumar-singh55/AERO_RIOT
Default branch: `master`
