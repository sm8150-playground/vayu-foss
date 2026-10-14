# libsnsdiaglog

Open-source reimplementation of the proprietary blob shipped as
`vendor/lib64/libsnsdiaglog.so` in the vayu vendor image, kept ABI-identical
so it can be dropped in without touching its consumers.

| Library | Replaces | Source group | ABI surface | How it was reverse engineered | Verification |
| ------- | -------- | ------------ | ----------- | ----------------------------- | ------------ |
| `libsnsdiaglog` | `vendor/lib64/libsnsdiaglog.so` | vayu (`device/xiaomi/vayu/proprietary-files.txt`, Sensors section) | `sensors_diag_log::{sensors_diag_log,~sensors_diag_log, log_request_msg, log_event_msg, log_response_msg, submit_log_packet, sensors_diag_log_get_diag_log_id}` (C++ linkage, `std::__1::basic_string` arguments) plus the two weak data objects `_ZZN16sensors_timeutil12get_instanceEvE4inst` and `_ZGVZN16sensors_timeutil12get_instanceEvE4inst` | AArch64 blob, 5488 bytes of `.text`, needed by `sensors.ssc.so` (which constructs the object and calls the three `log_*` entry points). The wrapper schemas came from the generated serialisers in `libsnsapi.so` (`sns_diag_sensor_log` 0x1220a0, `sns_diag_client_api_log` 0x11f6cc, `sns_std_suid` 0xfe6c8, `sns_client_request_msg` 0x1185dc, `sns_client_event_msg` 0x119f40, `sns_diag_client_resp_msg` 0x11e6f8) and the `set_allocated_*` oneof accessors (0x120f98/0x121064/0x1210d4/0x121144/0x1211b4, 0x11eb48/0x11ec3c/0x11ece0); the payload round trip came from the generated parser's unknown-field path (`sns_std_suid::MergePartialFromCodedStream` 0xfe48c, in particular 0xfe510-0xfe628) | Blob run under a Unicorn AArch64 harness with the diag transport, the sensorslog policy and the protobuf message classes interposed, against 269 driven cases (payload shapes, gates, environments); the drop-in driven identically and compared on return values, the full interposed call trace, the bytes committed to diag, and the object/singleton state. 269/269 identical, 12/12 mutant builds diverge. See `VERIFICATION.md` |

## What the library does

Each entry point takes an already-serialised sensor protobuf payload, wraps it
in two diag wrapper messages, serialises the result and hands it to libdiag's
`log_alloc()`/`log_commit()` for transport to the SSC's diag log code. The
three entry points differ only in the payload they carry and the diag log code
they select:

| entry point | payload message | `sensors_diag_logid` | diag log code | gate probe |
| ----------- | --------------- | -------------------- | ------------- | ---------- |
| `log_request_msg`  | `sns_client_request_msg` (parsed from the first string) | 100 | 6617 | `log_status(6617)` |
| `log_response_msg` | `sns_diag_client_resp_msg` (built here from the `unsigned int` argument) | 101 | 6655 | `log_status(6655)` |
| `log_event_msg`    | `sns_client_event_msg` (parsed from the first string) | 102 | 6618 | `log_status(6618)` |

Each returns 3 when `Diag_LSM_Init()` failed at construction time, 1 when the
`logging_module_type` argument is null, 2 when `log_status()` reports the log
code disabled, 2 when `log_alloc()` returns null, and otherwise the value
`submit_log_packet()` returns (0).

## Wire format

The wrapper schemas, field numbers and write order, all read from the
`SerializeWithCachedSizes` bodies named above:

```proto
message sns_std_suid {                 // 0xfe6c8
  required fixed64 1;                  // suid_low
  required fixed64 2;                  // suid_high
}

message sns_diag_client_api_log {      // 0x11f6cc
  required fixed64 1;                  // CNTVCT snapshot taken by the constructor
  required string  2;                  // the caller's "pretty" string
  oneof client_api_log_payload {
    sns_client_request_msg  request = 100;   // log_request_msg
    sns_client_event_msg    event   = 101;   // log_event_msg
    sns_diag_client_resp_msg resp   = 102;   // log_response_msg
  }
}

message sns_diag_sensor_log {          // 0x1220a0
  required fixed32 1;                  // the sensors_diag_logid selector
  required fixed64 2;                  // CNTVCT_EL0 read at call time
  required sns_std_suid 3;             // fixed suid, low 0x1628e8697d485ecd
  required string  4;                  // the logging_module_type argument
  required fixed64 5;                  // the address of the sensors_diag_log object
  oneof sensor_log_payload {
    sns_diag_client_api_log client_api_payload = 102;
  }
}
```

Field 3's value is the 16-byte constant at `.rodata` 0x1830
(`cd5e487d69e82816 2b9bef82da6d4eb4`, i.e. low `0x1628e8697d485ecd`, high
`0xb44e6dda82ef9b2b`), loaded once per entry point (`ldr q0, [x9, #0x830]`).
Field 5 is the `sensors_diag_log *this` the function was called with: the blob
stores the raw register (`stur x20, [x29, #-0x60]` at 0x250c, `str x20,
[sp, #0xb0]` at 0x3330, and the same in `log_event_msg`), so the serialised
packet carries a process-local address. The packet is emitted as
`log_alloc(log_code, min(len, 12000) + 12)` with the payload `memcpy`'d to
`pkt + 12` and then `log_commit(pkt)`.

## The payload round trip

`log_request_msg` and `log_event_msg` run the caller's bytes through
`ParseFromString` into the matching message class and then serialise that
object again - the parse result is discarded (`bl ParseFromString` at 0x2290 /
0x2ba4 is not tested), but the *value* that gets wrapped is the re-serialised
form. The field tables (from the serialisers named above) are:

