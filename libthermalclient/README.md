Section for `vendor/qcom/opensource/libraries/README.md`, in that file's format.

## Libraries

| Library | Replaces | Source group | ABI surface | How it was reverse engineered | Verification |
| ------- | -------- | ------------ | ----------- | ----------------------------- | ------------ |
| `libthermalclient` | `vendor/lib64/libthermalclient.so` and `vendor/lib/libthermalclient.so` | vayu vendor image (the blob carries no source tag; built for both ABI widths) | 14 exported symbols: `thermal_client_config_set`, `thermal_client_config_query`, `thermal_client_config_cleanup`, `thermal_client_request`, `thermal_bandwidth_client_request`, `thermal_bandwidth_client_cancel_request`, `thermal_client_register_callback`, `thermal_client_register_report_callback`, `thermal_client_unregister_callback`, the list helpers `add_to_list`, `remove_from_list`, `get_callback_node_from_list`, the transport helpers `send_to_fd`, `recv_from_fd`, and the object `list_head` | AArch64/AArch32 blob, 18664 bytes of `.text`, importing only `__android_log_print` (liblog) and libc. `.gnu_debugdata` (an xz-compressed ELF symtab left in the blob) names every static function, which is how the internal split was recovered. `.rodata` holds the four socket paths, the six client names, every log format and all three field-descriptor tables; `.data` holds the same tables as 32-byte `{name,type,flag,off,extra,add}` records. `llvm-objdump` decoded the record builders and the reply extractors, and the blob was then driven under a Unicorn AArch64 emulator with its sockets, malloc and libc string calls interposed, so exact request frames and the handling of each response/error case could be compared byte-for-byte against the reimplementation. | 55-case behavioural differential under an interposed IPC transport (identical traces), 11 behaviour mutants rejected, 5 semantics-preserving controls accepted, relocation accounting balanced on both ABI widths, and the interposable edge set reproduced exactly (6 targets, 16 edges). |

## What the library is

`libthermalclient` is a client of the legacy Qualcomm thermal-engine daemon
over its unix-domain sockets. It never touches the thermal HAL. Every request
is a fixed **15584-byte** (`0x3CE0`) record:

* `+0x000` `int` type (0 monitor, 1/2/3 pid/ss/tb, 7 step_wise/override, 8 framing)
* `+0x004` `char name[20]` client name
* `+0x018` `char desc[32]` — cooling-device name, request value, framing count or reply description, depending on the operation
* `+0x038` `int fields_mask` (config_set only)
* `+0x044` algorithm-specific payload (offsets and strides below)

Sockets (`.rodata`, all four names reproduced verbatim):

```
/dev/socket/thermal-send-client           requests + callback listening
/dev/socket/thermal-send-rule             report events
/dev/socket/thermal-recv-client           replies for override / config_set / config_query
/dev/socket/thermal-recv-passive-client   replies for spkr / camera_bw / display_bw
```

The six client names are `spkr`, `override`, `config_set`, `config_query`,
`camera_bw`, `display_bw`; the four accepted on a reply are `camera`,
`camcorder`, `spkr` and `config_query`. Algorithm ids are 0 `monitor`,
1 `pid`, 2 `ss`, 3 `tb`, 7 `step_wise`, 8 `all`; `check_for_algo_type`
(0x4320) accepts exactly ids `{0,1,2,3,7,8}` (mask `0x18F`) both ways.

**config_set (0x4890)** validates every instance first (non-NULL desc/algo/fields,
non-zero mask, every field name present in the algorithm's table, non-zero
`num_vals`), then sends a **type-8 framing record** carrying the instance count
in `desc`, one record per instance, and a terminating `type 8, count 0` record.
It returns the number of instances actually sent. The field-descriptor tables
are the blob's (`monitor` 6 fields, `pid/ss/tb` 5, `step_wise` 8):

