---
story: STORY-EP-035
parent_srs: [SRS-EP-10]
date: 2026-08-16
status: awaiting-instrumentation
---

# EP-035 — enclose area/length corpus

Formula: closed ring (last→first). `L` = polyline length. `A` = |shoelace|. Log `A/L` and `A/L²`.
Do not change enclose verdict in STORY-EP-035.

## Classes

- `handwriting` — must stay ink (`his`, other thin words)
- `boundary` — intended empty box / circle / wiggle-box
- `acceptable-FP` — O, D, P, B, C, G, fat-C, fat-W if they become a box

## Capture

| id | class | word/shape | size | L | A | A/L | A/L² | [recog] fail= | notes |
|---|---|---|---|---|---|---|---|---|---|
| | | | small/medium | | | | | | |

## Analysis (after capture)

- Do `handwriting` and `acceptable-FP` / `boundary` overlap on A/L?
- On A/L²?
- Candidate bar (or: does not separate):
