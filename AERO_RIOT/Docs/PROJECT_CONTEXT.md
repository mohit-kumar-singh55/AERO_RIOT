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


## CURRENT HIGH-SPEED LINEAR NaN — LIFT / INTEGRATION RUNAWAY
Latest code review after commits `3dd4355a...` and `8e179250...`:
- pitch/yaw rate controllers are structurally correct and working
- maxPitchRate=maxYawRate=2.5 rad/s
- pitch/yaw rate Kp=5
- pitch/yaw torque clamps=30/20
- bank Kp/Kd=40/10
- aero angular damping remains 0.05 and user reports current control feel works well
- minor cleanup: rename `m_pitchYawKp` -> `m_yawRateKp`; comments should say target rate, not angle

Observed speed threshold:
- maxThrust=100 -> steady ~22.3; hard maneuvers briefly ~23.4; appears safe
- maxThrust=110 -> equilibrium ~23.45 but speed can creep upward and eventually explode
- 120 -> ~24.5 then eventually explode
- 125 -> ~25.0 then eventually explode
- hard inversion/high AoA near ~25-26 can trigger explosive rotation/linear speed and NaN

Important root cause:
Current lift parameters are huge relative to default mass=1:
- wingArea=2
- liftSlope=4/rad
- stallAngle=15 deg, so max pre-stall Cl ~= 1.047
At V=25, q~=382.8 and max lift ~= 0.5*1.225*25^2*2*1.047 ~= 802 force units.
With mass=1 this is ~802 m/s^2 (~82g), producing ~13.4 m/s perpendicular delta-v in one 1/60 step.

Although lift direction is perpendicular to velocity, explicit Euler velocity integration numerically adds energy:
|v + a*dt|^2 = |v|^2 + |a*dt|^2 when v·a=0.
With very large lift acceleration, hard pitching/high AoA therefore increases speed magnitude numerically; higher speed increases V^2 lift, creating positive feedback -> runaway/NaN.

Game-first short stabilization:
1. Keep maxThrust=100 for now.
2. Add an emergency aircraft max linear speed around 24 (above normal ~22.3, below observed runaway threshold). Prefer a reusable optional maxLinearSpeed in KineticBody, default unlimited, clamped after velocity integration and before position integration.
3. Also cap lift acceleration/force so a single fixed step cannot add a huge perpendicular velocity impulse. Use `maxLiftForce = mass * maxLiftAcceleration`; clamp signed liftForce. Start with a gameplay-tuned max lift acceleration roughly 40-60 m/s^2 (about 4-6g), then tune by feel.
A speed cap is acceptable as a safety net but does not by itself fix oversized lift impulses.

Short glide option (only if desired now):
- decouple coasting drag from full-throttle drag with a throttle-dependent forward-drag coefficient
- full throttle keeps current forwardDrag=0.2, preserving ~22.3 top speed
- zero throttle uses a smaller coast/glide drag (e.g. ~0.04-0.06) so momentum decays slowly
- concept: effectiveForwardDrag = lerp(coastForwardDrag, normalForwardDrag, throttle)
- air brake still multiplies drag afterward, so deliberate braking remains strong
This is intentionally arcade/game-oriented, not physically literal.


## GAME-FIRST FLIGHT SIMPLIFICATION — NEXT MILESTONE
User stress-tested current model with maxThrust up to 200 and maxLinearVelocity 30-60. At high capped speeds:
- aircraft/camera visibly jitters
- static environment cubes also appear to jitter
- hard rotations can show severe visual oscillation/afterimage-like behavior
- before lift clamp, braking at max speed could visibly reduce forward motion while total speed UI remained at cap

Interpretation:
1. Normal intended envelope (maxThrust=100, maxLinearVelocity≈24) currently looks fine.
2. High-speed environment jitter is strongly consistent with rendering a FixedUpdate-driven aircraft/camera without render interpolation. At 60 units/s and 60 Hz physics, target position advances ~1 unit per physics tick; camera follows the raw target transform in LateUpdate, so static world objects appear to judder. Render interpolation is a later engine task, not required now.
3. Extreme thrust + hard velocity cap is also pathological: large forces are integrated each step and then velocity magnitude is truncated. Large directional/lift forces can still rotate the velocity vector even while magnitude stays capped.
4. The project has accumulated more aerodynamic forces than the gameplay requires.

