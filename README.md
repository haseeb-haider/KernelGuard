# KernelGuard — Kernel Object Integrity Monitor

[![Windows Kernel Driver Build](https://github.com/haseeb-haider/KernelGuard/actions/workflows/build-windows.yml/badge.svg)](https://github.com/haseeb-haider/KernelGuard/actions/workflows/build-windows.yml)

A Windows kernel driver that detects Direct Kernel Object Manipulation (DKOM)
in real time — the defensive mirror of the techniques implemented in
[Kernelious-Rootkit](../Kernelious-Rootkit-main). Where that project hooks and
modifies kernel structures (SSDT, IDT, EPROCESS lists, page tables) to hide
activity, KernelGuard takes baseline snapshots of those same structures and
periodically re-checks them for unauthorized modification.

## Research context

The goal is to build a detector that can identify the class of attacks
demonstrated by Kernelious-Rootkit:

| Attack technique (Kernelious-Rootkit) | Detection target (KernelGuard) |
|---|---|
| SSDT hooking (`SSDTHook.c`) | SSDT entry-address baseline + periodic diff |
| IDT hooking (`IDTHook.c`) | IDT gate-descriptor baseline + periodic diff |
| SYSENTER/MSR hooking (`SYSENTERHook.c`) | `IA32_LSTAR` MSR baseline + periodic diff |
| EPROCESS list unlinking (`Process.c`) | Cross-reference `PsGetNextProcess` walk vs. a raw linked-list walk / handle table enumeration |
| Page-table manipulation (`PagetableHook.c`) | PTE protection-bit / `CR3` sanity checks around `.text` sections |

## Warning

- This code is Windows kernel driver code and is intended for research or
  educational purposes only.
- Do not run this driver on production systems — a monitor with bugs in
  kernel mode crashes the machine just as readily as the thing it's watching.
- Building and testing requires a Windows development environment and the
  Windows Driver Kit (WDK).

## Loading the Driver (Test Signing)

This driver is not WHQL/attestation signed, so a stock Windows machine will
refuse to load it. On a test VM (never production):

```powershell
bcdedit /set testsigning on
# reboot
sc create KernelGuard type= kernel binPath= "C:\path\to\KernelGuard.sys"
sc start KernelGuard
```

Revert with `bcdedit /set testsigning off` and a reboot when you're done.

## Build Instructions

Same pipeline as Kernelious-Rootkit:

1. Open `src/KernelGuard.sln` in Visual Studio on a Windows machine.
2. Select the `x64` platform.
3. Build the solution in `Debug` or `Release` mode.
4. If you receive `C1083`, install the Windows Driver Kit (WDK) and reopen
   the solution.

```powershell
.\check-wdk.ps1
.\build-driver.ps1 -Configuration Release -Platform x64
```

Build output:
  - `src/KernelGuard/x64/Release/` for Release
  - `src/KernelGuard/x64/Debug/` for Debug

## GitHub Actions

`.github/workflows/build-windows.yml` runs on `windows-2025-vs2026`, restores
the WDK/SDK from the `Microsoft.Windows.WDK.x64` NuGet packages (see
`packages.config` / `Directory.Build.props` at the repo root — standalone WDK
install is deprecated on Windows Server 2025 runners), builds the solution,
and verifies the `.sys` output.

## Phase Plan

- **Phase 1 (done) — Scaffolding.** Driver skeleton, project/build files,
  periodic monitor worker thread, and stub integrity-check modules that log
  "not yet implemented" so the framework is provably wired up end-to-end
  before any detection logic is written.
- **Phase 2 (this commit) — SSDT & IDT/MSR baseline integrity.** Locates
  `nt!KeServiceDescriptorTable` via the same pattern-scan technique
  `SSDTHook.c` uses to hook it (bounded and `MmIsAddressValid`/SEH-guarded
  here, rather than crashing on an unsupported build), snapshots every SSDT
  entry, the full IDT (CPU 0 only — see the limitation note in
  `IDTIntegrity.c`), and `IA32_LSTAR`. Re-checks on each monitor tick and
  raises `KG_LOG_ALERT` (`DbgPrint` only for now) on any mismatch.
- **Phase 3 — Process-list cross-validation.** Walk `PsGetNextProcess` and
  compare against a secondary enumeration path (e.g. handle table / thread
  list cross-reference) to catch EPROCESS unlinking.
- **Phase 4 — Reporting surface.** IOCTL interface + a minimal usermode
  client to pull alert history out of the driver for demo purposes. Also
  where the event-log alert path (beyond `DbgPrint`) lands.

## Repository Notes

- No prebuilt driver binaries are included — build on Windows with the WDK.
- `Driver.c` registers `DriverEntry` and `DriverUnload` and starts/stops the
  periodic monitor worker (`Monitor.c`).
- `SSDTIntegrity.c` and `IDTIntegrity.c` implement real baseline-capture and
  diff logic (Phase 2). `ProcessIntegrity.c` is still a Phase 1 stub pending
  Phase 3.

## License

MIT — see [LICENSE](LICENSE).
