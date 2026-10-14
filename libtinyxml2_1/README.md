# libtinyxml2_1

Open-source replacement for the proprietary `vendor/lib64/libtinyxml2_1.so`.
Unlike the other entries in this tree this one is not a reimplementation: the
library *is* TinyXML-2, so the module builds the upstream source and the work
is identifying the exact upstream revision and reproducing the blob's build
flags. Same soname, same 203 exported symbols, same classes, same signatures,
same ABI. Reference-format entry for the `vayu open-source vendor libraries`
README — merge the table row into `## Libraries` and the bullets into
`## Notes`.

## Libraries

| Library | Replaces | Source group | ABI surface | How it was reverse engineered | Verification |
| ------- | -------- | ------------ | ----------- | ----------------------------- | ------------ |
| `libtinyxml2_1` | `vendor/lib64/libtinyxml2_1.so` | upstream `leethomason/tinyxml2`, commit `0d2cef0` (2016-11-25) — master between tags `4.0.1` and `5.0.0`; zlib licence. No copy of that revision exists in this tree (see Notes) | AArch64, 203 exports (189 functions, 14 objects) importing nothing but libc and `operator new/delete`: the complete C++ API of TinyXML-2 at that vintage — `XMLDocument`, `XMLNode`, `XMLText`, `XMLComment`, `XMLDeclaration`, `XMLUnknown`, `XMLAttribute`, `XMLElement`, `XMLPrinter`, `_errorNames`, the nine `_ZTV*` vtables and the members of `StrPair`, `XMLUtil` and `MemPoolT<72\|96\|104\|112>`, which are not `TINYXML2_LIB`-annotated at this vintage and are therefore exportable only with default visibility | Not reverse engineering: `llvm-nm`/`llvm-readelf` on the blob give the exact export set, `.rodata` gives the 20-entry `XMLDocument::_errorNames` table, and the API shape of each symbol pins the upstream revision (no line-number parameter on `ParseDeep`/`ParseText`/`SetError`, accessors out of line, 20-member `XMLError`). The revision was then confirmed by building every one of the 138 upstream commits between tags `4.0.1` and `5.0.0` with clang-r547379 and diffing the resulting dynamic symbol table against the blob's: exactly six commits reproduce all 203 exports with no extras, `0d2cef0` being the newest | Symbol set compared symbol by symbol: 203/203 present with identical type/binding/visibility, 0 blob symbols missing, 0 module symbols extra, and every tinyxml2 symbol imported by `libhdr_tm.so` (24) and `libsdm-color.so` (23) is exported. Raw dumps and the diff: `VERIFICATION.md`, `evidence/`, `../verify.py` |

## Notes

* **The source is not in this tree, and every copy that is here is the wrong
  ABI.** Checked: `external/tinyxml2` (11.0.0, the only copy with a `.cpp`),
  `prebuilts/vndk/v31`, `v32` (8.0.0 headers), `prebuilts/vndk/v33`, `v34`
  (9.0.0 headers), `prebuilts/module_sdk/art/7` (9.0.0),
  `.../art/13` (10.0.0), `.../art/{17,20,22,current}` (11.0.0), and the VNDK
  ABI dumps `prebuilts/abi-dumps/vndk/{30,31,32,33,34}` (`libtinyxml2.so.lsdump`,
  7.0.1 through 9.0.0). Every one of them is 8.0.0 or newer and exports a
  different symbol set — 5.0.0 added the `int* curLineNumPtr` parameter to
  `ParseDeep`/`ParseText`/`SetError`, 9.0.0 added the 64-bit-unsigned
  accessors, and later revisions added exports the blob does not have
  (`XMLDocument::ClearError`, `DeepClone`, `ShallowClone`, typeinfo, …). The
  two matching files are therefore carried in this directory
  (`tinyxml2.cpp`, `include/tinyxml2.h`), taken from the upstream repository at
  commit `0d2cef0`; they are unmodified upstream code under the zlib licence
  (`LICENSE.txt`).
