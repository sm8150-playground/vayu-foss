# libsysmon_cdsp_skel

This is the `vendor/qcom/opensource/libraries/README.md` section for the
`libsysmon_cdsp_skel` entry, in the reference format.

## Libraries

| Library | Replaces | Source group | ABI surface | How it was reverse engineered | Verification |
| ------- | -------- | ------------ | ----------- | ----------------------------- | ------------ |
| `libsysmon_cdsp_skel` | `vendor/lib{,64}/libsysmon_cdsp_skel.so` | CDSP (`device/xiaomi/vayu/proprietary-files.txt`, sha1 `8a951e03…` AArch64 / `b0dd56ca…` ARM32) | `sysmon_cdsp_skel_invoke(uint32_t, remote_arg *)`, `sysmon_cdsp_vote_l3(int32_t)`, `HAP_debug(const char *, int, const char *, int)`, and the IDL descriptor object `sysmon_cdsp_slim` | AArch64 blob, 1016 bytes of `.text`, plus a 660-byte Thumb-2 ARM32 twin. It is the AP-side FastRPC *skeleton* of the `sysmon_cdsp` interface: the CDSP remote-calls it, so `sysmon_cdsp_skel_invoke` is the marshalling entry point. `llvm-readelf`/`llvm-objdump` decoded it as a QAIC-generated dispatcher — `tst w0,#0x1f000000` is `REMOTE_SCALARS_METHOD`, `cmn`/`b.eq` over four `ubfx` fields is `REMOTE_SCALARS_LENGTH` folded to a zero test, and the 4-byte length check at `[x1,#8]` is `remote_arg.buf.nLen`. Failure returns are `AEE_EUNSUPPORTED` (20) and `AEE_EBADPARM` (14), the values in `external/fastrpc`'s `inc/AEEStdErr.h`. `sysmon_cdsp_vote_l3` formats `"%d000"` (`"%d"` with a literal 0 for a zero vote) into a 16-byte buffer, opens `…/soc:qcom,l3-cdsp/…/userspace/set_freq` with `"wb"` *before* validating, guards with `(unsigned)(len-1) > 14 \|\| freq < 0 \|\| !fp`, and on success writes `len+1` bytes (the kernel gets the NUL as well). All logging is the `external/fastrpc` FARF path: weak `HAP_debug_v2` first, else the `HAP_debug.h` static inline `_HAP_debug_v2` — which the blob inlines, `vsnprintf`-ing into a 256-byte buffer — ending in `HAP_debug` = `__android_log_print(level, __progname, "%s:%d: %s", file, line, msg)`; the file logged is the original TU's `__FILE__` and the line numbers (45/50/55) are baked into the message. `sysmon_cdsp_slim` decoded as the generated `const Interface` (one method `vote_l3`, scalars `0x00010000`, one non-nil uint32 `freqKHz`), matching `external/fastrpc`'s generated `*_skel.c` shape down to the padded `uint16` string tables. The source file imports nothing beyond libc/liblog; the logging `__progname` tag, the `"wb"` mode and the `errno != 0 ? errno : -1` idiom (`csinv w0,w8,wzr,ne` at 0x11c0) are reproduced verbatim. | AArch64 blob and build driven through the same 38-case matrix in the Unicorn AArch64 harness (both log transports, node present/absent, errno 0 and set): 38/38 identical return values, node bytes and log calls. ARM32 twin: 23/23 identical in the ARM32 harness. `.data.rel.ro` compared relocation-aware (same section size, same 8 relocated slots, every target resolving to the same content) for both widths, by two independently written loaders (file-image and emulator-memory walks). The intra-library call to `sysmon_cdsp_vote_l3` was measured interposable, not assumed: overwriting its GOT slot with a probe stub makes `sysmon_cdsp_skel_invoke` return the probe's sentinel in blob and rebuild alike (both widths), with a hidden-visibility rebuild as the negative control. 17 mutants of the reimplementation, 15 behaviour-changing and 2 controls: 0 wrong verdicts. |

## Notes

* Both ABI widths are shipped by the image (`vendor/lib` and `vendor/lib64`), so
  the module is intentionally not `compile_multilib`-restricted. The ARM32 blob
  is Thumb-2, which is what an Android ARM build produces.
* It is the *skeleton* (AP) side of the CDSP `sysmon_cdsp` interface, loaded by
  name from the FastRPC stack (`libcdsprpc.so`, `adsprpcd`, `cdsprpcd`); no
  library in the vayu image links against it, but `sysmon_cdsp_skel_invoke` and
  the `sysmon_cdsp_slim` descriptor are dlsym-able exports, so both are
  reproduced.
* Nothing in the blob references `sysmon_cdsp_slim`; it is reproduced rather
  than dropped because it is a dynamic export of the interface and its content
  is fully recovered (method scalars, parameter type code and the IDL string
  table), so a consumer that reads it sees the same descriptor.
* The descriptor's internal `methods`/`parameterArrays` objects land in
  `.data.rel.ro` in the opposite order in the rebuild (the blob has `methods`
  first). Both orders are semantically identical — the descriptor is reached
  only through the `Interface`'s pointers — and the rebuilt section has the same
  size and the same eight relocated slots with the same targets.
* `sysmon_cdsp_vote_l3`'s length guard cannot fire: an `int32_t` prints as at
  most 14 characters with `"%d000"` (`-2147483648000`), so `(unsigned)(len-1) >
  14` is always false. It is kept because it is in the blob, not because it is
  reachable — the effective condition is `freq < 0 || fp == NULL`.
* The `strerror()` argument is not a plain `errno` read: the blob computes
  `errno != 0 ? errno : -1` (`csinv w0,w8,wzr,ne`), so the failed-open message
  prints `strerror(-1)` when `errno` happens to be 0. The expression is
  reproduced verbatim; how the original source spelled it is not recoverable.
* The blob's `DT_NEEDED` list is liblog, libc++, libm, libdl, libc, but it
  imports nothing from libc++/libm/libdl, so the module needs only liblog.
* The blob's `__cxa_finalize`/`__stack_chk_fail` imports and the `__dso_handle`
  self-pointer at the head of `.data.rel.ro` come from the platform crtbegin and
  the default `-fstack-protector-strong` of the Android build, not from this
  source; a `-nostdlib` test link has neither.
* `HAP_debug` is exported by the blob even though `inc/HAP_debug.h` declares it
  in the framework: the CDSP skeleton has to log without liblog's FARF support,
  so it carries its own copy. It is reproduced with the upstream prototype
  (`msg, level, filename, line`), and its compiled size (44 bytes) matches the
  blob's exactly.
