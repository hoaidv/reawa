# Handoff: Scrum Master → Developer — empty reMarkable 2 shell

Date: 2026-10-03
Story: [STORY-EP-081](../stories/STORY-EP-081.md) (Empty reMarkable 2 shell)
Track: [TRACK-008](../../tracks/TRACK-008-rebuild-epaper.md) (Rebuild Epaper)

## Ask

Archive repo-root `epaper/` to `epaper_old/`. Create a new `epaper/` whose only application code is an empty Qt `main`, plus the Docker SDK build ceremony, and produce an ARM binary when Docker and the SDK installer are present.

## Do not

- Edit product documents or the plan control plane
- Delete the archive
- Port drawing, tools, sync, or tests
- Commit

## Done when

The acceptance criteria on the story hold, or the build step names the blocker.
