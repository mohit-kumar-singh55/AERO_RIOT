# AERO_RIOT — Development Challenges & Solutions

Last updated: 2026-10-05

## Purpose

This file records major technical problems encountered while developing AERO_RIOT / SNX, why they happened, how they were solved, and what was learned from them.

The goal is not to list every bug. Keep entries that are useful for:
- portfolio descriptions,
- job applications / ES,
- technical interviews,
- explaining engineering decisions and debugging process.

Maintenance rule: add an entry when a problem required a non-obvious technical fix, exposed an architectural limitation, or led to a meaningful design change. Keep each entry short enough to reuse later.

---

## 1. Fixed-step physics caused visible motion / camera mismatch

**Problem**  
Physics ran at a fixed timestep, while rendering ran every frame. Using only the latest simulation transform made aircraft/camera motion look less smooth and caused the camera to disagree with the rendered aircraft pose.

**Cause**  
Simulation state and render state were being treated as the same thing even though they update at different rates.

**Solution**  
`KineticBody` stores previous/current physics position and rotation. `Kinetics::UpdateInterpolation()` calculates the render pose using the fixed-step interpolation alpha, and `Transform` exposes separate render-space accessors. The camera follows the interpolated render pose instead of the raw simulation pose.

**Result / lesson**  
Separated authoritative simulation state from visual presentation state. This made motion smoother without changing the fixed physics step.

---

## 2. Aircraft lift became unstable at high speed

**Problem**  
The aircraft could gain excessive velocity / lift and become numerically unstable at high speed.

**Cause**  
Lift grows strongly with speed, so physically inspired formulas could create forces far beyond what the arcade-style game needed.

**Solution**  
Added game-oriented constraints instead of expanding into a full flight simulator: maximum linear velocity, tuned drag behavior, bounded lift force, and stall-aware lift coefficients.

**Result / lesson**  
For gameplay physics, physical plausibility is useful, but stability and controllability are more important than unrestricted realism.

---

## 3. Runtime object/component mutation could invalidate traversal

**Problem**  
Creating/removing objects or components while lifecycle vectors were being traversed risked invalidating iterators/references and causing unstable behavior.

**Cause**  
`GameObject` stores components in `std::vector<std::unique_ptr<Component>>`, and `GameObjectManager` also iterates owned object collections during frame phases.

**Solution**  
Introduced deferred lifetime handling: newly created GameObjects stay pending until `BeginFrame`, while destruction/component removal is requested first and finalized at `EndFrame`. Components needed at spawn time are attached during construction/setup rather than during active traversal.

**Result / lesson**  
Lifecycle timing became explicit and predictable, which also made later systems such as bullets and collision callbacks safer.

---

## 4. Fast bullets tunneled through colliders

**Problem**  
At high muzzle speeds, discrete collision checks could miss small targets because a bullet could move from one side of a collider to the other between fixed updates.

**Cause**  
Discrete collision only checks the current positions; it does not consider the path traveled during the timestep.

**Solution**  
Added per-collider `Discrete / Continuous` detection modes. Fast bullets use continuous collision detection:
- swept Sphere-Sphere using relative motion and closest approach,
- swept Sphere-OBB using a SLAB segment test against box extents expanded by the sphere radius.

A later Sphere-OBB bug was fixed by transforming both swept endpoints into the same box-local orientation space before the SLAB test.

**Result / lesson**  
High-speed projectiles became reliable while keeping normal objects on the cheaper discrete path.

---

## 5. Destroyed colliders could leave stale collision state

**Problem**  
Collision Enter/Stay/Exit tracking used previous/current collision-pair sets. Removing a collider without cleaning these sets could leave stale references and incorrect later events.

**Cause**  
The collision tracker owned pair state independently from the collider-registration list.

**Solution**  
`UnregisterCollider()` now purges every pair containing that collider from both current and previous collision sets. GameObject destruction remains deferred so collision callbacks can safely request destruction.

**Result / lesson**  
Systems that cache relationships between objects must clean those relationships when either object leaves the system.

---

## 6. DirectXTK primitives did not provide the custom shader workflow needed for learning/VFX

**Problem**  
The default DirectXTK `GeometricPrimitive` / `BasicEffect` path was convenient, but it hid too much of the GPU pipeline for custom HLSL materials and VFX.

**Cause**  
A custom VS/PS pipeline requires explicit shader bytecode, input layouts, constant buffers, bindings, and stage-specific data flow.

**Solution**  
Built an engine-side custom shader path:
- `UnlitEffect` loads compiled VS/PS bytecode and owns D3D11 shader/constant-buffer resources,
- `UnlitMaterial` owns the effect and reusable input layout,
- material color/emission and frame time are sent through PS constant buffers,
- WVP is sent through the VS constant buffer,
- project-wide HLSL compile rules standardize `VSMain` / `PSMain` and `.cso` output.

Shader file paths were made configurable so game-specific shaders such as the engine glow could reuse the same resource contract without duplicating a whole new effect type.

**Result / lesson**  
Established a reusable custom HLSL pipeline while keeping low-level D3D11 code inside SNX instead of game code.

---

## 7. Engine trail required dynamic geometry instead of a static primitive

**Problem**  
An aircraft exhaust trail changes shape every frame and could not be represented well by a static `GeometricPrimitive`.

**Cause**  
The trail needs historical world positions, camera-facing geometry, changing vertex count, and per-vertex fade data.

