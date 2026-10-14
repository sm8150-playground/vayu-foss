# vayu open-source vendor libraries — `libqcbor`

Section for `vendor/qcom/opensource/libraries/README.md`, in that file's
format.

## Libraries

| Library | Replaces | Source group | ABI surface | How it was reverse engineered | Verification |
| ------- | -------- | ------------ | ----------- | ----------------------------- | ------------ |
| `libqcbor` | `vendor/lib64/libqcbor.so` (sha1 `7da5294a577f5a71ede5b3804a026bf69bd025ee`) | vayu (`device/xiaomi/vayu/proprietary-files.txt`) | 27 exports: the `UsefulBuf` primitives `UsefulBuf_Copy`, `UsefulBuf_Compare`, `UsefulBuf_Set`, `UsefulBuf_FindBytes`, `UsefulOutBuf_Init`, `UsefulOutBuf_InsertUsefulBuf`, `UsefulOutBuf_OutUBuf`, `UsefulOutBuf_CopyOut`, `UsefulInputBuf_GetBytes`; the encoder `QCBOREncode_Init`, `_AddBytes_3`, `_AddText_3`, `_AddRaw`, `_OpenArray_3`, `_OpenMap_3`, `_CloseArray`, `_AddUInt64_3`, `_AddInt64_3`, `_AddRawSimple_3`, `_AddSimple_3`, `_AddFloat_3`, `_AddDouble_3`, `_Finish2`, `_Finish`; the decoder `QCBORDecode_Init`, `QCBORDecode_GetNext`, `QCBORDecode_Finish` | AArch64 blob, 7916 bytes of `.text`, importing only `memcpy`, `memmove`, `memset`, `memcmp`, `__strlen_chk`, `ldexp` and `__stack_chk_fail`. This is Qualcomm's CAF drop of Laurence Lundblade's QCBOR, so the encoder side decodes to the published CAF `qcbor_encode.c` line for line — `InsertEncodedTypeAndNumber` (0x28cc) is the width-selection chain, `AddBytesInternal` (0x21e0) is the 8-register-argument worker shared by every "add" entry point (major type in `w6`, item count in `w7`, `9` = `CBOR_MAJOR_NONE_TYPE_RAW` disables the type/length header), `AddLabelAndOptionalTag` (0x3458) is the label-then-tag prologue, and `QCBOREncode_CloseArray` (0x2838) backfills the header with `uCount / (uMajorType - 3)` — the map pair count — then inserts the bstr wrap. The vendor tree is *not* any published revision though: `QCBOREncode_Init` compares the buffer length with `>= UINT32_MAX` (`mov w8, #-0x1` zero-extends to 0xFFFFFFFF at 0x2020) where the published source uses `>`, `DecodeNesting_Decrement` also decrements the parent's count when it pops a level (0x3650..0x3694) which the published source of that era does not, `DecodeOptional` zeroes its local `QCBORItem` (0x3944..0x394c), `DecodeDateEpoch` also accepts floats and doubles (jump table at `.rodata`+0xb75 with entries for `QCBOR_TYPE_FLOAT` and `QCBOR_TYPE_DOUBLE`), and `DecodeSimple` converts `0xF9` half precision with `ldexp` using bit positions that are *not* the standard ones. Those five deviations are all reproduced as the blob has them. | Differential: the blob and the reimplementation (`src/UsefulBuf.c`, `src/qcbor_encode.c`, `src/qcbor_decode.c`) are both driven under the Unicorn AArch64 harness from one case list of 2522 encoder/decoder/buffer cases (plus a 65536-value half-precision sweep and a float/double sweep run inside the same case list) and every return value, every error code, every output byte and every context byte is compared — 0 mismatches. The corpus is shown to discriminate by re-running it against 44 mutants of the implementation (changed boundaries, changed error codes, removed branches): every one is detected. See `VERIFICATION.md`. |

## Notes

* `libqcbor` is a general-purpose CBOR (RFC 7049 / RFC 8949) codec kept as a
  library API, not a service: the callers allocate `QCBOREncodeContext`,
  `QCBORDecodeContext` and `QCBORItem` themselves and manipulate them through
  the inline functions in the public headers. The two headers the blob was
  built against (`qcbor.h`, `UsefulBuf.h`) are therefore part of the ABI and
  are shipped in `include/`; they are the upstream BSD-3-Clause headers of the
  CAF drop, and `src/UsefulBuf.c`, `src/qcbor_encode.c` and
  `src/qcbor_decode.c` are written against them unchanged. The structure
  sizes and field offsets were read out of the blob's disassembly and are
  enforced by `test/abi_check.c`:
  `sizeof(QCBOREncodeContext) == 0x88` (`memset` at 0x2014), `uError` at
  `+0x20`, `nesting` at `+0x28`, `pCurrentNesting` at `+0x80`,
  `sizeof(QCBORDecodeContext) == 0x60` (`memset` at 0x3510), `uDecodeMode` at
  `+0x20`, `sizeof(QCBORItem) == 0x38` with `uTag` at `+0x28` and `uTagBits` at
  `+0x30`, and `UsefulOutBuf`/`UsefulInputBuf` at `0x20` with the magic number
  at `+0x18` and the error byte at `+0x1a`.
