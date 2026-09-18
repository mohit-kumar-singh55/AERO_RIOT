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

## NEXT FLIGHT-FEEL FEATURE — MINIMUM FORWARD SPEED
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
