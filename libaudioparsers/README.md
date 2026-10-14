# libaudioparsers

Section to append to `vendor/qcom/opensource/libraries/README.md`, in that
file's format. The table row goes into the `## Libraries` table after the last
existing row; the bullets go into `## Notes`.

## Libraries

| Library | Replaces | Source group | ABI surface | How it was reverse engineered | Verification |
| ------- | -------- | ------------ | ----------- | ----------------------------- | ------------ |
| `libaudioparsers` | `vendor/lib/libaudioparsers.so` (sha1 `b786d5a2a8f94edb11b5334bbe981dbd2633325f`) | vayu vendor image (the blob carries no source tag; `NT_ANDROID_TYPE_IDENT` says API 30, build id `61b1e5aeb3cd1f8a49e62da7c8fe5079`) | `init_audio_parser`, `get_first_frame_info`, `get_audio_frame`, `get_channel_status`, plus the AC-3/DTS internals they are built on (`init_audio_parser_{ac3,dts}`, `get_{ac3,dts}_first_frame_info`, `get_{ac3,dts}_frame`, `get_{ac3,dts}_frame_info`, `get_{ac3,dts}_frame_sync_info`, `get_{ac3,dts}_frame_channel_info`, `get_channel_status_{ac3,dts}`, `get_silent_frame`, `init_chan_status`, `set_bits`, `extract_bits`, `get_iec61937_info`) and the exported data object `ac3_fr_sz_code_tbl[114]` — 32 `FUNC` exports plus one `OBJECT`, 33 dynamic symbols in total | Thumb-2 ARM32 blob, 4316 bytes of `.text`, 10 imports (nine libc functions and `stderr`). `llvm-readelf` gave the three relocation tables — a plain `.rel.plt` (23 `R_ARM_JUMP_SLOT`), an APS2-packed `.rel.dyn` (3 `R_ARM_GLOB_DAT`) and a 2-word `RELR` table — which pin every libc call and both `.data` accesses to an exact PLT slot; `llvm-objdump` decoded `.text` in Thumb mode for the body (0x1dc0-0x2e58) and in ARM mode for the 0x20-byte stretch that holds the nine `__aeabi_*` stubs (0x2e58-0x2e78, which is why their symbol values are even), and the three Thumb `movw/movt r12 / add r12, pc / bx r12` veneers at 0x2e78-0x2e9c that carry a `b.w` tail call into an ARM-state PLT stub. `.rodata` supplied all 23 format strings and the nine function-name strings (which is how the `get_first_frame_info` default arm's `"Unsupported codec type %u\n"` was pinned down) and `.data` the 38x3 frame-size table. The parser state is eight `.bss` words: an AC-3 cursor `{base, off, len}`, a DTS cursor `{base, off, len}`, a scratch-header pointer and a cached DTS frame length. Frame framing was recovered from the tables and the checks: AC-3 frame size is `ac3_fr_sz_code_tbl[fscod * 38 + frmsizecod]` 16-bit words, doubled to a byte count; DTS takes `nblks + 1` from bits 14..27, and searches in 16-bit steps from `cursor + (nblks & ~1)` for the `FE7F 0180` pair that follows the frame, caching the adjusted offset in the `.bss` word. The blob's own quirks are kept, not fixed: `get_ac3_frame`/`get_dts_frame` zero the destination and rewrite `info` *before* validating, `get_ac3_first_frame_info`/`get_audio_frame` ignore the AC-3 cursor, the frame copy is unbounded by the buffer length, `get_dts_frame_info` leaks its 12-byte scratch header on the two failure paths, and `dst_size` is judged as 16-bit words. | Both objects driven through one 1809-case scenario script under Unicorn (`drv.py`, diffed by `cmp.py`): every dispatcher codec value, 512 random + exhaustive AC-3 sync headers, the whole fscod/frmsizecod/bsid grid (including out-of-range), 300 random `set_bits`/`extract_bits` field writes, 256 random DTS sync words, single/multi-frame DTS streams with the `nblks+1` search, truncated and byte-swapped streams, destination-size bounds and EOF paths — 3131 calls, **0 mismatches** on returns, `info`, destination memory and every interposed `fprintf`/`fwrite` line. 25 mutants (20 behaviour-changing, 5 equivalent controls) rebuilt and re-diffed: all 20 diverge, all 5 stay identical. Relocation accounting (`relocs.py`) asserts `rela_printed == rela_parsed` per section and that every symbol relocation was applied. Interposable edges (`edges.py`) reconcile 16 interposable symbols (15 `JUMP_SLOT` functions + the `GLOB_DAT` table) with identical per-symbol call-site counts in both objects. |

## Notes

* `libaudioparsers` is 32-bit only, matching the blob (`vendor/lib`, no
  `vendor/lib64` counterpart). It is a `DT_NEEDED` entry of
  `vendor/lib/libhdmipassthru.so` and is already in `PRODUCT_PACKAGES`
  (`vendor/xiaomi/vayu/vayu-vendor.mk:347`), so dropping the prebuilt line from
  `device/xiaomi/vayu/proprietary-files.txt` is all the integration needs.
* The blob is Thumb-2 (`.ARM.attributes`: `CPU_arch ARM v7`,
  `THUMB_ISA_use Thumb-2`, `Advanced_SIMD_arch NEONv1`) and Soong's default for
  an `arm` module is Thumb too, so the module is built in the same instruction
  set as the blob. Both instruction sets were verified anyway: the ARM-mode
  build of the same source passes the identical 1809-case differential.
* Four of the exports are pure aliases in the blob, and are kept as aliases
  here: `__aeabi_memcpy`/`__aeabi_memcpy4`/`__aeabi_memcpy8` share one ARM-state
  stub whose address is even (0x2e58), and likewise
  `__aeabi_memset{,4,8}` (0x2e5c) and `__aeabi_memclr{,4,8}` (0x2e6c). They are
  written in `aeabi_stubs.c` as ARM-state trampolines to `memcpy`/`memset`, with
  `__aeabi_memset`'s swapped second and third arguments (`memset(dst, c, n)`)
  and `__aeabi_memclr`'s zero fill. Their symbol values must stay even — a
  consumer's `blx` to 0x2e58 is what puts the core into ARM state.
* The reimplementation keeps the blob's `.bss` state (two cursors, the DTS
  header pointer and the cached frame length) as file-static data, so the
  library is a single process-wide parser and is not re-entrant — exactly like
  the blob, whose four entry points share one cursor per codec.
