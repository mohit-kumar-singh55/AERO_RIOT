# AERO_RIOT — Project Context

Last updated: 2026-09-18

Compact handoff for continuing AERO_RIOT. Inspect latest `master` before assuming this file is perfectly current.

## Learning workflow
AERO_RIOT is a DirectXTK/C++ 3D fighter-dogfight game used to learn engine architecture, custom physics/aerodynamics, rendering/HLSL, VFX, cameras, AI, and optimization.

The user manually writes code to learn. Preferred flow:
UNDERSTAND -> DESIGN -> IMPLEMENT -> REVIEW -> IMPROVE.
Do not provide full copy-paste implementations unless explicitly requested, stuck, or the task is mechanical.

### Game-first rule
AERO_RIOT is a GAME, not a simulator. Physics should feel believable and useful for dogfighting, but gameplay/readability/control take priority over realism.

## Engine / physics conventions
- fixed simulation 1/60 s
- Forward=(0,0,-1), Right=(1,0,0), Up=(0,1,0)
- accumulated force/torque and linear/angular velocity are WORLD-space
- body-space diagonal moment of inertia implemented
- aircraft currently disables generic KineticBody damping and uses aircraft-specific aero damping

## Aircraft aerodynamics — ACTIVE
Implemented:
- thrust
- directional quadratic drag
- gravity
- AoA
- lift
- stall curve: linear to 15 deg AoA, then lift falls toward zero by 90 deg

Stall directly affects lift; it does not clamp orientation.

## Physical controls
Axes:
- pitch -> local X
- yaw -> local Y
- roll -> local Z
- Forward=-Z; semantic right bank corresponds to local -Z rotation

Input:
- pitch +1 = nose up
- raw gamepad Y inverted
- axial stick threshold suppresses cross-axis noise

Current important control state:
- pitchTorque=30
- yawTorque=20
- bank target max=50 deg
- bank PD recently reduced from Kp/Kd 120/15 to about 40/10 and is more stable
- aero angular damping was reduced from 1.0 to a much smaller value (~0.05 diagnostic region) to stop high-speed numerical explosion/NaN

## Roll bank assist
Normal roll uses target-bank PD:
- targetBank = turn * maxBankAngle
- bankError = shortest signed target-current difference
- bankRate = -localAngularVelocity.z
- rollCommand = Kp*bankError - Kd*bankRate
- local roll torque = (0,0,-rollCommand), transformed to world

`CalculateBankAngle()` returns `std::optional<float>` and skips near the vertical singularity.

Backflip conflicts with always-on horizon-relative bank assist. User currently prioritizes reliable auto-assist over clean backflips, so the upright-only (>0 Up dot WorldUp) condition is commented out.

## Minimum forward speed / glide
Force-based minimum-speed assist exists:
- minForwardSpeed ~12
- minSpeedGain ~6
- maxSpeedAssistAcceleration ~8

Forward drag was lowered to improve glide, which exposed high-speed control instability. With forwardDrag=0.2:
- full-throttle equilibrium ~22
- zero-throttle equilibrium ~6.5
- coast-down still too fast because quadratic forward drag both limits top speed and determines glide

Longer-term design direction:
- decouple top-speed control from base forward drag
- use lower base forward drag for momentum/glide
- use game-oriented propulsion/desired-speed control for min/max cruise
- air brake remains explicit fast-deceleration control

Do not resume this redesign until rotational behavior is stable.

## High-speed NaN — DIAGNOSED
Aerodynamic angular damping:
`tau = -k * omegaLocal * dynamicPressure`
with `q = 0.5*rho*V^2` became numerically unstable under explicit Euler when k=1 and speed rose ~22+, causing sign-flip/amplification of omega and eventual NaN.

Reducing aero damping coefficients into ~0.05–0.1 range stopped value explosion. forwardDrag=1 only masked the bug by keeping speed low.

## CURRENT CORE ISSUE — RAW TORQUE INPUT CAUSES RUNAWAY ANGULAR SPEED
Further testing showed the problem is not yaw-only. Holding the stick in any direction can make rotation continue accelerating, even with no player throttle. Built-in generic angular damping on/off did not materially solve it.

