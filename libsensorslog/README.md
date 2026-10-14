# libsensorslog

Open-source reimplementation of the proprietary `vendor/lib64/libsensorslog.so`
from the Xiaomi vayu (Redmi Note 10 Pro) vendor image.

## Libraries

| Library | Replaces | Source group | ABI surface | How it was reverse engineered | Verification |
| ------- | -------- | ------------ | ----------- | ----------------------------- | ------------ |
| `libsensorslog` | `vendor/lib64/libsensorslog.so` | Xiaomi vayu vendor image (API 30, AArch64) | `_ZN11sensors_log9get_levelEv`, `_ZN11sensors_log9set_levelENS_9log_levelE`, `_ZN11sensors_log7get_tagEv`, `_ZN11sensors_log7set_tagEPKc`, `_ZN11sensors_log18get_stderr_loggingEv`, `_ZN11sensors_log18set_stderr_loggingEb` — i.e. `sensors_log::get_level()`, `set_level(log_level)`, `get_tag()`, `set_tag(const char*)`, `get_stderr_logging()`, `set_stderr_logging(bool)` | AArch64 blob, 96 bytes of `.text`, six exported accessors over three static variables. The whole `.text` decodes to six `adrp`+load/store pairs, so each accessor was read off the disassembly directly (`.rodata` holds the single string `"sensors"`, `.data` holds level `2` and the RELR-relocated pointer to that string, `.bss` holds the one-byte `false`). The mini-debuginfo payload of the blob named the three non-exported `.text` stubs as lld's `__on_dlclose`, `__on_dlclose_late` and `__emutls_unregister_key`, confirming the code before `0x1014` is toolchain glue and not part of the ABI. The `log_level` value domain was recovered from the six consumers, which gate `__android_log_print` on `get_level() >= N`: 1↔`ANDROID_LOG_ERROR`, 2↔`ANDROID_LOG_INFO`, 3↔`ANDROID_LOG_DEBUG`, 4↔`ANDROID_LOG_VERBOSE` over 524 call sites. | Blob and reimplementation run side by side under a Unicorn AArch64 harness (`.tmp/rw/libtool/emu64.py`) over 31 snapshots — every level/tag/stderr set, plus NULL tags, buffer mutation, repeats and out-of-range arguments — comparing return values, the raw bytes of all three storage slots and the pointed-to tag strings: 26 identical, 5 differing for one documented reason (the bool store mask, below). Ten mutated builds were run as negative controls and all ten were caught, so the matrix discriminates. Raw output in [VERIFICATION.md](VERIFICATION.md). |

## Decode

`llvm-readelf --dyn-syms` gives six `FUNC GLOBAL DEFAULT` symbols; the dynamic
symbol table holds nothing else (one undefined `__cxa_finalize@LIBC`), so the
three variables are internal. `llvm-objdump -d --section=.text` accounts for
every one of the 96 bytes:

```
0000000000001014 <_ZN11sensors_log9get_levelEv>:          # return level
    1014: adrp x8, 0x3000
    1018: ldr  w0, [x8, #0x1d8]           -> 0x31d8, 32-bit load
    101c: ret
0000000000001020 <_ZN11sensors_log9set_levelENS_9log_levelE>:
    1020: adrp x8, 0x3000
    1024: str  w0, [x8, #0x1d8]           -> stores the argument verbatim
    1028: ret
000000000000102c <_ZN11sensors_log7get_tagEv>:
    102c: adrp x8, 0x3000
    1030: ldr  x0, [x8, #0x1e0]           -> 0x31e0, pointer load
    1034: ret
0000000000001038 <_ZN11sensors_log7set_tagEPKc>:
    1038: adrp x8, 0x3000
    103c: str  x0, [x8, #0x1e0]           -> stores the pointer, not a copy
    1040: ret
0000000000001044 <_ZN11sensors_log18get_stderr_loggingEv>:
    1044: adrp x8, 0x3000
    1048: ldrb w0, [x8, #0x1e8]           -> 0x31e8, byte load
    104c: ret
0000000000001050 <_ZN11sensors_log18set_stderr_loggingEb>:
    1050: and  w8, w0, #0x1
    1054: adrp x9, 0x3000
    1058: strb w8, [x9, #0x1e8]
    105c: ret
```

