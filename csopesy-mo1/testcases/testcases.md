# CSOPESY MO1 — Test Cases

Run from the `csopesy-mo1/` directory. Copy the relevant config file to `config.txt`
before each test, then launch `.\build\csopesy.exe`.

---

## Test Case 1 — FCFS Baseline

**Purpose:** Verify FCFS scheduling, `screen -ls` format, `report-util` output, and
that processes cycle through Running → Finished correctly on 2 cores.

**Setup:**
```
copy testcases\tc1_config.txt config.txt
.\build\csopesy.exe
```

**Config:** 2 cores · FCFS · 10–20 instructions · delays-per-exec 0 · batch-freq 1

**Commands to run (in order):**
```
initialize
scheduler-start
screen -ls
scheduler-stop
screen -ls
report-util
exit
```

**Expected output:**

| Command | Expected |
|---|---|
| `initialize` | `Initialized: 2 core(s), scheduler=FCFS` |
| `scheduler-start` | `batch generation started` |
| `screen -ls` (first, ~1 s after start) | CPU Utilization 50–100%, ≥1 process in Running with `X / total` |
| `scheduler-stop` | `batch generation stopped` |
| `screen -ls` (second, ~5 s later) | Running: (none) or near-empty; Finished: multiple processes each showing `N / N` |
| `report-util` | `Report generated at csopesy-log.txt`; file exists with plain-text copy of the table |
| `exit` | Clean shutdown (no hang) |

**What to verify:**
- Each finished process shows the same number for both sides of `N / N`.
- `csopesy-log.txt` contains no ANSI escape codes (plain text).
- `output/p01.txt` … `output/pXX.txt` exist with timestamped log lines.

---

## Test Case 2 — RR Preemption Visible

**Purpose:** Verify that RR with `quantum-cycles 3` actually cycles processes between
cores — i.e., on repeated `screen -ls` calls the progress counter (`X / total`)
advances slowly and multiple distinct processes appear in Running.

**Setup:**
```
copy testcases\tc2_config.txt config.txt
.\build\csopesy.exe
```

**Config:** 2 cores · RR · quantum=3 · 30–50 instructions · delays-per-exec 0 · batch-freq 1

**Commands to run:**
```
initialize
scheduler-start
screen -ls
screen -ls
screen -ls
scheduler-stop
screen -ls
exit
```

Type each `screen -ls` about 1–2 seconds apart.

**Expected output:**

| Observation | Expected under RR | Would indicate bug if... |
|---|---|---|
| 1st `screen -ls` | 1–2 processes in Running, each at low `X / total` | All finished on first call → quantum not firing |
| 2nd `screen -ls` | Same processes still Running, `X` incremented (or different process on that core) | Same `X` as before → RR not making progress |
| 3rd `screen -ls` | More processes in Running or Finished; `X` clearly advancing across calls | Stuck at same `X` → preemption not working |
| After `scheduler-stop` + wait | All admitted processes eventually reach Finished | Processes hang forever → scheduler deadlock |

**Key RR proof:** On at least two consecutive `screen -ls` calls you should see the
*same process name on the same core with a different* `X / total` value — meaning it
was preempted, re-queued, and dispatched again.

---

## Test Case 3 — `screen -s` / `screen -r` and Edge Cases

**Purpose:** Verify process attach/re-attach, `process-smi` live output, and the
"not found" rejection for unknown or finished processes.

**Setup:**
```
copy testcases\tc3_config.txt config.txt
.\build\csopesy.exe
```

**Config:** 4 cores · FCFS · 40–60 instructions · delays-per-exec 0 · batch-freq 3

**Commands to run:**
```
initialize
screen -s myproc
```
*(now inside the screen sub-prompt)*
```
process-smi
process-smi
exit
```
*(back at main menu)*
```
screen -r myproc
```
*(inside again — if process is still running)*
```
process-smi
exit
```
*(back at main menu — wait ~10 s for myproc to finish)*
```
screen -r myproc
screen -r ghost
exit
```

**Expected output:**

| Command | Expected |
|---|---|
| `screen -s myproc` | Screen clears; shows `Process name: myproc`, `ID: 1`, `Logs:`, `Current instruction line: X`, `Lines of code: Y` |
| 1st `process-smi` | Same view, `X` may have advanced (process runs in background) |
| 2nd `process-smi` | `X` higher than before; if finished shows `Finished!` instead of counter |
| `exit` (from screen) | Returns to `user@csopesy:~$` prompt |
| `screen -r myproc` (while running) | Re-attaches; shows updated `X / Y` |
| `screen -r myproc` (after finished) | `Process myproc not found.` |
| `screen -r ghost` | `Process ghost not found.` |

**What to verify:**
- Logs section shows `(timestamp) Core:N "Hello world from myproc!"` entries.
- `Current instruction line` strictly increases between `process-smi` calls.
- Once `Finished!` is shown, `screen -r` on that name is properly rejected.
- The `exit` inside the screen prompt does NOT exit the whole program.

---

## Known Limitations

| Item | Behaviour | Reason |
|---|---|---|
| FOR mid-loop preemption | A FOR loop with many iterations holds the core for all of them before RR can preempt | Sub-instruction command counter not wired into the quantum tracker |
| `screen -ls` Core spacing | Shows `Core:0` not `Core: 0` | Minor format deviation from spec mockup |
| Batch-freq timing | Approximated as wall-clock ms, not true CPU tick count | Requires a shared tick counter from the scheduler |
