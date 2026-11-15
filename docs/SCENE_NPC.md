# Scene NPC behavior: hidden actors, local floors and news marks

Ported selectively from upstream commit
[261553d432](https://github.com/DuIslingr/CRO-U/commit/261553d43245115563e0b0d305b10c5ff418ace2).
This is separate from the fork's field-shadow work (#37) and Joint attachment chains (#39).

## What changes

### Hidden scene actors retain their model state

The port previously kept only visible `NpcDraw` instances. After `Kill`, a hidden actor therefore
lost the instance that held its current action, animation clock, blend state and fade. A later
`Born` recreated it from the default action instead of restoring the scene's state.

The new `npcHeld` list preserves loaded-but-hidden model instances. Event commands resolve the
model by NPC id in both visible and held lists, but hidden models do not advance animation or
fade while they are outside the renderer's `mdl_put` path. A fade that lights a hidden model
puts it back into the visible list; `Kill` hides it again. `Born` restores the same instance.

Script-loaded models are registered when `loadNpc` reads them, including when their NPC hidden
bit is set. Room changes and title teardown clear the held instances with the visible ones.

### Script placement uses the local floor list

`scriptFloor` follows the original placement rule: find the KD node around the recorded point,
then query only that node's floor list with twice the requested radius. For `Setc` / `FrmSetc`
NPC placement, the radius is 10. If there is no floor in that node within 20 units of the record's
Y, the actor keeps the event record's Y; it is not snapped to an arbitrary floor elsewhere in the
map. The player keeps the existing placement path.

The collision parser now distinguishes a file with no node lists from a node whose entries are
only in the other-list slots (+8 / +10). In particular, a scene-stage node with a +10 list and
empty +4/+6 lists must keep its floor/wall lists empty. Its polygons must not be promoted to floors
by the no-lists fallback. This is exercised with a synthetic `.cmp` test.

### FlagWindow news markers

The red type-1 `!` is drawn for each active person whose NPC row's +22 flag is set. It is distinct
from the normal FollowWindow prompt. A nearby/lingering FollowWindow or FlagWindow prompt takes
the marker's place. Talking to the person suppresses the news mark until the person is reached
again; the FollowWindow/FlagWindow linger lasts ten field frames after leaving reach. Objects keep
priority over people when selecting the normal interaction prompt.

## Regression coverage

- `collision-scene-lists`: a node with only a +10 entry has no floor; a file with no node lists
  retains the normal-based legacy fallback.
- The in-process scene self-test checks hidden `Anim` state, a fade that lights a hidden model,
  `Kill` during that fade, and `Born` restoring the same action/alpha/fade state.
- The scene self-test also checks the local floor query's 20-unit Y range and the FlagWindow
  mark/talk/clear state transitions.

Run the collision parser regression with:

```sh
ctest --test-dir engine/build -R collision-scene-lists --output-on-failure
```

The full in-process self-test is invoked with the project's `--selftest` option.

## Reference and verification boundary

The user's Library contains the Omega Vita `eboot.bin` archive and its IDA listing
(`EbootOmegaIDA.zip`, identified as PCSG00499). The IDA listing was unpacked and inspected as
a separate reference. The upstream commit's address annotations span different release builds;
they must not be treated as exact Vita addresses without matching the relevant function and
callers in that build. This port therefore keeps the floor/marker semantics explicit in the C++
code and tests rather than claiming an unverified one-to-one address mapping.

Automated tests and CI are the pre-merge checks. The following runtime checks remain outstanding
until tested with Omega game data: the stage NPC whose scene record has no local floor, a hidden
Vieg/Olgar pose across `Kill` / `Born`, and the five red hall markers after the opening. Do not
record these as runtime-confirmed until that in-game check is performed.
