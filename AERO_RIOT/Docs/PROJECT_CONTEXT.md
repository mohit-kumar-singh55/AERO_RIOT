# AERO_RIOT — Project Context

Last updated: 2026-09-18

Compact handoff for continuing AERO_RIOT in a fresh chat. Inspect latest `master` before assuming this file is perfectly current.

## Learning workflow
AERO_RIOT is a DirectXTK/C++ 3D fighter-aircraft dogfight project used to learn engine architecture, custom physics/aerodynamics, rendering/HLSL, VFX, cameras, AI, and optimization.

The user manually writes code for learning. Preferred flow: UNDERSTAND -> DESIGN -> IMPLEMENT -> REVIEW -> IMPROVE. Do not provide full copy-paste implementations unless explicitly requested, stuck, or the task is mechanical.

### Game-first design rule
AERO_RIOT is a GAME, not a full flight simulator. Physics should be believable enough to create satisfying, readable, exciting flight, but realism is not a goal by itself. At meaningful complexity points ask: **"Are we going deeper than the game needs?"**

## Engine / physics conventions
- Fixed simulation 1/60 s.
- Forward=(0,0,-1), Right=(1,0,0), Up=(0,1,0).
- accumulated forces/torques and linear/angular velocities are WORLD-space.
- body-space diagonal moment of inertia is implemented.
- aircraft disables generic KineticBody linear/angular damping and uses aircraft-specific aerodynamic behavior.

## Aircraft aerodynamics — ACTIVE
Implemented and active:
- thrust
- directional quadratic drag
- gravity
- AoA
- lift
- stall curve (linear to 15 deg, then falls toward zero by 90 deg)

Stall directly affects lift via `CalculateLiftCoefficient(angleOfAttack)`; keep it as a soft aerodynamic consequence rather than hard orientation clamps unless gameplay later needs otherwise.

## Physical controls — WORKING FOUNDATION
Axes:
- pitch -> local X
- yaw -> local Y
- roll -> local Z
- Forward=-Z; semantic right bank corresponds to physical local -Z rotation

Input:
- pitch +1 = nose up
- raw gamepad Y inverted
- axial stick threshold suppresses unwanted cross-axis input

Current test values:
- pitchTorque=20, yawTorque=10, rollTorque=20
- pitch/yaw/roll aero damping=1
- yaw remains direct from turn input
- normal roll is being transitioned from raw torque to target-bank PD assistance

Aerodynamic angular damping uses local angular velocity per axis times dynamic pressure.

## Flight assist / stabilization — CURRENT
Goal:
- turn input chooses target bank
- release turn -> target bank 0 -> auto-level
- torque-based assistance, not Transform snapping/clamping
- no forced pitch-to-horizon yet

### Signed bank angle
`CalculateBankAngle()` projects world Up onto plane perpendicular to aircraft Forward and compares it with aircraft Up using atan2. Output wraps in [-pi,+pi], which is expected.

Near-vertical note:
- projected levelUp becomes near-zero when Forward ~ WorldUp
- current helper still needs a real invalid-bank handling path; simply skipping Normalize() but continuing atan2 is not sufficient
- skip bank assist for that frame rather than over-engineering vertical handling

### Target / error / rate
Implemented:
- analog target bank: `targetBankAngle = turn * maxBankAngle`, max currently 50 deg
- shortest signed error: `atan2(sin(target-current), cos(target-current))`
- bankRate = `-localAngularVelocity.z`
- debug testing confirms signs:
  - right bank rate positive
  - left bank rate negative
  - error crosses through zero correctly
  - release stick gives target 0 and opposite-signed leveling error

### PD controller — IMPLEMENTED BUT CURRENT REVIEW FOUND 2 ISSUES
Commit `19d48de5bb27a7716680b741a8595526cc0dfa8f` added:
`rollCommand = bankKp * bankError - bankKd * bankRate`
with test values Kp=1.5, Kd=2.0.

Important:
- rollCommand is a SEMANTIC bank-right/bank-left controller output
- positive rollCommand means "bank right"
- physical right-bank torque is local -Z
- therefore local assist torque should use Z = `-rollCommand`
- AddTorque expects WORLD torque, so local assist torque must be transformed by aircraft rotation before AddTorque
- current pushed line `AddTorque({0,0,rollCommand})` is wrong because it applies world-Z torque and has the semantic sign reversed for this project

Also, the old raw roll torque is still active in localTorque:
`-controlInput.turn * m_rollTorque`
This must be removed/zeroed for normal roll once the PD controller owns roll, otherwise raw roll and PD roll are both applied simultaneously.

Debug note:
- `rollCommand` is not an angle, so do not convert it with XMConvertToDegrees. Print it as a raw controller/torque-like value.

## NEXT IMMEDIATE STEP
1. Remove raw normal roll torque from the direct control torque vector; keep pitch X and yaw Y direct.
2. Convert PD roll command to local torque around -Z:
   semantic +right -> physical local -Z.
3. Transform that local assist torque to WORLD using aircraft rotation.
4. Add world assist torque through KineticBody.
5. Keep aero roll damping active.
6. Test:
   - half stick -> roughly half max bank target
   - full stick -> ~50 deg target
   - release -> auto-level toward 0
   - tune Kp/Kd based on overshoot/response
7. Fix near-vertical invalid-bank handling before relying on assist during vertical flight.
8. Revisit camera Up behavior after bank assist is stable.

## Temporary technical debt
- evade root displacement bypasses KineticBody
- evade Body spin presentation-only
- Aircraft Body lookup assumes child index 0
- direct Aircraft -> AircraftKinetics Apply bypasses normal Component enabled dispatch
- air brake still simplified
- no induced drag / sideslip model / render interpolation
- camera still uses world Up

Possible future fidelity upgrades only if gameplay needs them: airspeed-based control-authority curves, rotational-flow damping, gyroscopic coupling, arbitrary inertia tensor, detailed aerodynamic surfaces.

## Repository
GitHub: https://github.com/mohit-kumar-singh55/AERO_RIOT
Default branch: `master`
