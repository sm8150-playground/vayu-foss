# com.qti.hvx.addconstant

Reimplementation of the proprietary CHI HVX add-constant algorithm plug-in
installed as `vendor/lib64/camera/components/com.qti.hvx.addconstant.so`
(AArch64, 1128 bytes of `.text`, exactly one exported symbol, 64-bit only —
there is no `vendor/lib` counterpart).

Row for the table in `vendor/qcom/opensource/libraries/README.md`:

| Library | Replaces | Source group | ABI surface | How it was reverse engineered | Verification |
| ------- | -------- | ------------ | ----------- | ----------------------------- | ------------ |
| `com.qti.hvx.addconstant` | `vendor/lib64/camera/components/com.qti.hvx.addconstant.so` | vayu (`device/xiaomi/vayu/proprietary-files.txt`, Camera components section; sha1 `7d34a18924e8ec38b0f58c22ebaad58f00e14c40`) | `ChiISPHVXAlgorithmEntry(HVXFuncTable *)` — the entry point fills the caller's operation table with six local callbacks (`AddConstantHVXInitialize`, `AddConstantResolutionInfo`, `AddConstantGetHvxInfo`, `AddConstantSetConfig`, `AddConstantExecute`, `AddConstantDestroy`) | AArch64 blob whose only imports are `calloc`, `free`, `__strlcpy_chk` and `__android_log_print`. `llvm-nm -D` gave the single export; the blob's `.gnu_debugdata` section, xz-decompressed, holds the local symbol table that names all six callbacks and fixes their exact signatures. `llvm-objdump` showed the entry point storing six pointers at table offsets `+0x00, +0x08, +0x10, +0x18, +0x28, +0x30` and leaving `+0x20` alone, and showed that the blob contains no pixel loop at all — the frame is processed on the DSP through the two callbacks in the caller's `HVXDSPConfig`. Every log string, its argument list and its line number were read out of `.rodata`. | Blob and rebuild both driven through one scenario script in a Unicorn AArch64 harness, with the `HVXDSPConfig` callbacks stubbed emulator-side so their call sites, arguments and return-code handling are observable: 91 transcript lines (entry point, all six hooks over a matrix of inputs, NULL-argument cases, both callback failure paths) compare identical, including the memory each hook writes and every log record. 32 single-change mutants of the two reimplementations (step, wrap limit, wrap target, comparison, name, truncation, flag tests, clear, slot installation, log line numbers, log format text, freed pointer, callback data pointer, NULL checks) were all caught by the same test. |

## What the blob is

The camera CHI HVX node (`com.qti.chi.override.so`, which carries the string
`com.qti.hvx.addconstant`) `dlopen()`s the component out of the directory
`camera.qcom.so` probes, `/vendor/lib64/camera/components/`, and resolves its
one exported symbol:

```c
int32_t ChiISPHVXAlgorithmEntry(HVXFuncTable *funcTable);
```

The entry point does not process a frame. It fills the caller's operation table
with the algorithm's callbacks and returns 0 (or 1 if the table pointer is
NULL, which it logs). The frame itself is processed on the DSP: what this
component does is describe itself to the node (name, resolution), hand the DSP
configuration to the node, and push the algorithm's 4-byte OEM parameter
through the `HVXDSPConfig` write callback.

## ABI

One exported symbol, `ChiISPHVXAlgorithmEntry` (C linkage, `GLOBAL`/`DEFAULT`,
140 bytes in the blob at `0x1018`). Everything else is local. The six
callbacks, with the names and signatures taken from the blob's own
`.gnu_debugdata` symbol table:

| Table slot | Address | Mangled name | Signature |
| ---------- | ------- | ------------ | --------- |
| `+0x00` | `0x10a8` | `_ZL24AddConstantHVXInitializePPv` | `int32_t (void **pOEMData)` |
| `+0x08` | `0x1120` | `_ZL25AddConstantResolutionInfoPvPP17HVXResolutionInfo` | `int32_t (void *, HVXResolutionInfo **)` |
| `+0x10` | `0x1188` | `_ZL21AddConstantGetHvxInfoPvP13HVXGetSetInfo` | `int32_t (void *, HVXGetSetInfo *)` |
| `+0x18` | `0x1310` | `_ZL20AddConstantSetConfigPvP9HVXConfigPK12HVXDSPConfigPi` | `int32_t (void *, HVXConfig *, const HVXDSPConfig *, int32_t *)` |
| `+0x20` | — | — | never written by the blob |
| `+0x28` | `0x1240` | `_ZL18AddConstantExecutePvPK12HVXInputDataPK12HVXDSPConfigPi` | `int32_t (void *, const HVXInputData *, const HVXDSPConfig *, int32_t *)` |
| `+0x30` | `0x1418` | `_ZL18AddConstantDestroyPv` | `int32_t (void *)` |

