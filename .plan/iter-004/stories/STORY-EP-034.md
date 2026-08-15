---
id: STORY-EP-034
title: USB Ethernet stay-up and SSH/TCP keepalives
kind: implement
parent_srs: [SRS-EP-08]
parent_req: [REQ-07]
status: draft
priority: P2
iter: iter-004
estimate: 3
owner: dev
depends_on: []           # e.g. [STORY-EP-034] — implement UI waits on design
acceptance_criteria:
  - "Given … When … Then …"
# Filled by /designer when kind=design (outputs); copied onto implement UI stories after design done:
design_package: ""       # .plan/iter-004/design/<slug>/
ui_spec: ""              # path or [UI-<MOD>-NN]
scenes: []               # e.g. […/login-default.html, …/login-wrong-credentials.html]
hifi: ""                 # primary/default scene path (…/login-default.html)
wireframe: ""            # only when low-fi explicitly requested
---

# STORY-EP-034 — USB Ethernet stay-up and SSH/TCP keepalives

<!-- kind: design → Designs UI for … -->
<!-- kind: implement → Implements … -->
[SRS-EP-08](../../.docs/modules/<module>/features/<feature>/srs-logic.md)

## Kind

| Field | Value |
|---|---|
| Kind | `design` or `implement` |
| Owner persona | `designer` or `dev` |
| Depends on | list design story ids when this is implement + UI |

## Design story (when `kind: design`)

SM and Designer co-author from BRD/PRD/SRS*. Owner: **designer**.

**Output home (working package):** `.plan/iter-004/design/<slug>/`

| Artifact | Path |
|---|---|
| Design package | `design_package` frontmatter |
| UI Spec | `ui_spec` |
| Scene HTML (N files) | `scenes` — `<slug>-<scenario>.html` (package-contained; link tokens.css + common.css) |
| Primary scene | `hifi` — usually `…/<slug>-default.html` |
| HTML grey-box | `wireframe` (only if SRS/human requested low-fi) |
| Iter system WIP | `.plan/iter-004/design/system/` |
| Shared design system | `.docs/DESIGN.md` + `.docs/design/{tokens.*,components.md,system/**,index.md}` |

### Acceptance criteria (design)

- Platform profile and responsive strategy recorded in the UI Spec
- Trace matrix: every region/state → SRS + AC (or CHL)
- Design system reconciled; components classified `system` | `screen` with self-contained `.html` files
- **One package-contained scene `.html` per required business scenario** (flat files; folder only if complex); multi-scene ships iframe `index.html` (chrome stays; mobile 80% / desktop 100% preview) + relative hops; shared CSS allowlist only
- `.docs/design/index.md` row updated for this screen when it is the current final design
- `ui-spec-gate` passes

### Done when (design)

- `ui-spec-gate` pass; `ui_spec` + `scenes` + `hifi` + `design_package` set
- Linked implement stories have matching `ui_spec` / `scenes` / `hifi` / `depends_on`
- No BDD required for `kind: design`

## Implement story (when `kind: implement`)

Owner: **dev**. For visible UI, must `depends_on` a done design story before `status: ready`.

### UI handoff (when applicable)

| Artifact | Path / id |
|---|---|
| Design story | `depends_on` |
| UI Spec | `ui_spec` |
| Scene HTML | `scenes` / `hifi` (primary) |
| Design contract | `.docs/DESIGN.md` + tokens + `.docs/design/system/` |

### Done when (implement)

- All AC have green BDD scenarios
- Sync-Auditor reports no orphan for parent SRS
- For UI: implemented from approved regions/tokens/components (HTML is reference, not production code)
