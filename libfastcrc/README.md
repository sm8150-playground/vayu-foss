# libfastcrc

Section to append to `vendor/qcom/opensource/libraries/README.md`, in that
file's format. The table row goes into the `## Libraries` table after the last
existing row; the bullets go into `## Notes`.

## Libraries

| Library | Replaces | Source group | ABI surface | How it was reverse engineered | Verification |
| ------- | -------- | ------------ | ----------- | ----------------------------- | ------------ |
| `libfastcrc` | `vendor/lib/libfastcrc.so` | LA.UM.9.1.r1-13700-SMxxx0.QSSI13.0 | `VideoCRCCheckerInit`, `VideoCRCCheckerConfigure`, `VideoCRCCheckerDestroy`, `VideoCRCCheckerSetCRCCheckMode`, `VideoCRCCheckerCheckCRCofYUVframe`, `VideoCRCCheckerGetPendingCRCCount`, `VideoCRCCheckerTotalCRCCount`, `VideoCRCCheckerSkipFrameCRCCheck`, `VideoCRCCheckerGetPrevCRC`, `VideoCRCCheckerGetPendingMISRCount`, `VideoCRCCheckerTotalMISRCount`, `VideoCRCCheckerSkipFrameMISRCheck`, `VideoCRCCheckerGetPrevMISR`, `VideoCRCGetCRCFileInfo`, `VideoCRCSetSkipOpbMisr`, the nine `__aeabi_mem{cpy,set,clr}{,4,8}` veneers and the writable object `CRCVideoInterlaceType` (24 functions + 1 object; the MISR check entry point is *not* exported — it is reachable only through the checker's dispatch table) | Thumb-2 ARM32 blob, 6128 bytes of `.text`, importing only `liblog`'s `__android_log_print` and libc stdio/string. The checker object is caller allocated and 0x580 bytes: the caller's name at 0x000, `FILE *` at 0x12c, tag at 0x134, the CRC table (poly `0x04c11db7`) at 0x140, the counters at 0x540-0x55c and an eight-slot function-pointer table at 0x560 whose slots select the CRC or the MISR implementation of each entry point. `.rodata` supplied the whole on-disk surface: the `%d, %08x, …` CSV line format, the one-line header, the `" %d%*c %x%*c … %d%*c"` re-read format, the four reference-file names (`.fcrc`, `CrcFile`, `.misr`, `MisrFile`) and every log string. The CRC update is `(crc << 8) ^ table[(byte ^ (crc >> 24)) & 0xff]` over a table built from the same polynomial; the MISR path compares DPB/OPB Luma and Chroma plus crop and bit-depth fields, with a second per-frame array group and a `skipOpb` switch. The blob's PLT slots were resolved from `.rel.plt`, which also showed the three in-object `__aeabi_*` edges the rebuild had to keep. | Differential: the blob and the rebuild are relinked into identical freestanding qemu-arm harnesses and driven through one scripted scenario (init/configure, CRC compute over random geometry, CRC reference reads, MISR write/read, MISR branch-edge cases, malformed-configure cases, all nine `__aeabi_*` exports over random lengths, and the data object). **1180 transcript lines, 0 differences** after pointer normalisation. 12 one- and two-token mutants are all detected; 5 equivalent/inert controls survive (see `VERIFICATION.md`). The blob's 41 dynamic relocations are fully replayed (13 `.rel.dyn` + 7 `.relr.dyn` + 21 `.rel.plt`); the interposable self-edge set recovered from relocations equals the set recovered by GOT-probing the decoded `.plt` (3 edges, same symbols). |

## Notes

* `libfastcrc` is 32-bit only, matching the blob (`vendor/lib`, no `vendor/lib64`
  counterpart), so the module carries `compile_multilib: "32"`. It is not named
  by any other blob in the image — a scan of every `vendor/lib/*.so` `DT_NEEDED`
  entry and a symbol grep for `VideoCRCCheckerInit` over `proprietary/` both
  match only the blob itself — so it is reached by `dlopen()` from the video /
  camera stack; it must be added to `PRODUCT_PACKAGES` when the prebuilt entry
  is dropped from `proprietary-files.txt`.
* The blob's `DT_NEEDED` set (`liblog`, `libc++`, `libc`, `libm`, `libdl`) is
  decorative except for `liblog`: the only imports its relocation tables name
  are `__android_log_print@LIBLOG`, the stdio family (`fopen`, `fwrite`,
  `fprintf`, `fgets`, `feof`, `fscanf`, `fseek`, `fseeko`, `ftell`, `ftello`,
  `fclose`), `memcpy`/`memset`, the fortified `__strlcpy_chk`/`__strlcat_chk`
  and the stack-guard pair. The rebuild therefore declares
  `shared_libs: ["liblog"]` only.
* `CRCVideoInterlaceType` is an exported *data* object: a 4-byte writable
  `.bss` word that starts at zero. It is reproduced as a non-`const`
  `int` with default visibility, not folded into `const`, so a consumer that
  writes it still can.
* The nine EABI helpers occupy only three addresses in the blob
  (`0x3764`/`0x3768`/`0x3778`): all three copy forms are one veneer, all three
  set forms another, all three clear forms a third, each a PLT jump onto libc
  `memcpy`/`memset`. The rebuild reproduces the shared addresses with `alias`
  attributes. **The veneers must not be written as `memcpy(...)`/`memset(...)`**:
  on ARM EABI clang lowers such a call to `__aeabi_memcpy`/`__aeabi_memset`,
  which inside these very functions becomes a call to their own PLT slot — an
  infinite recursion that appears only at `-O2` with builtins enabled and is
  invisible to a `-fno-builtin` test build. The libc entry points are reached
  through volatile function pointers instead. `VERIFICATION.md` keeps a control
  that compiles the wrong spelling and shows the self-recursive branch being
  detected.
* `VideoCRCCheckerCheckMISR` is deliberately **hidden**. In the blob the MISR
  check lives in the dispatch table's slot 4 and is absent from `.dynsym`; the
  rebuild marks it `visibility("hidden")` so its dynamic symbol set is exactly
  the blob's 25.
* `VideoCRCCheckerConfigure` treats the two reference formats differently in
  read mode and it matters: a MISR reference file has its one-line CSV header
  consumed by `Configure` (so the first record re-read starts at the first data
  line), whereas a CRC file has no header but is rejected with status 7 when it
  is zero length. Swapping the two branches passes every CRC-only scenario and
  only shows up on the MISR read path; it is covered by the crafted MISR
  branch-edge phase.
