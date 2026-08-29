# Keyboard and mouse investigation plan

Updated: 2026-08-29 EDT
Status: completed

## Overview

- Problem: document and validate low-latency native keyboard and mouse capture.
- Deliverables: independently authored C++20 declarations, host checks,
  examples, analysis notes, and a bounded native hardware probe.
- Non-goals: pairing, raw USB/Bluetooth transport, process privilege changes,
  system interception, or publishing proprietary files and analysis artifacts.

## Acceptance criteria

| ID | Observable criterion | Required evidence |
| --- | --- | --- |
| A1 | Host checks verify the 96-byte keyboard and 40-byte mouse contracts | `make check` output |
| A2 | A frozen native candidate initializes and opens both interfaces | frozen revision plus title-specific lifecycle evidence |
| A3 | Attached devices produce non-neutral keyboard and mouse records | bounded `/download0/ps5-input-probe.log` extract |
| A4 | The title closes without damaging declared console services | title-specific stop and postflight checks |

## Goal map

| Goal | Outcome | Status |
| --- | --- | --- |
| G1 | Recover and host-test application-facing contracts | passed |
| G2 | Build and freeze the native probe | passed |
| G3 | Run one bounded device validation | passed |
| G4 | Publish only supported findings | passed |

## Result

The frozen probe loaded and opened both native interfaces, captured non-neutral
keyboard and mouse records from one wireless combination receiver, and reused
the same handles after physical removal and reconnection. It stopped without a
reported application crash and passed the postflight service-health check.
Public documentation contains aggregate interoperability findings only;
private identifiers, binaries, raw logs, and analysis artifacts remain outside
the repository.
