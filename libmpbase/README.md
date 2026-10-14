# libmpbase

This is the `vendor/qcom/opensource/libraries/README.md` section for the
`libmpbase` entry, in the reference format.

## Libraries

| Library | Replaces | Source group | ABI surface | How it was reverse engineered | Verification |
| ------- | -------- | ------------ | ----------- | ----------------------------- | ------------ |
| `libmpbase` | `vendor/lib64/libmpbase.so` | vayu (`device/xiaomi/vayu/proprietary-files.txt`, Camera section; sha1 `e6b51e2d501f434eea15ecdc275011d1498301ae`) | `MMemAlloc`, `MMemAllocStatic`, `MMemFree`, `MMemFreeStatic`, `MMemRealloc`, `MMemReallocStatic`, `MMemMgrCreate`, `MMemMgrDestroy`, `MMemCpy`, `MMemSet`, `MMemMove`, `MMemCmp`, `GetMaxAllocMemSize`, `Mpbase_GetVersion` (14, all C linkage, no C++ names) | AArch64 blob, 1008 bytes of `.text`, `DT_NEEDED libc.so` and no other import than `malloc`/`free`/`realloc`/`memcpy`/`memset`/`memmove`/`memcmp` — i.e. a self-contained first-fit pool allocator plus libc thins. `MMemMgrCreate` aligns the caller's buffer up to 8 and treats it as a chain of 16-byte `{uint64_t size; uint64_t used;}` headers terminated by an end marker of `size == -1`, with the manager returned in place of a separate control block (`and x0, x0, #0xfffffffffffffff8`, `sub x3, x0, #0x20`, `str x4, [x2, x1]` with `x4 = -1`); `MMemAllocStatic` is the first-fit walk that splits only when `size - request - 16 > 31` (signed `cmp x2, #0x1f; b.le`); `MMemFreeStatic` clears `used` and merges with the following block and then the preceding one, with the first block on its own `ptr == mgr + 16` path; `MMemReallocStatic` shrinks in place (carving only when the remainder `> 32`), else absorbs a free following block, else falls back to `MMemAllocStatic` + `memcpy` + free; `GetMaxAllocMemSize` reports the largest free block (`csel x2, x2, x1, ne`) and the literal 2 for a NULL manager; `Mpbase_GetVersion` fills a 56-byte `.data` record (four words 0, 1, 0, 4, then `"ArcSoft_Mpbase_0.1.0.4"`, `"Apr 24 2019"` and `"Copyright 2014-2015, ArcSoft Inc."` on three load-time-relocated pointers) and returns its address. Allocation sizes are 32-bit (`add w1, w1, #7; and w4, w1, #0xfffffff8`) while `MMemMgrCreate`'s size is a full 64-bit add (`add x0, x0, x1`), and `MMemCmp` sign-extends the `memcmp` result (`sxtw x0, w0`) — all three reproduced. | Blob and reimplementation both executed under the AArch64 Unicorn harness over 14 scenarios (hand-written boundary cases plus a 1600-operation randomised alloc/free/realloc sequence over four pool sizes), comparing every return value, the full pool image after each operation and the recorded libc calls: 14/14 scenarios, 5255 log entries, byte-identical, plus 27048 further entries from a boundary-biased stress run; 33 single- and two-token behavioural mutants are all caught and 2 equivalent controls survive. See `VERIFY.md` for the raw output, the branch-coverage and mutant tables and the two deliberate deviations. |

## Notes

* `libmpbase` is 64-bit only, matching the blob (`vendor/lib64`, no
  `vendor/lib` counterpart and no 32-bit dependent in the image). 23 libraries
  in the image carry it as a `DT_NEEDED` entry — 9 under
  `vendor/lib64/camera/components` (`com.arcsoft.node.*`,
  `com.mi.node.miaisuperlowlightraw`) and 14 in `vendor/lib64`
  (`libarcsoft_*`, `libdualcam_*`, `libarcdualcamsat`) — so it has to stay in
  `PRODUCT_PACKAGES` when the blob entry is dropped.
* The blob's `MMemAlloc`, `MMemFree` and `MMemRealloc` dispatch on their first
  argument: a non-NULL manager selects the pool allocator, a NULL one falls
  through to `malloc`/`free`/`realloc`. Only these three, `MMemMgrCreate`,
  `MMemMgrDestroy`, `MMemSet` and `MMemCpy` are called directly by the 23
  dependents (`Mpbase_GetVersion` is exported but nothing in the image calls
  it); the `*Static` variants are reached through the dispatchers (they are
  still exported and implemented, and the dispatchers are thin enough that
  this is what actually runs).
* The pool allocator performs no validation of the pointer handed to
  `MMemFreeStatic`/`MMemReallocStatic`: a pointer that is not a block payload
  makes the walk run off the end of the chain (it steps into the end marker
  and then, because the marker's size is `-1`, into whatever follows). That is
  a genuine defect of the original; it is reproduced rather than "fixed", and
  it is the one path with no dynamic test (see `VERIFY.md`).
* `MMemFreeStatic`, `MMemFree` and `MMemMgrDestroy` return `void`, so the value
  left in `x0` when they return is a dead register (the blob leaves an internal
  pointer or 0 there). The reimplementation's `x0` differs on 76 of the 85
  calls the tests make; no dependent observes it — all 5215 call sites to
  these three functions across the 23 dependent libraries were checked, and
  not one reads `x0` before overwriting it.
* `Mpbase_GetVersion` returns the address of a record inside the calling
  library, so the pointer itself necessarily differs between the blob and the
  replacement; the record it points at does not, and both return the same
  pointer on every call. The record is 56 bytes and the function itself only
  writes its first five words — the two trailing strings (`"Apr 24 2019"` and
  `"Copyright 2014-2015, ArcSoft Inc."`) come from the object's static image,
  so they were recovered from the `.rela.dyn` entries (three
  `R_AARCH64_RELATIVE` records at record offsets 0x20, 0x28 and 0x30) rather
  than from the disassembly, which never touches them.
* The module is built `-fno-semantic-interposition`, in addition to the usual
  `-fvisibility=hidden`. The blob's `.rela.plt` contains nothing but its seven
  libc imports, so it has no interposable intra-library edges: a consumer that
  defines `MMemAllocStatic`, `MMemFreeStatic` or `MMemReallocStatic` is not
  honoured, because the dispatchers reach them through direct branches. With
  semantic interposition (the default) the build gains one such edge
  (`MMemRealloc` → `MMemReallocStatic` through the PLT) — consumer-visible
  dispatcher behaviour that no behavioural test can detect, so it is pinned by
  the flag rather than left to the toolchain.
* `GetMaxAllocMemSize`'s NULL-manager result of `2` is reproduced verbatim
  even though it looks like an error code; nothing in the image checks it, but
  a consumer-visible difference here would be a gratuitous one.
