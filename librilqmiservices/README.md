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
| `librilqmiservices` | `vendor/lib64/librilqmiservices.so` | LA.UM.9.1.r1-13700-SMxxx0.QSSI13.0 | `embms_get_service_object_internal_v01`, `Qtuner_get_service_object_internal_v01`, `ims_qmi_get_service_object_internal_v01` — each `qmi_idl_service_object_type (int32_t idl_maj_version, int32_t idl_min_version, int32_t library_version)` — plus the three data objects they return: `embms_qmi_idl_service_object_v01`, `Qtuner_qmi_idl_service_object_v01`, `ims_qmi_qmi_idl_service_object_v01` (72 bytes each) | AArch64 blob, 104 bytes of `.text`, 1145 bytes of `.rodata`, depended on by `deviceInfoServiceModule.so` and `libril-qc-hal-qmi.so`. The three "service object" symbols are 28-byte thunks, not aliases: each loads a *different* module-local object through its own GOT slot (`adrp`/`ldr` off 0x2000 into `.got` 0x27b8/0x27c0/0x27c8, the slots `.rela.dyn` binds with `R_AARCH64_GLOB_DAT` to `embms_`/`Qtuner_`/`ims_qmi_qmi_idl_service_object_v01`), validates `(1, idl_min, library_version)` with `cmp`/`ccmp`/`csel` and returns `NULL` otherwise. The 72-byte objects are `struct qmi_idl_service_object` and the 40-byte tables they point at are `struct qmi_idl_type_table_object` from `qmi_idl_lib_internal.h` (qualcomm/qmi-framework, BSD-3-Clause); the whole pointer graph — encoded type/message descriptor tables, `msgid_to_msg` arrays, `p_referenced_tables` — was walked out of `.data`/`.data.rel.ro`/`.rodata` with `llvm-readelf` (including the 96-entry RELR table and the three `R_AARCH64_ABS64` references to the imported `common_qmi_idl_type_table_object_v01`) and re-emitted as the tables the IDL compiler would have produced. `.gnu_debugdata` supplied the local symbol names and confirmed the object boundaries. | Differential test: blob and reimplementation both loaded under the Unicorn AArch64 harness (`~/.tmp/rw/libtool/emu64.py`) and compared node by node — service-object scalars, all six `msgid_to_msg` tables (73 entries), every type/message table entry (81 entries, 71 descriptor arrays compared byte-for-byte, 707 bytes), the `p_referenced_tables` arrays and all three accessors over 343 systematic + 500 random argument triples with the returned object compared after masking pointer words. **3053 comparisons, 0 mismatches.** 15 mutated builds run as negative controls: all 13 behaviour-changing mutants (descriptor byte, `c_struct_sz`, table counts, `msgid_to_msg` index, `max_msg_len`, `n_msgs`, `p_ranges`, accessor constants) were caught, while a behaviour-neutral reordering of two descriptor arrays still passed — the comparison is not address/layout sensitive. |

## Notes

* The three accessors are **not** aliases or re-exports of the imported data
  symbol. `llvm-nm -DS` gives three `FUNC` symbols of 28 bytes at three
  distinct addresses (0x1014, 0x1030, 0x104c); no dynamic relocation points at
  them, and the only import (`common_qmi_idl_type_table_object_v01`, a 40-byte
  data object) is referenced from the three `p_referenced_tables` arrays at
  `.data`+0x3840/0x3898/0x38a8, never from `.text`. The symbol sizes reported
  as "1 byte" in the task description are actually 28 (`0x1c`) — the dynamic
  symbol table of the blob says so for both the functions and the 72-byte
  (`0x48`) data objects.
* `librilqmiservices` links against `libqmi_common_so` for one symbol only:
  `common_qmi_idl_type_table_object_v01`. It must be that exact object —
  every encoded descriptor addresses the common types as
  `QMI_IDL_TYPE16(1, n)` and element `[1]` of each `p_referenced_tables` array
  is this imported pointer, which is why the blob has three `R_AARCH64_ABS64`
  relocations against it. It is the only symbol the library imports itself:
  the one other undefined symbol in the dynamic symbol table is
  `__cxa_finalize@LIBC`, called by the toolchain's `__on_dlclose` veneer
  (`.text`+0x1000, the `.fini_array` entry), and the `libc++`/`libm`/`libdl`
  `DT_NEEDED` entries are never called from `.text` — the blob's only `PLT`
  import is `__cxa_finalize`.
* The blob stores `ims_qmi_qmi_idl_service_object_v01` in read-only
  `.data.rel.ro` while the eMBS and Qtuner objects live in writable `.data`;
  the reimplementation keeps that split by declaring only the IMS one `const`.
* `ims_qmi_get_service_object_internal_v01` checks an IDL minor version of `1`,
  but the object it returns stores `idl_minor_version = 0`. That inconsistency
  is in the blob (the accessor compares header macros, not the object's
  fields); it is reproduced verbatim, so the accessor accepts exactly
  `(1, 1, 2)` and refuses `(1, 0, 2)`.
* The reimplementation's `.rodata` is laid out differently (this compiler
  groups the 2-byte-aligned `msgid_to_msg` arrays together and the byte-aligned
  descriptor arrays after them, instead of interleaving them per service). The
  contents, lengths and cross-references of every array are identical, which is
  what the verification compares; nothing reads an array relative to a
  neighbouring one.
* Descriptor-array lengths are not stored in the data, so they were inferred
  from the fact that the generated arrays are packed contiguously in `.rodata`
  (array *n* ends where array *n+1* starts; the last ends at the end of the
  section). Every *type* descriptor array ends with `QMI_IDL_FLAG_END_VALUE`
  (0x20) exactly at its inferred end, which confirms the rule. Message (TLV)
  descriptor arrays are not 0x20-terminated, so they rely on the contiguity
  argument alone; an over-long array could only absorb bytes the blob also has
  at those addresses, so the reachable graph is unaffected either way.
