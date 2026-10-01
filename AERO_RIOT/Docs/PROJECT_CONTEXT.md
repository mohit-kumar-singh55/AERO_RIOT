# AERO_RIOT — Project Context
Last updated: 2026-10-02. Source of truth: inspect latest master in GitHub before new review.

For the October 2026 game-completion schedule and the subsequent DX12/DXR learning plan, see `Docs/DEVELOPMENT_ROADMAP.md`. Keep this file about current technical implementation and immediate next work.

## Goal / workflow
AERO_RIOT is a DirectXTK / C++ aircraft-dogfight GAME, not a full simulator or general-purpose game engine. Priority: responsive/fun gameplay, believable momentum, dramatic rendering, shader learning. Frequently ask: "Are we going deeper than the game needs?"
The user MANUALLY writes their code to learn. Workflow: UNDERSTAND -> USER DESIGNS -> REVIEW -> USER IMPLEMENTS -> inspect pushed commit -> review -> next. Give concept/architecture and hints rather than complete paste-ready code unless requested. Do not continually add new requirements to a completed milestone.

## Engine baseline
- GameObject final: Transform + vector<unique_ptr<Component>>. GameObjectManager owns live/pending GameObjects. Creation queued until BeginFrame, destruction/removal deferred to EndFrame. Adding components during OnStart/Update/other component vector traversal is unsafe (reallocation); add them during spawn/construction.
- Scene owns Kinetics, GameObjectManager, RenderContext. FixedUpdate sequence: gameplay/component FixedUpdate -> Kinetics::Integrate -> Kinetics::DetectCollision.
- KineticBody holds physics authority, previous/current world position/rotation for render interpolation; Transform has simulation and render poses. Camera uses render interpolation. Fixed step ~1/60s.
- Coordinate convention Forward -Z, Right +X, Up +Y.
- Static Debug overlay is working.
- Aircraft thrust/drag/lift/stall/turn controls are good enough and frozen unless actual gameplay demands changes. Earlier numerical instability due huge lift impulses was addressed with game-oriented constraints; do not restart aircraft simulation redesign gratuitously.
- Current render architecture: PrimitiveRenderer owns DirectXTK GeometricPrimitive and optional shared IPrimitiveMaterial; fallback uses GeometricPrimitive's default Draw path. BasicPrimitiveMaterial wraps DirectXTK BasicEffect. UnlitMaterial is now a working custom-HLSL material path using UnlitEffect.

## Working weapon / Bullet flow
AircraftController -> Aircraft::Fire -> WeaponController::TryFire -> FireGun creates pending Bullet GameObject and attaches PrimitiveRenderer, KineticBody, SphereCollider, Bullet during spawn. Sets spawn position; Bullet::RequestLaunch; OnStart retrieves KineticBody, disables gravity/damping, sets explicit velocity. Lifetime ~4 seconds. Bullet collider is explicitly Continuous. As of 2026-09-28 Bullet::OnCollisionEnter/Stay/Exit still just logs; actual impact destroy/owner-ignore logic is a SMALL future gameplay cleanup and should not delay shaders/VFX.

