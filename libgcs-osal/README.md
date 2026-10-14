| `libgcs-osal` | `vendor/lib/libgcs-osal.so` | Xiaomi vayu vendor image (API 30, AArch32/Thumb-2; sha1 `2f1ff749e6c2210d715486ac0d01046f2aa93abc`) | the nine AEABI memory routines (`__aeabi_memcpy{,4,8}`, `__aeabi_memset{,4,8}`, `__aeabi_memclr{,4,8}`) plus 23 OSAL wrappers: `osal_mem_{alloc,zalloc,calloc,realloc,free,cpy,set}`, `osal_lock_{create,destroy,acquire,release}`, `osal_cond_{create,destroy,wait,timedwait,signal}`, `osal_thread_{create,destroy,close_from_thread_fn}`, `osal_dev_{open,close,read,write}` — 32 exports in total, all reproduced | Thumb-2 ARM32 blob, 2308 bytes of `.text`, needed by `libgcs.so`, `libgcs-ipc.so` and `libgcs-calwrapper.so` (which between them call `osal_lock_{create,destroy,acquire,release}`, `osal_mem_{zalloc,set,cpy,free,realloc}`). `.rel.plt` names all 34 imports and, because the PLT stubs are ARM (`ldr pc,[r12,#imm]!`), it also fixes the exact libc entry point behind every `blx`: `malloc`, `calloc`, `realloc`, `free`, `memcpy`/`memset` (through the three AEABI symbols the module defines itself), `pthread_mutex_{init,destroy,lock,unlock}`, `pthread_cond_{attr_init,attr_setclock,init,destroy,wait,timedwait,signal}`, `pthread_attr_{init,setdetachstate,destroy}`, `pthread_create`, `pthread_join`, `clock_gettime`, `__open_2`, `__errno`, `close`, `__read_chk`, `__write_chk`, `__android_log_print`. `.rodata` holds every `%s:` format string, each call site's `__func__` literal (resolved from the literal pools as `pc + <offset>`), and thus which failures log an `rc`; the handle layouts come from the blob's own `malloc` sizes and field offsets (4-byte `{pthread_mutex_t}`, 8-byte `{pthread_mutex_t; pthread_cond_t}` with the condattr clock set to `CLOCK_MONOTONIC`, 12-byte `{pthread_t; fn; arg}` with a 56-byte frame whose `ldrd r1, r0, [sp, #60]` at 0x2824 reads the *sixth* and *seventh* arguments, and a 4-byte `{int fd}`). | Blob and rebuild run side by side in the ARM32 Unicorn harness (`libtool/emu.py`, extended for this module: deterministic libc/pthread/`__android_log_print` stubs, a virtual `/dev` node, and trap-based emulation of the four VFP/NEON instructions Unicorn cannot execute) over a 101-case matrix — every export, both returned values and side effects: 101/101 identical (return value, rendered log text, the full libc call trace with arguments, handle contents, and read/write buffers). Eight single-line mutants of the rebuild (wrong `errno` constant, off-by-one argument, `timeout_ms` used as seconds, a `free` added to an error path, a propagated `pthread_cond_signal` rc, `calloc` instead of `malloc`+clear, `-1` instead of `-errno`, and a wrong `osal_mem_cpy` length) are each caught by 1-16 of the same cases, and one of them is the argument-shift bug the matrix found in the first draft of this rebuild. Relocation view: the interposable (`R_ARM_JUMP_SLOT` naming a symbol defined in the image) edge set is `__aeabi_memcpy`, `__aeabi_memset`, `__aeabi_memclr`, one call site each — identical in the blob and the rebuild. |

Notes to file with the row:

* `libgcs-osal` is 32-bit only, matching the blob (`vendor/lib`, no `vendor/lib64`
  counterpart), and is already in `PRODUCT_PACKAGES` (`vayu-vendor.mk`), so no
  build-system change is needed beyond adding the module.
* The blob is built with `_FORTIFY_SOURCE`: it calls `__open_2`, `__read_chk`
  and `__write_chk` rather than `open`/`read`/`write`, and every read/write call
  site loads a buffer-size argument of `-1` (`mov.w r3, #0xffffffff`, 0x2a68 and
  0x2ac4), which disables the runtime check. The reimplementation calls the same
  three entry points with the same arguments so the import set and the
  (never-taken) abort path match exactly.
* Three call sites delegate deliberately, and the reimplementation keeps that:
  `osal_lock_destroy` returns the negated `pthread_mutex_destroy` rc without
  freeing the box (the blob leaks it too), `osal_cond_destroy` logs a failing
  `pthread_cond_destroy` but returns the *mutex* result, and `osal_cond_signal`
  logs a failing `pthread_cond_signal` and still returns 0.
* `osal_mem_cpy` takes four register arguments and ignores the second: the blob
  copies `(arg1, arg3, arg4)` - both in-tree call sites fill all four (e.g.
  `libgcs.so` 0x3824 passes `(dst, 12, src, 12)`), so the dead parameter is kept
  for ABI compatibility and documented as such rather than removed.
* `osal_thread_create` takes seven arguments and ignores 2-5; `osal_cond_timedwait`
  adds `timeout_ms / 1000` to `CLOCK_MONOTONIC`'s `tv_sec` and leaves `tv_nsec`
  untouched, so sub-second timeouts do not reach the deadline they name.
* The reimplementation is compiled with `-fvisibility=hidden`, as the other
  modules here are; the 32 exports carry explicit `visibility("default")`, and
  the interposable-edge set above was measured on a build with the production
  flags, not a looser scratch build.