The reimplementation keeps those names and signatures, so its local symbol
table matches the blob's (the callbacks are static, exactly as in the blob).

The structures the hooks exchange are the CHI node's; only the offsets the blob
touches are known, so the header documents each field by offset and marks the
two words of `HVXResolutionInfo` width/height only on the strength of the
binning sibling halving both of them:

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

* `AddConstantHVXInitialize` (`0x10a8`) — `calloc(1, 4)`, stored through its
  argument; returns 1 without storing when the allocation fails.
* `AddConstantResolutionInfo` (`0x1120`) — checks all three of `pOEMData`,
  `pResolutionInfo` and `*pResolutionInfo`, then copies the input pair to the
  output pair and clears the word at `+0x08`. Adding a constant does not change
  the resolution, so the two pairs are equal.
* `AddConstantGetHvxInfo` (`0x1188`) — reports the name `add_constant` into the
  32-byte field (a `strlcpy`), sets `+0x10` to 1, and applies the two
  conditional flags described above.
* `AddConstantSetConfig` (`0x1310`) — calls `dspConfig->adspConfig_call(pResult)`
  first, logs that call's failure but carries on, stores 5 into the 4-byte OEM
  parameter, then calls `dspConfig->write(pResult, pOEMData, 4)` and returns
  that call's result.
* `AddConstantExecute` (`0x1240`) — the per-frame hook. It advances the OEM
  parameter and pushes it; it never reads `pInput`:

  ```c
  uint32_t value = (uint32_t)*(int32_t *)pOEMData + 5;
  *(int32_t *)pOEMData = (value > 99u) ? 5 : (int32_t)value;
  ret = dspConfig->write(pResult, pOEMData, 4);
  ```

  The comparison is the unsigned `csel` condition `hi`, so the parameter walks
  5, 10, … 95 and then starts over at 5 — it never reaches 100. The value is
  the DSP-side "constant"; what the DSP does with it is not in this blob.
* `AddConstantDestroy` (`0x1418`) — `free()` on the OEM parameter block.

All six hooks return 1 without calling anything when one of their pointers is
NULL, and each of those paths logs. Every log record — tag `ISPHVX`, priority
6, the format string, the `%s` function name and the line number — is
reproduced verbatim from the blob's `.rodata`, because it is observable in
logcat.

## Verification

See `../VERIFICATION.md` for the commands and the raw output. In short:

* `test/build.sh` compiles the module with the ROM's clang, the Android.bp
  flags and `-Wall -Wextra -Werror`; it is clean.
* `test/hvx_diff.py` drives the original blob and the rebuild through the same
  91-line scenario script in the Unicorn AArch64 harness and diffs the
  transcripts; they are identical.
* `test/mutate.py` breaks the reimplementation 32 ways (16 of them this
  library) and the same test catches every one, so the matrix discriminates.

## Notes

* The blob's `.text` holds no vector instruction, no pixel buffer and no loop:
  neither this component nor its binning sibling touches frame data. A "width
  that is not a multiple of the vector length" cannot be exercised here because
  the blob has no width-dependent path at all; the only vector operation in
  either blob is binning's two-lane shift of the resolution pair.
* The blob's `AddConstantDestroy` logs `hvx_lib_close pOEMData %p` although it
  only calls `free()`. That text is a leftover from the sample this component
  was derived from; it is reproduced verbatim rather than corrected.
* `AddConstantExecute`'s OEM-parameter cycle (add 5, wrap to 5 above 99) is
  reproduced as-is, including the unsigned comparison. It is the blob's
  behaviour, odd as it looks, and the DSP is the only consumer of the value.
* The 4-byte OEM parameter is handed to the DSP write callback on every frame,
  so its layout is the DSP's business; the blob treats it as one `int32_t`.
* The `libc++`/`libcutils`/`libm`/`libdl` `DT_NEEDED` entries the blob carries
  are unused by its code — its only imports are `calloc`, `free`,
  `__strlcpy_chk` and `__android_log_print` — so the reimplementation links
  only `liblog` and `libc`. The name copy compiles to the same fortified
  `__strlcpy_chk` call the blob makes, or (when the compiler can see the whole
  copy, as here) to the identical inline constant store; either way the memory
  effect is the same 13 bytes, which is what the differential test compares.
* The Android.bp installs the module to `camera/components` under
  `/vendor/lib64`, matching where the blob lives and where `camera.qcom.so`
  probes for CHI components.
