# AERO_RIOT — Project Context

Last updated: 2026-09-17

Compact handoff for continuing AERO_RIOT in a fresh chat. Inspect latest `master` before assuming this file is perfectly current.

## Learning workflow
AERO_RIOT is a DirectXTK/C++ 3D fighter-aircraft dogfight project used to learn engine architecture, custom physics/aerodynamics, rendering/HLSL, VFX, cameras, AI, and optimization.

The user manually writes code for learning. Preferred flow: UNDERSTAND -> DESIGN -> IMPLEMENT -> REVIEW -> IMPROVE. Do not provide full copy-paste implementations unless explicitly requested, stuck, or the task is mechanical. Teach hard physics/math/rendering concepts before implementation. The game drives engine development.

## Core engine / lifecycle
Ownership: Game -> SceneManager -> Scene -> GameObjectManager -> GameObject -> Component.

Fixed simulation order:
1. Scene OnFixedUpdate()
2. GameObjectManager FixedUpdate() — gameplay/components submit forces/torques
3. Kinetics Integrate(FixedDeltaTime) — world physics consumes them

Current fixed step: 1/60 s.

Transform conventions:
- local Forward = (0,0,-1), Right = (1,0,0), Up = (0,1,0)
- Transform::GetForward/Right/Up return world-space directions
- Transform normalizes stored rotations
- current linear/angular velocities and accumulated force/torque are WORLD-space

## Kinetics / KineticBody
Current linear state:
- mass / inverse mass
- world-space linear velocity and acceleration
- world-space accumulated force
- useGravity + gravityScale
- generic scalar linear damping

Current angular state:
- per-axis moment of inertia Vector3 in BODY/LOCAL principal axes
- inverse per-axis inertia Vector3
- world-space angular velocity / acceleration / accumulated torque
- generic angular damping using body-space coefficients

Rotational physics is COMPLETE / validated for the current simplified model:
- angular velocity integration by axis-angle quaternion
- torque accumulation
- scalar and body-space diagonal inertia
- world torque -> local torque -> inverse inertia -> local alpha -> world alpha -> world omega
- generic angular damping opposes local angular velocity
- full gyroscopic coupling / arbitrary inertia tensor intentionally postponed

Generic damping is COMPLETE / validated:
- linear damping force: `F = -v * mass * linearDamping`, giving mass-independent decay `a = -v * linearDamping`
- angular damping torque: `tau_local = -omega_local * angularDamping`
- generic bodies use both dampings by default
- aircraft explicitly disables generic linear/angular damping in AircraftKinetics to avoid double-counting aerodynamic resistance
- SetLinearDamping clamps negative values to zero as of commit `9bfab98c8a2742b634fa329dcfe739b6e97746ca`

## Aircraft architecture / aerodynamics
Current flow:
AircraftController -> Aircraft -> AircraftKinetics -> KineticBody

Aerodynamics already implemented:
- thrust
- directional quadratic drag on Forward/Right/Up projections
- gravity
- AoA = -atan2(verticalSpeed, forwardSpeed)
- lift = 0.5 * rho * Vpitch^2 * S * Cl
- stall curve linear to 15 deg, then falloff to zero by 90 deg

Aircraft generic linear/angular damping is disabled; aircraft-specific aerodynamic angular damping will live in AircraftKinetics.

## Physical aircraft control torques — IN PROGRESS
Commit `ce88a7242077c68b169b189f54d158d3a77cfb17` begins replacing direct Transform rotation with torque-based control.

Current implementation:
- old direct pitch/turn/bank Transform rotation in Aircraft::OnFixedUpdate is commented out
- AircraftKinetics::Apply builds BODY-space pitch torque on local X only
- local pitch torque is transformed by aircraft world rotation into WORLD-space
- resulting world torque is passed to KineticBody::AddTorque
- separate tuning fields exist for pitch/yaw/roll torque; yaw/roll remain zero for now

Axis mapping:
- pitch -> local X
- yaw -> local Y
- roll -> local Z / forward axis sign handled carefully because Forward is -Z

Current pitch-sign issue:
- AircraftController currently assigns `controlInput.pitch = rotVal.y`
- for aircraft-style controls, pulling the stick back/down should pitch the nose UP
- define semantic control input as `pitch +1 = nose up`, `pitch -1 = nose down`
- therefore raw stick Y should be inverted once in AircraftController: conceptually `pitch = -rawLeftStickY`
- do NOT hide this inversion in AircraftKinetics; physics should consume semantic pitch input

Current behavior after torque conversion:
- pitch torque/body->world conversion works structurally
- without aircraft-specific angular damping, holding pitch continuously increases angular velocity
- after releasing the stick, nonzero angular velocity persists, so the aircraft keeps rotating; this is expected, not a physics bug

## NEXT IMMEDIATE STEP
1. Fix pitch input sign in AircraftController so stick back/down = nose up.
2. Validate with short pitch taps, including while aircraft is banked/pre-rotated, to confirm torque follows BODY X rather than global X.
3. Add aircraft-specific aerodynamic angular damping while pitch is still the only active control torque.
4. Then add yaw and roll torques and tune per-axis authority/inertia.
5. Add stabilization / bank behavior and revisit camera Up after physical bank.

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
- `m_kb->SetUseGravity(false)` on aircraft is intentional during rotational debugging

## Repository
GitHub: https://github.com/mohit-kumar-singh55/AERO_RIOT
Default branch: `master`
