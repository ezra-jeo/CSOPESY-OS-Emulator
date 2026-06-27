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

## Known Limitations

| Item | Behaviour | Reason |
|---|---|---|
| FOR mid-loop preemption | A `FOR` runs all its iterations in one instruction-line step, so RR can't preempt mid-loop and the instruction-line counter doesn't advance per iteration | `ForCommand::execute` runs the whole body inline; the sub-instruction counter isn't wired into the quantum. Relevant to TC6 (flattening needed). |
| `screen -ls` Core spacing | Shows `Core:0` not `Core: 0` | Minor format deviation from the spec mockup |