| algorithm | field | type | flag | off | extra | strides |
| --------- | ----- | ---- | ---- | --- | ----- | ------- |
| monitor | disable, sampling | int | 1, 2 | 0x3C, 0x40 | | |
| monitor | thresholds, thresholds_clr | array | 4, 8 | 0x44 | 0, 4 | `0x48C` group stride, `0x24` slot |
| monitor | actions, action_info | array/limits | 0, 0x10 | 0x44 | 8 | `0x48C`, `0x24` |
| pid/ss/tb | disable, sampling | int | 1, 2 | 0x3C, 0x40 | | |
| pid/ss/tb | set_point, set_point_clr | int | 4, 8 | 0x44, 0x48 | | |
| pid/ss/tb | device | string | 0 | 0x4C | | |
| step_wise | disable, polling_delay, passive_delay | int | 1, 2, 0x40 | 0x3C, 0x3CD8, 0x3CDC | | |
| step_wise | set_temp, clr_temp | array | 4, 8 | 0x44 | 0, 4 | `0x50C`, `0x24` |
| step_wise | device | array | 0 | 0x44 | 8 | `0x50C`, `0x28` |
| step_wise | upper_limit, lower_limit | limits | 0x10, 0x20 | 0x44 | 8 | `0x50C`, `0x28` |

The payload builder `add_algo_specific_data_to_sock_data` (0x7168) switches on
the field type and, for therm arrays, on the **field name**: `set_temp` and
`clr_temp` take the `0x50C` stride with the `>10` slot bound (0x73f8), every
other name takes the `0x48C` stride unbounded (0x72e0). The bound is tested
*after* the store in both the array and the limit writers, so 12 values fill
12 slots and a 32-value limit block fills 32.

**config_query (0x3B04)** sends one `type = algo id` record with
`name = "config_query"`, waits for a `type 8` framing reply carrying the
instance count, then parses that many instance records. Every instance is
turned into a `config_instance` whose fields come from the same descriptor
tables, using per-field extractors: `add_int`, `add_string`, `add_thresholds`,
`add_thresholds_clr`, `add_actions` (names joined with `+`),
`add_action_info` (33 ints per group, count at `[32]`), `add_set_points`,
`add_set_points_clr`, `add_cdev_array` (device names joined with `+`),
`add_upper_limits` and `add_lower_limits` (33 ints per group). On any error it
frees what it built, returns 0 and sets `*insts = NULL`.

