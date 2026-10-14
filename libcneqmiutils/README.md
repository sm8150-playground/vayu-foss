# libcneqmiutils

Section to append to `vendor/qcom/opensource/libraries/README.md`, in that
file's format. The table row goes into the `## Libraries` table after the last
existing row; the bullet goes into `## Notes`.

## Libraries

| Library | Replaces | Source group | ABI surface | How it was reverse engineered | Verification |
| ------- | -------- | ------------ | ----------- | ----------------------------- | ------------ |
| `libcneqmiutils` | `vendor/lib64/libcneqmiutils.so` | LA.UM.9.1.r1-13700-SMxxx0.QSSI13.0 | `_ZN11CneQmiUtils9isQmiPortEPKc` (`CneQmiUtils::isQmiPort(char const *)`) plus the data object `qmiPorts` (47 pointers, 376 bytes) | AArch64 blob whose entire `.text` is 116 bytes. `llvm-readelf` gave the two exports and the GOT relocation (`R_AARCH64_GLOB_DAT qmiPorts`) that points into `.data`; `llvm-objdump` decoded the body as a null check (`cbz x0`), a walk of the table loaded from GOT slot `0x21e8` and a `strcasecmp` call per entry (`blk` at `0x1040`), with the counter compared against `0x2f` = 47, matching the object's `376 / 8` elements. The table's contents came from `.rodata`: the 47 pointers were read out of `.data` and each dereferenced to its string, in file order. The three 4-byte stubs ahead of the function are the compiler's `__on_dlclose` / `__emutls_unregister_key` / `__on_dlclose_late` slots recovered from the blob's compressed `.gnu_debugdata`, and they are not in `.dynsym`. | Both objects — the blob and the rebuild — loaded into a Unicorn AArch64 harness with a `strcasecmp` handler, driven over 168 inputs (all 47 table entries, each also in upper- and alternating-case form, plus prefixes, single-character near misses, trailing space/tab/newline/dot, a 256-byte name, UTF-8) and then a run-time mutation of `qmiPorts[0]`; the two agree on x0, on the number of `strcasecmp` calls and on which table entry each call compared, and both agree with a Python oracle evaluated from the table read out of the blob. 168/168 + mutation phase: no mismatch. Negative controls: 9 deliberately mutated builds all diverge (0 survive) while a semantically equivalent rewrite and an inert include both survive, so the matrix discriminates. Re-verified by a second loader built independently of the shared harness (own ELF/APS2/RELR decoding, comparison delegated to host glibc `strcasecmp`): 20/20 identical, including call counts. |

## Notes

* `libcneqmiutils` is 64-bit only, matching the blob (`vendor/lib64`, no
  `vendor/lib` counterpart). No library in this image names the symbol or the
  library — a `grep` for `isQmiPort`/`libcneqmiutils` over `vendor/` matches
  only the prebuilt module definition and the `PRODUCT_PACKAGES` list — so it is
  reached only by `dlopen`; it stays in `PRODUCT_PACKAGES` under its own name
  and the prebuilt module of the same name is simply replaced.
* The blob's `DT_NEEDED` set (`libutils`, `libc++`, `libc`, `libm`, `libdl`) is
  decorative: the only imports its relocation tables name are `strcasecmp` and
  `__cxa_finalize`, both in libc. The rebuild therefore declares no
  `shared_libs`. Nothing can resolve a symbol through those entries — the
  library exports nothing but its own two symbols.
* `qmiPorts` is exported, not static, and lives in the writable `.data`
  segment in the blob, so the rebuild reproduces it as a writable
  default-visibility array with the same 47 entries in the same order rather
  than folding the names into the comparison loop. The verification harness
  confirms the property is observable: after a run-time repoint of entry 0 both
  objects answer `true` for the new name and `false` for `rmnet0`.
