---
id: ADR-0006
title: System OpenSSH for pen streaming
status: accepted
date: 2026-07-05
deciders: [architect]
supersedes: null
---

# ADR-0006 — System OpenSSH for pen streaming

## Context

The Python app used Paramiko for SSH. The Swift port needed a native transport without adding a third-party SSH library dependency. Pen stream is a long-running `dd bs=16 if=/dev/input/event1` pipe over SSH.

## Decision

Use macOS system `/usr/bin/ssh` via `Process` for streaming and authentication, and `/usr/bin/ssh-keygen` for 3072-bit RSA key generation. Key installation uses temporary `SSH_ASKPASS` helper with Keychain-stored password on first setup.

## Consequences

- Faster port path; shell-debuggable transport.
- Launch and error handling are process-oriented (exit codes, stderr).
- No in-process SSH reconnection API — acceptable given single long-lived stream per session.
- Python Paramiko behavior archived in `legacy/python/` for reference.

## Alternatives Considered

| Alternative | Why rejected |
|---|---|
| Paramiko equivalent (Swift SSH lib) | Extra dependency; integration cost during port |
| In-process libssh2 | Same dependency cost; less shell-debuggable |
| Direct USB/IP without SSH | Out of scope; RM2 exposes input via Linux evdev over SSH |

## Porting note

If Python reaches tablet while Swift does not on same machine, likely OS network routing state (e.g. `10.11.99.1` routed via Wi-Fi not USB) — not a transport trick in Python. See [swift-porting.md](../memory/swift-porting.md).