Decision: do ONE short cleanup/simplification pass, then freeze flight physics and move forward.

Target arcade flight model:
KEEP:
- KineticBody force/torque integration
- gravity
- thrust along aircraft Forward
- throttle-dependent forward drag for gliding (current glideForwardDrag/normalForwardDrag)
- air brake as explicit strong forward deceleration
- minimum forward-speed assist
- max linear speed as safety envelope (≈24 for now)
- pitch/yaw angular-rate controllers
- bank-angle PD assist

REMOVE/DISABLE FOR NOW:
- AoA lift model
- stall curve / CalculateLiftCoefficient
- wingArea/liftSlope/stallAngle/airDensity fields that only support that lift model
- dynamic-pressure pitch/yaw/roll angular damping; rate controllers and bank D term already stabilize rotation
- quadratic side/vertical aero drag if it continues to complicate behavior

Replace lift/gravity balancing with a simpler game mechanism:
- easiest: set aircraft KineticBody gravityScale below 1 (e.g. tune somewhere ~0.3-0.6) rather than adding another upward force
- this preserves downward gravity but makes altitude loss gentle
- pitched thrust can still provide climb/descent behavior

To avoid spaceship-like sideways sliding after removing side/vertical quadratic drag, use ONE bounded velocity-alignment force if needed:
- forwardSpeed = dot(velocity, aircraftForward)
- lateralVelocity = velocity - aircraftForward * forwardSpeed
- apply a linear force opposite lateralVelocity, proportional to mass and an alignment gain
- clamp the alignment acceleration/force
This is simpler and more numerically predictable than separate V² side/vertical forces.

Supported-envelope policy:
- maxThrust=100
- maxLinearVelocity≈24
- do not spend time making 200 thrust / 60 speed stable now
- revisit higher-speed smoothness only if gameplay later actually requires it
- when needed, add render interpolation between previous/current physics transforms for camera/rendering

After this cleanup, flight physics should be considered feature-complete enough to move on to actual game systems/rendering.


## Flight physics freeze — current decision
The current aircraft configuration is stable and feels acceptable in the intended gameplay range, so keep the existing aerodynamic calculations enabled for now rather than spending more time simplifying them.

If later gameplay requires much higher aircraft speeds, or the flight code becomes too difficult to tune/maintain, revisit this system. At that point:
- remove calculations that are not meaningfully improving gameplay
- replace them with simpler bounded/game-oriented forces or controllers
- prioritize fun, responsiveness, readability, and player experience over simulation fidelity
- treat the current high-speed stress-test instability as a reason to redesign only if the real game actually needs that speed range

Do not clean up working physics just for theoretical purity while major game systems are still missing.

Next engine/feel milestone: add render interpolation for FixedUpdate-driven physics transforms, then improve/smooth the aircraft camera on top of the interpolated motion.


## Physics interpolation architecture — CURRENT DESIGN
Interpolation should be prepared once per rendered frame after the fixed-step loop and before normal Update/LateUpdate, so the camera can already read the smooth pose.

Minimal current placement:
- at the beginning of `Scene::Update()`, before `OnUpdate()` and `m_gameObjects.Update()`
- call a Kinetics interpolation-prep function with `Time::FixedInterpolationAlpha()`

Do NOT pass the fixed accumulator into `KineticBody::Integrate()`; physics integration should continue to depend only on fixedDeltaTime. `Time::FixedInterpolationAlpha()` already exposes the correct render alpha.

Ownership:
- KineticBody stores previous physics position/rotation
- Transform remains the source of the current real/simulation pose
- before each KineticBody integration step, copy current Transform pose into previous pose
- after all fixed steps, Kinetics loops bodies and asks each to prepare its interpolated render pose using previous + current + alpha

Rendering separation:
- real getters (`GetPosition/GetRotation/GetWorldMatrix`) remain simulation/gameplay state
- add render/interpolated getters/matrix for visuals and camera
- renderers use render world matrix
- aircraft camera uses target render position/forward/up/right
- child transforms should build their render world matrix from their normal local transform and the parent's render world matrix, so one interpolated aircraft root smoothly carries Body/Wing children

