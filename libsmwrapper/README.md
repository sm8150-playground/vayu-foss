# libsmwrapper

Open-source reimplementation of the proprietary `vendor/lib/libsmwrapper.so`
shipped in the vayu vendor image. The library keeps the ABI of the blob it
replaces — soname, exported symbols, signatures and observable behaviour — so it
can be dropped in without touching its consumer.

The sound trigger HAL that consumes it is
`vendor/lib/hw/sound_trigger.primary.msmnile.so`, which loads the library named
by `device/xiaomi/vayu/configs/audio/sound_trigger_platform_info.xml`
(`<param library="libsmwrapper.so" />` under the QTI SVA sound model entry).
The blob is the glue between the HAL and a sound model: it expands a compact
per-phrase description into a recognition event, and it flattens the per-user
keyword confidence levels of a recognition configuration into a "user id
tracker" byte array.

## Library

| Library | Replaces | Source group | ABI surface | How it was reverse engineered | Verification |
| ------- | -------- | ------------ | ----------- | ----------------------------- | ------------ |
| `libsmwrapper` | `vendor/lib/libsmwrapper.so` (sha1 `af37e8bd00fd4130094349c1d94e83c5b0c20550`, 32-bit only — the image has no `lib64` counterpart) | not recorded in this tree; `device/xiaomi/vayu/proprietary-files.txt` lists the blob with that sha1 and no group tag | 13 dynamic symbols: `generate_sound_trigger_phrase_recognition_event`, `generate_sound_trigger_phrase_recognition_event_v2`, `generate_sound_trigger_recognition_config_payload`, `generate_sound_trigger_recognition_config_payload_v2`, and the nine `__aeabi_memcpy{,4,8}` / `__aeabi_memset{,4,8}` / `__aeabi_memclr{,4,8}` runtime helpers (one address each per family) | 1340 bytes of `.text`, Thumb-2 with ARM-mode stubs. `llvm-nm -D` gives the exports and `llvm-readelf -r` the GOT/PLT mapping; `llvm-objdump --triple=thumbv7` recovers the four functions; every `ldr rX,[pc,#imm]` + `add rX, pc` pair resolves into `.rodata` and yields the log tag, priorities and all eleven format strings; the object layouts come from the offsets the code addresses (asserted with `_Static_assert` in the source) and are corroborated by the vendor platform info (`max_ape_users="10"` matches the ten-entry user array). | Blob and reimplementation driven through 70 differential cases in a Unicorn ARM32 harness, comparing return values, out-parameters, the liblog trace, the `calloc`/`free` trace and the whole heap image byte for byte — 0 mismatches. Plus 17 mutated builds as negative controls, all detected. |

## Objects

The blob works on three objects. Offsets are the ones the disassembly actually
addresses; the reimplementation asserts every one of them.

```
struct sm_phrase                            /* 0x60 bytes, input and event */
    +0x00  uint32_t id                      copied verbatim into the event
    +0x04  uint32_t field4                  never read from the input; the event
                                            fills it from sm_handle.field34
    +0x08  uint32_t conf                    low byte becomes tracker[i]; the
                                            event replaces it with data[i]
    +0x0c  uint32_t num_users
    +0x10  struct sm_user users[10]         { uint32_t id; uint32_t level; }

struct sm_config
    +0x00  uint32_t reserved[3]
    +0x0c  uint32_t num_phrases
    +0x10  struct sm_phrase phrases[]

struct sm_handle
    +0x2c  uint32_t max_phrases             upper bound checked against num_phrases
    +0x34  uint32_t field34                 copied into every event phrase's +0x04

struct sm_event_header                      /* the generated event */
    +0x000 uint8_t  reserved[0x78]
    +0x078 uint32_t data_size               = the caller's payload size
    +0x07c uint32_t data_offset             = 0x448
    +0x080 uint32_t num_phrases             = sm_config.num_phrases
    +0x084 struct sm_phrase phrases[10]     SM_MAX_PHRASES
    +0x448 uint8_t  data[data_size]         the payload, copied from the caller
```

`0x10 + 10 * sizeof(struct sm_user) == 0x60` and `0x84 + 10 * 0x60 + 4 == 0x448`,
so the phrase stride and the payload offset agree with the array bounds the
vendor platform info declares (`max_ape_users="10"`).

## Entry points

All four return 0 on success, `-22` (`-EINVAL`) for bad input or an invalid
sound model, `-12` (`-ENOMEM`) when an allocation fails, and the two event entry
points additionally return `-19` (`-ENODEV`) when the event cannot be allocated.
The blob materialises these as `mvn #0x15`, `mvn #0x0b` and `mvn #0x12`; the
`.rodata` log call sites bracket every one of them.

* `int generate_sound_trigger_phrase_recognition_event(void *handle, void *config,
  const uint8_t *data, uint32_t data_size, void **event)`

  Logs `%s: Enter payload_size %d` at DEBUG *before* validating anything, so a
  rejected call still logs. Then, for each argument that is NULL, fails with
  `%s: Null params`. On success `calloc(1, data_size + 0x448)`s the event, fills
  the three header words, `memcpy`s the payload to `+0x448` and appends one
  phrase record per `config->num_phrases`: `id` copied from the configuration,
  `field4` from `handle->field34`, `conf` from `data[i]`, `num_users` from the
  configuration, and for each user the id from the configuration with the level
  taken from `data[id]`. Nothing is validated here — see the notes.

