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

## NEXT IMMEDIATE STEP
1. Add baseline Y-axis angular damping independent of dynamic pressure, preferably via existing KineticBody generic angular damping.
2. Test low-speed + airbrake + held turn:
   - should still snap-turn sharply
   - yaw rate should approach a finite value rather than accelerate forever
   - releasing stick should stop yaw within a reasonable time
3. If successful, tune baseYawDamping to preserve the maneuver.
4. Only then return to propulsion/glide redesign.

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
