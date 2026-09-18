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
Current simplified rigid-body model supports mass/inverse mass, force accumulation, gravity, linear integration, body-space diagonal moment of inertia, world angular state, quaternion orientation integration, generic damping, and torque accumulation. Aircraft disables generic damping. Full gyroscopic coupling / arbitrary inertia tensor postponed.

## Aircraft aerodynamics — ACTIVE
Implemented and active:
- thrust
- directional quadratic drag
- gravity
- AoA
- lift
- stall curve (linear to 15 deg, then falloff toward zero by 90 deg)

Stall directly feeds lift via `CalculateLiftCoefficient(angleOfAttack)`; keep this as soft aerodynamic consequence rather than hard orientation clamps unless gameplay later proves necessary.

## Physical aircraft controls — WORKING FOUNDATION
Axes:
- pitch -> local X
- yaw -> local Y
- roll -> local Z (Forward=-Z; turn-right uses negative Z)

Input:
- `pitch +1 = nose up`
- raw gamepad Y inverted
- axial stick threshold suppresses unintended pitch/yaw cross-talk

Current test values:
- pitchTorque=20, yawTorque=10, rollTorque=20
- pitch/yaw/roll damping=1
- turn currently drives yaw directly and roll behavior is being replaced by target-bank assistance

Aerodynamic angular damping uses local angular velocity per axis times dynamic pressure. Dynamic pressure uses translational speed only and is sufficient for the current game model.

## Flight assist / stabilization — CURRENT MILESTONE
Goal:
- normal left-stick X chooses turn/bank intent
- target bank limits normal-turn attitude without hard-clamping aircraft rotation
- releasing turn input makes target bank zero so aircraft auto-levels
- torque-based assistance, not direct Transform snapping
- roll assist first; no forced pitch-to-horizon yet

### Signed bank angle
Commit `09e7f1d098a43ea4f9256089941e027f134e2996` added `CalculateBankAngle()`.

Formula:
- project world Up onto plane perpendicular to aircraft Forward
- compare level reference Up vs aircraft Up around Forward using atan2
- output principal angle [-pi,+pi]

The observed +pi -> -pi wrap is correct.

### Bank target / error — IMPLEMENTED
Commit `796f49782c3062c3d20d10ae0a21a3dd725a4c2a` added:
- currentBankAngle
- targetBankAngle (currently sign-only ±50 deg / 0)
- shortest bankError using `atan2(sin(target-current), cos(target-current))`
- max bank test value 50 deg

Debug values confirm bankError signs:
- as current bank approaches +50 deg, positive error shrinks toward 0
- after overshoot, error becomes negative
- when input released, target=0 and bankError becomes the negative of positive bank
- opposite-turn command produces a large negative shortest error when appropriate

Game-feel improvement to make before/with controller:
- use analog target: `targetBankAngle = turn * m_maxBankAngle` rather than sign-only ±max, so partial stick requests partial bank.

### Near-vertical bank validity
Current code avoids normalizing near-zero levelUp, but still continues into atan2. This is not a complete invalid-bank guard.
When `levelUp.LengthSquared()` is below threshold, bank assist should be skipped for that frame (or helper should report invalid) rather than using the tiny unnormalized vector. Do not over-engineer vertical flight.

### Bank rate
Bank rate is NOT a new integrated field. It is the current signed roll angular velocity derived from existing `KineticBody::GetAngularVelocity()`.
- convert world angular velocity to local/body space (already done in AircraftKinetics)
- local omega.z is angular velocity around +Z
- our intuitive positive bank/right-bank direction is around -Z
- therefore use conceptual `bankRate = -localAngularVelocity.z`
- units: radians/second
- no extra dt and no finite-difference of bank angle needed

## NEXT IMMEDIATE STEP
1. Fix/handle the near-vertical invalid-bank case.
2. Prefer analog target bank: `turn * maxBankAngle`.
3. Compute `bankRate = -localAngularVelocity.z`.
4. Verify signs in debug:
   - banking right -> bankRate positive
   - banking left -> bankRate negative
   - stopped roll -> bankRate near zero
5. Then form PD command:
   - proportional: `Kp * bankError`
   - derivative: `-Kd * bankRate`
   - resulting positive controller command means "bank right"
   - convert that semantic command to physical local Z torque with the required sign (right bank -> local -Z)
6. Replace raw normal roll torque with controller roll torque; yaw remains direct.
7. Tune for dogfight feel.
8. Revisit camera Up behavior after bank assist is stable.

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