* `int generate_sound_trigger_phrase_recognition_event_v2(void *handle, void *config,
  const uint8_t *payload, uint32_t payload_size, void **event)`

  Rejects a NULL `payload` with `%s: Null params` and otherwise logs
  `%s: payload_size %d, minor_version = %d, num_active_models = %d` from the
  caller's `payload_size`, `payload[0]` and `payload[1]`, then tail-calls the
  entry point above with `payload + 2` and `payload[1]` as the data and its size,
  forwarding `event` unchanged (the blob's `b.w` goes through a veneer to the
  event entry point's PLT slot). The caller's `payload_size` is never used for
  anything but the log.

* `int generate_sound_trigger_recognition_config_payload(void *handle, void *config,
  void **tracker, uint32_t *tracker_size)`

  Rejects any NULL argument with `%s: ERROR. Invalid inputs`, then requires
  `0 < config->num_phrases <= handle->max_phrases` (`%s: ERROR. Invalid
  phrases %d!=%d`) and allocates a tracker of `num_phrases + sum(num_users)`
  bytes. `%s: ERROR. conf levels alloc failed` marks the allocation failure.
  The tracker is then filled by the shared helper and handed back with its size;
  a helper failure is reported as `%s: fill config payload failed, error %d`
  after the tracker is freed.

* `int generate_sound_trigger_recognition_config_payload_v2(void *handle, void *config,
  void **payload, uint32_t *payload_size)`

  Same validation and same tracker size, but wraps it in a `2 + 2 * total` byte
  object: byte 0 is set to 1, byte 1 to `total`, the tracker is written at `+2`,
  and the `total` bytes that follow it are set to 1. Returns that size in
  `*payload_size`.

The shared helper (`fill_sound_trigger_recognition_config_payload`,
`.text+0x1b60`) writes `tracker[i] = (uint8_t)config->phrases[i].conf` for every
phrase and `tracker[id] = min(level, 100)` for every user entry of every phrase,
rejecting with `%s: ERROR. Invalid params user id %d>%d` any id outside
`[num_phrases, tracker_size)` and with `%s: ERROR. Duplicate user id %d` any id
already used by an earlier entry, in both cases freeing its own scratch tracker
(allocated with `%s: failed to allocate user_id_tracker`) first.

## Notes

* The blob performs **no** validation in the event entry points and does not
  bound `num_phrases` against the ten phrase slots or `num_users` against the ten
  user slots; it writes straight past them into whatever follows. The
  reimplementation loops the same way (`ev_users12`, `ev_n11_overflow`,
  `cfg_users12` in the differential matrix cover both). That is preserved
  deliberately rather than "fixed": on this image the sound models come from the
  HAL, which sizes them from the platform info XML above.
* `data_size + 0x448` and `2 + 2 * total` are 32-bit additions that wrap, like
  the blob's `add.w`. Sizes large enough to wrap (a ~4 GiB payload, or ~2^31
  tracker bytes) are not exercised by the matrix — the harness heap cannot hold
  them.
* The nine `__aeabi_mem*` exports are the compiler's AAPCS runtime helpers, which
  happen to be defined and exported here. The blob keeps each family at one
  address, and its bodies are plain `memcpy`/`memset`:
  `mov r3,r1; mov r1,r2; mov r2,r3; b memset` for `__aeabi_memset` (the AAPCS
  `(dest, n, c)` argument order), `mov r2,r1; movs r1,#0; b memset` for
  `__aeabi_memclr`. The reimplementation reproduces the same bodies; the
  `no_builtin` attribute on them keeps clang from lowering the inner `memcpy()`
  call back to `__aeabi_memcpy`, which would be infinite recursion, and
  `target("arm")` keeps them ARM-mode stubs like the blob's.
* The blob also carries the usual compiler boilerplate: a `.fini_array` holding
  `__cxa_finalize(&__dso_handle)` (`.text+0x18a0`, reaching its PLT slot through
  the 0x1dc4 veneer) and a no-op thunk (`.text+0x18ae`), plus the corresponding
  `__cxa_finalize` import. A C reimplementation has no destructors to run, so it
  neither emits `.fini_array` nor imports `__cxa_finalize`; nothing observes the
  difference.
* `DT_NEEDED` in the blob is `liblog.so libcutils.so libc++.so libc.so libm.so
  libdl.so` (the vendor's blanket link line), but the only symbols it imports
  are `__android_log_print`, `calloc`, `free`, `memcpy`, `memset` and
  `__cxa_finalize`. The reimplementation links against liblog only.
* The verified object was built with:
  `clang --target=armv7a-linux-androideabi21 -O2 -Wall -Wextra
  -fvisibility=hidden -std=gnu11 -fPIC -shared -nostdlib -fuse-ld=lld`
  (no warnings). It contains no NEON, which is what makes it emulatable; a build
  that vectorises one of the copy loops is behaviourally identical but cannot be
  run under the harness (Unicorn does not implement NEON on this CPU model).
  See `VERIFICATION.md` for the full evidence and the list of branches that were
  not exercised.
