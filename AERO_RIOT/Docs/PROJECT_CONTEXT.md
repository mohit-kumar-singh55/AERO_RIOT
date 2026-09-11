# AERO_RIOT — Project Context

Last updated: 2026-09-11

This file is a compact handoff for continuing the AERO_RIOT learning/project workflow in a fresh chat. It should describe the latest state, major architectural decisions, temporary systems, pending work, and collaboration rules. It is not intended to be a full development log.

## Project

AERO_RIOT is a DirectXTK/C++ 3D fighter-aircraft dogfight project. The game is being used primarily as a learning vehicle for C++, DirectX/DirectXTK, engine architecture, custom physics/aerodynamics, shaders/HLSL, rendering, lighting/shadows, VFX, particles/trails, animation, cameras, AI, and optimization.

Development rule: the game drives engine development. Do not build a whole engine upfront; add systems when the game creates a real need for them.

## Collaboration / learning workflow

The user manually writes the code for learning and does not want copy-paste final implementations unless explicitly requested or stuck.

Preferred workflow:
1. Decide the next practical goal.
2. User proposes architecture/design when appropriate.
3. Review and improve the design, explaining why.
4. Give concepts, requirements, constraints, pitfalls, and small hints rather than full final code.
5. User implements manually and pushes to GitHub.
6. Inspect the latest repository state.
7. Review bugs, architecture, safety, performance, and cleaner alternatives.
8. User fixes issues where practical.
9. Move on once the step is solid.

For difficult topics such as physics, math, rendering, HLSL, memory, ownership, and algorithms, teach the underlying theory before expecting the user to design the system.

Priority: UNDERSTAND -> DESIGN -> IMPLEMENT -> REVIEW -> IMPROVE.

## Current engine architecture

Core ownership/lifecycle:

Game -> SceneManager -> Scene -> GameObjectManager -> GameObject -> Component

Important details:
- GameObject is final and uses composition.
- GameObject owns Transform and Components.
- GameObjectManager owns GameObjects with unique_ptr.
- Scene owns GameObjectManager and scene-local systems.
- GameObject stores a non-owning pointer to its GameObjectManager.
- GameObjectManager stores a non-owning pointer to its owning Scene.
- Component can reach its owning Scene through the GameObject/GameObjectManager chain.
- Scene exposes scene-level capabilities such as RequestSceneLoad() and GetKinetics(); SceneManager itself remains hidden from gameplay components.
- GameObjects created during a frame are kept pending and initialized at BeginFrame().
- Destruction/removal is deferred until safe EndFrame processing.

Scene member order intentionally keeps Kinetics alive while GameObjects/components are destroyed:

Kinetics m_kinetics;
GameObjectManager m_gameObjects;

C++ destroys members in reverse declaration order, so GameObjects disappear/unregister first and Kinetics dies afterward.

## Transform / coordinate conventions

- Canonical local Forward = (0, 0, -1)
- Right = (1, 0, 0)
- Up = (0, 1, 0)
- Transform::GetForward()/GetRight()/GetUp() return those axes transformed into world space.
- Vector3::Forward itself is just the canonical numeric axis; coordinate meaning comes from how/where it is used.

Important rule already learned:
- local roll axis -> Vector3::Forward when creating a local rotation delta
- world thrust direction -> Transform::GetForward()

## Update / simulation phases

Time has a fixed accumulator with FixedDeltaTime currently 1/60 s.

Scene::FixedUpdate() currently does:
1. Scene OnFixedUpdate()
2. GameObjectManager::FixedUpdate() so gameplay/components can submit forces
3. Kinetics::Integrate(Time::FixedDeltaTime())

This explicit phase separation avoids hidden component-order dependencies where a KineticBody might integrate before another component submits a force.

Camera/gameplay still use normal Update/LateUpdate where appropriate.

## Kinetics / KineticBody — current physics foundation

Kinetics is scene-local and holds non-owning KineticBody* references.

KineticBody:
- owns mass
- stores inverse mass
- owns linear velocity
- stores calculated linear acceleration
- accumulates forces during a fixed step
- integrates position through the GameObject Transform
- registers with Kinetics on initialization and unregisters on destruction
- does not integrate from Component::OnFixedUpdate(); only Kinetics may call Integrate()

Current semi-implicit Euler integration:
- acceleration = accumulatedForce * inverseMass
- velocity += acceleration * fixedDeltaTime
- position += velocity * fixedDeltaTime
- clear accumulatedForce

Position is not duplicated in KineticBody yet; Transform is the current source of truth.

Mass is clamped to a small positive minimum for the current dynamic-body-only version. Static/infinite-mass behavior will later motivate inverseMass = 0 semantics.

Kinetics skips bodies that are disabled, remove-requested, or whose GameObject is inactive/destroy-requested.

## Physics experiments completed

Test cube experiments validated the system:
- One force applied once changes velocity once, then the cube continues at constant velocity because there is no opposing force.
- Constant force each fixed step causes velocity to increase continuously.
- Linear drag F = -k*v caused velocity to approach zero after an initial push.
- Constant thrust + linear drag reached the analytically predicted equilibrium speed 14.142 for diagonal force (5 Forward + 5 Right) with k=0.5.
- Simplified quadratic drag F = -k*|v|*v reached the analytically predicted equilibrium speed ~3.76 with the same thrust and k=0.5.

