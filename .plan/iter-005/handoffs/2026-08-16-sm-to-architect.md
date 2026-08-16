---
from: sm
to: architect
date: 2026-08-16
iter: iter-005
---

# Hand-off: SM → Architect — TRACK-005 W0 SRS bind

Human: add REQ-10→18 except REQ-15 to iter-005; lock TRACK-005 **vertical · verified · wip 2**.

Stories exist as **draft**, parent_srs = nearest *existing* section. Bind real SRS (and ADRs) then SM will retarget `parent_srs`.

## Scope

Must: REQ-10 (incl. two-finger pan, formerly REQ-16), 11, 12, 13, 14, 18 + infini REQ-05.  
Should: REQ-17.  
Won't this track: REQ-15.

## Asks

1. Decompose those REQs into feature SRS (`lifecycle: active`).
2. ADRs as needed: viewport last-writer (REQ-10), clipboard ops, barrel button vs eraser nib, endpoint-ink membership, attachment `t` on rest spine.
3. Do not spend W0 on REQ-15 / REQ-08.

## Next after you

SM retargets stories · **`/designer` EP-037 ∥ IN-034**.
