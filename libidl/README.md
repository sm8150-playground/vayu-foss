# libidl

Open-source reimplementation of the proprietary vendor blob
`vendor/lib64/libidl.so` (AArch64, Android 13 / vayu vendor image).

### README entry for the `vendor/qcom/opensource/libraries` table

| Library | Replaces | Source group | ABI surface | How it was reverse engineered | Verification |
| ------- | -------- | ------------ | ----------- | ----------------------------- | ------------ |
| `libidl` | `vendor/lib64/libidl.so` | LA.UM.9.1.r1-13700-SMxxx0.QSSI13.0 | All 17 exports of the QMI IDL runtime: the two codec entry points `qmi_idl_message_encode` / `qmi_idl_message_decode` and their v6 core `qmi_idl_message_decode_v6`, the descriptor lookups `qmi_idl_find_msg` / `qmi_idl_decode_find_tlv` / `qmi_idl_decode_verify_mandatory`, `qmi_idl_encode_resp_tlv` / `qmi_idl_get_std_resp_tlv_len`, and the nine service-object accessors `qmi_idl_get_idl_version`, `qmi_idl_get_idl_minor_version`, `qmi_idl_get_service_id`, `qmi_idl_get_max_service_len`, `qmi_idl_get_max_message_len`, `qmi_idl_get_message_c_struct_len`, `qmi_idl_get_max_c_struct_len`, `qmi_idl_inherit_service_object`, `qmi_idl_get_inherited_service_object` | AArch64 blob, 8944 bytes of `.text`, 17 `FUNC` exports in `.dynsym` and no exported data. `llvm-readelf` gave the export list, the 12 `R_AARCH64_JUMP_SLOT` records and the libc import set; `llvm-objdump` recovered the descriptor walkers (the linear `msgid_to_msg` scan, the `find_tlv` TLV walk, the `decode_element`/`encode_element` switch tables at `.rodata+0x970`) and the accessor version checks. The 40-byte type-table object and the 72-byte service object are `qmi_idl_lib_internal.h`'s layouts from `qualcomm/qmi-framework` (BSD-3-Clause), pinned by `_Static_assert`. Both the blob and the rebuild were then driven through 210 identical constructed IDL scenarios under Unicorn — valid, malformed, out-of-range-id, empty-table, inherited-service and range-check cases — comparing returns, exception records, produced buffers and every interposed libc call. | `difftest.py`: 210 cases, 0 divergent. `mutants.py`: 9 behaviour-changing mutants all diverge, 5 equivalent controls all pass. `reloc_check.py`: `rela_printed == rela_parsed ==` bytes-level count for both objects. `edge_check.py`: identical 4-symbol / 6-call-site PLT edge set. See `verification.md`. |

## Notes

* The library is the userspace half of Qualcomm's QMI IDL compiler: it turns
  the encoded type/message descriptor tables the compiler emits into
  encode/decode operations on QMI messages, and answers the size and version
  questions a QCCI/QCSI client asks about a service object. It is loaded by
  ten libraries in this image (`libqmi*.so`, `libril-qc-hal-qmi.so`,
  `libqmiservices.so`, `libcne.so`, `lib-imsrcs*.so`); `libidl.so` is listed
  even though the blob's `DT_NEEDED` entry alone says nothing about who uses
  which symbol.
* All the logic lives in a single translation unit, `libidl.c`. The internal
  helpers the blob's compiler folded away (`qmi_idl_next_element`,
  `qmi_idl_decode_element`, `qmi_idl_encode_element`, `qmi_idl_encode_length`)
  are `static` here too — they have no dynamic symbol — while the 17 exported
  names carry `__attribute__((visibility("default")))`.
* The blob's `DT_NEEDED` list is `libc++.so`, `libc.so`, `libm.so`,
  `libdl.so`, but **not one symbol is imported from `libc++`, `libm` or
  `libdl`**: the only undefined symbols in `.dynsym` are eight libc functions
  (`memcpy`, `memset`, `__memcpy_chk`, `strlen`'s fortified entry
  `__strlen_chk`, `setjmp`, `longjmp`, `__stack_chk_fail`, and the
  toolchain's `__cxa_finalize` dlclose veneer). The reimplementation keeps
  only the libc dependency it actually uses; a consumer that relied on
  `libidl` to pull `libc++` transitively would have been relying on a
  linker-dropped dependency.
* Four of the exports are reached through the blob's own PLT, so a consumer
  that overrides one of them is honoured: `qmi_idl_find_msg` (3 call sites:
  `.text+0x1374`, `+0x243c`, `+0x30e4`), `qmi_idl_get_inherited_service_object`
  (1: `+0x103c`), `qmi_idl_decode_verify_mandatory` (1: `+0x1750`) and
  `qmi_idl_message_decode_v6` (1: `+0x2314`). clang emits `bl <sym@plt>`
  precisely for an exported function it does not inline, so the reimplementation
  marks those four `__attribute__((noinline))` and routes the two calls the
  blob's build inlined (`qmi_idl_decode_find_tlv` inside
  `qmi_idl_message_decode_v6`, and `qmi_idl_get_inherited_service_object`
  inside the two length accessors) through internal definitions. The result is
  the blob's exact edge set — 4 symbols, 6 call sites — which `edge_check.py`
  asserts from the relocation tables and from a byte-level scan of `.text`.
  `libqmi_encdec.so` in this same image is the same library built with
  `-fsemantic-interposition` and carries a 5-symbol edge set
  (`qmi_idl_decode_find_tlv` included); the edge set is a property of the
  build, not of the source, so each blob is reproduced on its own terms.
* `qmi_idl_find_msg` searches `msgid_to_msg` **linearly** and then walks the
  parent chain (`.text+0x1044`: load `n_msgs[type]`, scan the 6-byte entries
  comparing `qmi_message_id`, `bl qmi_idl_get_inherited_service_object@plt`
  at `+0x103c` when the count runs out). The BSD-3-Clause reference
  implementation bisects, so a rewritten table that is not sorted by message
  id would behave differently between the two; the blob's linear scan is
  reproduced and the 0x30 case (present in the blob's own table) is compared
  in the differential.
* `qmi_idl_message_encode` rejects a NULL `p_src` or `p_dst` **unconditionally**
  (`.text+0x240c` / `+0x2410` test the pointers alone), not only when the
  matching length is non-zero as the published source and its tests do, so a
  NULL buffer with a zero length is `PARAMETER_ERROR` here.
* `qmi_idl_inherit_service_object` accepts only `library_version` 5 and 6
  (`.text+0x329c` / `+0x32ac` reject `< 5` with `-50`, `+0x32d0` rejects the
  upper bound), while the codec and the accessors accept up to 6. The
  asymmetry is in the blob and is reproduced (`QMI_IDL_LIB_ENCDEC_VERSION` is
  5).
* The one import the rebuild cannot reproduce by name is the string length
  scan: the blob calls bionic's fortified `__strlen_chk(s, (size_t)-1)` at
  `.text+0x2744`, and clang folds a direct call of that form back to `strlen`
  (verified in this tree). With an unknown bound that fortified call *is*
  `strlen` plus a comparison that cannot fire, so `difftest.py` canonicalises
  exactly that argument to the plain `strlen` trace entry and records any
  other bound verbatim.