**thermal_client_request (0x37C0)** copies the name and value into the
`client_request` record and spawns `do_request_to_thermal` (0x3988), which
retries connect/send every 5 s. **thermal_bandwidth_client_request (0x3014)**
stores the name/value in `bw_request` and starts `do_bw_request_to_thermal`
(0x32B4); the worker builds its own 15584-byte stack record, memsets it, copies
the name and value out of the mutex-guarded `bw_request` and send()s it, then
`select()`s for either a cancel pipe (`bw_cancel` / `do_listen`'s rule path) or
the peer closing, in which case it logs "Server is crashed" and reconnects. A
second `thermal_bandwidth_client_request` while one is running pokes the pipe
with `"exit"` and returns 0 without restarting the worker.
**thermal_bandwidth_client_cancel_request** zeroes the stored value, sets
`bw_cancel`, pokes the pipe, joins the worker and clears the running flag.

Callbacks are kept in a singly-linked list rooted at the exported `list_head`,
with a 31-bit `registered_handle_mask` allocating handles 1..31.
`add_to_list` pushes front, `remove_from_list` validates the mask and unlinks,
`get_callback_node_from_list` scans by name (20-byte `strncmp`).
`thermal_client_register_callback` accepts `camcorder`, `spkr`, `config_set`
and `config_query`, and starts the `do_listen` thread on
`/dev/socket/thermal-send-client` if it is not already running.
`thermal_client_register_report_callback` registers under the name `report` and
starts a listener on `/dev/socket/thermal-send-rule`, whose framing is a single
bounded datagram delivered to the `report` callback.
`thermal_client_unregister_callback` removes the node and, when the list is
empty, sets `listen_stop`, pokes the pipe and joins the thread.

## Notes

* The library is **not** the HAL `thermal_client`; nothing in this image links
  it through a binder interface. Consumers resolve the plain symbols above, so
  the module keeps default visibility and `stl: "none"`.
* `-fsemantic-interposition` is deliberate (see `Android.bp`). The blob routes
  `send_to_fd`, `recv_from_fd`, `add_to_list`, `remove_from_list`,
  `get_callback_node_from_list` and `thermal_client_config_cleanup` through the
  PLT even though it defines them. Without the flag clang inlines most of the
  internal calls: measured, the set collapses from 6 targets / 16 interposable
  edges to 2 targets / 3 edges (the four list/transport helpers lose every call
  site, `send_to_fd` and `thermal_client_config_cleanup` keep only one each).
* The step_wise algorithm is selected by the **name** `step_wise` (id 7). The
  string `override` is *not* in the name table, so a config instance whose
  `algo_type` is `"override"` is rejected by `check_for_algo_type` before the
  override table is ever consulted — the blob's `set_override`-style request is
  invalid, and the differential records that (`RET -1`).
* `thermal_client_config_query` passes the **literal `0x14`** to `setsockopt`,
  i.e. `SO_RCVTIMEO` on this ABI, not `SO_SNDTIMEO` (`0x15`) as the call's
  shape suggests; the value is replicated as compiled. The 1-second `timeval`
  is on the stack.
* `do_bw_request_to_thermal` uses a private stack record, not `bw_request`
  itself, so the exported `bw_request` object keeps the caller's name and value
  (visible in the differential's `OUT BUFFER bw_request` dump).
* The limit and therm-array writers test their slot bound *after* storing
  (0x7018 / 0x742C: `cmp` then `str` then `bhi`), so `LIMIT_MAX = 30` yields 32
  stored slots and `STEPWISE_MAX = 10` yields 12; the reimplementation keeps
  the store-then-test order.
* `add_lower_limits_to_field` reads each value at `+4` within the slot
  (`MON_SLOT_INFO + 4`) while `add_upper_limits_to_field` reads at `+0`; both
  keep the blob's per-slot offset.
* `config_query`'s cleanup unconditionally frees `list[num].cfg_desc` and
  `list[num].algo_type` before delegating to `thermal_client_config_cleanup`;
  on the "one instance too many" path `num` is one past what it built, so the
  blob frees two out-of-range slots. The blob's harness cannot distinguish a
  `NULL` free from a dangling one, and a well-formed daemon never sends the
  extra record; the reimplementation reproduces the access.
* `config_cleanup` frees the instance array itself (`free(insts)`), matching
  the blob; callers must not free it again.

## Verification

Everything below was produced by the scripts in the scratch workspace
(`tblob.py` drives the original blob under Unicorn with its sockets, `malloc`,
`select`, `pthread_*` and libc string calls interposed; `tmain.c` is the same
program compiled against the reimplementation so both traces are byte-compared;
`relocacct.py`, `edges.py` and `mutant.py` are the structural and negative
instruments; `verify.py` runs them all behind the controls).

**Controls gate the run.** `verify.py` executes a positive control (the edge
checker must find the known `send_to_fd` edge in the blob) and a refusal (an
AArch32 object must produce **no** edge number, not a plausible zero) before any
measurement is printed, and exits non-zero if either fails. Both pass.

**Behavioural differential — `tblob.py` / `tmain.c` / `cmp.py`**

```
set_pid set_ss_neg set_tb set_monitor set_override set_two_partial
set_null_insts set_mask0 set_bad_algo set_bad_field set_bad_type
set_type_mismatch set_actions_only set_null_desc set_null_fields
set_connect_fail query_pid query_monitor query_all query_null_name
query_bad_name query_count0 query_count_too_many query_wrong_header_type
query_short_message query_bad_client_name query_unknown_client_name
query_fewer_instances query_bad_algo_type query_type_mismatch query_long_desc
query_recv_error query_extra_message request_ok request_config_set
request_null request_bad_name request_thread_fail bw_ok bw_zero bw_neg
bw_over bw_null bw_bad_name bw_cancel regcb_null regcb_bad_name unregcb_bad
listops helper_fd request_run bw_run listen_msg set_stepwise query_stepwise

55 tests identical, 0 differing (total 55)
```

Each case compares, in order: the interposed call trace (socket/connect/setsockopt/
sendto/recvfrom/close/pipe/read/write/select/pthread/`__android_log_print` with
arguments), the exact request frames (every non-zero word of the 15584-byte
record plus an FNV-1a hash), the returned `config_instance` trees from
`config_query` (desc, algo, mask, per-field values), memory-allocation counts,
and every return value. `set_stepwise` exercises the step_wise request geometry
and `query_stepwise` the five step_wise extractors, using a reply reconstructed
from the blob's own request frame so no hand-written layout can bias the test.

**Negative controls — `mutant.py`** (11 behaviour mutants, each a one- or
two-token edit, rebuilt and re-run; every one rejected)

```
b_stride_mon     0x48C -> 0x490 monitor group stride      rejected by set_monitor
b_stride_dev     0x50C -> 0x510 step_wise group stride    rejected (frame overflows and aborts)
b_off_desc       0x018 -> 0x01C description offset        rejected by set_pid
b_stepwise_max   10 -> 9 slot bound                       rejected by set_stepwise
b_storeval_order bound tested before the store            rejected by set_stepwise
b_cfgset_name    "config_set" -> "config_se"              rejected by set_pid
b_therm_name     "set_temp" -> "set_point" fast-path test rejected by set_stepwise
b_limit_order    limit bound tested before the store      rejected by query_stepwise
b_lower_off      lower limit slot +4 removed              rejected by query_stepwise
b_reply_name     "spkr" dropped from the reply names      rejected by listen_msg
b_report_limit   spkr level ceiling 150 -> 40             rejected by listen_msg
11 behaviour mutants, 5 controls; 0 unexpected results
```

The five controls (`c_comment`, `c_string_split`, `c_const_hex`,
`c_whitespace`, `c_rename`) are semantics-preserving rewrites and all leave the
trace byte-identical, so the suite neither passes mutants spuriously nor is
blind to real changes.

**Relocation accounting — `relocacct.py`** (AArch64 counts from
`Arm64Elf.reloc_counts`, AArch32 from an independent REL reader; every object
balances)

```
blob64   rela_printed=42  rela_parsed=42   (JUMP_SLOT 41, GLOB_DAT 1) + 73 RELR lines
build64  rela_printed=106 rela_parsed=106  (JUMP_SLOT 37, RELATIVE 68, GLOB_DAT 1)
blob32   rela_printed=45  rela_parsed=45   (JUMP_SLOT 43, GLOB_DAT 2)
build32  rela_printed=42  rela_parsed=42   (JUMP_SLOT 41, GLOB_DAT 1)
```

**Interposable edges — `edges.py <blob> <build>`** (set from relocations,
multiplicity from instructions; the tool path and raw line counts are printed
beside every number so a zero cannot be mistaken for a tool that did not run)

```
blob : AArch64, readelf -r lines 123, objdump -d lines 4958
       reloc view 6 targets ; text view 324 edges (16 interposable + 308 external)
       reconciliation: 41 stubs, 0 annotations; strict 365 = 324 + 41 ; loose 365
build: AArch64, readelf -r lines 112, objdump -d lines 4235
       reloc view 6 targets ; text view 304 edges (16 interposable + 288 external)
       reconciliation: 37 stubs, 58 annotations; strict 341 = 304 + 37 ; loose 399
target set: blob 6, build 6 ; only in blob none ; only in build none
text counts differing: 0/6 targets
```

Per-target interposable edges, identical on both sides:

```
add_to_list reloc=1 text=2   get_callback_node_from_list reloc=1 text=2
recv_from_fd reloc=1 text=3  remove_from_list reloc=1 text=2
send_to_fd reloc=1 text=6    thermal_client_config_cleanup reloc=1 text=1
```

The 58 offset annotations in the build (none in the blob) are the clang GOT-page
`<sym@plt+0x…>` references the current toolchain emits; they are counted
separately precisely because a mention-keyed count would inflate the build and
not the blob.

**Residual, stated rather than hidden.** The build has 37 PLT entries against
the blob's 41; the four extra imports are linker/toolchain artefacts, not code
the reimplementation omits: `__stack_chk_fail` and `__cxa_finalize` (the blob is
built with `-fstack-protector-strong` and its crt objects), and `__strlcpy_chk`
/ `__strlen_chk` (bionic fortify, which rewrites plain `strlcpy`/`strlen` when
the destination size is known). The blob's local
`snprintf/_ZL8snprintfPcU17pass_object_size1mPKcz`+`vsnprintf` pair comes from
the same fortify header and appears in no reimplementation path, so `vsnprintf`
is not imported. None of these is a symbol either object defines, so none can
change an interposable edge, and all of them remain available from libc.

**Not exercised, and why.** The differential drives `pthread_create` inline on
the same emulated thread, so the second thread's own arrival timing is not
observed: `do_listen`'s `select` loop is fed a scripted ready set and the
`do_bw_request` reconnect-after-peer-close branch is entered with a scripted
`select`. `do_request_to_thermal`'s 5-second retry loop and the `config_set`
validation failures are covered through their return values and log text, not
by sleeping. Everything reachable without a real second thread is compared
byte-for-byte.
