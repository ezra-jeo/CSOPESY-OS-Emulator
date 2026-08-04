# CSOPESY MO1 — Test Cases

Run from the `csopesy-mo1/` directory. Copy the relevant config file to `config.txt`
before each test, then launch the emulator (`.\build\csopesy.exe` / `./build/csopesy`).

**TC3–TC6 below mirror the graded quiz questions exactly** (same parameters, same command
sequence, same expected output). TC1–TC2 are internal dev checks.

> Config note: the quiz writes `scheduler "rr"` (quoted) and `delay-per-exec 0` (singular).
> The parser accepts both — quoted/unquoted scheduler, and `delay-per-exec` / `delays-per-exec`.

---

## Test Case 1 — FCFS Baseline (dev)

**Config (`tc1_config.txt`):** 2 cores · FCFS · 10–20 instructions · batch-freq 1.

```
initialize
scheduler-start
screen -ls
scheduler-stop
screen -ls
report-util
exit
```

Verify: Running shows `X / total`; after stop+wait, processes reach Finished `N / N`;
`report-util` writes a plain-text `csopesy-log.txt` matching `screen -ls`.

---

## Test Case 2 — RR Preemption Visible (dev)

**Config (`tc2_config.txt`):** 2 cores · RR · quantum 3 · 30–50 instructions · batch-freq 1.

```
initialize
scheduler-start
screen -ls   (repeat 3x ~1-2 s apart)
scheduler-stop
screen -ls
exit
```

Verify: on consecutive `screen -ls` calls the same process on the same core shows a different
`X / total` — proof it was preempted, re-queued, and dispatched again.

---

## Test Case 3 — 100% CPU Utilization (Quiz Q3)

**Config (`tc3_config.txt`):** 4 cores · RR · quantum 5 · batch-freq 1 · 1000–2000 instructions ·
delay-per-exec 0.

**Sequence:**
1. `scheduler-start`
2. wait 5 seconds
3. `screen -ls`

**Expected:** `screen -ls` shows **CPU Utilization: 100%**, Cores Used `4 / 4`, Cores Available `0`,
and **mostly running** processes.

*Verified:* util 100%, cores 4/4, processes in Running.

---

## Test Case 4 — 0% Utilization + report-util (Quiz Q4)

**Config (`tc4_config.txt`):** 32 cores · RR · quantum 1 · batch-freq 2 · 100 instructions (min=max) ·
delay-per-exec 0.

**Sequence:**
1. `scheduler-start`
2. wait 10 seconds
3. `scheduler-stop`
4. wait 30 seconds
5. `screen -ls`
6. `report-util`

**Expected:** `screen -ls` shows **CPU Utilization: 0%** and **mostly finished** processes; the
`csopesy-log.txt` written by `report-util` shows the **same output** as `screen -ls`.

---

## Test Case 5 — `screen -s` Manual Processes (Quiz Q5)

**Config (`tc5_config.txt`):** 4 cores · RR · quantum 5 · batch-freq 1 · 1000 instructions (min=max) ·
delay-per-exec 0. (Instruction count per process comes from the config.)

**Sequence:**
1. Create three processes with `screen -s`: `proc-01`, `proc-02`, `proc-03`.
2. `screen -ls`
3. wait 5 seconds
4. For each process: `screen -r <name>`, then `process-smi`.

**Expected:**
- `screen -s` creates each of the 3 processes successfully.
- `screen -ls` lists all 3.
- `screen -r` + `process-smi` lets you access each one and prints its **current instruction line**
  and **total lines of code** (plus the Logs block and the Instructions listing).

---

## Test Case 6 — Increasing x / y / z (Quiz Q6)

> **Status: config only.** The quiz also requires a temporary emulator change (every process built
> with `x=y=z=0` and the fixed set `FOR([ADD(x,x,1), PRINT("Value from: "+x), ADD(y,y,1),
> PRINT("Value from: "+y), ADD(z,z,1), PRINT("Value from: "+z)], 100)`). That generator override is
> **not applied yet** — only the config is provided here.