* The half-precision conversion is the blob's, not the standard one, and it
  does not decode IEEE 754 binary16: it takes the exponent from bits 2..6, the
  mantissa from bits 8..15 plus bits 0..1 and the sign from bit 7
  (`ubfx w9, w20, #2, #5`, `ubfx w8, w20, #8, #8` with `bfi w8, w20, #8, #2`,
  `sxtb w8, w20` at 0x3a24..0x3b30) — a byte-swap is missing relative to the
  big-endian half in the input, so `F9 3C 00` (half 1.0) decodes to
  3.5762786865234375e-06 and `F9 00 80` decodes to -0.0. Observed values for
  all 65536 payloads are reproduced exactly; the deviation is deliberate
  bug-for-bug compatibility.
* CBOR has an encoder/decoder error state that is only reported by `Finish`,
  and the numbers are the blob's own enum: 1 buffer too small, 2 array nesting
  too deep, 3 array too long, 5 unsupported (indefinite lengths), 6 hit end of
  input, 7 buffer too large, 9 integer overflow, 10 wrong map label type,
  11 array/map still open, 12 bad simple value, 13 date overflow, 14 invalid
  CBOR (`f8` with a payload below 32), 15 bad optional tag, 16 extra bytes.
  The reimplementation returns the same code for the same input.
* The blob is built from three translation units (`UsefulBuf.c`,
  `qcbor_encode.c`, `qcbor_decode.c`, the upstream QA/CAF layout) and the
  reimplementation keeps that split, in `src/`. That is not cosmetic: which
  intra-library calls go through the PLT rather than being inlined is part of
  the library's observable behaviour (a consumer that interposes an entry point
  is honoured only if the call is a PLT edge), and compiling the three files
  separately is what reproduces the blob's edge set. Measured both ways — the
  relocation view (`JUMP_SLOT`/`GLOB_DAT` naming a defined `FUNC`) gives the
  identical 6-symbol set for the blob and the rebuild:
  `UsefulOutBuf_Init`, `UsefulOutBuf_InsertUsefulBuf`, `UsefulOutBuf_OutUBuf`,
  `UsefulInputBuf_GetBytes`, `QCBOREncode_AddInt64_3` and
  `QCBOREncode_AddRawSimple_3`. The last two are the ones this compiler would
  inline, so both definitions carry `__attribute__((noinline))`;
  `QCBOREncode_Finish2` is deliberately left inline-able because the blob
  inlines it into `QCBOREncode_Finish` too. The instruction-level counts
  differ (29 call sites in the blob, 23 here) purely at the level of call
  *sites* — tail-sharing of the value insert across width paths and
  `AddBytesInternal` left out of line instead of outlined-and-inlined — and the
  runtime call sequence per operation is the same, which the partial-write
  cases pin: with a one-byte output buffer an item needing a three-byte header
  leaves exactly the type byte in the buffer in both builds.
  `test/edges.py` derives the set from the ELF relocations with no subprocess
  and the multiplicity from the `bl`/`b …@plt` pattern, prints the
  `strict = edges + definitions` and `loose = edges + definitions + annotations`
  reconciliations, and refuses a non-AArch64 object rather than reporting a
  plausible zero; `test/run_all.sh` additionally requires it to reject a
  `-Wl,-Bsymbolic` build, which has no own-defined PLT edges at all.
* The blob declares `liblog`, `libutils`, `libcutils`, `libc++` and `libdl` as
  `DT_NEEDED` but imports no symbol from any of them, so the module is built
  with `stl: "none"` and only `libm` (`ldexp`) is listed; `libc` is implicit.
  It exports nothing besides the 27 symbols above — every helper is a `static`
  function, which is why the module needs no version script or visibility
  attribute.
* `-fno-builtin-bcmp` is deliberate: clang sees the definition of
  `UsefulBuf_Compare` in this translation unit and lowers the `== 0` test in
  `UsefulBuf_FindBytes` to `bcmp`, while the blob calls `memcmp`. The flag
  keeps the imported symbol set identical to the blob's.
* `QCBOREncode_Init` returns no error, and the "buffer too large" test
  (`>= UINT32_MAX`) deliberately leaves the context uninitialized apart from
  the error byte, exactly as the blob does (0x2044..0x2050) — a caller that
  ignores it and keeps adding items gets `QCBOR_ERR_BUFFER_TOO_LARGE` from
  `Finish`.
* `UsefulOutBuf_Init`'s pointer sanity check is enabled in this build (the
  published source of the same era has it behind `#if 0`), so
  `UsefulOutBuf_Init(me, p, size)` with `p > UINTPTR_MAX - size` sets the
  error byte and every later insert becomes a no-op. That is reproduced.
* No indefinite-length support: `9F`, `BF`, `5F`, `7F`, `FF` and the reserved
  additional-information values 28..30 return `QCBOR_ERR_UNSUPPORTED` (5), and
  that matches the blob (jump table at `.rodata`+0xb50 sends 28..31 to the
  "return 5" exit at 0x37fc).
* Consumers: `libril-qc-hal-qmi.so`, `qcril_hal_client.so`,
  `libqcrilFramework.so`, `qcrildatactl.so` and the rest of the QCRIL family
  in the RIL section of `proprietary-files.txt` follow it. Nothing in the
  image resolves symbols against it at run time other than by linking, so it
  must stay in `PRODUCT_PACKAGES` when the blob line is dropped.
