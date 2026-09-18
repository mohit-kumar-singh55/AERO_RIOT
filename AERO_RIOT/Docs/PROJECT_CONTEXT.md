# AERO_RIOT — Project Context

Last updated: 2026-09-18

Compact handoff for continuing AERO_RIOT in a fresh chat. Inspect latest `master` before assuming this file is perfectly current.

## Learning workflow
AERO_RIOT is a DirectXTK/C++ 3D fighter-aircraft dogfight project used to learn engine architecture, custom physics/aerodynamics, rendering/HLSL, VFX, cameras, AI, and optimization.

The user manually writes code for learning. Preferred flow: UNDERSTAND -> DESIGN -> IMPLEMENT -> REVIEW -> IMPROVE. Do not provide full copy-paste implementations unless explicitly requested, stuck, or the task is mechanical.

### Game-first design rule
AERO_RIOT is a GAME, not a full flight simulator. Physics should be believable enough to create satisfying, readable, exciting flight, but realism is not a goal by itself. Ask periodically: **"Are we going deeper than the game needs?"**

## Engine / physics conventions
- Fixed simulation 1/60 s.
- Forward=(0,0,-1), Right=(1,0,0), Up=(0,1,0).
- accumulated forces/torques and linear/angular velocities are WORLD-space.
- body-space diagonal moment of inertia is implemented.
- aircraft disables generic KineticBody damping and uses aircraft-specific aero damping.

## Aircraft aerodynamics — ACTIVE
Implemented and active:
- thrust
- directional quadratic drag
- gravity
- AoA
- lift
- stall curve (linear to 15 deg, then falls toward zero by 90 deg)

Stall directly affects lift via `CalculateLiftCoefficient(angleOfAttack)`; do not replace it with hard orientation clamps unless gameplay later needs otherwise.

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

Current tuning after commit `45d403980db7bc11b4f901f885f40baf42188d70`:
- pitchTorque=30
- yawTorque=20
- pitch/yaw/roll aero damping=1
- maxBankAngle=50 deg
- bankKp=120
- bankKd=15
- this PD tuning currently feels good to the user

## Roll bank-assist — WORKING
Normal roll is controlled by target-bank PD assistance rather than raw roll torque.

Current flow:
- targetBank = turn * maxBankAngle
- currentBank from horizon-relative signed bank calculation
- bankError = shortest signed angular difference
- bankRate = -localAngularVelocity.z
- rollCommand = Kp*bankError - Kd*bankRate
- semantic +rollCommand = bank right
- local roll torque = (0,0,-rollCommand)
- local torque transformed to WORLD before KineticBody::AddTorque
- old direct local-Z roll input has been removed

Current pushed torque path is structurally correct.

### m_rollTorque status
`m_rollTorque = 20` remains in AircraftKinetics.h but is currently unused.
Do not keep it as dead state:
- either remove it, or preferably repurpose/rename it to `m_maxRollAssistTorque` if adding a PD-output clamp
- with Kp=120 and a 50-deg error, normal initial PD output is ~105, so a cap of 20 would destroy the current good feel
- if a cap is added, choose/tune it high enough to preserve normal response while limiting pathological large commands (e.g. around the current normal maximum rather than the old raw-roll value)

## Signed bank angle / vertical edge case
`CalculateBankAngle()` projects world Up onto the plane perpendicular to aircraft Forward and uses atan2 against aircraft Up. Principal-angle wrap [-pi,+pi] is expected.

Current vertical guard is incomplete:
- code checks `levelUp.LengthSquared() > 0.000001f` before Normalize()
- but if below threshold, it still continues into atan2 and then the PD controller
- therefore bank assist is NOT actually skipped while vertical

Needed design:
- bank-angle calculation needs a validity result, because bank is undefined near vertical and `0` is also a valid bank
- good options: `std::optional<float>` return, or `bool TryCalculateBankAngle(float& outBank)`
- when invalid, skip the bank-assist torque for that frame
- use a gameplay-meaningful vertical cone rather than only an almost-zero numerical epsilon if testing needs it; for example levelUp length-squared around 0.01 corresponds to roughly within 5.7 deg of vertical

Important diagnostic:
- in this simplified diagonal-inertia model, roll-assist torque is local Z and should not directly stop local-X pitch rotation
- if the aircraft cannot finish a backflip, temporarily disable the entire bank-assist PD section and retest
- if the backflip returns, vertical bank-assist instability is involved
- if it still stalls near vertical, inspect pitch control torque vs aerodynamic pitch damping/dynamic pressure instead
- stall/lift itself reduces lift and does not directly clamp orientation

## NEXT IMMEDIATE STEP
1. Remove dead `m_rollTorque` or repurpose it as a max roll-assist torque.
2. Make bank-angle calculation explicitly report valid/invalid.
3. Skip PD bank assist when bank is invalid near vertical.
4. Retest backflip with bank assist on.
5. If backflip still sticks, temporarily disable bank assist completely to isolate whether pitch aero damping is the cause.
6. Only then tune pitch damping/control authority if needed.
7. Revisit camera Up behavior after flight-assist behavior is stable.

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
