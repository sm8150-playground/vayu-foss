# libGPreqcancel_svc

Open-source reimplementation of `vendor/lib64/libGPreqcancel_svc.so` from the
vayu vendor image (DRM / QSEECom set), written from static analysis of the blob
and verified against it under emulation.

## Libraries

| Library | Replaces | Source group | ABI surface | How it was reverse engineered | Verification |
| ------- | -------- | ------------ | ----------- | ----------------------------- | ------------ |
| `libGPreqcancel_svc` | `vendor/lib64/libGPreqcancel_svc.so` | DRM — from LA.UM.9.1.r1-13700-SMxxx0.QSSI13.0 | `int gp_reqcancel_svc_sock_init(const char *, size_t, int)`, `void gp_reqcancel_svc_sock_terminate(void)`, `int gp_reqcancel_svc_sock_rcv_msg(unsigned int, unsigned int)`, `void gp_reqcancel_svc_sock_send_msg(int, const char *, size_t)` | AArch64 blob, 4376 bytes of `.text`, four exported C functions, no C++ and no data exports. `.data+0x0` (0x62e8, initialised to `-1`) is the module socket and the 108-byte `.bss` buffer at 0x62f0 is the remembered name; every function reaches them off `adrp x27, 0x6000` (`ldr w8, [x27, #0x2e8]`). `.rela.plt`/`.relr.dyn` map the 24 imports, all libc/liblog. The single datagram socket turned out to be the **abstract** AF_UNIX address `"\0gplisnr"`: `gp_reqcancel_svc_sock_init` rejects `name[0] != '\0'` (0x310c `ldrb`/`cbz`) and then writes `sun_family = AF_UNIX` (`strh`), `sun_path[0] = 0` (0x3208 `strb wzr, [sp, #2]`), and `strlcpy(&sun_path[1], name+1, len-1)` (`orr x0, sp, #3`), with the address length `(uint32_t)len + 2`. Both in-tree consumers confirm the calling convention: `libGPreqcancel.so` passes the `.rodata` literal `"\0gplisnr"` with `len 9` and `do_bind 1` (0x3014-0x3070), while the qteeconnector HAL's `gp_request_cancellation` passes the same 9 bytes with `do_bind 0` and then `send_msg` (0xee44-0xef40). The bind loop retries every failure up to 1024 times (0x3240 `cmp x20, #0x400`), logging and `sched_yield()`ing only for `EADDRINUSE` (0x326c). `rcv_msg` installs `SO_RCVTIMEO` built from a millisecond budget (`umull` by `0x10624dd3` >> 38 for `/1000`, remainder ×1000 for `tv_usec`), then `clock()`/`recvfrom(fd, &msg, 4, …)`/`clock()` and converts the delta to milliseconds as `(t1-t0) * 1000.0` through a saturating `fcvtzu`; a 4-byte payload equal to the requested session id returns `-65534` (0x360c), everything else is logged, retried while budget remains, and otherwise returns `0`/`-65536` (0x36dc). All 18 log sites use the tag `"LibGPCancelSvc"` (0x18ea) with `__func__` and the remembered name in the two `%s` slots. | Differential: blob and reimplementation loaded in the same AArch64 Unicorn harness with recording stubs for every libc/liblog import, driven through 47 scenarios that compare return values, the full syscall trace (arguments included, e.g. the exact `sockaddr` bytes handed to `bind`/`sendto`), the fully formatted liblog output and the final `fd`/name state — 47/47 identical, 0 divergences. 26 mutated builds used as negative controls: 25/26 caught by the matrix, the survivor proved to be an equivalent mutation (see VERIFICATION.md). Real-kernel end-to-end: the same source linked against the host libc and run as a service/client pair over the abstract socket, 25/25 expectations. See `VERIFICATION.md` for the commands and raw output. |

## Notes

* The name argument is a NUL-prefixed buffer, not a plain string: `name[0]`
  must be `'\0'`, the socket name starts at `name + 1`, and `len` is the size of
  the whole buffer (`"\0gplisnr"` → 9). Only `len` in `[2, 108]` is accepted;
  the leading byte is also what makes the resulting address abstract, so it is
  written to `sun_path[0]` and never dropped. The blob's string copy truncates
  at 106 characters, which is preserved.
* `gp_reqcancel_svc_sock_init` returns `0` when the socket already exists
  ("already initialized") rather than an error, and `-EINVAL`/`-errno`
  otherwise; every failure that leaves a live descriptor closes it and resets
  the latch, so a later call can retry.
* `gp_reqcancel_svc_sock_rcv_msg` measures elapsed time with `clock()`, whose
  Linux/bionic resolution is microseconds, but multiplies the delta by 1000.
  The value it logs as "ms" is therefore 1000× the real elapsed time, and the
  retry budget after a non-matching datagram is consumed about 1000× faster
  than the caller asked for. This is the blob's behaviour and is reproduced
  verbatim; it means a misdelivered datagram effectively ends the wait
  (the first mismatch usually zeroes the budget), and it is why the "retry
  after mismatch" path needs an unusually large timeout to be observable.
* The error returns are literals: `-65534` (0xFFFF0002) when the awaited
  cancellation arrives and `-65536` (0xFFFF0000) when the socket failed. The
  service side (`gp_reqcancel_start` in `libGPreqcancel.so`) forwards that value
  straight to `QSEECom_send_resp`, so they are part of the GP protocol.
* The blob is a UBSan-instrumented build. One of its checks is observable:
  `gp_reqcancel_svc_sock_send_msg` with `len == 0` traps in
  `__ubsan_handle_sub_overflow_minimal_abort` (0x3720 `subs`/`b.lo 0x3828`)
  before doing anything. A non-instrumented build — including this
  reimplementation as AOSP builds it — simply computes `len - 1` and carries
  on, which is what the source does; that single case therefore diverges, and
  it is outside the API contract (every in-tree caller passes 9).
* The blob's `.fini_array` holds `__on_dlclose` (0x3000, the compiler's
  `__cxa_finalize` veneer) and `__on_dlclose_late` (0x3010, an immediate
  `ret`), plus a self-pointing `.data.rel.ro` DSO handle. They are crt
  boilerplate and carry no library behaviour, so nothing is reproduced for
  them; `.gnu_debugdata` is left out for the same reason.
* The `.rodata` string `"%s(%s):timeout %u ms t"` (0x18be) really is truncated
  there — the linker tail-merged it — and the blob logs it verbatim, so the
  reimplementation keeps the same (odd-looking) text.
* No consumers declare the API in a header; the two dependents call it through
  the dynamic symbol table, and `include/libGPreqcancel_svc.h` here documents
  the recovered signatures for the replacement build.
* The blob's only interposable intra-image call edges are four `__ubsan_handle_*`
  trap sites routed to its own exported UBSan runtime (four symbols, six call
  sites); the reimplementation has none because it is not sanitizer-instrumented.
  Both dependents define their own 44 `__ubsan_handle_*` symbols and import none,
  so nothing can name, override or observe the difference — a bounded residual
  rather than a gap. The alternative would be to keep exporting 44 compiler
  runtime symbols from a hand-written C file, which is not library behaviour.
* `clock()`, `usleep()`, `sched_yield()` and the socket calls are the whole libc
  surface, so the replacement needs `liblog` and nothing else at build time.
