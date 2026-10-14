# com.qti.hvx.binning

Reimplementation of the proprietary CHI HVX binning algorithm plug-in
installed as `vendor/lib64/camera/components/com.qti.hvx.binning.so`
(AArch64, 904 bytes of `.text`, exactly one exported symbol, 64-bit only —
there is no `vendor/lib` counterpart).

Row for the table in `vendor/qcom/opensource/libraries/README.md`:

| Library | Replaces | Source group | ABI surface | How it was reverse engineered | Verification |
| ------- | -------- | ------------ | ----------- | ----------------------------- | ------------ |
| `com.qti.hvx.binning` | `vendor/lib64/camera/components/com.qti.hvx.binning.so` | vayu (`device/xiaomi/vayu/proprietary-files.txt`, Camera components section; sha1 `e6c469709ef1162f8e85c7bd61cbe12d2289fd70`) | `ChiISPHVXAlgorithmEntry(HVXFuncTable *)` — the entry point fills the caller's operation table with five local callbacks (`BinningHVXInitialize`, `BinningResolutionInfo`, `BinningGetHvxInfo`, `BinningSetConfig`, `BinningDestroy`) | AArch64 blob whose only imports are `calloc`, `free`, `__strlcpy_chk` and `__android_log_print`. `llvm-nm -D` gave the single export; the blob's `.gnu_debugdata` section, xz-decompressed, holds the local symbol table that names all five callbacks and fixes their exact signatures. `llvm-objdump` showed the entry point storing five pointers at table offsets `+0x00, +0x08, +0x10, +0x18, +0x30` and leaving `+0x20` and `+0x28` alone, and showed the algorithm's whole arithmetic to be `ushr v1.2s, v0.2s, #1` — a logical, per-lane halving of the resolution pair. Every log string, its argument list and its line number were read out of `.rodata`. | Blob and rebuild both driven through one scenario script in a Unicorn AArch64 harness, with the `HVXDSPConfig` callbacks stubbed emulator-side so their call sites, arguments and return-code handling are observable: 69 transcript lines (entry point, all five hooks over a matrix of resolutions including odd, zero, negative and `INT32_MIN`/`INT32_MAX` values, NULL-argument cases, both callback failure paths) compare identical, including the memory each hook writes and every log record. 32 single-change mutants of the two reimplementations (logical vs arithmetic shift, shift amount, the third NULL check, the `+0x08` clear, name, flag tests, slot installation, log line numbers, OEM write, callback data pointer, NULL checks) were all caught by the same test. |

## What the blob is

The camera CHI HVX node (`com.qti.chi.override.so`) `dlopen()`s the component
out of the directory `camera.qcom.so` probes,
`/vendor/lib64/camera/components/`, and resolves its one exported symbol:

```c
int32_t ChiISPHVXAlgorithmEntry(HVXFuncTable *funcTable);
```

The entry point does not process a frame. It fills the caller's operation table
with the algorithm's callbacks and returns 0 (or 1 if the table pointer is
NULL, which it logs). Binning has no per-frame hook of its own — its table is
one entry shorter than the add-constant one — so the frame is binned on the
DSP, driven by the resolution this component reports and by the DSP
configuration it installs.

## ABI

One exported symbol, `ChiISPHVXAlgorithmEntry` (C linkage, `GLOBAL`/`DEFAULT`,
132 bytes in the blob at `0x1018`). Everything else is local. The five
callbacks, with the names and signatures taken from the blob's own
`.gnu_debugdata` symbol table:

| Table slot | Address | Mangled name | Signature |
| ---------- | ------- | ------------ | --------- |
| `+0x00` | `0x10a0` | `_ZL20BinningHVXInitializePPv` | `int32_t (void **pOEMData)` |
| `+0x08` | `0x1320` | `_ZL21BinningResolutionInfoPvPP17HVXResolutionInfo` | `int32_t (void *, HVXResolutionInfo **)` |
| `+0x10` | `0x1118` | `_ZL17BinningGetHvxInfoPvP13HVXGetSetInfo` | `int32_t (void *, HVXGetSetInfo *)` |
| `+0x18` | `0x11d0` | `_ZL16BinningSetConfigPvP9HVXConfigPK12HVXDSPConfigPi` | `int32_t (void *, HVXConfig *, const HVXDSPConfig *, int32_t *)` |
| `+0x20` | — | — | never written by the blob |
| `+0x28` | — | — | never written by the blob (no per-frame hook) |
| `+0x30` | `0x12d0` | `_ZL14BinningDestroyPv` | `int32_t (void *)` |

The reimplementation keeps those names and signatures, so its local symbol
table matches the blob's (the callbacks are static, exactly as in the blob).

The structures the hooks exchange are the CHI node's; only the offsets the blob
touches are known, so the header documents each field by offset. The two words
of `HVXResolutionInfo` are read as width and height because the binning hook
divides exactly those two words by two:

