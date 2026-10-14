# vayu open-source vendor libraries — `qtimutex`

Section for `vendor/qcom/opensource/libraries/README.md`, in that file's format.

## Libraries

| Library | Replaces | Source group | ABI surface | How it was reverse engineered | Verification |
| ------- | -------- | ------------ | ----------- | ----------------------------- | ------------ |
| `qtimutex` | `vendor/lib64/qtimutex.so` | LA.UM.9.1.r1-13700-SMxxx0.QSSI13.0 | 68 dynamic symbols: `qtimutex::QtiMutexDefaults` (15, including both `C1`/`C2` spellings), `QtiRecursiveMutex` (3), `QtiSharedMutex` (6), `QtiShmMutex` (5), `QtiShmConditionVariable` (6), the 13 `qtimutex_*` C entry points, the `std::__1::__function::__func<void (*)(), …>` instantiation of `std::function` (5, weak) and 14 compiler-rt/libgcc quad precision builtins | AArch64 blob, 13528 bytes of `.text`, linking `libcutils`, `libc++`, `libc`, `libm` and `libdl`; consumed by `libqcrilFramework.so`, `libril-qc-hal-qmi.so` and `qtibus.so`, which import the C++ methods, the C API and (from `libril-qc-hal-qmi.so`) the two `setDefault*Logger(std::function<void (std::string)>)` setters. `llvm-objdump` decoded each entry point; the class layouts came from the access offsets (`std::function` = 32 byte buffer with `__f_` at +0x20, `std::string` SSO/long forms, `std::recursive_timed_mutex` = `mutex`@0, `cv`@0x28, `__count_`@0x58, `__owner_`@0x60, `std::shared_timed_mutex` = `mutex`@0, `__gate1_`@0x28, `__gate2_`@0x58, `__state_`@0x88) and the two `new`-site allocation sizes (0x68 / 0x8c) that confirm them; the property name, the `"false"` bypass value and every log string came from `.rodata`; the singleton's default deadlock action came from the `R_AARCH64_GLOB_DAT` relocation at `.got` 0x8238, which names `abort`. The reimplementation is driven through a Unicorn harness against the blob over an 18 case matrix (returns, log ordering, condition variable waits/notifies, aborts, ABI-level `std::function` inspection) with 7 mutants as negative controls. | `18/18` cases identical, `7/7` mutants rejected, `reloc_counts` balanced on both objects, 23/23 interposable targets preserved (details below) |

## What the library is

Every entry point is the same shape, reproduced exactly:

1. read `QtiMutexDefaults::getInstance().getPropertyName()`; if it is not empty
   and `property_get(name, value, "")` reads exactly `"false"`, call the
   underlying primitive directly and return (0x494c in
   `QtiRecursiveMutex::lock()`, identical in all twelve entry points);
2. otherwise copy the configured `debugLogger`, `deadlockLogger` and
   `deadlockAction` out of the singleton (0x49b8-0x49f0);
3. if `getDebugFlag()` is set, log `"Entering …"` (the blob re-reads the flag at
   every call site and invokes the logger unconditionally, so an active flag
   with no logger installed aborts through libc++'s `__throw_bad_function_call`,
   reached from 0x4a3c);
4. `lock()`/`lock_shared()` only: build a deadline of
   `steady_clock::now() + milliseconds(getDeadlockTimeout())` (0x4a60-0x4a78,
   the `madd` by 0x4240/0xf = 1000000 ns per ms) and retry
   `try_lock_until(deadline)` / `try_lock_shared_until(deadline)` until it
   succeeds or the deadline passes;
5. on timeout log `"FAILURE - QtiSharedMutex timeout."` through the *deadlock*
   logger, then call the deadlock action (0x4aa4-0x4b14); the action is
   `std::function<void()>` over `abort` by default, so an unconfigured
   deadlock kills the process;
6. if `getDebugFlag()` is set, log `"Exiting …"` — including the `"with return
   true"`/`"with return false"` variants for the `try_lock` family.

## Notes