* `get_channel_status_dts`'s first store to the caller's block
  (`dst[0] = old | 0x80`) is dead in the blob: `get_dts_frame_channel_info`
  always leaves `st[0] == 1`, so the immediately following
  `dst[0] = (old & 0x3F) | 0x40` overwrites it. It is reproduced as written,
  and a mutant that changes the dead store confirms the differential cannot see
  it (it is one of the equivalent controls, not a false pass).
* `get_ac3_frame_info` indexes `ac3_fr_sz_code_tbl` with
  `fscod * 38 + frmsizecod` and does **not** bounds-check `frmsizecod`; only
  `get_ac3_frame_sync_info` does. Reaching it directly with `frmsizecod > 37`
  therefore reads past the table in both objects — reproduced, not fixed.
* The blob's `DT_NEEDED` list (`libutils`, `liblog`, `libc++`, `libc`, `libm`,
  `libdl`) is decorative: the only symbols its relocation tables name are libc's
  (`fprintf`, `fwrite`, `malloc`, `free`, `memcpy`, `memset`, `stderr`,
  `__stack_chk_guard`, `__stack_chk_fail`, `__cxa_finalize`) plus its own. The
  rebuild declares no `shared_libs`, and its extra `__stack_chk_*` imports are
  absent because it is not built with a stack protector; `__cxa_finalize` is
  likewise not called.