| message | known fields |
| ------- | ------------ |
| `sns_client_request_msg` | 1 message, 2 fixed32, 3 message, 4 message |
| `sns_client_event_msg`   | 1 message, 2 repeated message |

and `sns_diag_client_resp_msg` (single enum field 1) is never parsed - the
response path builds it here, sign-extending the `unsigned int` into a varint
exactly as `WriteEnum` does.

The round trip has three parts, each taken from the generated parser:

* fields the class knows are written in ascending field-number order, and a
  singular field is written once with the last value the parser kept;
* fields it does not know (or whose wire type does not match the field number)
  are re-encoded through `WireFormatLite::SkipField(input, tag, output)`, whose
  output stream writes into the message's unknown-field string - so the tag and
  the length are written canonically and only a length-delimited payload is
  copied across verbatim (0xfe614-0xfe628, with the `StringOutputStream` /
  `CodedOutputStream` pair built at 0xfe510/0xfe528);
* on a malformed byte the parser stops where it was, keeping everything it had
  already stored (`mov w19, wzr` / return false at 0xfe654).

Both the wrapper and this round trip are implemented here directly against the
wire format; there is no protobuf dependency.

## Logging

Every log site has the same shape: skip unless `sensors_log::get_level() >= 4`,
then `__android_log_print(2, sensors_log::get_tag(), fmt, func, line, ...)`
followed by the same message on stderr when `sensors_log::get_stderr_logging()`
says so. `sensors_log::get_level()` is evaluated once per site, not once per
call, and the three sites in `submit_log_packet` appear in that order
(0x2870, 0x292c, 0x29a4) - which is why it is not hoisted into a local. The
function names, line numbers and format strings are the blob's `.rodata`
(0x1840-0x1b3a); the stderr copy of each differs only by its own newline.

## ABI notes

* `sensors_diag_log_get_diag_log_id` is an ordinary member function, not a
  static one: the blob reads its selector out of `w1` (`cmp w1, #0x64` at
  0x274c), so `this` occupies `x0`. The mangled name cannot distinguish the two.
* `sensors_timeutil::get_instance()` is an inline helper whose body the blob
  emitted as a local block of `.text` (0x20c0) - it is not exported - but its
  function-local singleton and guard *are*: `_ZZN16sensors_timeutil12get_instanceEvE4inst`
  (0x38 bytes) and `_ZGVZN16sensors_timeutil12get_instanceEvE4inst` (8 bytes),
  both weak objects in `.bss`. `sensors.ssc.so` defines its own weak copies of
  both, so whichever library initialises the singleton first must fill it the
  way the blob does: `CNTFRQ_EL0`, `1000000000`, `elapsedRealtimeNano()`, the
  four words of the 0x1820 literal, `20`, and
  `elapsedRealtimeNano() - (1e9 / CNTFRQ_EL0) * CNTVCT_EL0` truncated, guarded
  by the *signed* subtraction overflow test the blob emits (`subs` + `b.vs` at
  0x215c - an unsigned borrow test is not the same thing, and the difference is
  observable when `fcvtzu` saturates).
* The exported strings are `std::__1::basic_string`, so the exported functions
  mangle with that type. This library is built with `stl: "none"` like the
  other open-source vendor libraries here and declares the 24-byte layout in
  `include/libsnsdiaglog.h` instead of pulling in libc++ - the file never
  constructs, copies or destroys a string through it, only reads `size()` and
  `data()`, which are exactly the accesses the blob itself inlines (0x2908,
  0x297c, 0x22ec).
* `sizeof(sensors_diag_log)` is 0x38 (`bool` at +0, padding, `CNTVCT_EL0` at
  +8, `pthread_mutex_t` at +0x10) because the consumers allocate it.

## Dependencies

The blob's `DT_NEEDED` list is `libprotobuf-cpp-lite-3.9.1, liblog, libdiag,
libsensorslog, libutils, libcutils, libssc, libsnsapi, libc++, libc, libm,
libdl` (note that this contradicts a "libc/liblog/libutils only" reading of the
task). The reimplementation keeps only what its observable behaviour needs:

| dependency | why |
| ---------- | --- |
| `libdiag` | `log_status`, `log_alloc`, `log_commit`, `Diag_LSM_Init`, `Diag_LSM_DeInit` |
| `libsensorslog` | `get_level`, `get_tag`, `get_stderr_logging` - the log policy |
| `libutils` | `android::elapsedRealtimeNano`, the same symbol the blob imports |
| `libc++_shared` | `__cxa_guard_acquire`/`__cxa_guard_release`, the guard protocol `sensors_timeutil::get_instance()`'s function-local singleton runs and libc does not provide. Linked only - no libc++ header or type is touched, hence `stl: "none"` |
| `liblog`, `libc` | `__android_log_print`, `fprintf`/`stderr`, `memcpy`, `malloc`/`free`, `abort`, pthreads |

`libprotobuf-cpp-lite-3.9.1`, `libsnsapi` and `libssc` are gone: the wire
format is written directly and the string type is declared rather than linked.
`libc++_shared` remains, but only for the two guard functions above - the same
reason the blob has `libc++` in its `DT_NEEDED`.

## Consumers

`sensors.ssc.so` is the only library in the image that references the exported
methods (it also constructs the object and destroys it):
`log_request_msg`, `log_event_msg`, `log_response_msg`, `C1`, `D1`. Nothing
calls `submit_log_packet` or `sensors_diag_log_get_diag_log_id` from outside.