No `ldar`/`stlr`, no barriers, no locks: the state is plain unsynchronised
memory, which is what the reimplementation uses.

### Static state and defaults

| Offset | Section | Content in the blob | Meaning |
| ------ | ------- | ------------------- | ------- |
| `0x31d8` | `.data` | `02 00 00 00` | `level = 2` |
| `0x31e0` | `.data` | `28 05 00 00 00 00 00 00` | `tag = 0x528`, made absolute by the `.relr.dyn` entry for `0x31e0`; `0x528` is `.rodata` = `"sensors"` |
| `0x31e8` | `.bss` | 1 byte, zero-filled | `stderr_logging = false` |

There is no `.init_array`, so nothing executes at load time; the only writers
are the two setters. `.fini_array` contains only lld's `__on_dlclose` (calls
`__cxa_finalize(__dso_handle)`) and `__on_dlclose_late` (a `ret`), which is
where the blob's single undefined import comes from; the reimplementation has
no registered destructors, so current lld emits neither stub, and the
difference is unobservable (there is nothing to finalise).

### What `sensors_log::log_level` means

The enum's identifiers are erased from the binary, but its *values* are not:
the consumers call `get_level()` and branch on it before printing, and the
threshold/priority pairs line up exactly (`llvm-objdump` over `libssc.so`,
`sensors.ssc.so`, `libsnsdiaglog.so`, `libsensorcal.so`):

| `get_level()` | liblog priority actually used | call sites |
| ------------- | ----------------------------- | ---------- |
| `>= 1` | `__android_log_print(6 = ANDROID_LOG_ERROR, ...)` | 300 |
| `>= 2` | `__android_log_print(4 = ANDROID_LOG_INFO, ...)` | 64 |
| `>= 3` | `__android_log_print(3 = ANDROID_LOG_DEBUG, ...)` | 96 |
| `>= 4` | `__android_log_print(2 = ANDROID_LOG_VERBOSE, ...)` | 64 |

No consumer ever compares against 0, and 4 is the highest value requested:
`libsensorcal.so` calls `set_tag("sensors_calibrate")`, `set_level(4)` and
`set_stderr_logging(true)` on its initialisation path, and `sensors.ssc.so`
calls `set_tag("sensors-hal")`. The header therefore names the values
`LOG_LEVEL_NONE/ERROR/INFO/DEBUG/VERBOSE` = 0..4, which is a labelling of the
recovered values, not a recovered declaration; the setter accepts any 32-bit
value unchanged (verified), so nothing else depends on the names.

## Consumers

Six files carry `libsensorslog.so` in `DT_NEEDED` (the library itself lives
only in `vendor/lib64`; there is no 32-bit build):

| Consumer | Symbols it resolves |
| -------- | ------------------- |
| `bin/sensors.qti` | `get_level`, `get_tag`, `set_tag`, `get_stderr_logging` |
| `lib64/libssc.so` | `get_level`, `get_tag`, `get_stderr_logging` |
| `lib64/libsnsdiaglog.so` | `get_level`, `get_tag`, `get_stderr_logging` |
| `lib64/libsensorcal.so` | all six |
| `lib64/sensors.ssc.so` | `get_level`, `set_level`, `get_tag`, `set_tag`, `get_stderr_logging` |
| `lib64/sensors.touch.so` | none — it merely depends on the soname |

`sensors.touch.so` is the reason the module must keep its soname even though it
resolves nothing against it: dropping the library outright would leave that
`DT_NEEDED` entry unsatisfiable and the library would fail to load. Grepping
the whole proprietary tree for `_ZN11sensors_log` finds exactly these five
symbol consumers plus the library itself, and finds no other caller of any
accessor.