* The library's ABI is C++: twelve of its entry points take or return
  `std::__1::function`, `std::string` or `std::chrono::milliseconds`, so the
  module is built with `stl: "libc++"`. It is nevertheless compiled with
  `-fvisibility=hidden` and every symbol of the blob's surface carries
  `QTIMUTEX_API`: the blob exports its own API but none of the libc++ inline
  helpers it instantiates (its libc++ 14-era headers made them local), so the
  exported set has to be enumerated rather than left at the compiler default.
  The 68 symbols this produces are exactly the blob's 68.
* Five of those 68 are the virtual members of `std::__1::__function::__func<
  void (*)(), std::allocator<void (*)()>, void ()>`, the libc++ object behind
  the default `deadlockAction_`. The blob's libc++ declared them with no
  visibility attribute, so they are weak and default-visibility; libc++ 20
  declares them `_LIBCPP_HIDE_FROM_ABI_VIRTUAL`, i.e. hidden, and the code
  comes out local. `qtimutex.cpp` therefore redefines that one macro after
  `<__config>` (include-guarded, so the override survives into `<functional>`)
  and puts `__base`'s destructor back with an assembler `.hidden` directive —
  the same header marks both, and they cannot be separated at C++ level. The
  bodies are the compiler's own; nothing is hand-written.
* `-fsemantic-interposition` is deliberate, not cargo cult: the blob routes
  every intra-library call to its default-visibility methods through the PLT
  (`getInstance` 45 call sites, `getDebugFlag` 18, `getDebugLogger` and
  `getPropertyName` 9 each), so a consumer that defines one of them is honoured.
  clang's default for `-fPIC` is *not* to assume that (the flag is a GCC
  default, not a clang one): without it the optimiser inlines the internal
  calls and the measured interposable target set collapses from 18 to 11.
* `QtiRecursiveMutex::lock()` reports the timeout with the *shared* mutex's
  message, `"FAILURE - QtiSharedMutex timeout."` — a copy/paste artefact in the
  original, reproduced verbatim rather than corrected (0x4aa4, rodata 0x29e5).
* `QtiShmMutex::try_lock()` returns `pthread_mutex_trylock(...) != 0`, i.e.
  **true means the lock was not taken** — the opposite of
  `std::mutex::try_lock()`. `QtiShmMutex::lock()`/`unlock()` discard the
  pthread return value, and `QtiShmMutex` has no destructor: the class is a
  bare `pthread_mutex_t` wrapper whose only exported members are the
  constructor and the three lock operations.
* `QtiShmConditionVariable`'s constructor aborts if
  `pthread_condattr_setpshared(PTHREAD_PROCESS_SHARED)` fails (0x67d4) but
  `QtiShmMutex`'s ignores the same failure; both ignore
  `pthread_condattr_setclock(CLOCK_MONOTONIC)`/`pthread_mutexattr_setpshared`
  results. The attribute objects are zeroed on the stack before the setters run.
* `QtiMutexDefaults` is 0xc0 bytes with eight bytes at +0x08 that no code in the
  blob ever reads or writes (the constructor leaves them zero-initialised, no
  getter or setter touches them). They are kept as `reserved08_` so the class
  keeps the original size and the offsets of every other member.
* The constructor's default action and timeout are set in that order
  (`deadlockAction_ = abort;` then `deadlockTimeout_ = 300000 ms`, 0x4074-0x40dc),
  and `deadlockTimeout_` is `std::chrono::milliseconds`; the 300000 ms (5 min)
  default is what every wrapper uses unless `setDefaultDeadlockTimeout` is
  called.
* `qtimutex_*_create()` value-initialises (the blob zeroes the whole allocation
  before running the member constructor, 0x48d4-0x48e8), `_destroy()` runs the
  destructor then `operator delete` and is a no-op for `NULL` like every other
  entry point; the `try_lock` C entry points return `false` for `NULL`.
