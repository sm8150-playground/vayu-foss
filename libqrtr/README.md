# libqrtr

Open-source reimplementation of the proprietary QRTR socket helper shipped in
the vayu vendor image, written to be dropped in without touching its consumers
(same soname, same 13 exported symbols, same signatures, same observable
behaviour). Reference-format entry for the `vayu open-source vendor libraries`
README — merge the table row into `## Libraries` and the bullets into
`## Notes`.

## Libraries

| Library | Replaces | Source group | ABI surface | How it was reverse engineered | Verification |
| ------- | -------- | ------------ | ----------- | ----------------------------- | ------------ |
| `libqrtr` | `vendor/lib64/libqrtr.so` | vayu vendor image (the blob carries no source tag) | `qrtr_open`, `qrtr_close`, `qrtr_sendto`, `qrtr_new_server`, `qrtr_remove_server`, `qrtr_new_lookup`, `qrtr_remove_lookup`, `qrtr_publish`, `qrtr_bye`, `qrtr_poll`, `qrtr_recv`, `qrtr_recvfrom`, `qrtr_decode` | AArch64 blob, 2284 bytes of .text, 13 exports, needed by five vendor libraries. `llvm-readelf` gave the PLT relocations, which name every libc call the library makes (socket, setsockopt, bind, getsockname, sendto, recvfrom, poll, close, fprintf, strerror, `__errno`, stderr) and thus pinned each socket operation to an exact PLT slot; `llvm-objdump` then decoded all of them. `.rodata` is 0xd0 bytes: the `{1,0}` timeval of `SO_RCVTIMEO`, the `{family=0, node=1}` literal that seeds the `bind()` address, and the eight `E|qrtr: ...` format strings, which fix every error path. `qrtr_open` is `socket(42, 2, 0)` + `setsockopt(fd, 1, 20, {1,0}, 16)` + an optional `bind()` to `{42, node=1, port=rport}`, closing the descriptor and returning -1 on any failure. `qrtr_sendto` builds the same 12 byte address from its arguments and calls `sendto(fd, buf, len, 0, &sq, 12)`, replacing a negative result with -1. The four publication entry points share one internal `getsockname(fd, &sq, &addrlen)` helper (0x12fc) that rejects a family other than 42 and an `addrlen` other than 12, then send a 20 byte `struct qrtr_ctrl_pkt` to `(sq_node, 0xfffffffe)`: the command words are the literal immediates 4, 5, 10 and 11 (`QRTR_TYPE_NEW_SERVER`, `DEL_SERVER`, `NEW_LOOKUP`, `DEL_LOOKUP`) and the instance word is `version \| instance << 8`; `NEW_LOOKUP` zeroes the packet's node/port words while `DEL_LOOKUP` fills them from the local address, and each returns the sendto result if negative, else 0 (`and w0,w0,w0,asr#31`). `qrtr_poll` polls one `pollfd` whose `events` is the immediate 0x9, `qrtr_recv`/`qrtr_recvfrom` are `recvfrom()` with and without the sender address, and `qrtr_decode` splits data from control datagrams (payload pointer aliased into the caller's buffer, control packets shorter than 20 bytes rejected with -EMSGSIZE) filling the 40 byte `struct qrtr_packet` whose offsets are read straight off its stores. | Blob and reimplementation are executed side by side in the same AArch64 Unicorn harness, under one driver that records every libc call, its arguments, the returned values, the contents of every output buffer and the stderr text: 76 scenarios covering all 13 entry points, every error path and 0x0/0xffffffff boundary values produce byte-identical logs (`test/diff_test.log`, `test/diff_test.py`). Mutation controls (poll events, control command, decoded instance field) each make the expected scenarios differ, so the comparison has teeth. |

## Notes

* The interface is socket-only: the module needs `libc` and nothing else. The
  blob's `DT_NEEDED` entries for `libc++.so`, `libm.so` and `libdl.so` are
  toolchain defaults — no C++, libm or libdl symbol is imported.
* `qrtr_publish` and `qrtr_bye` are real exported functions, not macros: in the
  blob they are 4 byte PLT tail calls to `qrtr_new_server` /
  `qrtr_remove_server`, and `qrtr_close` is a 4 byte PLT tail call to `close()`.
  The reimplementation keeps them as functions so symbol interposition behaves
  as before.
* `qrtr_poll` polls with `events = 0x9`. That is the blob's own immediate
  (`mov x9, #0x900000000`, one 64 bit store covering `events` and `revents`);
  it is written as `POLLIN | POLLERR` because that is what the value is,
  although `POLLERR` is normally a result bit rather than a request bit. The
  kernel reports it either way, so this is preserved rather than "fixed".
* `qrtr_remove_lookup` is not symmetric with `qrtr_new_lookup`: the DEL_LOOKUP
  packet carries the local node and port in its two spare words, while
  NEW_LOOKUP transmits them as zero. This was caught by the differential test
  after the first draft had both packets zeroed.
* `qrtr_decode` writes only the fields the message type carries and never
  clears the structure: for `QRTR_TYPE_DATA` it writes type/node/port/data/
  data_len, for `NEW_SERVER`/`DEL_SERVER` type/node/port/service/instance/
  version, for `DEL_CLIENT` and `BYE` only type/node (plus port for
  `DEL_CLIENT`), and for any other control command it writes a zero type and
  nothing else. `data` aliases the caller's receive buffer — there is no copy.
* The 40 byte `struct qrtr_packet` layout (offsets 0x00/0x04/0x08/0x0c/0x10/
  0x14/0x18/0x20) is part of the ABI for `qrtr_decode` callers and is
  reproduced, with the store each offset came from, in `include/libqrtr.h`.
* All stderr messages are the blob's `E|qrtr: ...` strings byte for byte, and
  every one of them goes through `strerror(errno)` and `fprintf(stderr, ...)`
  exactly like the blob (including `qrtr_recv` and `qrtr_sendto`, which log but
  still return the -1 that `sendto`/`recvfrom` produced).
* `qrtr_sendto`, `qrtr_open` and the receive paths zero the padding half of the
  12 byte `sockaddr_qrtr` they pass to the kernel, exactly as the blob does
  (`stp wzr, w1, [...]` before the family store); this is not observable through
  the socket API but is reproduced so the arguments are bit-identical.
* Verification report, commands and raw output: `VERIFICATION.md`.
