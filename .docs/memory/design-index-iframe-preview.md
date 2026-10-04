---
source: .plan/iter-005/memory/design-index-iframe-preview.md
captured: 2026-08-20
promoted: 2026-10-04
related:
  - STORY-EP-037
  - STORY-IN-034
---

# Design package navigators: paint scenes in the same document

The Cursor / VS Code HTML preview renders the open file but does not load a nested local
`<iframe src="other.html">`. A multi-scene `index.html` whose only paint path is that iframe
shows blank scenes in the IDE, while Chrome renders them.

- Inline each scene as a `.scene-frame` article in `index.html`. Keep the named iframe at
  `src="about:blank"` for the design gate.
- Scope states-showcase CSS to its article. An unscoped rule such as
  `.c-tool-chip { position: static }` breaks every scene.
- Keep the sibling scene files. They stay the canonical handoff to the developer.

The template, the designer skills and `design_gate.py` were fixed on 2026-08-20.
