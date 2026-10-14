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
| `lib-imsvtextutils` | `system_ext/lib64/lib-imsvtextutils.so` (4256 bytes of `.text`, 64-bit only) | LA.UM.9.1.R1.10.00.00.604.052 — the blob's own `__FILE__` is `vendor/qcom/proprietary/commonsys/ims/vtextutils/src/qpImsVTGpuUtil.cpp` | `rotateFrame()`, `setupEGL()`, `setupShader()`, `destroyEGL()` (C linkage, no argument read, `rotateFrame` returns 0), `__cfi_check()`, and the data objects `VertexShader` / `FragmentShader` | AArch64 blob. `llvm-readelf` gave the exports and the four `GLOB_DAT` slots that name the logging state (`logPriorityValue`, `bInit_Success`, `log_rw_mutex`, `adbLogPropertyValue`); `.rodata` holds the two shader sources, the `SIMSVT` tag, the `__FILE__` string and the one message format, `.data` the two shader pointers, `.data.rel.ro` the 24-byte `msg_send_2` descriptor. `.gnu_debugdata` named the bodies (`rotateFrame.cfi` @0x103c, `setupEGL.cfi` @0x1048, `setupShader.cfi` @0x1050, `destroyEGL.cfi` @0x10f4) and proved the four exports are 8-byte `bti c; b <body>` CFI trampolines @0x2080/0x2088/0x2090/0x2098. Three of the four entry points are empty and `rotateFrame` is a constant `mov w0, wzr`; all the real work is `setupShader`'s logging helper, which was decoded from the two `ldrb`/`ldrh` gated branches (0x106c `bInit_Success` picks the `msg_send_2` path, 0x10d0 `adbLogPropertyValue` picks the logcat line, and 0x10c4 `autiasp` falls through into the second test instead of returning). The `__cfi_check` type ids were identified by compiling candidate signatures with the same toolchain: `0x7E04A0FB7AD8BCD5` is clang's id for `void()`, confirming the three void entry points take no parameters. | Blob and build run side by side under the Unicorn AArch64 harness (`verify_diff.py`), every reachable import replaced by a recording stub: **439 of 439 observations identical** — the return value of all four entry points in 5×4×4 states of the three globals, the full call sequence and arguments of the `msg_send_2`/`pthread_rwlock`/`__android_log_print` paths with the descriptor decoded into its fields and strings, both data objects byte for byte, and `__cfi_check` for three type ids × deltas 0..32 × diag variants × all four function addresses. **25 of 25 single-behaviour mutants detected**, so the matrix discriminates every behaviour it claims to. `dump_cfi.py` confirms both objects accept deltas `{0, 8, 16}` and reject all others. |

## Notes

* Despite the name there is nothing textual about this library: it is the IMS
  video-text GPU path (`qpImsVTGpuUtil.cpp`) and the four entry points are the
  frame-rotation shaders' setup, of which only `setupShader` does anything.  The
  GLES/EGL work is compiled out of the shipped build — the dynamic symbol table
  contains no import from `libGLESv2.so` or `libEGL.so` even though both are
  `DT_NEEDED` — so the reimplementation is a no-op shim plus its logging helper,
  and those dead `DT_NEEDED` entries (with `libutils`, `libcutils`, `libc++`,
  `libm`, `libdl`) are dropped.
* The two logging gates are the opposite way round from what their names
  suggest, and getting them right mattered: `bInit_Success` (read with `ldrb`)
  selects the diag `msg_send_2` path and only the *second*, `ldrh`-read
  `adbLogPropertyValue` selects the logcat line, and the diag branch falls
  through into the logcat test rather than returning.  Reading the two `cbz`es
  the other way round differs from the blob in 16 of the 320 call observations;
  returning instead of falling through differs in 12.
* `libdiag_system.so` (`msg_send_2`) and `lib-imsvtutils.so` (the four logging
  globals) are kept as dependencies because the blob takes those symbols from
  them and the logging state has to be the same object the rest of the IMS stack
  sees.  Defining private copies would compile and link but would silently make
  `setupShader` a no-op, since nothing would ever set them.
* The 24-byte descriptor handed to `msg_send_2` (`id = 0x0033015d`, `field = 4`,
  the format string and `__FILE__`) is reproduced byte for byte.  What the two
  leading words *mean* is defined inside the proprietary `libdiag_system.so` and
  is not guessed here.
* `rotateFrame`'s C-level signature is not recoverable: its CFI type id is not
  clang's id for `int(void)`, `void*(void)`, `int(int)`, `int(int,int)` or any
  other candidate tried, so only its register-level ABI is known (the argument
  registers are never read, `w0` is set to 0).  `int rotateFrame(void)` is a
  faithful declaration for every caller that exists — a caller compiled against
  the real header passes its arguments in registers the body ignores.
* `__cfi_check` is reproduced as a hand-written function with the blob's two type
  ids and its `brk #0x5502` trap, including the rotate (`extr x8, x8, x8, #3`, not
  a shift) that makes the three-target group exactly the three trampolines 8
  bytes apart.  The three trampolines are therefore emitted as 8-byte
  `bti c; b <body>` stubs in a dedicated section, as in the blob.  Full
  agreement with the blob's accepted/rejected sets over deltas 0..32 is
  verified.
* Nothing in the image calls these four entry points.  The library's single
  `DT_NEEDED` consumer, `lib-imsvideocodec.so`, imports no symbol from it, so the
  ABI is preserved for the interface's sake rather than for a live caller.
* Metadata-only differences from the blob, none of which the dynamic linker
  observes: the four imported data symbols are `NOTYPE` rather than `OBJECT`
  (resolution is by name), the export sizes of `setupEGL`/`setupShader`/
  `destroyEGL` are 8 rather than the blob's linker-assigned 24, and the build has
  no `__cxa_finalize` import because it has no destructors.
