# libqmi_encdec

Open-source reimplementation of the proprietary vendor blob
`vendor/lib64/libqmi_encdec.so` (AArch64, Android 13 / vayu vendor image).

### README entry for the `vendor/qcom/opensource/libraries` table

| Library | Replaces | Source group | ABI surface | How it was reverse engineered | Verification |
| ------- | -------- | ------------ | ----------- | ----------------------------- | ------------ |
| `libqmi_encdec` | `vendor/lib64/libqmi_encdec.so` | LA.UM.9.1.r1-13700-SMxxx0.QSSI13.0 | All 17 exports of the QMI IDL codec: `qmi_idl_message_encode`, `qmi_idl_message_decode`, `qmi_idl_message_decode_v6`, `qmi_idl_find_msg`, `qmi_idl_decode_find_tlv`, `qmi_idl_decode_verify_mandatory`, `qmi_idl_encode_resp_tlv`, `qmi_idl_get_std_resp_tlv_len`, the nine service-object accessors (`qmi_idl_get_idl_version`, `qmi_idl_get_idl_minor_version`, `qmi_idl_get_service_id`, `qmi_idl_get_max_service_len`, `qmi_idl_get_max_message_len`, `qmi_idl_get_message_c_struct_len`, `qmi_idl_get_max_c_struct_len`, `qmi_idl_inherit_service_object`, `qmi_idl_get_inherited_service_object`) | AArch64 blob, 8540 bytes of `.text`, 6 unused `DT_NEEDED` entries and *no* external data dependency at all; it imports only `memcpy`/`memset`/`strlen`(+fortify) and `setjmp`/`longjmp`. Each entry point was reimplemented from its disassembly against the IDL type-table format (flags byte → offset → array size → size offset → range index → aggregate type), and the two `longjmp`-based error paths released from the `jmp_buf` layout in the blob's frame. | Blob and rebuild driven side by side under a Unicorn AArch64 emulator over a synthetic but complete IDL graph — 128 comparisons (accessor matrices, encode/decode of scalars/enums/strings/fixed and variable arrays/nested aggregates/optional TLVs, truncation, malformed TLVs, every documented error code, and the interposed libc-call timeline) all identical; 10 behaviour-changing mutants diverge and 5 behaviour-preserving controls do not; export set, import set, interposable PLT edge set (byte view and objdump view, gated on a libqrtr positive control and an ARM32 refusal) and relocation accounting all identical. |

## Notes

* The blob is the userspace QMI IDL encoder/decoder, a sibling of the kernel
  driver `drivers/soc/qcom/qmi_encdec.c` in this very tree. The two share the
  IDL type-table format but not the code: the kernel version uses fixed
  `qmi_elem_info` arrays and returns `int`, whereas this library walks the
  *encoded* descriptor tables emitted by the QMI IDL compiler, raises errors
  through `longjmp`, and takes a 40-byte type-table object plus a 72-byte
  service object (layouts identical to `qmi_idl_lib_internal.h` from
  `qualcomm/qmi-framework`, pinned here by `_Static_assert`).
* All the logic lives in a single translation unit, `libqmi_encdec.c`, written
  from the blob's disassembly and cross-checked against the semantics of
  `qmi_idl_lib.c`/`qmi_idl_accessor.c`. `qmi_idl_message_encode_v6`,
  `qmi_idl_encode_element`, `qmi_idl_decode_element`, `qmi_idl_encode_length`
  and `qmi_idl_next_element` are `static` exactly as in the blob (they have no
  dynamic symbol); the 17 exported symbols are spelled out with
  `__attribute__((visibility("default")))`.
* The blob's `DT_NEEDED` list is `libcutils`, `libutils`, `libc++`, `libc`,
  `libm`, `libdl`, but **not one symbol is imported from any of them**: the
  only undefined symbols are the four libc functions above plus the toolchain's
  `__cxa_finalize` dlclose veneer. The reimplementation keeps only the libc
  dependency it actually uses; a drop-in consumer that relied on
  `libqmi_encdec` to pull `libcutils`/`libutils` transitively would have been
  relying on a linker-dropped dependency, and every real consumer
  (`libril-qc-hal-qmi.so`, `libqmi_common_so.so`, …) links those itself.
* Five of the exports are reached through the blob's own PLT, so a consumer
  that overrides one of them is honoured: `qmi_idl_find_msg`,
  `qmi_idl_get_inherited_service_object`, `qmi_idl_decode_find_tlv`,
  `qmi_idl_decode_verify_mandatory` and `qmi_idl_message_decode_v6`. The
  Android.bp therefore builds with `-fsemantic-interposition`, without which
  clang inlines the `get_inherited_service_object` and `decode_find_tlv` calls
  and the edge set shrinks from 5 to 3.
* The blob's compiler specialised `qmi_idl_message_decode_v6` into two TLV
  loops (one for responses, which watches TLV 2 for the error-reporting
  contract, and one for requests/indications). The reimplementation keeps a
  single loop that carries the same `resp_error` flag, so it has one
  `qmi_idl_decode_find_tlv` PLT call site where the blob has two. That is a
  multiplicity difference on an edge both objects *have*; an interposing
  consumer is honoured identically, and both loops' behaviour is compared by
  the differential matrix.
* The only relocated data in the blob is the toolchain's dlclose veneer:
  `.data.rel.ro` (one self-pointer, the DSO handle) and `.fini_array` (the two
  `__on_dlclose`/`__cxa_finalize` entries). It is not library data, and the
  rebuild's matching `.data.rel.ro` slot is the same toolchain artefact, so
  there is no library-owned data region whose relocation tables could be
  compared — the relocation comparison that matters here is the `JUMP_SLOT`
  set (see the verification report).
* The scratch build in this directory (`build.sh`) mirrors how Soong links a
  `cc_library_shared`: it adds `-fstack-protector-strong` and
  `-D_FORTIFY_SOURCE=2` (soong defaults) to reproduce the blob's exact import
  spelling (`__memcpy_chk`, `__strlen_chk`, `__stack_chk_fail`), and
  `-Wl,--pack-dyn-relocs=relr -Wl,--gc-sections -ffunction-sections` so the
  relocation tables come out the same shape as the blob's (13
  `R_AARCH64_JUMP_SLOT` records and one 16-byte `.relr.dyn`). The Android.bp
  leaves all of those to the platform. The only difference the fortify flags
  make is which libc symbol performs the same copy/scan, which the
  differential harness canonicalises.