**Config (`tc6_config.txt`):** 1 core · RR · quantum 20 · batch-freq 1 · 1000 instructions (min=max) ·
delay-per-exec 0.

**Sequence:**
1. `scheduler-start`
2. wait 5 seconds
3. `screen -r <process>`
4. `process-smi` every 2 seconds for 10 seconds
5. `exit` (leave the screen)
6. Repeat 3-5 three times, viewing a different process each time.

**Expected:** for all three viewed processes, the variables x, y, z clearly show an increasing value
(visible in the Logs as `Value from: 1`, `Value from: 2`, …).

**To make this pass you still need to:** apply the test-6 generator override, and flatten the FOR so
each `ADD`/`PRINT` is a top-level instruction (otherwise the single FOR runs all 100 iterations in
one un-preemptible step under quantum 20 — see Known Limitations).

---

## Test Case 7 — Demand-Paging Memory Manager (Memory Management Activity)

**Config (`tc7_config.txt`):** 2 cores · RR · quantum 4 · batch-freq 1 · 100 instructions (min=max) ·
delay-per-exec 0 · max-overall-mem 16384 · mem-per-frame 16 · min-mem-per-proc 64 ·
max-mem-per-proc 4096 (every rolled process footprint is exactly 4096 bytes, same effective range
as the old fixed-size config this test started from).

**Sequence:**
1. `initialize`
2. `scheduler-start`
3. wait 5 seconds
4. `scheduler-stop`
5. `screen -ls` every 2 seconds until all processes reach Finished, or >1 minute elapses
6. `exit`

**Expected:** at most 4 processes resident in memory at once (16384 / 4096); a process that can't
acquire memory when it's its turn reverts to the tail of the ready queue instead of running; a
process keeps its memory across quantum preemptions and only releases it on finish; several
processes finish over the run.

---

## Test Case 8 — Demand Paging Happy Path (spec's own worked example)

**Config (`tc8_config.txt`):** 2 cores · RR · quantum 4 · batch-freq 1 · 10-20 instructions ·
delays-per-exec 0 · max-overall-mem 2048 · mem-per-frame 256 · min-mem-per-proc 64 ·
max-mem-per-proc 1024.

```
initialize
screen -c happy 2048 "DECLARE varA 10; DECLARE varB 5; ADD varA varA varB; WRITE 0x500 varA; READ varC 0x500; PRINT(\"Result: \" + varC)"
(wait ~3 seconds — the process is already running, screen -c enters its screen immediately)
process-smi
exit
scheduler-start
(wait ~2 seconds)
scheduler-stop
screen -ls
exit
```

> Deviation from a literal 1024-byte process: `WRITE 0x500`/`READ 0x500` address byte 1280
> (`0x500`), which is out of bounds for a 1024-byte address space and would legitimately trigger
> TC9's access-violation path instead of the happy path. `screen -c` accepts any power-of-two
> size in `[64, 65536]` as an explicit override (it does not have to fall inside
> `[min-mem-per-proc, max-mem-per-proc]`), so the process is sized 2048 bytes here — matching
> `max-overall-mem` exactly (8 frames) — so `0x500` lands inside the address space.

**Verify:** `process-smi` while still attached (before ever detaching) shows `Finished!` and a
Logs line `(HH:MM:SS...) Core:N "Result: 15"` — confirmed: `varA` becomes `10+5=15`, is written to
`0x500`, read back into `varC`, and printed. Because this build's `screen -r` reports
`Process <name> not found.` for any finished, non-violated process (a pre-existing MO1-era
limitation — see Known Limitations), the log can only be inspected while still attached right
after `screen -c`, not by detaching and re-attaching later. `screen -ls` afterward lists `happy`
under Finished Processes as `6 / 6`.

---

## Test Case 9 — Access Violation

