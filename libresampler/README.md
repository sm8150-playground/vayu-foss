# vayu open-source vendor libraries — `libresampler`

Section for `vendor/qcom/opensource/libraries/README.md`, in that file's format.

## Libraries

| Library | Replaces | Source group | ABI surface | How it was reverse engineered | Verification |
| ------- | -------- | ------------ | ----------- | ----------------------------- | ------------ |
| `libresampler` | `vendor/lib/libresampler.so` (0x1f04 = 7940 bytes of `.text`, 32-bit ARM, Thumb-2, 55 exports) | "Mi Sound FX" — the blob's only strings are its log tag `Mi_Resampler` and three messages (`process`, `CharToInt size is %d bw_in is %d`, `IntToChar size is %d bw_out is %d`); it is a rate converter with a biquad-cascade low-pass, a look-ahead limiter and a phase-accumulator resampler | the C entry points `mi_resampler_init` / `mi_resampler_process` / `mi_resampler_delete` / `mi_resampler_CharToInt` / `mi_resampler_IntToChar`; `Biquad_New`, `Biquad_Process`, `Biquad_Process_Fix`, `Biquad_Process_ScaleB0PreMult`, `Biquad_ResetHistory`, `Biquad_SetCoefsDirectly`, `Biquad_CalcCoeff`; `IirSos_New`, `IirSos_Process`, `IirSos_Process_Buffer`, `IirSos_Process_Buffer_ScaleB0PreMult`, `IirSos_SetFilterCoefs`, `IirSos_SetFilterScales`, `IirSos_ResetHistory`, `IirSos_Delete`, `_Z20SosIir_Delete_ScalesP20CMusicIirSos_State_t`, `_Z21SosIir_Delete_FiltersP20CMusicIirSos_State_t`; `CMusicLimiter_Init`, `CMusicLimiter_ProcessSample`, `CMusicLimiter_ProcessBuffer`, `CMusicLimiter_ProcessBufferHardLimit`, `CMusicLimiter_Reset`, `CMusicLimiter_Delete`; `CMusicResampler_Init`, `CMusicResampler_UpSample`, `CMusicResampler_UpSample1`, `CMusicResampler_DownSample`, `CMusicResampler_Reset`, `CMusicResampler_Delete`, `_Z22CMusicResampler_Interpfffff`; `CMusicIntFloat_Init`, `CMusicIntFloat_Add`; and the C++ members `_ZN12Mi_Resampler{C1,C2,D1,D2}*`, `_ZN12Mi_Resampler7processEPfS0_Pjj`, `_ZN12Mi_Resampler9getLenOutE{v,j}`, `_ZN12Mi_Resampler9setEnableEi`, `_ZN12Mi_Resampler29CMiSoundFX_Resampler_LPF_InitEv`, `_ZN15Mi_Resampler_Fx{C1,C2,D1,D2}*`, `_ZN15Mi_Resampler_Fx7processEPPiS1_Pjj`, `_ZN15Mi_Resampler_Fx9CharToIntEPciPi`, `_ZN15Mi_Resampler_Fx9IntToCharEPiiPc`, `_ZN15Mi_Resampler_Fx9setEnableEi`, `_ZN15Mi_Resampler_Fx11getFrameLenEv` | Thumb-2 disassembly (`llvm-objdump --triple=thumbv7-none-eabi --mattr=+hwdiv`; without `+hwdiv` 68 entries decode as `<unknown>`, 7 of them the `sdiv` the modulo loops use, 4 the ARMv8 `VCVTP.S32.F32`/`VRINTP.F32` pair and 57 literal-pool data words) plus a freestanding harness that relinks the blob's PT_LOAD segments into a static binary and runs the blob's own code under `qemu-arm` with the libc/libm surface stubbed, so every scenario is driven through the blob itself rather than through a decompiler's guess | Bit-exact differential over 11 rate pairs × 4 blocks of 256 samples (17 188 output samples per object in each of the two output domains, max per-sample difference 0), plus eight further scenarios covering every export, 9 behaviour-changing mutants that all diverge and 6 equivalent mutants that all stay identical, relocation reconciliation (48 = 48) and an interposable edge-set comparison (28 self-defined `.plt` symbols in both objects) |

