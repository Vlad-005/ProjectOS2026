# AI Usage Report — Phases 1 and 2

## Overview
This document describes how AI tools were used to develop and update the `city_manager` and `monitor_reports` programs across Phases 1 and 2.

## Phase 1
Phase 1 established the core `city_manager` functionality and district/report storage structure.

### What was done
- Added district creation and initialization.
- Implemented report creation, listing, and deletion.
- Implemented snapshot creation and comparison logic.
- Added `remove_district` using `fork()` + `execlp("rm", "rm", "-rf", district, NULL)`.

### AI usage
- Used the AI programming assistant in VS Code to inspect and modify existing code.
- Applied safe code changes in the workspace by editing specific functions and validating behavior.

## Phase 2
Phase 2 added monitor notification support and completed required deliverables.

### What was done
- Implemented `monitor_reports.c` with:
  - `.monitor_pid` creation at startup.
  - SIGUSR1 handling.
  - SIGINT handling and graceful shutdown with PID file cleanup.
- Updated `city_manager.c` to:
  - read `.monitor_pid` from the project root.
  - send `SIGUSR1` to the monitor when adding a report.
  - log a success or failure message to `district/logged_district`.
- Verified both programs compile cleanly with `clang -Wall -Wextra -std=c11`.
- Added data so the workspace contains at least 2 district directories and 5 total reports.

### AI usage
- Used the AI assistant to review current file contents and determine required fixes.
- Added monitor notification and logging support by editing the relevant code sections.
- Used terminal commands to compile the programs and confirm the updated code built successfully.
- Added sample district reports using the compiled `city_manager` program.