**Solution**  
Created `TrailRenderer : Renderer` with:
- distance-based world-space point sampling,
- point lifetime tracking,
- camera-facing ribbon generation (two vertices per point),
- side-direction continuity to avoid sudden ribbon flips,
- a dynamic vertex buffer using capacity growth + `Map(D3D11_MAP_WRITE_DISCARD)` / `memcpy` / `Unmap`,
- custom `TrailVS` / `TrailPS` and an explicit `POSITION + ALPHA` input layout,
- triangle-strip rendering.

**Result / lesson**  
This became the first fully custom dynamic mesh path in SNX rather than geometry owned by DirectXTK.

---

## 8. Trail disappeared depending on viewing direction

**Problem**  
The trail was invisible most of the time and only flashed briefly during sharp turns.

**Cause**  
The ribbon is a thin two-sided visual effect, but normal backface culling removed triangles depending on winding / camera orientation.

**Solution**  
Created a trail-specific rasterizer state with `CullMode = D3D11_CULL_NONE`, applied it only around the trail draw, then restored the previous rasterizer state. Ribbon-side continuity was also added to prevent adjacent left/right vertices from flipping 180 degrees.

**Result / lesson**  
Pipeline state is part of a renderer's visual contract; correct geometry alone is not enough.

---

## 9. Alpha values existed in the shader but did not make the trail transparent

**Problem**  
The trail shader produced alpha, but the framebuffer still showed an opaque ribbon.

**Cause**  
Writing an alpha value from a pixel shader does not automatically blend it with the existing render target.

**Solution**  
Added a trail blend state. Normal alpha blending was first used to verify fade behavior, then RGB blending was changed to additive (`SrcAlpha + Dest One`) for a brighter engine-energy look. VS/PS interface data was also made explicit with matching shader input/output structures, and the material constant-buffer binding was cleaned up.

**Result / lesson**  
Pixel-shader output and Output-Merger state must work together; shader alpha alone is only data until blending is enabled.

---

## 10. Fully faded trail geometry still hid objects behind it

**Problem**  
The visually invisible tail of the trail appeared as a black silhouette over opaque debug walls behind it.

**Cause**  
Color blending made the trail invisible, but the transparent geometry was still writing depth. Later opaque geometry failed the depth test even though the trail contributed no visible color.

**Solution**  
Added a trail-specific depth-stencil state:
- depth test ON,
- depth writes OFF (`D3D11_DEPTH_WRITE_MASK_ZERO`).

The previous depth/stencil state and stencil reference are saved and restored around the trail draw.

**Result / lesson**  
Transparent rendering usually needs depth testing without depth writing.

---

## 11. Disabling transparent depth writes exposed a render-order architecture limitation

**Problem**  
After disabling trail depth writes, opaque walls created/rendered after the trail could overwrite even the fully visible part of the trail.

**Cause**  
`GameObjectManager::Render()` originally rendered GameObjects once in creation order. Transparent and opaque renderers were mixed together. With depth writes disabled, a transparent object rendered early cannot protect its pixels from opaque objects drawn later.

A further design constraint was that one GameObject (`EngineExhaustGlow`) contains both an opaque `PrimitiveRenderer` and a transparent `TrailRenderer`, so classifying the whole GameObject as opaque/transparent would be wrong.

**Solution**  
Introduced renderer-level `RenderPass { Opaque, Transparent }` and `Renderer::GetRenderPass()`.

`GameObjectManager` now schedules two world-render passes:
1. Opaque renderers first,
2. Transparent renderers second.

`GameObject::Render(context, pass)` filters components so only `Renderer` subclasses belonging to the active pass receive `OnRender()`.

**Result / lesson**  
A local VFX bug revealed a real engine architectural limitation. Moving pass classification to the renderer-component level solved the current transparency problem without incorrectly classifying mixed-purpose GameObjects.

---

## 12. Trail geometry could pop when the oldest point was removed

**Problem**  
The final trail segment could disappear suddenly when the oldest point reached its lifetime, even when its visual alpha was near zero.

**Cause**  
Point lifetime controls geometry existence. Removing the oldest point deletes the connected ribbon segment in one frame; blending mode cannot prevent geometry removal.

**Solution**  
Keep points alive for a small cleanup grace period (`m_lifeTime + ~0.05s`) after the visible fade reaches zero, then erase them after they are already visually invisible.

**Result / lesson**  
Visual lifetime and storage/cleanup lifetime do not always need to be identical.

---

## Strong portfolio / interview stories so far

If only a few examples fit in a portfolio or interview, the strongest current candidates are:

1. **Transparent rendering -> RenderPass architecture** — a visible VFX bug exposed an engine-level render-order limitation and led to an opaque/transparent renderer-pass design.
2. **Fast projectile tunneling -> CCD** — discrete collision failed for high-speed bullets, leading to swept Sphere-Sphere and Sphere-OBB detection.
3. **Fixed physics vs render rate -> interpolation** — separated simulation transforms from render poses to keep fixed-step physics while improving visual smoothness.
4. **DirectXTK default rendering -> custom HLSL pipeline** — implemented shader bytecode loading, input layouts, constant buffers, configurable materials, and time-driven effects.
5. **Static primitive limits -> dynamic TrailRenderer** — implemented world-space history, CPU ribbon generation, dynamic GPU vertex buffers, custom shader IO, blending, and render-state management.

---

## Future-entry template

### N. Short challenge title

**Problem**  
What was visibly/functionally wrong?

**Cause**  
What did investigation show was actually causing it?

**Solution**  
What was changed, and why was that design chosen?

**Result / lesson**  
What improved, and what engineering lesson is worth mentioning in a portfolio/interview?