## Notes

* **The C entry points put the handle in the *last* argument slot.**  The
  exported wrappers are argument shuffles: `mi_resampler_process` at `0x3036`
  moves its own `r3` into the callee's only stack slot and its 5th argument into
  `r0`, so the real signature is
  `mi_resampler_process(int **out, int **in, unsigned *frame_len, unsigned n, void *handle)`
  — the callee receives `(handle, out, in, frame_len, n)`.  `CharToInt`,
  `IntToChar` and `delete` follow the same convention.  Declaring the handle
  first compiles, links and then faults inside the blob on the first call
  (`this` becomes the sample count), which is what a previous pass of this
  library was chasing when it stopped.
* **`mi_resampler_init`'s second and fifth arguments are floats that the blob
  truncates toward zero** (`vcvt.s32.f32`, stored at `+0x14`/`+0x18`), while the
  fourth is a plain `int` stored verbatim at `+0x08`; the mangled constructor
  name `_ZN15Mi_Resampler_FxC1Eiffiff` fixes the order as
  `(int, float, float, int, float, float)`.  A caller compiled against the
  vendor header passes `16.0f`, not `16`, for the bit widths — the reimplementation
  takes `float bw_in, int32_t f08, float bw_out` so the conversion matches.
  The `+0x08` field is written and never read anywhere in the image.
* **A32 `VMLA`/`VMLS` are not fused.**  The architecture defines them as
  `product = FP_Multiply(Sn,Sm); Sd = FP_Add(Sd, product)`, and the image uses them 28 times (23 `VMLA` - one of them `.f64` - plus 5 `VMLS`) while using `VFMA` zero times.  Emulating `a*b+c` as a fused
  multiply-add (`__builtin_fmaf`, or clang's default contraction) therefore
  changes the result: measured under the same emulator,
  `vmla.f32(a*b+c)` returns 0 where `vfma.f32` returns 5.96e-08.  The module is
  built with `-ffp-contract=off` and contains no fused builtins.
* **`CMusicResampler_DownSample` and `CMusicResampler_UpSample1` are one
  routine** (identical bodies) that primes two samples past the end of the
  output block: `out[out_len]` when the phase is zero *before* the advance, and
  `out[out_len + 1]` when the post-advance phase is exactly zero — which is the
  case for every integer ratio.  `CMusicResampler_UpSample` then reads
  `out[out_size - 2]` / `out[out_size - 1]` into `o34`/`o38` and frees the
  block, so those two priming writes are what the *next* call's first two
  outputs are built from.  Writing the second one anywhere else (a previous
  pass wrote it into `s->hist`, where the trailing `memcpy` immediately
  overwrote it) leaves `o38` at zero and only shows up in the following block.
* The output length is `(int)ceilf(inv_ratio2 * n)`, computed with
  `VCVTP.S32.F32` (round toward +∞) followed by `VRINTP.F32` + `VCVT.S32.F32`;
  `out_size` is that value plus two.  Truncating instead of rounding up gives a
  different sample count for every non-integer ratio.
* **Every exported definition is marked `noinline`.**  The vendor build keeps
  all 28 intra-library calls interposable (each goes through a `.plt` entry),
  while clang inlines calls to same-translation-unit definitions; without the
  attribute the rebuild routes only 6 of the 28 through the `.plt` and a
  consumer that interposes e.g. `Biquad_Process` would be ignored where the blob
  honours it.  With it, both objects call exactly the same 28 exported symbols
  through their `.plt`.
* The blob's libc++ dependency is dropped: `operator new[]`/`new`/`delete`
  become `malloc`/`free`, so the module builds with `stl: "none"` and links only
  `liblog` and `libm`.
* `Biquad_CalcCoeff` has no guard for a design whose cutoff exceeds Nyquist:
  with `type=5, fc=20000, fs=8000, gain=-12, q=0.5` both the blob and the
  reimplementation fault at the same instruction with a byte-identical 262 024
  byte prefix.  That is reproduced, not fixed — a drop-in replacement has to
  fault where the blob faults.
