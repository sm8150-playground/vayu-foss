# vayu open-source vendor libraries

Open-source reimplementations of the proprietary shared libraries shipped in
the vayu vendor image. Each library keeps the ABI of the blob it replaces
(soname, exported symbols, signatures and observable behaviour) so it can be
dropped in without touching its consumers.

Every entry is written from static analysis of the blob — disassembly plus
emulation of the original code for behavioural ground truth — and is verified
bit-exactly against that ground truth before the blob is removed from
`proprietary-files.txt`. Libraries are done one at a time, smallest and
most self-contained first.

## Libraries

| Library | Replaces | Source group | ABI surface | How it was reverse engineered | Verification |
| ------- | -------- | ------------ | ----------- | ----------------------------- | ------------ |
| `libmdmimgload` | `vendor/lib64/libmdmimgload.so` | _(not recorded in the vayu tree)_ | `wait_for_com_port(const char *)`, `mdm_img_transfer(const char *, const char *, const char *, const char *, const char *, int)` plus the data object `sahara_map_arr` | AArch64 blob, 2096 bytes of `.text`, needed by `vendor/bin/mdm_helper`. `.rela.plt` mapped the PLT to `stat`/`usleep`/`strncmp`/`strlen`/`strerror`/`fork`/`execvp`/`waitpid`/`_exit`/`__android_log_print`/`errno`, and a single `R_AARCH64_GLOB_DAT` entry pinned the exported data object `sahara_map_arr` (`.data`, 0x42a8, 22240 bytes) as the table the selector walks. `wait_for_com_port` decoded as two `stat()` polling rounds (250 × 20 ms then 950 × 100 ms) with INFO/ERROR logging. `mdm_img_transfer` decoded as a row lookup — `strncmp(row->name, mdm, strlen(row->name))` plus `stat(row->path)`, advanced in 0x1160-byte strides until a row named `DEFAULT` — followed by the `/dev/mhi_%s%s` + `_pipe_2` device path, an argv build for `/vendor/bin/ks` and `fork()`/`execvp()`/`waitpid()` with the exit-status mapping 0 → 0, 11 → 2, other → 1. The five table rows (name, firmware directory, and the `-s <sahara id>:<file>` argument list, all NULL-terminated) and every log format string were read straight out of `.data`/`.rodata`, including the row argument lists the 149-entry RELR table relocates. | Reimplementation built as an AArch64 `.so` and executed next to the blob in the same Unicorn harness over 40 scenarios (both `stat()` rounds, all five rows, the `DEFAULT` fallback, prefix/short/empty names, non-`PCIe` link types, NULL arguments, device-path truncation at the 4095-byte formatter bound, all four `mode` values with and without `-g`, fork failure, execvp failure, and exit statuses 0/5/11/127/signal/256) comparing return value, all liblog calls with formatted arguments, every `stat()` and `usleep()` call, the `execvp()` argv and the child's `_exit()` code; plus a relocation-aware word-wise walk of all five `sahara_map_arr` rows in both objects (146 relocated pointer words compared by the string they point at, the remaining 2634 words compared byte-for-byte, no pointer heuristics). All 40 scenarios and 2780/2780 table words matched exactly. 16 mutants of the reimplementation were run as negative controls: the 15 behaviour-changing ones diverge from the blob in 1 to 31 scenarios, the remaining one is equivalent for this table (the `DEFAULT` terminator compare shortened from 7 to 3 bytes, which no row name can distinguish) |

## Notes

* `libmdmimgload` is `vendor/bin/mdm_helper`'s firmware-transfer back end (the
  only consumer in the image, `DT_NEEDED` and a `mdm_img_transfer` PLT call
  from two sites). Both call sites pass pointers into one 0x190-byte record of
  `mdm_helper`'s parsed device table — `mdm_img_transfer(record, record+0x50,
  port_or_NULL, record+0xa8, *(*(record+0x148)+0x10), mode)`, with `+0x50`
  holding the link type and the port argument NULLed when the string at
  `record+0x150` is `N/A`. `mode` comes from a global as 3 or 0 on the main
  path and is the literal 1 on the ESOC path.
* The blob links `libcutils`, `libutils`, `libc++`, `libc`, `libm` and `libdl`
  but resolves no symbol from any of them: every import comes from libc or
  liblog, so the production module depends on `liblog` only.
* The exported `sahara_map_arr` object is part of the ABI (the blob exports it
  with a dynamic size of 0x56e0), so the reimplementation defines it with the
  same five `{ name[32]; path[4096]; args[40]; }` records in `.data` rather
  than hiding the table inside the library.
* The `-s <id>:<file>` arguments really do carry the Sahara image id inside
  the string (`"21:.../sbl1.mbn"`, `"25:.../tz.mbn"`, `"34:.../mdmddr"`), and
  `mdm_img_transfer` appends a fixed `-a 9:mdmddr` pair; both are reproduced
  verbatim because they are passed straight to `ks`.
* Every argument the blob hands to `/vendor/bin/ks` is bounded by a hard
  64-entry argv array whose overrun path logs `No more room for execve
  arguments` and returns 0. No shipped table row can reach it (the longest row
  plus the fixed arguments needs 48 of the 64 slots), but the check and its
  log are reproduced, and the path was exercised by rewriting the table in the
  emulator.