Latest user tuning (not yet pushed at time of review):
- bank Kp/Kd reduced to about 40/10 and seemed more stable
- lower aero damping stopped NaN/explosion
- still gets continual angular acceleration while stick is held

Root architectural cause:
- pitch currently uses raw local X torque proportional to stick input
- yaw currently uses raw local Y torque proportional to turn input
- holding input therefore continuously applies angular acceleration
- aero damping is speed-dependent and can become weak at low speed
- generic damping is not a good primary control solution; even if enabled with coefficient 1, torque 30/20 implies enormous terminal rates (~30 or 20 rad/s with simple linear damping)
- current repo still disables generic angular damping in AircraftKinetics::OnInitialize; local user experiments may differ

Game-first control redesign:
**stick should command angular RATE, not raw torque, for pitch/yaw.**
Use a rate controller:
- targetPitchRate = pitchInput * maxPitchRate
- currentPitchRate = localAngularVelocity.x
- pitchRateError = targetPitchRate - currentPitchRate
- pitchTorqueCommand = pitchRateKp * pitchRateError, clamped to max pitch torque

Yaw semantic convention:
- current semantic yaw rate = -localAngularVelocity.y
- targetYawRate = turnInput * maxYawRate
- yawRateError = targetYawRate - currentYawRate
- yawTorqueCommand = yawRateKp * yawRateError
- physical local Y torque = -yawTorqueCommand

Behavior:
- hold stick -> rate rises toward finite target and stops accelerating
- release stick -> target rate becomes zero and controller actively brakes rotation
- works at low speed even when aero damping is weak
- physics still uses torque/inertia; controller just decides torque from rate error

Start with a simple P rate controller; no D term yet. This is already equivalent to drive + damping around a target angular velocity.

Roll can remain the existing target-bank PD controller for now. If it later needs the same robustness, use a cascaded bank-angle -> target-roll-rate -> roll-rate controller, but that is deeper than needed today.

During pitch/yaw rate-controller tuning:
- temporarily keep aero pitch/yaw damping very small or zero to avoid hiding controller behavior
- generic angular damping is not needed as the main solution
- add torque clamps to pitch/yaw commands
- choose gameplay max rates in radians/sec (e.g. around 60–120 deg/s as starting test ranges, tuned independently)
- after stable controls, reintroduce small aero damping only for feel if desired

## CURRENT ISSUE — LOW-SPEED / BRAKE YAW RUNAWAY
After reducing bank PD to ~40/10, overall roll behavior is more stable, but Y-axis rotation can still start accelerating automatically, especially when holding air brake and turning.

This behavior is now well explained by control/damping structure:
- direct yaw control is raw torque: `localTorque.y = -turn * yawTorque`
- yaw control torque remains full-strength regardless of airspeed
- aero yaw damping is proportional to dynamic pressure ~ V^2
- air brake rapidly lowers speed
- as speed falls, yaw damping collapses toward zero
- while turn input remains held, constant yaw torque continues integrating angular velocity
- result: local omega.y keeps increasing; if stick is released at low speed, weak damping means spin persists for too long

This also explains the user's fun emergent "secret maneuver": braking at low speed + stick gives a very sharp arcade post-stall-style turn. User likes this behavior and does not want to lose it, but runaway acceleration must be bounded.

Game-first preferred fix:
- DO NOT make yaw control authority strongly airspeed-dependent yet; that would kill the fun low-speed snap maneuver
- add a small always-on BASE yaw angular damping independent of airspeed, while keeping aero damping on top
- existing KineticBody generic body-space angular damping can provide this; aircraft currently disables it
- minimal test: enable generic angular damping for aircraft but set only Y nonzero, e.g. `SetAngularDamping({0, baseYawDamping, 0})`, while keeping linear damping disabled
- this makes low-speed full-stick yaw approach a finite terminal rate instead of accelerating forever
- approximate low-speed steady yaw rate: `|omegaY| ~= yawTorque / baseYawDamping`
- with yawTorque=20, baseYawDamping=10 -> ~2 rad/s (~115 deg/s); base=5 -> ~4 rad/s (~229 deg/s)
- start with a value that preserves the fun sharp turn, then tune by feel
- after this works, consider whether pitch/roll also need small baseline damping, but do not disturb them preemptively

