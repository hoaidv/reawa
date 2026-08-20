---
persona: designer
captured: 2026-08-20
trigger: human-steering
iter: iter-005
related:
  - STORY-EP-037
  - STORY-IN-034
  - .plan/iter-005/design/hand-touch/index.html
  - .plan/iter-005/design/pen-button-map/index.html
  - .plan/iter-005/design/viewport-follow-epaper/index.html
  - .plan/iter-005/design/viewport-follow-infini/index.html
scope: adlc-upstream
---

# Design index.html iframe is blank in IDE HTML preview

## What happened

Human reported that `.plan/iter-005/design/hand-touch/index.html` and `pen-button-map/index.html` navigators did not render scenes, while the same sibling scene files painted when opened on their own. Chrome `file://` screenshots of the iframe navigators *did* paint. The failure is the IDE HTML preview: it inlines the open document (so CSS/images on a scene file work) but does not load nested local `<iframe src="other.html">`.

The same defect then shipped again on `viewport-follow-epaper` and `viewport-follow-infini` because the designer template still encoded iframe-as-the-only-paint-path.

A first CSS-only attempt (drop `./`, comment `.hint`) did not help. Working iter-003/004 packages still use iframes and would fail the same way in that preview.

## What I learned

- **Worked:** Inlining each scene body into `.scene-frame` articles in `index.html`, keeping a named `iframe[name=scene-preview]` at `src="about:blank"` for the gate, and `preventDefault` on sidebar links so chrome stays while switching. Scope states-showcase CSS to that article (unscoped `.c-tool-chip { position: static }` breaks every scene). Desktop: `min-height: 28rem`, not `height: calc(100vh - 4rem)` (0 in some nested previews).
- **Didn't work:** Nested iframe `src` as the only paint path. Template `design-package-index.html` encoded that path; it is fine in a real browser and blank in Cursor/VS Code HTML preview. Skill text that said “scenes load in the named iframe” caused the repeat.
- **Do differently:** When shipping a multi-scene navigator that humans open via IDE preview, paint scenes in the same document. Keep sibling scene files as the canonical `/dev` handoff.

## Skill hotfix (landed 2026-08-20, human-requested)

Root cause was the template + Validation-nav wording, not a one-off package miss. Landed in:

- `.agent/templates/plan/design-package-index.html` + `design-package.md`
- designer `orchestrator.md` Validation-nav invariant, `rules/core.md` 9b, `execute-design-story`, `spec-2-html-design`, `ui-spec-gate` 9d, `html-ui-quality`, `agent.md`
- `adlc` `design_gate.py` `check_multi_scene_index`: current-iter packages must have `.scene-frame` and must not point iframe `src` at a local `.html`

## Promotion note (iter close only)

_Review at retro. Promote to `.docs/memory/` (project). ADLC skill/template/gate already patched under human hotfix request._