Important: interpolation never writes back into the real physics Transform.


## Interpolation implementation review — latest
Commit `5c5bd49d9f1a85a3a7ad3abb03f1da35177ead92` added the first interpolation data-flow step:
- KineticBody stores previous position/rotation
- previous pose is captured at the start of each Integrate()
- Kinetics has UpdateInterpolation(alpha)
- Scene::Update() calls it before OnUpdate/GameObject Update
- KineticBody::UpdateInterpolation(alpha) is intentionally still empty

Review result:
- overall placement/data flow is correct
- initialize previous position/rotation from the current Transform in KineticBody::OnInitialize() so newly spawned bodies do not interpolate from zero/identity before their first physics step
- do not pass accumulator into Integrate(); Time::FixedInterpolationAlpha() remains the correct render-time source

Next implementation:
- add a separate render/interpolated pose path to Transform; do not overwrite the real simulation pose
- KineticBody::UpdateInterpolation(alpha) computes position Lerp(previous,current,alpha) and rotation Slerp(previous,current,alpha)
- write that result into Transform render pose
- add render getters/matrix separately from GetPosition/GetRotation/GetWorldMatrix
- child render world matrices should use parent render world matrix so an interpolated physics root carries its visual hierarchy smoothly
- only after this is working should MeshRenderer/PrimitiveRenderer and the aircraft camera be switched to render-pose getters


## Render-pose Transform review
Latest render-pose declarations are on the right track. Important invariant:
- `GetWorldMatrix()` remains completely unchanged and always represents the real simulation/gameplay hierarchy.
- interpolation must never alter what `GetWorldMatrix()` returns.

Recommended first implementation of `GetRenderWorldMatrix()`:
- return Matrix by value initially rather than `const Matrix&`; there is no render-matrix cache yet, and returning by value avoids adding render dirty-state/invalidation complexity
- if `m_hasRenderPose`: build a WORLD render matrix from current world scale + m_renderRotation + m_renderPosition
- else if parent exists: `GetLocalMatrix() * parent->GetRenderWorldMatrix()`
- else: normal local/world matrix

Render getters:
- GetRenderPosition: if own render pose exists, return m_renderPosition; otherwise derive from GetRenderWorldMatrix
- GetRenderRotation: if own render pose exists, return m_renderRotation; otherwise decompose GetRenderWorldMatrix
- no local-render fields/getters needed

This creates two separate hierarchies:
simulation = local * parent simulation world
visual = local * parent render world

Minor API cleanup: SetRenderPose inputs should preferably be const references (or values), not mutable non-const references.


## Render-pose implementation review — latest
Commit `241cd158c234f74c4ca5d4d3281fa5931ca592d6` implemented Transform render pose.

Review:
- SetRenderPose const-ref signature is good
- GetRenderWorldMatrix hierarchy logic is conceptually correct
- CRITICAL: GetRenderWorldMatrix currently returns `const Matrix&` while returning temporary Matrix expressions. This creates a dangling reference / undefined behavior. Change it to return `Matrix` by value in both declaration and definition.
- GetRenderPosition/GetRenderRotation currently always return m_renderPosition/m_renderRotation. For transforms without their own render pose (e.g. aircraft visual children), that would return zero/identity if called directly. Add fallback behavior:
  - if m_hasRenderPose -> return stored render value
  - otherwise derive world render position/rotation from GetRenderWorldMatrix()
- render hierarchy remains: own render pose = world visual override; otherwise local * parent render world; root fallback = normal world matrix
- after these fixes, switch MeshRenderer and PrimitiveRenderer to GetRenderWorldMatrix() and visually test interpolation before changing camera.


## Interpolation visual test — camera mismatch diagnosed
After switching MeshRenderer/PrimitiveRenderer to GetRenderWorldMatrix(), the aircraft became visible again after fixing GetRenderWorldMatrix to return by value, but the aircraft appears to jitter.