* `HVXDSPConfig` — two function pointers, `+0x00` called with
  `(pResult, pOEMData, 4)` and `+0x08` with `(pResult)`. The blob names `+0x08`
  in its own log text (`adspConfig_call ret %d`).
* `HVXResolutionInfo` — input pair at `+0x00`, a word at `+0x08` that is
  cleared, and the output pair at `+0x10`; `+0x0c` and beyond are untouched.
* `HVXGetSetInfo` — a word at `+0x10` set to 1, a 32-byte algorithm name at
  `+0x14`, a word at `+0x34` set to 1 when the input word at `+0x00` is
  non-zero, and a word at `+0x38` set to 1 when the input word at `+0x04` is at
  least 1.

## Behaviour

* `BinningHVXInitialize` (`0x10a0`) — `calloc(1, 4)`, stored through its
  argument; returns 1 without storing when the allocation fails. Unlike the
  add-constant variant, nothing ever writes to those 4 bytes, so the DSP is
  handed the zeroes `calloc()` left behind.
* `BinningResolutionInfo` (`0x1320`) — the algorithm's entire arithmetic:

  ```asm
  ldr   d0, [x8]             ; the input pair
  str   wzr, [x8, #0x8]      ; clear the word at +0x08
  ushr  v1.2s, v0.2s, #1     ; out[i] = (uint32_t)in[i] >> 1
  str   d1, [x8, #0x10]      ; the output pair
  ```

  Both 32-bit lanes are halved with a *logical* shift, so 1921 becomes 960 and
  `0xFFFFFFFF` becomes `0x7FFFFFFF`. The pair is a single 64-bit value, so both
  lanes are always processed — there is no vector remainder and no
  width-dependent path anywhere in the blob. Note that this hook tests only
  `pOEMData` and `pResolutionInfo` and then dereferences the latter: a NULL
  `*pResolutionInfo` is a fault here, not a failure return, exactly as in the
  blob (the add-constant variant does test it).
* `BinningGetHvxInfo` (`0x1118`) — reports the name `binning` into the 32-byte
  field (a `strlcpy`), sets `+0x10` to 1, and applies the two conditional flags
  described above.
* `BinningSetConfig` (`0x11d0`) — calls `dspConfig->adspConfig_call(pResult)`,
  logs that call's failure but carries on, then calls
  `dspConfig->write(pResult, pOEMData, 4)` and returns that call's result. It
  never writes to the OEM parameter.
* `BinningDestroy` (`0x12d0`) — `free()` on the OEM parameter block.

All five hooks return 1 without calling anything when one of their pointers is
NULL, and each of those paths logs. Every log record — tag `ISPHVX`, priority
6, the format string, the `%s` function name and the line number — is
reproduced verbatim from the blob's `.rodata`, because it is observable in
logcat.

## Verification

See `../VERIFICATION.md` for the commands and the raw output. In short:

* `test/build.sh` compiles the module with the ROM's clang, the Android.bp
  flags and `-Wall -Wextra -Werror`; it is clean.
* `test/hvx_diff.py` drives the original blob and the rebuild through the same
  69-line scenario script in the Unicorn AArch64 harness and diffs the
  transcripts; they are identical.
* `test/mutate.py` breaks the reimplementation 32 ways (16 of them this
  library) and the same test catches every one, so the matrix discriminates.
  The mutant that adds a `*pResolutionInfo` NULL check is the one that caught
  the only real difference found during development: this hook has two checks,
  not three.

## Notes

* The blob's `.text` holds no frame buffer, no pixel loop and no
  width-dependent path. The only data-transforming instruction in it is the
  two-lane halving of the resolution pair, which is reproduced lane for lane
  (including the logical, not arithmetic, shift — the `ushr` mnemonic and the
  `0xFFFFFFFF → 0x7FFFFFFF` case both matter). Binning of actual frame data
  happens on the DSP side of `HVXDSPConfig`.
* The blob's `BinningDestroy` logs `hvx_lib_close pOEMData %p` although it only
  calls `free()`. That text is a leftover from the sample this component was
  derived from; it is reproduced verbatim rather than corrected.
* The algorithm name string `binning` appears in no other file in the image;
  the node gets it from its own configuration, so the component is only ever
  reached through this entry point.
* The `libc++`/`libcutils`/`libm`/`libdl` `DT_NEEDED` entries the blob carries
  are unused by its code — its only imports are `calloc`, `free`,
  `__strlcpy_chk` and `__android_log_print` — so the reimplementation links
  only `liblog` and `libc`. The name copy compiles to the same fortified
  `__strlcpy_chk` call the blob makes, or (when the compiler can see the whole
  copy, as here) to the identical inline constant store; either way the memory
  effect is the same 8 bytes, which is what the differential test compares.
* The Android.bp installs the module to `camera/components` under
  `/vendor/lib64`, matching where the blob lives and where `camera.qcom.so`
  probes for CHI components.