* Nothing resolves the five `std::__1::__function::__func<…>` members across a
  DSO boundary: `llvm-nm -D --undefined-only` on `libqcrilFramework.so`,
  `libril-qc-hal-qmi.so` and `qtibus.so` names zero `__function6__func`,
  `__function6__base` and `__function12__value_func` symbols (the 189
  `__function` hits in `libril-qc-hal-qmi.so` are `std::function<…>` inside
  unrelated signatures; its 12 qtimutex imports are the twelve API entry
  points). The in-place `__func` object carries its own copy. They are exported
  because the blob exports them, not because anything consumes them.
* The 14 quad precision builtins are compiler-rt/libgcc, not qtimutex code: the
  blob statically links them because libc++'s timed wait converts the
  `steady_clock` deadline to a `system_clock` one through `long double`
  (0x4d4c-0x4d80 calls `__floatditf`, `__subtf3`, `__multf3`, `__lttf2`). They
  are provided by `static_libs: ["libcompiler_rt"]`, except `__fe_getround` and
  `__fe_raise_inexact`, which current compiler-rt no longer ships and which are
  therefore reimplemented in `qtimutex.cpp` straight from the blob (0x7498:
  `FPCR[23:22]` through the `{2, 1, 3}` table at rodata 0x2d48, 0 mapped to 0;
  0x74c4: `FPSR |= 0x10`, return 0). libc++ 20's chrono does the conversion with
  integer arithmetic instead, so the builtins are exported but not called — the
  one residual edge difference below.
* `__fe_getround`/`__fe_raise_inexact`/the twelve compiler-rt builtins are the
  only symbols that are not part of qtimutex's own API; every one of them is
  still defined and exported by this module.

## Verification

Everything below was produced by the scripts in the scratch workspace
(`harness.py` implements the libc/libc++ imports on guest memory, `matrix.py`
runs the case list against both objects, `mutant.py` builds and scores negative
controls, `syms.py` compares the exported symbol sets, `reloc.py` checks the
relocation accounting, `edges.py` measures interposable edges from relocations
and from instructions). `syms.py`, `reloc.py` and `edges.py` gate their runs and
exit non-zero on failure; the full account, including the negative controls that
make each instrument falsifiable, is in `VERIFICATION.md`.

**Exported symbols — `syms.py <blob> <build>`**: 68 == 68, no missing, no extra,
bindings identical (the five `std::__1::__function::__func<…>` members are `W`
in both). The pre-fix build is the negative control: it reports exactly those
five missing and exits 1.

**Behavioural differential — `matrix.py <blob> <build>`**

```
c01_defaults_readback                match     c10_shared_basic                      match
c02_null_handles                     match     c11_shared_wrapper_logs               match
c03_recursive_basic                  match     c12_shared_bypass                     match
c04_recursive_bypass                 match     c13_debug_flag_off                    match
c05_recursive_wrapper_logs           match     c14_setters_roundtrip                 match
c06_recursive_try_lock_contended     match     c15_shm_primitives                    match
c07_recursive_deadlock_timeout       match     c16_recursive_wrapper_no_logger       match
c08_recursive_timeout_default_action match     c17_bypass_silences_logging           match
c09_shared_reader_then_writer        match     c18_property_not_false_keeps_wrappers match

18/18 cases match
```

Each case compares the ordered trace (property reads, logger invocations with
their argument strings, condition variable waits/notifies, aborts, configured
string stores), the return value of every entry point, and the ABI-level state
of the `std::function` objects the getters hand back (`__f_` null / in-place /
heap). Addresses are normalised to non-nullness because the two objects place
their images differently. Representative trace, blob and build identical:

```
c07_recursive_deadlock_timeout      ok
  trace debug('Entering QtiRecursiveMutex::lock()')
  trace cv.timed_wait(at 1300000000000 ns)
  trace cv wait timed out (skipped 300000000000 ns)
  trace deadlock('FAILURE - QtiSharedMutex timeout.')
  trace action()
  trace debug('Exiting QtiRecursiveMutex::lock()')
c09_shared_reader_then_writer       ok
  trace ... gate2 notify on unlock_shared, then write-entered rollback and
        gate1.notify_all inside the timed exclusive attempt ...
```

**Negative controls — `mutant.py`** (7 mutants, each a behaviour-visible edit of
`qtimutex.cpp`; every one rejected):