## Collision foundation — FROZEN / VERIFIED
- Collider is abstract (pure GetShape), owns common local offset, optional cached KineticBody, trigger flag (not separately operational), DetectionMode {Discrete default, Continuous}. OnInitialize/OnDestroy registers/unregisters with Kinetics. GetCenter transforms local offset through simulation world matrix. GetPreviousCenter rebuilds prior world SRT from KineticBody previous position/rotation, current scale; static with no body returns GetCenter(). Collider::GetRotation exposes world rotation.
- SphereCollider stores local radius, GetWorldRadius accounts for max absolute scale. BoxCollider stores local half extents, GetWorldExtents componentwise abs world scale, rotation from Collider. OBB assumes no problematic parent shear.
- CollisionDetection.h dispatches canonical Sphere-Sphere and Sphere-Box/Box-Sphere; if EITHER collider requests Continuous, selects continuous. Box-Box is NOT implemented.
- Discrete Sphere-Sphere: center squared distance vs squared radii sum.
- Continuous Sphere-Sphere: relativeStart=A.prevCenter-B.prevCenter; relativeEnd=A.currCenter-B.currCenter; closest point from origin to relative segment vs radius sum; handles moving target.
- Discrete Sphere-OBB: sphere center translated by box center and inverse rotated into box axes; clamp closest point to ±world extents; test squared distance vs sphere radius². Handles face, edge, corner, inside.
- Continuous Sphere-OBB: relative sphere/box previous/current centers; BOTH endpoints inverse rotated by CURRENT box rotation into same box-local orientation space; expand box extents by sphere radius; SLAB segment-vs-expanded-AABB with tEnter=0/tExit=1, per-axis tNear/tFar, parallel-axis bounds checks. Box rotation during one fixed step and exact rounded corners are intentionally approximated for game-first cost.
- Kinetics tests unique i<j collider pairs, filters disabled/removed/inactive and skips both-static pairs. CollisionPair unordered identity via pointer ordering and hash; previous/current collision sets yield OnCollisionEnter/Stay/Exit. UnregisterCollider purges BOTH sets. Collision struct currently only Collider& other (each recipient sees opposite collider); GameObject fans events out to Components. RequestDestroy deferred; no impulse/response, layers/masks, contacts, TOI.
- User tested collision enter/stay/exit and destruction in callbacks; no stale exit/crash. Bullet speed 600: Discrete misses many small spheres; Continuous catches every tested shot. Discrete Sphere-OBB tested against rotated box. Latest CCD Sphere-OBB fix commit `9af90041734850e4cf27224efb079c2897d5804d`: corrected previous mixed world/local SLAB inputs and Sphere-Box cast ordering, bullet back to Continuous; user tested it works, including a moving rotated debug BoxCollider.
- Keep future collision additions driven by real gameplay needs. No Box-Box SAT, rigid-body solver, broadphase, contact manifold now. A surviving CCD object may report transient Enter then Exit next step; bullets normally die on impact in future gameplay hookup.
- Implementation detail: CollisionDetection.h directly includes <algorithm> for std::clamp; BoxCollider.h <cmath>; Collider.h exposes quaternion alias.

## CUSTOM HLSL PHASE — WORKING PIPELINE
- First custom shader milestone is RUNTIME VERIFIED as of 2026-10-02. User can render a DirectXTK GeometricPrimitive cube through own HLSL VS/PS instead of BasicEffect.
- `UnlitVS.hlsl`: position-only `SV_Position` input, `row_major float4x4 WVP` in `cbuffer Transform : register(b0)`, output clip-space `SV_Position`, row-vector `mul(float4(position,1), WVP)`. Visual Studio emits `.cso` files under `$(OutDir)Shaders/`.
- `UnlitEffect : DirectX::IEffect` loads VS/PS `.cso` bytecode, owns VS bytecode, D3D11 vertex/pixel shaders, VS transform constant buffer, PS material constant buffer, implements `Apply()` and `GetVertexShaderBytecode()`. `Apply()` uploads via `UpdateSubresource`, binds VS b0 + PS b0 independently, then binds custom VS/PS. WRL ComPtr input binding uses local raw pointer from `.Get()`; do not pass `&ComPtr` to input APIs.
- `UnlitMaterial : IPrimitiveMaterial` owns `UnlitEffect` + reusable input layout created from a temporary GeometricPrimitive cube. `Draw()` computes `WVP = world * view * projection`, passes material data, then calls GeometricPrimitive custom-effect Draw overload.
- MainScene creates/initializes one shared UnlitMaterial with device/context and uses it on a debug cube. Initial hard-coded magenta shader rendered successfully after Initialize call was added.
- Commit `240df2a6852015a92a6ac682edce90b11a31428d` added PS material constant data: CPU `MaterialBuffer` with `XMFLOAT4 Color` + `XMFLOAT4 Emission`, GPU PS constant buffer bound at b0, `UnlitMaterial::Draw()` converts PrimitiveRenderer diffuse/emissive inputs, and `UnlitPS.hlsl` reads Material b0. User runtime-tested green `PrimitiveRenderer` color successfully through own HLSL. VS b0 and PS b0 coexist because slots are stage-specific.
- Cleanup: emission is additive, so CPU-side/default emission should semantically default to zero rather than white. Current UnlitMaterial explicitly passes renderer emission, so this did not block the green test.
- NEXT SHADER LESSON: introduce shader time / animation parameters and use HLSL math (`sin`, remap, emission/pulse) to make the custom object visibly pulse. Keep it small: learn time constant-buffer flow before noise, trails, particles, or postprocess.
- After pulse: stylized aircraft/exhaust material -> muzzle flash -> trails -> simple particle system -> hit sparks/explosion -> postprocess only if beneficial.
- Do not build a full shader/material manager yet. Return to bullet impact destroy/owner filtering as a small gameplay integration before impact VFX if needed.

Repo: https://github.com/mohit-kumar-singh55/AERO_RIOT, branch master.