**Config (`tc9_config.txt`):** 2 cores · RR · quantum 4 · batch-freq 1 · 10-20 instructions ·
delays-per-exec 0 · max-overall-mem 512 · mem-per-frame 64 · min-mem-per-proc 64 ·
max-mem-per-proc 256.

```
initialize
screen -c violator 256 "WRITE 0x500 42"
exit
scheduler-start
(wait ~2 seconds)
scheduler-stop
screen -r violator
screen -ls
exit
```

**Verify:** `0x500` (1280) is outside `violator`'s 256-byte address space, so the very first
instruction faults as a `Violation`, not a `PageFault`. Confirmed actual output —
`screen -r violator` prints exactly:
```
Process violator shut down due to memory access violation error that occurred at 06:12:49. 0x500 invalid.
```
and `screen -ls` lists `violator` under **Finished Processes** as `1 / 1`, not under Running.

---

## Test Case 10 — Forced Thrashing / Survives Low Memory

**Config (`tc10_config.txt`):** 2 cores · RR · quantum 4 · batch-freq 1 · 20-40 instructions ·
delays-per-exec 0 · max-overall-mem 128 · mem-per-frame 64 (**only 2 frames total**) ·
min-mem-per-proc 64 · max-mem-per-proc 128 (every admitted process needs all 2 frames for itself,
and `batch-process-freq 1` keeps admitting more competitors).

```
initialize
scheduler-start
(wait ~10-20 seconds)
vmstat
process-smi
screen -ls
scheduler-stop
exit
```

**Verify:** confirmed `vmstat`'s `num paged in` / `num paged out` are both > 0 and track each
other closely (one run: 3687 in / 3685 out after 10s — FIFO eviction keeps paging in as fast as
it pages out). The emulator stays fully responsive throughout — `vmstat`, `process-smi`,
`screen -ls`, `scheduler-stop`, and `exit` all return promptly; there is no deadlock/hang.
`screen -ls`'s Running section does **not** show `0 / N` frozen forever — command counters creep
forward across repeated snapshots (e.g. `3 / 29` and `9 / 33` after 10s) — but with only 2 frames
shared across every concurrently-admitted process, throughput stays far below what the CPU-tick
count alone would suggest (100% CPU-Util, 100% Memory Util the whole time). That slow-but-nonzero,
paging-dominated
progress *is* the thrashing signature this test is checking for, as opposed to a livelocked or
crashed scheduler.

---

## Test Case 11 — Backing Store Round-Trip

**Config (`tc11_config.txt`):** identical to `tc10_config.txt` (same 2-frames-total pressure, to
force real page-out/page-in traffic against the backing store).

```
initialize
scheduler-start
(wait a few seconds so paging activity occurs)
scheduler-stop
exit
```

Plus, from a second terminal while the emulator is running: `cat csopesy-backing-store.txt`,
repeated a few seconds apart.

**Verify:** confirmed `csopesy-backing-store.txt` exists in the working directory from the moment
`MemoryManager` is constructed at `initialize` (before any fault occurs — it starts as an empty
`entries: 0` file) and is rewritten on every `handleFault` eviction thereafter. A snapshot taken
~2s after `scheduler-start` showed `entries: 2`; a second snapshot ~4s later showed `entries: 10`
with different byte values at previously-seen offsets (e.g. `owner=p01 page=0 [2]` changed from
`0` to `34954`) — proof the backing store is a live, changing on-disk mirror of evicted pages, not
a static file.

---

## Test Case 12 — Symbol Table Cap (32 Variables, 33rd Is Ignored)

**Config (`tc12_config.txt`):** 2 cores · RR · quantum 4 · batch-freq 1 · 10-20 instructions ·
delays-per-exec 0 · max-overall-mem 2048 · mem-per-frame 256 · min-mem-per-proc 64 ·
max-mem-per-proc 1024.

34-instruction program (33 `DECLARE`s, one per variable `v0`..`v32`, followed by one `PRINT`),
under the 50-instruction `screen -c` cap:

