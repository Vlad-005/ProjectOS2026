# AI Usage Report — All Phases

## Overview
This document describes how AI tools were used to develop and update the application through Phases 1, 2, and 3.

## Phase 1
Phase 1 established the core `city_manager` functionality and district/report storage structure.

### What was done
- Added district creation and initialization.
- Implemented report creation, listing, and deletion.
- Implemented snapshot creation and comparison logic.
- Added `remove_district` using `fork()` and `execlp("rm", "rm", "-rf", district, NULL)`.

### AI usage
- Used the AI programming assistant in VS Code to inspect and modify existing code.
- Applied safe code changes in the workspace by editing specific functions and validating behavior.

## Phase 2
Phase 2 added monitor notification support and completed the required deliverables from earlier phases.

### What was done
- Implemented `monitor_reports.c` with:
  - `.monitor_pid` creation at startup.
  - SIGUSR1 handling to produce notifications.
  - SIGINT handling and graceful shutdown with PID file cleanup.
- Updated `city_manager.c` to:
  - read `.monitor_pid` from the project root.
  - send SIGUSR1 to the monitor when adding a report.
  - log success or failure messages to `district/logged_district`.
- Verified compilation of the updated programs.
- Ensured the workspace contains multiple district directories and at least 5 total reports.

### AI usage
- Used the AI assistant to review current file contents and determine required fixes.
- Added monitor notification and logging support by editing the relevant code sections.
- Used terminal commands to compile the programs and confirm the updated code built successfully.
- Added sample district reports using the compiled `city_manager` program.

## Phase 3
Phase 3 added the new `city_hub` program and the scorer pipeline.

### What was done
- Added `city_hub.c` with an interactive CLI supporting:
  - `start_monitor`
  - `calculate_scores <list_of_districts>`
- Added `scorer.c` as a standalone external scorer program.
- Updated `monitor_reports.c` so it:
  - checks `.monitor_pid` before starting.
  - writes structured messages to stdout in the form `TYPE:message`.
  - emits `ERROR`, `INFO`, `NOTICE`, and `EXIT` messages for reliable pipe consumption.
- Implemented the hub's `start_monitor` flow:
  - city_hub forks a background `hub_mon` child.
  - `hub_mon` creates a pipe, forks `./monitor_reports`, and redirects the monitor's stdout into the pipe.
  - `hub_mon` reads lines from the pipe and prints them immediately.
  - if the monitor reports termination via `EXIT:` or `ERROR:`, `hub_mon` prints a specific hub notice.
- Implemented the hub's `calculate_scores` flow:
  - spawns a separate `./scorer` process per district.
  - redirects each scorer's stdout through a pipe.
  - collects and prints a combined workload report.
- Kept all Phase 1 and Phase 2 code intact and compiled successfully.

### AI usage
- Used the AI assistant to inspect the current workspace and verify existing files.
- Generated the combined report file and confirmed implementations using compilation and quick runtime tests.
- Used terminal-based checks to verify:
  - `city_manager` usage output
  - `monitor_reports` structured behavior
  - `scorer` execution
  - `city_hub` command parsing and scoring output


## Notes
- The current implementation preserves prior Phase 1 and Phase 2 behavior while adding Phase 3 features.
- The AI assistant aided in design, debugging, and verification steps across all phases.
