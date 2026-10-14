# libmdmdetect

Section to append to `vendor/qcom/opensource/libraries/README.md` (table row in
the "Libraries" list, plus the note entries). It is kept here until the blob's
line is dropped and the library is moved into the tree.

| Library | Replaces | Source group | ABI surface | How it was reverse engineered | Verification |
| ------- | -------- | ------------ | ----------- | ----------------------------- | ------------ |
| `libmdmdetect` | `vendor/lib64/libmdmdetect.so` | LA.UM.9.1.r1-13700-SMxxx0.QSSI13.0 | `esoc_framework_supported`, `esoc_supported`, `get_soc_name`, `get_soc_link`, `get_soc_extra_link_info`, `get_soc_port`, `get_soc_ramdump_path`, `get_esoc_details`, `get_subsystem_info`, `get_system_info`, `get_peripheral_state`, and the 9-entry data object `supported_modems` | AArch64 blob, 5016 bytes of `.text`, needs only libc. It is pure sysfs: the four paths it ever touches are `.rodata` literals — `/sys/bus/esoc/devices` (0xda7), `/sys/bus/msm_subsys/devices` (0xf77), `/data/vendor/tombstones` (0xebf) and the `%s/%s/%s` builder (0x1085) — and every attribute read funnels through one helper at 0x21b8 (`open`, `memset`, `read(size-1)`, strip exactly one `\n`). The three `get_soc_*` accessors (0x2014, 0x2338, 0x2434) are the same `malloc`+`snprintf("<root>/<dev>/<attr>")`+read function three times, `get_soc_port` (0x25e0) takes the first non-`.` dirent of `<dev>/esoc-dev`, and `get_soc_ramdump_path` (0x2534) builds `/data/vendor/tombstones/<soc_name>/`. The two output structures were recovered from their addressing: `struct esoc_details` has stride 0x384 (`madd x8, x8, #0x384` at 0x2edc) with `status`, `soc_name[32]`, `link[32]`, `dev[255]`, `port[255]`, `ramdump[255]`, `esoc_name[32]`, `link_info[32]`, and `struct system_info` is 0x2a38 = 10808 bytes (`mov w2, #0x2a38` at 0x302c) holding `n_mdm`, `n_subsys`, `mdm[4]`, `subsys[8]`. Every formatting call goes through an outlined variadic helper at 0x2110 that hands `__vsnprintf_chk` a hard-coded maximum of 255 and the caller's size only as the fortify check. | Blob run under the AArch64 Unicorn harness against 24 virtual sysfs trees (present/absent/empty/malformed attributes, unreadable and unopenable trees, caps reached, targeted allocation failures, read() failures, and trees mutated between the two walks) side by side with the build of `mdmdetect.c`; all 200 recorded observations per fixture — return values, every returned string, the modified name buffers, the raw bytes of every 900-byte and 10808-byte output record, and all 212-384 liblog calls — are identical. 24 mutated builds were used as negative controls (23 diverge as expected; the one survivor is provably equivalent — the mutants are listed
in the verification report). |

## Notes

* `libmdmdetect` keeps no state, spawns nothing and only ever opens sysfs files
  and directories. `esoc_framework_supported()` returns **nonzero when
  `/sys/bus/esoc/devices` is absent**, which is the inverse of what its name
  suggests; that is the blob's behaviour (`cset w0, ne` at 0x28e4) and callers
  in this image rely on it.
* Two blob quirks are deliberately preserved because a caller can observe them.
  `get_peripheral_state()` tests for offline with a seven-byte compare rather
  than a string compare (an overlapping 4+4-byte compare of `OFFL`/`LINE` at
  0x324c-0x3278), so a state of `OFFLINEX` also reads as offline; and
  `get_subsystem_info()` leaves the caller's record completely untouched for an
  unrecognised name, returning 3, so `get_system_info()` drops those nodes
  without counting them.
* Three log sites cannot be reached through the public API and are therefore
  carried over untested: the two NULL guards inside the attribute reader (no
  caller in the library can pass a NULL path or a NULL buffer), and the
  `"Failed to get information on : %s"` abort in `get_system_info()`, which
  needs a `get_subsystem_info()` return of 1 and therefore a NULL argument. The
  `"Failed to locate registered ssr name for %s"` branch cannot be reached with
  a static sysfs tree either — the device it searches for is the very one that
  produced the record it is resolving — so it is covered only by serving the
  second walk from an empty listing.
* `get_system_info()` classifies an msm_subsys node by its `name` attribute with
  a five-byte prefix test (`mode` + `m`), but then hands that name to
  `get_subsystem_info()`, which requires it to be *exactly* `modem`. An SSR node
  named `modemx` therefore fails the whole call with
  `"Failed to get internal modem info"` and leaves the record it was filling
  untouched (verified against the blob: return 1, `n_mdm` unchanged, the record
  still holding the caller's fill pattern). The quirk is preserved.
* The blob leaks the `soc_name` strings of the non-matching ESOC devices it
  walks while resolving an `esoc*` device in `get_peripheral_state()`. The
  reimplementation frees each as it goes; this is not observable in any return
  value, output record or log line, and the transcripts are identical.
