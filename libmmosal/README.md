# vayu open-source vendor libraries

Open-source reimplementations of the proprietary shared libraries shipped in
the vayu vendor image. Each library keeps the ABI of the blob it replaces
(soname, exported symbols, signatures and observable behaviour) so it can be
dropped in without touching its consumers.

Every entry is written from static analysis of the blob — disassembly plus
emulation of the original code for behavioural ground truth — and is verified
against that ground truth before the blob is removed from
`proprietary-files.txt`. Libraries are done one at a time, smallest and
most self-contained first.

## Libraries

| Library | Replaces | Source group | ABI surface | How it was reverse engineered | Verification |
| ------- | -------- | ------------ | ----------- | ----------------------------- | ------------ |
| `libmmosal` | `system_ext/lib{,64}/libmmosal.so` | LA.UM.9.1.r1 (the blobs carry `vendor/qcom/proprietary/commonsys/mm-osal/src/*.c` in their log strings) | 106 exported entry points (`MM_*` memory/critical-section/debug/time/timer/thread/signal/file wrappers and the `std_*` string helpers), plus the data objects `mOSALMutex` and `MM_Thread_DefaultPriority` and the compiler-generated `__cfi_check` | AArch64 and Thumb-2 ARM32 blobs of the same 106-symbol API. `.gnu_debugdata` supplied the internal symbol table (file/line names, `decorate_msg`, the UBSan handlers); `.rodata` gave every log format, the `LOGMASK` config syntax and the `mm->file` mode→`open(2)` flag table; `.bss`/`.data` gave the 37-entry log-mask table, the `mOSALMutex` initialiser (`{0x4000,0,…}`, i.e. `PTHREAD_MUTEX_INITIALIZER`) and the `__aarch64_have_lse_atomics` byte. Every wrapper was decoded with `llvm-objdump`, the structures recovered from their allocation sizes and field offsets (critical section, timer, signal-queue, 33-slot signal node, `MM_Time` date/time record), and the two ABIs cross-read against each other to separate ABI-dependent layout from source-level logic. | Differential emulation of blob vs rebuild over a 28-case input matrix on **both** architectures — 28/28 cases pass on each ABI, 10/10 behaviour mutants caught and 4/4 equivalent controls clean on each ABI, and the interposable-edge set is identical (18 symbols) per ABI; see `VERIFICATION.md`. |

## Notes

* `libmmosal` is built for both ABI widths: the image ships
  `system_ext/lib64/libmmosal.so` and `system_ext/lib/libmmosal.so` with the
  same 109 dynamic exports, and the reimplementation is a single source file
  whose structures are sized by the ABI exactly as the blob's are
  (`pthread_mutex_t` 40 bytes on LP64 / 4 on ILP32, one 32-byte signal node vs
  20, etc.).

* **The blob is UBSan-instrumented, and two of its traps are reachable.**
  `std_scanux` computes `c - '0'` unconditionally and traps on any byte below
  `'0'`, and it also traps on `(c|0x20) - 'a'` inside the `0x` prefix probe;
  `GetLogMask` traps on an id below 6000 and on an index ≥ 37;
  `std_scanul`/`std_scanull` trap on `INT_MIN`/`INT64_MIN`. `abort()` is
  reached through `write(2, "ubsan: <op>-overflow by 0x<value>\n")` and a weak
  `android_set_abort_message` call. The reimplementation reproduces the trap
  *positions* (same inputs abort in both), not the hex value, which is the
  caller's return address and therefore differs by construction.

* The interpreter's strings are `1.1`, `/data/mmosal_logmask.cfg`,
  `/data/misc/media/mmosal_logmask.cfg`, `vendor.debug.mmosal.config`,
  `ro.arch`/`exynos9810` (the `__aarch64_have_lse_atomics` workaround, which
  lives in the compiler's built-in CPU-feature runtime and is not part of this
  library's source) and `/data/misc/media/mmosal_...`. `MM_File_CreateW`
  genuinely opens the *uninitialised* conversion buffer first and only then
  converts the wide string; that defect is reproduced verbatim because the
  first `open()` is observable.

* The blob calls its own exported entry points through the PLT, so a consumer
  can interpose them. The rebuild must therefore keep those symbols
  default-visibility and must be compiled with `-fsemantic-interposition`
  (clang's default would inline the intra-library calls and silently drop the
  interposable edges). Both the `Android.bp` and the verification build do
  this; the edge set then matches the blob exactly, per target, on both ABIs.

* `__cfi_check` is compiler-generated CFI glue in the blob (a 2804-byte type-id
  binary search) and `__cfi_slowpath` is an imported stub. The rebuild is not
  CFI-instrumented, so neither is reached; `__cfi_check` is still exported so
  the dynamic symbol set stays a superset of the blob's.

* `mOSALMutex` is exported (a `pthread_mutex_t` in `.data`, 40 bytes on LP64 /
  4 on ILP32) and `MM_Thread_DefaultPriority` is a `.rodata` `int` worth 0.
  Both are re-exported with the same types because they are part of the ABI.