Cause: renderer now uses the interpolated render pose while AircraftCameraController still follows the raw simulation pose using GetPosition/GetForward/GetUp/GetRight. This creates a frame-varying offset between camera and rendered aircraft:
- aircraft visual = previous/current interpolation
- camera target = latest fixed-step Transform
- relative screen-space aircraft position therefore oscillates across render frames

Next step:
- switch AircraftCameraController target sampling to render pose
- use GetRenderPosition()
- derive forward/right/up from GetRenderRotation(), or add GetRenderForward/GetRenderRight/GetRenderUp helpers to Transform
- this is still physics interpolation, NOT camera smoothing
- after camera and aircraft both use the same interpolated pose, test again
- only then add separate camera follow smoothing/cinematic lag if desired

Minor cleanup: GetRenderWorldMatrix returning `const Matrix` by value works but the const qualifier on a returned value is unnecessary; plain `Matrix` is preferable.


## Physics interpolation — COMPLETE
Commit `d15362bcd34d9bba43333085da38ee9c9dbc1698` switched AircraftCameraController to Transform render-pose getters and added GetRenderForward/GetRenderRight/GetRenderUp. GetRenderWorldMatrix now returns Matrix by value.

Verified by user: interpolation is working and the previous aircraft jitter disappeared once both the rendered aircraft and camera followed the same interpolated pose.

Completed pipeline:
- fixed-step physics owns the real Transform pose
- KineticBody stores previous physics pose
- Scene::Update prepares interpolation using Time::FixedInterpolationAlpha()
- Transform exposes separate render position/rotation/world matrix/directions
- visual children inherit the parent's render hierarchy
- MeshRenderer/PrimitiveRenderer draw from GetRenderWorldMatrix()
- AircraftCameraController follows the target's render pose

Important invariant: simulation getters remain authoritative; render getters are visual-only.

Next: camera smoothing/cinematic follow is now a separate optional presentation layer on top of correct interpolation, or move directly into the next gameplay/rendering system if current camera feel is acceptable.


## Camera update/smoothing architecture decision
- Keep AircraftCameraController in LateUpdate, not Update. Interpolation is prepared before Update, but the camera is a dependent/follower and should run after target gameplay/visual updates. Aircraft currently performs PerformEvadeRoll() in Aircraft::OnLateUpdate(), so moving the camera to Update would sample the target before that late visual change and make ordering more brittle.
- Keep Camera as a low-level rendering/view component (projection, view, LookAt, pose access). Do not bake follow smoothing into Camera itself.
- Smoothing algorithm can be reusable, but smoothing policy/state/tuning belong in a camera controller/rig. Different cameras may require no smoothing, different axes, different strengths, spring behavior, cutscene snapping, etc.
- If the same smoothing math is reused later, extract only the generic math helper (e.g. frame-rate-independent exponential damping) or a reusable follow controller once a second real use case exists. Do not generalize prematurely.


## Basic weapon subsystem review — latest
Commit `f2b802ff1ae6100122a5b60efde3d5d912730827` added the first working weapon path:
AircraftController -> Aircraft::Fire(WeaponType) -> WeaponController::TryFire -> spawned Bullet GameObject with PrimitiveRenderer + KineticBody.
GunMuzzle is a child Transform of Body. Gun uses held LeftShoulder input; missile input is currently only a placeholder.

Important review findings:
- WeaponController::TryFire currently ignores the requested WeaponType, so pressing the missile input will also fire the gun whenever the gun cooldown allows. Split/switch by WeaponType before implementing missile behavior.
- Current gun launch uses one AddForce(direction * 9000) call. With current KineticBody semantics this is a one-fixed-step force, so resulting bullet speed depends on mass and fixedDeltaTime. For a projectile muzzle speed, prefer SetLinearVelocity(...) now, or implement proper ForceMode semantics first.
- Unity-style correction: ForceMode should conceptually be Force, Acceleration, Impulse, VelocityChange. Explosion is not a force mode; an explosion is a separate radial-force operation (e.g. AddExplosionForce / overlap + impulses). Rename/remove the current ForceMode::Explosive when implementing modes.
- Bullet currently has no Projectile/lifetime component in this commit, so bullets persist indefinitely. Add a temporary lifetime/destruction path before pooling.

