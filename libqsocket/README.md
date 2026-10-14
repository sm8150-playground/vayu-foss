# vayu open-source vendor libraries — `libqsocket`

Section for `vendor/qcom/opensource/libraries/README.md`, in that file's format.

## Libraries

| Library | Replaces | Source group | ABI surface | How it was reverse engineered | Verification |
| ------- | -------- | ------------ | ----------- | ----------------------------- | ------------ |
| `libqsocket` | `vendor/lib64/libqsocket.so` | LA.UM.9.1.r1-13700-SMxxx0.QSSI13.0 | `ipcr_find_name(int fd, const struct ipcr_name *name, void *entries, uint32_t *ids, int32_t *num, int flag)` | AArch64 blob, 412 bytes of `.text` (392 of them the export), importing only `malloc`, `free`, `ioctl` and `__errno`. The whole function is one `ioctl()` on the caller's descriptor: the request is `_IOWR(0xc3, 2, 20-byte header)` = `0xc014c302` (`mov w1, #0xc302` + `movk w1, #0xc014, lsl #16`), prefixed by `malloc(0x14 + count * 16)` (`lsl x8, x26, #4` / `add x0, x8, #0x14`). The header is filled from the two words of arg2 (arg2[1] zeroed and the 0x10 field set to 0 instead of 0xffffffff when arg6 == 1, via `csel`/`csetm w9, ne`), the requested count from `*num`, and a zeroed 0x0c result field; `*num` is cleared before the allocation and left alone entirely when `fd < 0` (`tbnz w0, #0x1f` returns -9 first). A failed `ioctl` `free()`s the buffer, then reads `__errno` and returns its negation. On success the copy-out loop is bounded by the sign-extended 0x0c field the kernel wrote (`ldrsw x9, [x20, #0xc]`) on entry and by equality against the requested count on exit, so at most `min(found, count)` results are copied: each 20-byte record (`str x14, [x11], #0x14`) gets 0x001b at +0, 0x02 at +4 and the kernel entry's first 8 bytes at +8, and each result's entry word at +0x0c goes to `ids[i]` (`str w14, [x21, x8, lsl #2]`). With `ids == NULL` the blob runs the same loop without that store. The return value is the 0x0c field, i.e. the count the kernel reported, even when fewer records were copied. | 92-scenario differential run against the blob under the AArch64 Unicorn harness: both objects loaded at their link addresses, the `ioctl()` served by a scripted fake IPC Router device, and the return value, every `malloc` size, the request header (fd, request code and 20 header bytes), the caller's count cell, the 20-byte-stride record area and the ids array (both poisoned with 0xAA first) and the errno cell compared. All 92 scenarios byte-identical. Eighteen deliberately broken builds of the same source (record tag, flag test, ioctl code, bad-fd error, loop bound, count clearing, reported count, ids store, errno read ordering, request size, dropped request word, record stride, entry value offset, result-count arithmetic, return value, record base) each diverge from the blob on 2-84 of them, confirming the matrix exercises every branch it covers; the two remaining mutants (`count >= 1` relaxed, non-volatile read of the second request word) are known-equivalent rewrites and are called out as such in the verification report. |

## Notes

* `libqsocket`'s export has no consumer in this image: `vendor/bin/tftp_server`
  is the only object carrying the library in its `DT_NEEDED` list and it never
  resolves `ipcr_find_name` (no call site, no symbol-string reference, no
  `dlsym()`). It is still reproduced in full, because it is the library's
  entire ABI surface and `PRODUCT_PACKAGES` installs the module by name.
* The blob links `libc++.so` but imports nothing from it — the four imports are
  all libc — so the replacement is `stl: "none"` and does not carry the
  dependency.
* Field names in `struct ipcr_name_req` / `struct ipcr_name_entry` are
  descriptive; only the offsets are observable from the blob. The kernel side
  of `0xc014c302` is not part of the library, so the meaning of the request
  header's two lookup words and of the reply entry's 8-byte value and id is
  not recoverable from the blob.
* Every 20-byte output record leaves bytes 0x02..0x03, 0x05..0x07 and
  0x10..0x13 untouched, entries the kernel reported but the caller did not ask
  for are not copied, and neither `entries` nor `ids` is NULL-checked (a NULL
  `entries` faults, exactly as the blob does).
* The blob reads both words of its second argument before selecting, so the
  replacement reads them through a `volatile` pointer; otherwise `-O2` sinks
  the second load into the `flag == 1` branch and the read would not happen.
* `ipcr_find_name` returns `-errno` computed **after** `free()`, which a libc
  whose `free()` sets `errno` would expose; the replacement keeps that order,
  and the differential run pins it with a fake `free()` that clobbers `errno`.
