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
- aircraft disables generic KineticBody damping and uses aircraft-specific aero damping

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

Current tuning:
- pitchTorque=30
- yawTorque=20
- pitch/yaw/roll aero damping=1
- maxBankAngle=50 deg
- bankKp=120
- bankKd=15

Input:
- pitch +1 = nose up
- raw gamepad Y inverted
- axial stick threshold suppresses cross-axis noise

## Roll bank assist — CURRENT POLICY
Normal roll is controlled by target-bank PD assistance:
- targetBank = turn * maxBankAngle
- bankError = shortest signed target-current difference
- bankRate = -localAngularVelocity.z
- rollCommand = Kp*bankError - Kd*bankRate
- local roll torque = (0,0,-rollCommand)
- transform local torque to WORLD before AddTorque

`CalculateBankAngle()` returns `std::optional<float>` and returns nullopt near the vertical singularity.

Backflip testing showed horizon-relative bank assist fights inverted/over-the-top flight. An upright-only condition fixed backflips but disabled assist above 90 deg bank, and normal input can reach/invert too easily.

Current user decision:
- prioritize reliable auto-assist over backflip freedom
- the upright-only `aircraftUp.Dot(WorldUp) > 0` condition is COMMENTED OUT again
- bank assist runs whenever bank angle is valid
- clean backflips are temporarily sacrificed
- do not deepen the global orientation/recovery math yet
- later special maneuver / recovery states can explicitly override assist

Dead `m_rollTorque` field was removed.

## MINIMUM FORWARD SPEED — IMPLEMENTED, COASTING ISSUE DIAGNOSED
Commit `40babebeb388f1e8bc737cfb47ad3a6e69520049` added force-based minimum forward speed assist.
Later commit `cda8efd29f890990ea4777eef475ea7f789e4110`:
- clamps assist acceleration to [0, maxSpeedAssistAcceleration]
- current values: minForwardSpeed=12, minSpeedGain=6, maxSpeedAssistAcceleration=8
- forwardDrag was reduced to 0.2

Observed:
- zero-throttle total speed ~6.5
- full-throttle total speed ~22
- releasing throttle still drops speed extremely quickly

This is mathematically explained by the current quadratic forward drag:
`Fdrag = -forwardDrag * v * abs(v)`.
With forwardDrag=0.2 and mass~1:
- at v=22, drag ~96.8
- max thrust=100, so full-throttle equilibrium ~sqrt(100/0.2)=22.36
- zero-throttle max speed-assist force ~8, so equilibrium ~sqrt(8/0.2)=6.32
- when throttle is released at 22, nearly 97 units of backward force suddenly remain, causing the rapid drop

Core design diagnosis:
**forward drag is currently doing two separate gameplay jobs: limiting top speed and determining glide/coast decay. These need to be decoupled.**

Recommended next design:
- lower base forward drag substantially to get satisfying momentum/glide
- do NOT let low drag make top speed unlimited
- replace raw constant-thrust top-speed limiting with a game-oriented speed/engine controller or thrust falloff
- clean arcade option: throttle chooses desired forward speed between min cruise and max cruise; propulsion applies only positive forward acceleration when below desired speed; when above desired speed, no active braking, so low drag creates natural glide
- this can eventually unify current raw thrust + minimum-speed assist into one propulsion controller
- do not hard-clamp linear velocity
- air brake can remain the explicit fast-deceleration control


Game goal: even with zero PLAYER throttle input, aircraft should continue moving forward at a minimum cruising speed so dogfights do not stall out into awkward low-speed control.

Do NOT directly clamp/set linear velocity during normal flight. That would snap momentum/direction and fight the force-based physics.

Preferred design:
- keep current player thrust force
- add a separate automatic minimum-forward-speed assist force
- use existing forward speed: `forwardSpeed = velocity.Dot(aircraftForward)`
- if forwardSpeed < minForwardSpeed, compute speed deficit
- convert deficit into a forward acceleration/force with a proportional gain
- clamp the assist acceleration/force so a 180-degree orientation change does not instantly reverse velocity
- use KineticBody mass if converting desired acceleration to force
- no extra dt in the force formula; KineticBody integration handles dt
- likely disable or reduce minimum-speed assist while airBrake is intentionally held, otherwise hidden propulsion and airbrake fight each other

Conceptual flow:
`speedDeficit = minForwardSpeed - forwardSpeed`
if deficit > 0:
`assistAccel = clamp(speedDeficit * minSpeedGain, 0, maxAssistAccel)`
`assistForce = aircraftForward * assistAccel * mass`
`AddForce(assistForce)`

This is a gameplay speed-assist controller, not a realistic engine model.

Note: a pure proportional speed assist plus drag may settle slightly below the nominal target because drag still exists at the target. That is acceptable initially; tune by feel. If exact minimum-speed tracking later matters, add small feed-forward/idle thrust or a richer speed controller instead of directly clamping velocity.

## HIGH-SPEED ROTATION / NaN DIAGNOSIS — CURRENT PRIORITY
User lowered forward drag to improve glide, which allowed speed to rise to ~22+. At that speed, even small rotation input became violently unstable and total velocity eventually displayed NaN(ind). User temporarily restored forwardDrag to 1.0, which hides the issue by keeping speed lower.

Root cause identified in aerodynamic angular damping:
`localDampingTorque = -damping * localAngularVelocity * dynamicPressure`
with
`dynamicPressure = 0.5 * rho * V^2`.

KineticBody integrates angular velocity with explicit Euler. With MOI currently default ~1, per-axis damping behaves approximately:
`omegaNew = omega * (1 - k*q*dt)`.

At V=22, rho=1.225:
- q ~= 296
- k=1, dt=1/60
- multiplier ~= -3.94
- damping flips sign and amplifies angular velocity each fixed step -> explosive oscillation -> overflow/NaN

At V=10:
- q ~= 61
- multiplier with k=1 is ~= -0.02
- not explosive, but still extremely overdamped and slightly sign-reversing

Therefore forwardDrag=1 is masking the problem, not fixing it.

Immediate diagnostic/fix:
- reduce aircraft pitch/yaw/roll aero damping coefficients from 1.0 to roughly 0.05–0.1
- test again with forwardDrag around 0.2 and high speed
- start around 0.05; at V=22, multiplier becomes ~0.75, giving real decay instead of sign reversal
- tune upward gradually only if rotation is too loose
- do not change damping architecture yet; for expected bounded game speed, properly scaled coefficients may be enough
- if future speeds make stability a recurring issue, add a bounded/exponential damping formulation or equivalent protection so damping cannot numerically reverse/amplify omega in one step

Also note lift scales with V^2 and should be sanity-checked after angular stability is fixed, but the violent rotation/NaN is most directly explained by the angular damping instability.

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
