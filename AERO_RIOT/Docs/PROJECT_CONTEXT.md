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
- current angular velocity and accumulated torque convention is WORLD-space

## Kinetics / KineticBody
Current linear state:
- mass / inverse mass
- linear velocity, linear acceleration, accumulated force
- useGravity + gravityScale

Linear integration:
- a = F * inverseMass
- v += a * dt
- x += v * dt

Current angular state:
- per-axis moment of inertia Vector3 in BODY/LOCAL principal axes
- inverse per-axis inertia Vector3
- world-space angular velocity
- world-space angular acceleration
- world-space accumulated torque

Angular velocity integration is COMPLETE:
- omega is rad/s, world-space
- angleThisStep = |omega| * dt
- axis = normalize(omega)
- orientation integrates via axis-angle quaternion
- pre-rotated cube confirmed global-Y omega rotates around GLOBAL Y
- near-zero threshold is 0.001^2

Torque + scalar inertia milestone is COMPLETE / validated:
- AddTorque accumulates until integration
- alpha = torque / I
- omega += alpha * dt
- accumulated torque clears once per fixed step
- one-frame torque changes omega once and rotation persists without damping
- I=2 vs I=4 produced the expected 2:1 angular-acceleration response

## Body-space diagonal inertia — IMPLEMENTED, ONE BUG FOUND
Commit `146b4026da29dba4a355e068bc6d6309151fc545` changed scalar inertia to Vector3 and added the correct coordinate-space structure:

`world torque -> inverse world rotation -> local torque -> apply per-axis inverse inertia -> local angular acceleration -> world rotation -> world angular acceleration -> world angular velocity`

The world/local conversion itself is correct.

However, current code calculates local angular acceleration with `m_momentOfInertia` instead of `m_inverseMomentOfInertia`:

```cpp
localAngularAcc.x = localTorque.x * m_momentOfInertia.x;
localAngularAcc.y = localTorque.y * m_momentOfInertia.y;
localAngularAcc.z = localTorque.z * m_momentOfInertia.z;
```

This is physically reversed. It must conceptually be `alpha = torque / I`, i.e. component-wise multiplication by inverse inertia. With I=(1,2,4), a given local torque should produce acceleration ratios 1, 1/2, 1/4, not 1,2,4.

Current setter already computes `m_inverseMomentOfInertia = 1.0f / m_momentOfInertia`, so the intended inverse values are available.

Validation after the fix:
- keep the body pre-rotated so local and world axes differ
- use I=(1,2,4)
- apply equal torque magnitude around one BODY axis at a time
- expected angular-acceleration magnitudes: local X = 4, local Y = 2, local Z = 1 for torque magnitude 4
- those ratios should stay tied to BODY axes regardless of world orientation

Do not mark diagonal inertia complete until the inverse-inertia fix and this body-axis validation pass.

## Aircraft architecture / aerodynamics
Current flow:
AircraftController -> Aircraft -> AircraftKinetics -> KineticBody

Aircraft still temporarily mutates Transform directly for normal pitch/turn/bank, then calls AircraftKinetics::Apply(controlInput). This is the major behavior to replace after generic rotational physics is solid.

Aerodynamics milestone is COMPLETE for now:
- directional quadratic drag on Forward/Right/Up projections
- gravity
- AoA = -atan2(verticalSpeed, forwardSpeed)
- lift = 0.5 * rho * Vpitch^2 * S * Cl
- stall curve: linear to 15 deg, then falloff to zero by 90 deg
- observed powered-flight/glide behavior is plausible

Debug-test state intentionally retained:
- rotating/torque debug cube remains until rotational physics tests are complete
- `m_kb->SetUseGravity(false)` on the aircraft is intentional during rotational debugging

## NEXT IMMEDIATE STEP
Fix diagonal inertia to use inverse inertia component-wise, then validate body-axis response on the pre-rotated cube.

After diagonal inertia is complete, likely next progression:
- angular damping / aerodynamic rotational damping as needed
- aircraft control torques
- stabilization / bank behavior
- revisit camera Up behavior after physical bank

Full arbitrary inertia tensor / gyroscopic term `omega x (I omega)` is deliberately postponed until justified.

## Temporary technical debt
- normal aircraft pitch/turn/bank still directly mutates Transform
- evade root displacement bypasses KineticBody
- evade Body spin is presentation-only
- Aircraft Body lookup assumes child index 0
- direct Aircraft -> AircraftKinetics Apply bypasses normal Component enabled dispatch
- air brake is not a separate aerodynamic contribution
- no induced drag
- sideslip/beta not modeled
- no render interpolation
- camera still uses world Up

## Repository
GitHub: https://github.com/mohit-kumar-singh55/AERO_RIOT
Default branch: `master`
