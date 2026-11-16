# Cutscene animation, camera and effect parity

Target: `metastigma/CRO-U-metastigma`, branch `fix/cutscene-animation-camera-effects`.

This is a targeted port from the current `DuIslingr/CRO-U` upstream behavior, not a wholesale upstream merge. It leaves the already merged PR #51 / #52 fixes and their texture/material work intact.

## Included

- Event opcode 49 (`Anim2`) reads the start-frame and blend-frame operands and restarts the requested action from that frame.
- NPC animation changes preserve a per-instance previous pose and blend into the next action; explicit Anim2 uses the supplied blend duration.
- Opcode 52 (`FrmAdjc2`) distinguishes snap-to-record-angle, turn-over-move, and vertical movement without turning.
- `CameraPlayer` cancels an in-flight interpolation while retaining the last scripted eye/target.
- NPC move/scale, 3D camera changes, and painted-field camera slides take their first interpolation step during the command frame.
- Scripted camera eye and target both interpolate from their fixed start values to the selected scene-record values; the target must not use its changing current value as the interpolation origin.
- Player-follow camera eases by 30% per event update and snaps once after map entry.
- Painted Rooms modes share the scrolling viewport path; Fill zoom and the layer-3 phone/Vita viewport clamp are included.
- Scripted 3D views use the map/camera-derived near/far range.
- `FrmPlayEff` at a scene-record position faces the camera; `FrmPlayEff2` retains its record Euler angles and material depth-write flag.
- Foreground is explicitly drawn in the background's stage-space origin.
- OpenGL and Vulkan alpha tests use the upstream half-8-bit-step tolerance.

## Not copied again

- Vita logo positioning through `screenToLayout()` and `title1` atlas selection from the actual loaded sheet are already present in this fork.
- The current horizontal `FrmPanCam` X direction, CRHD/PNG/DDS support, material/texture fixes, and merged PR #51 / #52 changes are left intact.
- Unrelated upstream game-system, packaging, iOS and UI changes are excluded.

## Validation status

The port was compared against the upstream implementation at source level. CI and runtime playback still need confirmation on this PR head. Recommended game-data checks:

1. Intro `Anim2`: the scripted initial frame and blend to the later frame (the upstream example uses 30 -> 70).
2. A camera set / pan / CameraPlayer sequence.
3. One `FrmPlayEff` and one `FrmPlayEff2` instance.
4. Painted Rooms Original / Whole / Fill on desktop and phone/Vita layouts.
5. Both GL and Vulkan alpha-tested character materials.


### Intro camera target interpolation correction

The first `c101_t01.evt` camera slide (scene 1, frames 500–800) changes the eye and its paired look-at actor together. The previous target expression subtracted the current target rather than `tgtFrom`, causing a 300-frame slide to stop roughly halfway to its authored target. The target now uses the same fixed-origin linear interpolation as the eye. Camera actor pairing and the event data remain unchanged.
