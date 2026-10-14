# vayu open-source vendor libraries

Open-source reimplementations of the proprietary shared libraries shipped in
the vayu vendor image. Each library keeps the ABI of the blob it replaces
(soname, exported symbols, signatures and observable behaviour) so it can be
dropped in without touching its consumers.

Every entry is written from static analysis of the blob — disassembly plus
emulation of the original code for behavioural ground truth — and is verified
bit-exactly against that ground truth before the blob is removed from
`proprietary-files.txt`. Libraries are done one at a time, smallest and
most self-contained first.

## Libraries


| Library | Replaces | Source group | ABI surface | How it was reverse engineered | Verification |
| ------- | -------- | ------------ | ----------- | ----------------------------- | ------------ |
| `libdepthmapwrapper` | `vendor/lib64/camera/components/libdepthmapwrapper.so` | n/a — Xiaomi vendor camera component (no QCOM source drop) | `DepthMapWrapperCreate(int, int, int, int, unsigned long *, int *, int)`, `DepthMapWrapperProcessFrame(void *, WrapperDepthInput, WrapperDepthCamConfig *, unsigned int, WrapperDepthOutput *)`, `DepthMapWrapperDestroy(void *)`, plus the C++ members of `DepthMapWrapper`: the constructor `(int, int, int, int, unsigned long *, int *, unsigned char)` (C1 and C2), `processFrame(WrapperDepthInput, WrapperDepthCamConfig *, unsigned int, WrapperDepthOutput *)` and the destructor (D1 and D2) | AArch64 blob, 652 bytes of `.text`, eight exports and no use of any of its six `DT_NEEDED` entries except libc++'s `_Znwm`/`_ZdlPv`. `llvm-readelf` gave the `.dynsym` (which shows `C1`/`C2` and `D1`/`D2` at a single address each — they are aliases) and `.rela.plt` (`__cxa_finalize`, `_Znwm`, `_ZdlPv` only); the `.gnu_debugdata` mini-debug-info confirmed the eight function boundaries and the two `__on_dlclose*` thunks in `.fini_array`; `llvm-objdump` decoded the bodies, and every behavioural claim below is cited as an instruction address in that listing. The remaining half of the specification came from the consumer: `com.qti.node.depth.so`, the bundled QTI depth node, `dlopen()`s `libdepthmapwrapper_secure.so` and falls back to `libdepthmapwrapper.so` (`.rodata` 0x102f/0x1049), `dlsym()`s the three C entry points and calls them with arguments built on its own stack, so its call sites fixed the by-value `WrapperDepthInput` size class (one 32- or 64-bit metadata word loaded into x1) and the 24-byte descriptor layout (`mov x2, sp`, `ldr x0, [x22, #0x38]`, `add x4, sp, #0x90`, and the fill loop storing type at +0, a word at +8 and the length at +0x10). | Blob and replacement run side by side in the same AArch64 Unicorn harness (`emu64.py`), with libc++ `operator new`/`delete` and libc `malloc`/`free` routed to one guest allocator so heap addresses are identical in both runs: 248 observations over all eight exports — return values, every out-parameter and object byte, the plane descriptors, and the size/order of every allocation and free — 0 mismatches. A separate edge probe covers out-parameters aliasing the object under construction, the destructor-then-`Destroy()` path and an unmapped handle: 11/11 identical (including both libraries faulting on the unmapped handle). Raw output in `REPORT.md`. The matrix is also checked for discrimination: 29 of 30 single-behaviour mutants of the replacement are caught (the thirtieth is a comment-only control), including one whose only observable effect is the store order in the constructor. |

## Notes

* `libdepthmapwrapper` is 64-bit only, matching the blob (`vendor/lib64` has no
  32-bit counterpart, and the depth node is 64-bit). The module is installed
  with `relative_install_path: "camera/components"` so it replaces the blob at
  its original path; nothing links against it, the depth node only `dlopen()`s
  it, so it has to be added to `PRODUCT_PACKAGES` when the blob entry is
  dropped.
* The blob is a stub, not a depth-map implementation: it ignores all four
  frame integers given to `DepthMapWrapperCreate` (and the character passed to
  the constructor) and the seventh argument, allocates a single 1-byte buffer
  that is never read or written again, and reports success. The only values it
  ever produces are the two literals `0x00c65d40` (13000000, through the
  `unsigned long *` out-parameter of the create path) and `0x00e4e3eb`
  (15000555, at `WrapperDepthCamConfig + 8` from both `processFrame` entry
  points). The consumer only reads the returned 0/1 status (it returns `!status`
  to its own caller), so the 15000555 store looks like filler left in the
  vendor's stub; it is reproduced verbatim rather than "fixed".
* The consumer prefers `libdepthmapwrapper_secure.so`, which does not exist
  anywhere in this image; the fallback name is the one reproduced here.
* The blob allocates through libc++'s `operator new`/`operator delete`. The
  replacement uses `malloc()`/`free()` directly and therefore needs no C++
  standard library at all (`stl: "none"`); both objects are allocated and
  released inside the library, and libc++'s operators are malloc/free wrappers
  on Android, so handles remain interchangeable with a libc++ `delete`.
* `DepthMapWrapperCreate`'s constructor returns the address of its 1-byte buffer
  in x0 rather than `this` — the blob never reloads x0 after the `_Znwm` call
  (`str x0, [x21]` at 0x1040 and no later write to x0). That is reproduced
  exactly, because it is deterministic; the return registers of `Destroy` and
  the destructors are not part of any contract on either side.
* The blob has no `.init_array`; its `.fini_array` holds two compiler-generated
  thunks, `__on_dlclose` (a `__cxa_finalize(&__dso_handle)` veneer) and
  `__on_dlclose_late` (an `__emutls_unregister_key` whose body is a bare `ret`).
  Both are artefacts of the blob being a C++ translation unit with a
  destructor-using class; neither is exported and neither has any observable
  effect, and the replacement (plain C, no static initialisation) needs no
  counterpart.