## Divergence: the bool store mask

The blob's `set_stderr_logging` masks the value with `and w8, w0, #1` before
storing the byte; the reimplementation's `strb` stores the raw low byte,
because every runnable clang in this tree (r547379, r563880c, r584948,
r584948b, r596125; `clang-3289846` needs a `libtinfo.so.5` this host does not
have) folds that mask away for the equivalent source: a
`bool` parameter is guaranteed 0 or 1 by the AArch64 ABI, so the mask is a
no-op for any conforming caller. The blob was built by an older toolchain that
kept the `zext i1 -> i8` sequence; no source-level phrasing reproduces it
today (`x & 1`, `!!x`, `x == true`, ternary, `(bool)(unsigned char)x` and
one-bit bitfield variants were all tried; the bitfields also change the getter,
which the blob's plain `ldrb` rules out, and everything else folds).

Observable consequence, from the differential run: for arguments that are not
0/1 — i.e. for a caller that violates the C++/AAPCS `bool` contract — the blob
stores `value & 1` and the reimplementation stores `value & 0xFF`. Both agree
for 0 and 1, and for any value whose low bit equals the value (e.g. `0x100`
stores 0 in both). No consumer in the image passes anything but a literal
`bool`: grepping the whole proprietary tree for
`_ZN11sensors_log18set_stderr_loggingEb` finds exactly one other file,
`libsensorcal.so`, whose call site is `mov w0, #0x1` followed by the call. To make the
emulator comparison honest the out-of-contract inputs are part of the matrix
and are reported as the difference they are, rather than hidden — the same 5
snapshots also flag the "masks with `& 1`" mutant as a difference, which is
what that run would look like if the mask ever became expressible again.

## Files

```
Android.bp              cc_library_shared, vendor:true, stl:"none"
libsensorslog.cpp       the six accessors and the three static variables
include/sensors_log.h   declarations with default visibility (+ the enum)
verify.py               differential test driver (blob vs build)
mutants.py              ten mutated builds as negative controls
VERIFICATION.md         raw command output and results
verify_out.txt          raw differential run
mutants_out.txt         raw negative-control run
disasm_compare.txt      blob vs reimplementation disassembly
raw_build.txt           build + ELF inspection transcripts
raw_blob.txt            blob inspection transcripts
raw_consumers.txt       DT_NEEDED / symbol-usage scan over the blob tree
raw_boolcheck.txt       bool-store codegen across the tree's clang versions
```

## Notes

* The blob declares `DT_NEEDED` on `liblog.so`, `libc++.so`, `libc.so`,
  `libm.so` and `libdl.so`, but imports nothing from any of them — the only
  undefined symbol is `__cxa_finalize`, which comes from lld's `__on_dlclose`.
  The reimplementation is built with `stl: "none"` and has no undefined
  symbols at all. All six consumers (`libssc.so`, `sensors.ssc.so`,
  `libsnsdiaglog.so`, `libsensorcal.so`, `sensors.qti`, `thermal-engine`) carry
  their own `liblog`/`libc++`/`libc`/`libm`/`libdl` `DT_NEEDED` entries
  (checked with `llvm-readelf -d`), so nothing relies on this library to pull
  those in transitively and none of them is declared here.
* The blob exists only in `vendor/lib64` (no 32-bit counterpart), so the module
  is `compile_multilib: "64"`.
* The accessors are reproduced as ordinary non-atomic variables because the
  blob uses no `ldar`/`stlr` and no barriers. Any caller that expects
  cross-thread visibility must therefore do its own synchronisation, exactly
  as before.
* The logging entry points the sensors stack actually calls
  (`sensors_log::log_*`) live inline in the vendor's unavailable header; this
  library only supplies the configuration state and the tag.