The test cube has now been removed.

## Aircraft architecture

Hierarchy concept:

AircraftRoot  <- gameplay/simulation transform
|- Aircraft
|- AircraftController
|- KineticBody
|- ThirdPersonCameraAnchor
`- Body       <- visual/presentation transform
   |- Base
   `- Wing

AircraftRoot is the simulation/gameplay orientation. Body is a visual child used for presentation-only effects such as the 360-degree evade spin. This keeps fake visual motion separate from future physical state.

AircraftController writes semantic AircraftControlInput:
- pitch [-1, +1]
- turn [-1, +1]
- throttle [0, 1]
- airBrake [0, 1]
- evadeRoll None/Left/Right

## Aircraft linear physics — current state

The old scalar-speed movement was removed:
- m_maxSpeed removed
- m_speedRate removed
- m_currentSpeed removed
- m_speedDrag removed

Aircraft now resolves its sibling KineticBody in OnStart().

Current configurable linear-physics values include:
- m_maxThrust
- m_airDrag
- m_airBrakePower

Aircraft::OnFixedUpdate() now:
- performs TEMPORARY direct kinematic aircraft rotation using FixedDeltaTime
- calculates thrust along world-space aircraft Forward
- adds thrust through KineticBody::AddForce()
- reads KineticBody velocity
- calculates simplified quadratic aerodynamic drag
- increases drag when air brakes are applied
- adds drag through KineticBody::AddForce()

The small timing/air-brake issues found during review were fixed in commit 504956ec73d045f62f241ada0c134013e05358c2:
- temporary fixed-step rotation now uses FixedDeltaTime
- air-brake drag is treated as additional drag instead of accidentally reducing drag at small input
- OnStart error message corrected

No artificial max-speed clamp remains; straight-line terminal speed now emerges from thrust vs drag.

Important expected behavior: orientation and velocity are now different quantities. When turning at speed, the nose may rotate before the velocity vector follows. This is not a bug; it motivates local airflow, directional resistance, lift, angle of attack, etc.

## Temporary aircraft systems / technical debt

Temporary normal rotation:
- still directly rotates Transform instead of using angular velocity/torque
- now runs in FixedUpdate because orientation affects physical thrust direction
- will be replaced by angular dynamics later

Evade roll:
- visual Body performs a 360-degree local-space roll
- AircraftRoot receives temporary direct lateral displacement
- evade presentation remains in LateUpdate
- root displacement currently bypasses KineticBody and is temporary

Visual body discovery still assumes child index 0 and is marked TODO for explicit wiring/factory/prefab work later.

## Camera

A separate chase camera root follows ThirdPersonCameraAnchor.

Camera supports spherical freelook with right stick and delayed recentering. Camera position uses the target basis while LookAt currently uses world Up for a stabilized horizon.

Previously tested targetUp produced a more cinematic roll-following feel. Final behavior is intentionally deferred until physical bank/roll stabilization exists.

Known future task:
- flight stabilization / bank behavior + control readability
- prevent unrestricted normal bank/roll from becoming disorienting/inverted-feeling
- later decide final camera up behavior: world Up, target Up, or stabilized/blended Up

Camera/aircraft LateUpdate creation-order dependence remains known technical debt; introduce explicit update priority/PostLateUpdate/camera phase only when justified.

## Rendering interpolation

Physics runs at 60 Hz, but render interpolation has not yet been implemented. RenderContext already carries Time::FixedInterpolationAlpha(), but PrimitiveRenderer currently renders the Transform world matrix directly.

The aircraft can look smoother than the old debug cube mainly because the chase camera moves with it and the current displacement per fixed step is small. True interpolation remains future work.

## Next immediate step

Before returning to deeper aircraft aerodynamics, introduce and understand a Visual Studio/C++ precompiled header (PCH) because the project now repeatedly includes stable STL/DirectX headers.

Important distinction:
- PCH: compile-time optimization for widely-used, stable headers
- common/umbrella header: source-code convenience/dependency grouping; should stay small and not become an include-everything dumping ground
- AppConfig/EngineConfig: runtime/startup settings such as window size, VSync, FPS cap, etc.; separate from PCH and gameplay definition/config data

After PCH, make a small AppConfig only for genuinely global application/startup settings, then return to aircraft physics.

## Planned physics progression

Linear foundation is now working. Planned progression:
- aircraft-local airflow / directional aerodynamic resistance
- lift
- angle of attack
- stall behavior
- angular velocity
- torque
- inertia / inertia tensor
- real physical aircraft controls / stabilization
- impulses/physical evade behavior where appropriate
- gravity when useful
- collision detection/response later

Do not jump to all of this at once. Continue progressively and validate each physical concept with observable tests.

## Major future learning targets

- custom HLSL/shaders
- real shadows/lighting
- particles/trails/explosions/VFX
- animation
- aircraft AI/missiles
- physics/collision expansion
- render interpolation
- optimization

## Repository

GitHub: https://github.com/mohit-kumar-singh55/AERO_RIOT
Default branch: master

When continuing in a fresh chat, inspect the latest repository before assuming this document is perfectly current, then continue from the current milestone described here.