```
initialize
screen -c fulltable 1024 "DECLARE v0 0; DECLARE v1 1; DECLARE v2 2; DECLARE v3 3; DECLARE v4 4; DECLARE v5 5; DECLARE v6 6; DECLARE v7 7; DECLARE v8 8; DECLARE v9 9; DECLARE v10 10; DECLARE v11 11; DECLARE v12 12; DECLARE v13 13; DECLARE v14 14; DECLARE v15 15; DECLARE v16 16; DECLARE v17 17; DECLARE v18 18; DECLARE v19 19; DECLARE v20 20; DECLARE v21 21; DECLARE v22 22; DECLARE v23 23; DECLARE v24 24; DECLARE v25 25; DECLARE v26 26; DECLARE v27 27; DECLARE v28 28; DECLARE v29 29; DECLARE v30 30; DECLARE v31 31; DECLARE v32 32; PRINT(\"v32=\" + v32)"
(wait ~10-15 seconds — screen -c enters its screen immediately, so stay attached)
process-smi
exit
exit
```

**Verify:** confirmed the process completes (`Finished!`) and its only log line is
`"v32=0"` — not `"v32=32"`. `v0`..`v31` fill all 32 symbol-table slots (`SymbolTable::MAX_VARS`);
the 33rd `DECLARE v32 32` finds `offsetFor("v32")` returns -1 (table full) and is silently
ignored per spec. The later `PRINT("v32=" + v32)` then hits `Process::readVar`'s
auto-declare-as-0 path for the still-unknown `v32`, printing `0`.

---

## Test Case 13 — Config Rejection (Power-of-Two / Range Validation)

Two named variants instead of one config, since this test demonstrates two independent
validation failures that can't both be expressed as a single `config.txt`:

**Config (`tc13a_config.txt`):** everything else default; `mem-per-frame 100` (not a power of 2).

**Config (`tc13b_config.txt`):** everything else default; `max-overall-mem 32` (a power of 2, but
below the 64-byte floor).

```
initialize
screen -ls
exit
```
(run once per config variant)

**Verify:** confirmed real `SystemConfig::validate()` output, quoted exactly, no paraphrasing:
- `tc13a_config.txt` → `initialize failed: mem-per-frame must be a power of 2, got 100`
- `tc13b_config.txt` → `initialize failed: max-overall-mem must be in [64, 65536], got 32`

In both cases the system stays un-initialized: the following `screen -ls` prints
`error: run initialize first`.

---

## Test Case 14 — Invalid Memory Allocation on `screen -s`/`screen -c`

**Config:** `tc14_config.txt`, identical to `tc8_config.txt`.

```
initialize
screen -s p1 100
screen -s p2 32
screen -ls
exit
```

**Verify:** confirmed both `screen -s` attempts print `invalid memory allocation` (100 isn't a
power of 2; 32 is below the 64-byte floor — `Console::resolveMemSize` rejects both before a
process is ever constructed) and `screen -ls` afterward shows 0 cores used and empty Running /
Sleeping / Finished sections — neither `p1` nor `p2` was created.

---

## Known Limitations

| Item | Behaviour | Reason |
|---|---|---|
| FOR mid-loop preemption | A `FOR` runs all its iterations in one instruction-line step, so RR can't preempt mid-loop and the instruction-line counter doesn't advance per iteration | `ForCommand::execute` runs the whole body inline; the sub-instruction counter isn't wired into the quantum. Relevant to TC6 (flattening needed). |
| `screen -ls` Core spacing | Shows `Core:0` not `Core: 0` | Minor format deviation from the spec mockup |
| `screen -r` on a finished, non-violated process | Prints `Process <name> not found.` — logs of a finished process are unrecoverable once detached | Same pre-existing MO1-era behaviour noted in STATE.md; relevant to TC8, whose Verify step reads the log while still attached instead of by re-attaching |