```
msg          timeout message text                                 rejected by 4 cases
trymap       recursive try_lock result -> log string mapping swapped rejected by 3 cases
timeout      deadlock timeout ignored, always 1 ms                rejected by 3 cases
noaction     deadlock action never run                            rejected by 3 cases
invprop      bypass property test inverted                        rejected by 3 cases
unlockorder  unlock logs the exit before releasing                rejected by 2 cases
shmflag      QtiShmMutex serialised as private instead of process shared rejected by 1 case
7/7 mutants rejected; survivors: none
```

**Relocation accounting** — `reloc.py` asserts that every `.rela*` record
`llvm-readelf` prints is one the emulator applied, and refuses (non-zero) an
object with no relocation sections at all or one where its positive controls
(a `JUMP_SLOT` and a data relocation) are missing, so `0 == 0` cannot pass by
vacancy. Both objects reconcile: blob 76/76 (5 `ABS64`, 2 `GLOB_DAT`,
69 `JUMP_SLOT`, plus 6 RELR listing lines whose values are already absolute in
the file), build 73/73 (5 `ABS64`, 2 `GLOB_DAT`, 64 `JUMP_SLOT`,
2 `RELATIVE`).

**Interposable edges — `edges.py <blob> <build>`** (set from relocations,
multiplicity from instructions; the run is gated by controls that execute first
and exit non-zero on failure — a non-AArch64 object is *refused* rather than
scored zero, and the known interposable edge
`QtiMutexDefaults::getInstance()` must be found before any figure is printed.
Tool path and raw line counts accompany every zero):

```
blob : AArch64, readelf -r lines 90, objdump -d lines 3930
       reloc view 23 targets ; text view 122 edges over 23 targets
       (291 PLT branches = 122 interposable + 169 external; 69 stub
        definitions, 0 offset annotations -> strict = loose = 360)
build: AArch64, readelf -r lines 79, objdump -d lines 3160
       reloc view 18 targets ; text view 85 edges over 18 targets
       (221 PLT branches = 85 interposable + 136 external; 64 stub
        definitions, 69 offset annotations -> strict 285, loose 354)
```

Seventeen of the blob's 23 interposable targets are dispatched interposably by
the build — `getInstance`, `getDebugFlag`, `getDebugLogger`, `getPropertyName`,
`getDeadlockAction`, `getDeadlockLogger`, `getDeadlockTimeout`, all six
`QtiSharedMutex` methods, all three `QtiRecursiveMutex` methods. Two bounded
residuals, each with its reason:

| residual | direction | why it cannot matter |
| -------- | --------- | -------------------- |
| `__floatditf`, `__lttf2`, `__multf3`, `__subtf3`, `__fe_getround`, `__fe_raise_inexact` | blob has the edge, build has none | the calls do not exist in the build: libc++ 20's timed wait converts clocks with integer arithmetic, so there is nothing to interpose. All six symbols are still exported and still work if called. |
| `QtiMutexDefaults::QtiMutexDefaults()` | build has the edge (1), blob has none | the reimplementation's `getInstance()` initialises the function-local singleton with a call to the out-of-line constructor; the blob inlines that construction (0x4250-0x4320). A gained edge on a symbol the blob already exports cannot break a consumer. |

Call-site *counts* differ on two targets without any target being lost: the blob
repeats the property check inline in all twelve entry points (`getInstance` 45
vs 37, `getPropertyName` 9 vs 1), while the reimplementation factors it into one
`wrapperBypassed()` helper — fewer call sites to the same, still interposable,
targets.

**Not exercised, and why.** The harness has no second guest thread, so a
condition wait that can never be notified is reported as a deadlock rather than
emulated, and no case can observe a *real* wakeup (a notify that releases a
waiter). Everything reachable single-threaded is covered: the timed wait itself
is driven for real, with virtual time advanced to the requested deadline, so the
guest's own deadline arithmetic — including the 300 s default — runs unchanged.
`QtiShmConditionVariable`'s notify paths are exercised only as `pthread_cond_
signal`/`broadcast` calls with no waiter.
