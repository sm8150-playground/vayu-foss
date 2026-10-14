# libStDrvInt

Open-source reimplementation of `vendor/lib64/libStDrvInt.so` (vayu vendor
image). Written to be spliced into the `## Libraries` table and the `## Notes`
section of `vendor/qcom/opensource/libraries/README.md`.

## Libraries

| Library | Replaces | Source group | ABI surface | How it was reverse engineered | Verification |
| ------- | -------- | ------------ | ----------- | ----------------------------- | ------------ |
| `libStDrvInt` | `vendor/lib64/libStDrvInt.so` | vayu vendor image (the blob carries no source tag; `NT_ANDROID_TYPE_IDENT` says API 30) | `stStartSession`, `stTerminateSession`, `stWaitForEvent`, plus `__cfi_check` and the 44 `__ubsan_handle_*_minimal[_abort]` handlers of the UBSan minimal runtime the blob was linked with (48 dynamic symbols in total) | AArch64 blob, 8348 bytes of `.text`, 48 exports, needed by `libSecureUILib.so` (which imports all three entry points) and `libsecureui.so` (which imports `stTerminateSession`). The three exported symbols are 4-byte veneers (`b` at 0x5088/0x5090/0x5098) to the real bodies at 0x391c/0x3e2c/0x3f88; `.rela.dyn`/RELR placed the two `.data` descriptor slots (0x72f8/0x72fc, both `-1`) and `.rodata` supplied every log tag, path component and format string. `stStartSession` walks `/data/vendor/ts_loopback` for a regular file named `secure_touch` (recursive `opendir`/`readdir` over `d_type == DT_DIR`, skipping `.`/`..`), composes `<dir>/secure_touch` and `<dir>/secure_touch_enable`, reads the third `-`-separated field of the enable path up to the next `/` as hex with `strtol(...,16)` into the caller's token, then `open(...,O_WRONLY)` and `pwrite(fd,"1",1,0)`. `stTerminateSession(force)` re-runs the lookup when forced and no session is armed, writes `"0"` and closes both descriptors. `stWaitForEvent(cancel_fd,timeout)` `pread`s the touch file at offset 0, returns 0 when it already reads back `'1'`, otherwise `poll`s the touch descriptor (events `0xb`) plus the optional cancel descriptor (events `0x1`), re-reading the file whenever the descriptor is ready and looping while the byte is not `'1'`; it returns `-ETIMEDOUT` for a zero timeout or an empty poll, `-ECONNABORTED` when the cancel descriptor fires, `-ENOMEM` when `calloc` fails and errno for I/O failures. | Blob and reimplementation run side by side under a Unicorn AArch64 harness with a virtual `/data/vendor/ts_loopback`, a scripted `poll`, and the same recorded `open`/`close`/`pread`/`pwrite`/`__android_log_print`/`write(2)`/`abort` behaviour: 67 cases, 114 entry-point calls, 0 divergences in return values, errno, log records, stderr bytes, pollfd requests, session files or descriptor traces. 16 mutants (hex/base-10 token parse, every return code, arm/disarm byte, poll event mask, open flags, `readdir` result, `.`/`..` skip, UBSan dedup and cap, CFI diagnostic kind) are all caught, and a behaviour-preserving edit still passes. |

## Notes

* `libStDrvInt` is 64-bit only, matching the blob (`vendor/lib64`, no
  `vendor/lib` counterpart). `vayu-vendor.mk` already lists `libStDrvInt` in
  `PRODUCT_PACKAGES`, so dropping the prebuilt entry is all the integration
  needs.
* The three entry points keep the blob's process-wide state: one control
  descriptor (`.data` 0x72f8), one event descriptor (0x72fc) and the two
  100-byte path buffers (`.bss` 0x73a8/0x740c). They are therefore not
  re-entrant across threads, exactly as in the blob.
* `stWaitForEvent` returns `-110` (`-ETIMEDOUT`) whenever the caller passes
  `timeout == 0`, even if `poll()` did report a ready descriptor, and `-103`
  (`-ECONNABORTED`) as soon as the cancel descriptor has any `revents`. Both are
  reproduced as-is; `-103` in particular is not the obvious "cancelled" errno.
* The blob exports the UBSan minimal runtime it was built with, so the
  reimplementation carries the same 46 handlers and `__cfi_check`. The dedup
  filter (a 20-entry site table behind a compare-and-swap, plus
  `ubsan: too many errors`) and the per-handler messages are reproduced;
  `__cfi_check`'s type ids (`0xac3778598d51adbd` terminate,
  `0xacf1de79d22706ab` start, `0x6cf58e448911dfd5` wait) come straight from
  the compare chains at 0x5000-0x5064.
* The blob guards its indirect `poll()` call with `__cfi_slowpath(0x83827495feaef41e, poll)` before every `poll`. The reimplementation calls `poll()` directly: `__cfi_slowpath` is a no-op for a pointer that is a valid function of a checked DSO, so the only difference is that the replacement does not import `__cfi_slowpath@LIBC_OMR1`.
* The blob's fortify and stack-protector imports (`__open_2`, `__strlen_chk`,
  `__strlcpy_chk`, `__strlcat_chk`, `__stack_chk_fail`) come from Soong's
  default `_FORTIFY_SOURCE`/`-fstack-protector-strong` cflags, not from
  anything in the source; the source uses plain `open`/`strlen`/`strlcpy`/
  `strlcat` and inherits the same hardening when built by Soong. `open()`
  passes a constant `O_WRONLY`/`O_RDONLY`, so the fortified `__open_2` never
  has a mode argument to check.
* The token field is parsed as hex, so a node directory whose third `-`-field
  is `"16"` yields 22 and one with `"99"` yields 153 — matching the blob, which
  passes base 16 to `strtol`.
