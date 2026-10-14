# libsns_fastRPC_util

This is the `vendor/qcom/opensource/libraries/README.md` section for the
`libsns_fastRPC_util` entry, in the reference format.

## Libraries

| Library | Replaces | Source group | ABI surface | How it was reverse engineered | Verification |
| ------- | -------- | ------------ | ----------- | ----------------------------- | ------------ |
| `libsns_fastRPC_util` | `vendor/lib64/libsns_fastRPC_util.so` | vayu (`device/xiaomi/vayu/proprietary-files.txt`, Camera section; sha1 `4b65d8aa6f0cf869a2621c423728e7328660b725`) | `get_lib`, `sns_rpc_get_handle`, `sns_rpc_reset` | AArch64 blob, 1160 bytes of `.text`, depended on by `sensors.ssc.so` (which resolves `sns_rpc_get_handle` from `direct_channel::init_fastRPC_symbols()` and `sns_rpc_reset`), `libsns_device_mode_stub.so` and `libsns_low_lat_stream_stub.so`. `.rodata` supplied every tag, format and name; `get_lib` is a `stat("/sys/kernel/boot_slpi")` selecting `"libsdsprpc.so"` over `"libadsprpc.so"` (`csel` at 0x1070). `sns_rpc_get_handle` is a mutex-guarded lazy loader: `dlopen(.., RTLD_NOW)`, eight `dlsym` calls into a 64-byte `.bss` table at 0x3268 in a fixed order (`remote_handle_open`, `_close`, `_invoke`, `remote_register_buf`, `remote_register_buf_attr`, `rpcmem_alloc`, `rpcmem_free`, `rpcmem_to_fd`), then NULL checks on six of them — `remote_register_buf`/`_attr` are deliberately unchecked. The handle word at 0x32a8 is both `remote_handle_open`'s output and the "already opened" flag: the blob stores a literal `1` into it on success (0x1398 `mov w8, #1`), so `sns_rpc_reset` always closes handle 1, which the reconstruction preserves. `sns_rpc_reset` closes the handle, `dlclose`s and logs through five distinct calls. | Differential test against the blob under the Unicorn AArch64 harness: 14 scenarios (stat present/absent, `dlopen` failure, each of the six checked and both unchecked `dlsym` slots NULL, `remote_handle_open` failure, double `sns_rpc_get_handle`, reset-before-init, handle re-open, stub returning a non-1 handle) — **14/14 identical**, comparing return values, the 64-byte output table and the ordered trace of every `stat`/`dlopen`/`dlsym`/`dlclose`/`pthread_mutex_*`/`__android_log_print`/`remote_handle_open`/`remote_handle_close` call with all arguments. `.rodata` string set is identical (27/27). Compiles warning-free with `-Wall -Wextra` against the tree's real bionic headers and links against the real aarch64 `libc.so` with `SONAME=libsns_fastRPC_util.so` and exactly the three exported symbols. |

## Notes

* `libsns_fastRPC_util` is 64-bit only, matching the blob (`vendor/lib64`, no
  `vendor/lib` counterpart). `sensors.ssc.so` and the two `libsns_*_stub.so`
  libraries carry `libsns_fastRPC_util.so` as a `DT_NEEDED` entry, so it has to
  stay in `PRODUCT_PACKAGES` when the blob entry is dropped.
* The blob lists `libutils`, `libcutils`, `libc++` and `libm` as `DT_NEEDED`
  but imports no symbol from any of them; the reimplementation only needs
  `liblog`, `libdl` (and implicitly `libc`).
* The blob stores the literal `1` into the variable it just received the real
  handle in, so the handle passed to `remote_handle_close()` is always `1` and
  not whatever the stub returned. That is a bug in the original, but it is
  reproduced verbatim: a consumer-visible difference here would change which
  remote handle gets closed.
* Only the first two entries of the returned table are ever called by this
  library, so their signatures are the ones recovered from the blob. The other
  six are resolved and handed out untouched — their prototypes cannot be
  recovered from this blob and the header documents them as the upstream
  libadsprpc declarations.