Alternative future solution if more deterministic arcade control is desired:
- replace raw yaw torque with a yaw-rate controller (stick -> target yaw rate -> torque)
- air brake could intentionally raise max yaw rate to formalize the secret maneuver
- do not do this yet unless baseline damping is insufficient

Do not hard-clamp angular velocity as the first fix.

## PITCH/YAW RATE CONTROLLERS — IMPLEMENTED AND WORKING
Commit `257c08eceedcddf01ea53ac55516cfa20a260b0b` replaced raw pitch/yaw torque input with proportional angular-rate control. Commit `f58edc064bb8054dd4f07891457f39e22943c79e` corrected yaw torque sign.

Current hardcoded prototype:
- currentPitchRate = localAngularVelocity.x
- currentYawRate = -localAngularVelocity.y
- targetPitchRate = pitchInput * 1.1 rad/s
- targetYawRate = turnInput * 1.1 rad/s
- pitchError = target-current
- yawError = target-current
- local torque X = 2.0 * pitchError
- local torque Y = -2.0 * yawError
- local torque Z = 0; roll remains bank-angle PD controlled

User testing: seems to work; sustained stick no longer produces unlimited angular acceleration.

Review:
- architecture/signs are correct
- hardcoded 1.1 and 2.0 are fine for proof-of-concept but should become named fields
- existing m_pitchTorque/m_yawTorque fields are now unused; prefer repurposing them as max pitch/yaw torque clamps rather than deleting them
- add torque clamp after rate-controller output so large opposite-rate errors cannot produce unbounded torque
- suggested field concepts: maxPitchRate, maxYawRate, pitchRateKp, yawRateKp, maxPitchTorque, maxYawTorque
- 1.1 rad/s ~= 63 deg/s; tune later for gameplay

Important: aero angular damping is still active at ~0.05 on pitch/yaw/roll. The rate controller itself already contains a damping term:
`torque = K*(targetRate-currentRate) = K*targetRate - K*currentRate`.
Therefore pitch/yaw aero damping is no longer required for basic stability and can make control authority strongly speed-dependent. At high speed q becomes large, so even 0.05 aero damping can substantially reduce achieved pitch/yaw rate below the requested target. For consistent arcade controls, next test should set pitch/yaw aero damping to 0 (or extremely small), while keeping the rate controller responsible for stopping rotation. Roll bank PD already contains a derivative/rate term as well.

## NEXT IMMEDIATE STEP
1. Replace hardcoded 1.1 and 2.0 with named pitch/yaw rate-controller fields.
2. Repurpose old pitch/yaw torque values as maximum controller torque clamps.
3. Clamp pitch/yaw torque commands before local->world transform.
4. Temporarily set pitch/yaw aero angular damping to zero and test at both low and high speed.
5. Verify:
   - full held stick approaches a finite repeatable rate
   - release returns rate toward zero
   - control feel does not change dramatically with airspeed
6. Once stable, consider rotational control milestone complete and resume propulsion/glide redesign.

## Temporary technical debt
- evade root displacement bypasses KineticBody
- evade Body spin presentation-only
- Aircraft Body lookup assumes child index 0
- direct Aircraft -> AircraftKinetics Apply bypasses normal Component enabled dispatch
- air brake still simplified
- no induced drag / sideslip model / render interpolation
- camera still uses world Up
- backflip currently conflicts with always-on horizon-relative bank assist

Possible future fidelity only if gameplay needs it: airspeed-based control-authority curves, rotational-flow damping, gyroscopic coupling, arbitrary inertia tensor, detailed aerodynamic surfaces.

## Repository
GitHub: https://github.com/mohit-kumar-singh55/AERO_RIOT
Default branch: `master`