Planned engine/gameplay directions:
- visual projectile shape and collision shape should remain independent; a sphere collider is reasonable even if future visuals are an elongated bolt/trail
- BasicPrimitiveMaterial already supports BasicEffect emissive color when supplied to PrimitiveRenderer, but true visible glow requires bloom/post-process; trail rendering can provide the laser-streak look later
- object pooling is appropriate for bullets/missiles after basic projectile lifetime/destruction and collision behavior are established; pooled reuse must reset Transform, KineticBody velocity/forces/interpolation state and active/enabled state
- current GameObjectManager already has deferred destruction via Destroy(GameObject&) -> RequestDestroy() -> EndFrame removal. A Unity-like Destroy convenience can wrap this; do not duplicate destruction ownership
- Instantiate should eventually mean cloning/spawning a prefab/prototype, not just alias CreateGameObject. Ownership belongs to GameObjectManager/Scene; prefer Scene/GameObjectManager Instantiate once prefab/template data exists rather than instance GameObject owning creation
- add an engine-level debug text/log overlay so remote components can call a simple Debug::Log/ScreenLog-style API; central renderer consumes queued messages using existing SpriteBatch/SpriteFont. Keep it separate from MainScene UI and preferably available only/primarily in debug builds.
- NVIDIA overlay presence is not evidence that the GPU is or is not being used. AERO_RIOT already creates a D3D11 device/context and renders through it; overlay appearance depends on NVIDIA app/overlay detection, support/settings/hooking. Verify GPU usage via Task Manager GPU engine/performance metrics or NVIDIA performance tools instead.


## Projectile component ownership decision
Latest user code fixed WeaponType dispatch, changed gun launch to explicit muzzle velocity, initialized pointers, and corrected ForceMode names to Force/Acceleration/Impulse/VelocityChange. Note: KineticBody::AddForce currently still ignores the mode argument; actual semantics are not implemented yet.

Next projectile architecture:
- WeaponController owns weapon-level policy: fire rate/cooldown, selected weapon, ammo later, muzzle/hardpoint selection, spawning/configuring shots.
- Each spawned projectile owns its own per-instance runtime behavior. Add a Bullet component for bullet lifetime and bullet-specific KineticBody setup/behavior; later add a HomingMissile (or Missile) component for target tracking/guidance/lifetime.
- WeaponController should not track lifetime timers for individual projectiles.
- With current deferred GameObject lifecycle, all components can be attached before the pending GameObject initializes next frame; Bullet can safely acquire its sibling KineticBody during OnInitialize/OnStart.
- For current destruction-based lifecycle, Bullet can count lifetime and RequestDestroy its own GameObject.
- Important for future pooling: OnStart runs only once, so pooled projectiles will need an explicit per-shot reset/Launch/Activate method (or future OnEnable lifecycle) to reset timer, velocity, interpolation state, target, etc. Do not rely solely on OnStart for reusable projectile initialization.
- Prefer WeaponController to provide spawn-specific data while the projectile component applies it to its own KineticBody. Visual and collision shape remain independent.


## Bullet lifecycle — WORKING
Commit `de90ca180d0c632e023b3d69f124ca88dbb41735` fixed the Bullet lifecycle:
- WeaponController calls Bullet::RequestLaunch(direction, speed)
- RequestLaunch stores launch data while the GameObject is still pending
- Bullet::OnStart acquires/configures sibling KineticBody and consumes the pending launch request
- Bullet owns its lifetime timer
- lifetime expiry destroys the whole Bullet GameObject via RequestDestroy
User verified it is working.

Current caveat for future pooling: RequestLaunch currently defers only into OnStart; once a pooled Bullet has already started, reactivation will need an explicit relaunch/reset path (or OnEnable/OnDisable lifecycle). Do not address until pooling is actually implemented.

Next milestone: add a reusable engine-level on-screen debug/log overlay before collision work, so arbitrary components can emit temporary diagnostics without routing through a Scene subclass.

## Repository
GitHub: https://github.com/mohit-kumar-singh55/AERO_RIOT
Default branch: `master`