* **Version pinning.** The blob has 20 `XMLError` values ending at
  `XML_NO_TEXT_NODE` with the pre-5.0.0 spellings `XML_ERROR_ELEMENT_MISMATCH`
  and `XML_ERROR_IDENTIFYING_TAG` in `_errorNames`; it has no
  `int* curLineNumPtr` on any `ParseDeep`/`ParseText`/`SetError` (that landed
  upstream on 2016-11-29, before 5.0.0); and `IntText`, `IntAttribute`,
  `BoolText`, `DoubleAttribute`, … are out-of-line (moved out of the header on
  2016-10-11, still inline in 4.0.1). The window between those two upstream
  changes is 2016-10-11 … 2016-11-29. Building every commit in
  `4.0.1..5.0.0` and comparing export sets narrowed it to six commits
  (`c9a6102`, `13cbc9a`, `7de0b6d`, `ebb1660`, `f80d78d`, `0d2cef0`) that are
  symbol-identical to the blob; `XMLNode::ParseDeep` is the only function whose
  code differs between the two source variants inside that window, and the
  blob's copy of it (1084 bytes) sits at 1.067× the later variant's 1016 bytes
  against a median of 1.021× for the other 185 functions, while the earlier
  variant's 960 bytes is 1.129× — an outlier. `0d2cef0` is the newest of the
  six and is what the module vendors; all six are ABI-identical to the blob, so
  this is a tie-break, not a correctness boundary.
* **The build flags are part of the ABI, and one of them is a Soong default
  worth restating.** `-fno-rtti`: the blob defines no `_ZTI*`/`_ZTS*` at all
  (Soong already defaults to `-fno-rtti`; `rtti: false` is stated explicitly).
  `-fvisibility-inlines-hidden`: the blob exports no weak inline member and no
  template member, yet it does export the four `MemPoolT<*>` vtables and every
  out-of-line member — 92 symbols the source defines are absent from the blob
  for exactly this reason. `-fno-exceptions`: the blob imports no
  `__gxx_personality_v0`/`_Unwind_Resume`/`__cxa_begin_catch`. Fortification and
  stack protection were also on in the vendor build (`__strlen_chk`,
  `__vsnprintf_chk`, `__stack_chk_fail`); both are Soong defaults and are left
  to it.
* **Do not add `-fvisibility=hidden`,** which every other module in this
  directory uses. `StrPair`, `XMLUtil` and `MemPoolT<*>` are not
  `TINYXML2_LIB`-annotated at this vintage, so hiding them would drop 28 of the
  blob's 203 exports (the nine `StrPair` members, the fifteen `XMLUtil` members
  and the four `MemPoolT` vtables).
* **Consumers.** `vendor/lib64/libhdr_tm.so` resolves 24 tinyxml2 symbols from
  this library and `vendor/lib64/libsdm-color.so` 23, including
  `_ZTVN8tinyxml210XMLPrinterE` (the vtable) and
  `XMLPrinter::XMLPrinter(__sFILE*, bool, int)`; all of them are in the export
  set. The header installed by `export_include_dirs` is the same vintage, so a
  consumer rebuilding against the module gets the same declarations it was
  compiled against.
* **64-bit only.** The blob has no `vendor/lib` (32-bit) counterpart —
  `compile_multilib: "64"`. `libc++.so` is a real dependency
  (`operator new`/`delete` are imported from it), so the module is
  `stl: "c++_shared"` rather than `stl: "none"`.
* When the blob entry is dropped from `proprietary-files.txt`,
  `libtinyxml2_1` must be added to `PRODUCT_PACKAGES`, otherwise
  `libhdr_tm.so` and `libsdm-color.so` lose their dependency.
* Only the ABI was verified, not behaviour: no XML was parsed by either the
  blob or the module here. What is proven is in `VERIFICATION.md` — identical
  symbol sets, types, bindings and visibility, and a source revision whose API
  is exactly the blob's. The module's `.text` is 32512 bytes against the blob's
  36248 because clang-r547379 emits tighter code than the toolchain the vendor
  used (the blob carries an API 30 ident note and is fortified and
  stack-protected); code size is not part of the ABI.
* Verification report, commands and raw output: `VERIFICATION.md`.
