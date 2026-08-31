# decTest coverage against Boost.Decimal

What of the IBM/Cowlishaw General Decimal Arithmetic suite runs, what does not,
and why. Regenerate the numbers by building the drivers in `Jamfile`; each one
prints a per-file breakdown of verified and skipped cases.

## How a case is chosen

`test_harness.hpp` offers every test line to `decimal32_t`, `decimal64_t` and
`decimal128_t`, and `fits_format` in `dectest_parser.hpp` decides which of them
the line actually means something for:

* every literal on the line must land in the format unchanged, and
* where the file's context shaped the expected result, that context has to be
  the format's own. A value rounded at precision 9 says nothing about a 7-digit
  type; a `Clamped`, `Subnormal`, `Underflow` or `Overflow` result depends on
  the exponent range it was produced in; and a NaN the context produced because
  the answer did not fit says nothing about a wider format where the same
  operation succeeds.

Operations whose answer is defined by the format rather than by the operand
values pass `format_specific`, which demands an exact context match: one step of
`nextplus` is one ulp of the format, and `remaindernear` needs
`digits(quotient) + digits(y)` of precision to come out exact.

Skips are counted and printed by category, and `parse_error` must be zero.

## Covered

27 drivers, roughly 84,000 verified checks:

abs, add, subtract, multiply, divide, remainder, remaindernear, quantize,
compare, comparesig, comparetotal, max, min, minus, plus, fma, nextplus,
nextminus, nexttoward, logb, scaleb, samequantum, reduce, class, copysign,
tointegral, tointegralx, power, squareroot, toSci (`base`), apply (`clamp`).

## The ds files

The upstream suite has no `ds` arithmetic files at all: `decSingle` is a
storage-only format in decNumber, so it ships only `dsBase` and `dsEncode`. That
left `decimal32_t` with no test of its own rounding, overflow or subnormal
boundaries, which sit at precision 7 / Emax 96 / Emin -95 while every generic
file is written for a wider context.

`tools/generate_ds.py` authors them from the `dd` files, narrowing the operand
literals and recomputing every expected result and condition under a decimal32
context with Python's `decimal`. Because the answer is recomputed rather than
copied, a poor operand rewrite can only make a test less interesting, never
wrong. `--self-check` replays the pristine upstream files through the same
oracle and must stay at or above 99.9% (currently 99.956%).

Do not edit `dectest/ds*.decTest` by hand; change the generator and re-emit.

## Not reachable with the current library

No public API, so the archived files stay in `archive/`:

| Operation | Note |
|---|---|
| `and`, `or`, `xor`, `invert`, `shift`, `rotate` | no digit-wise logical operations (18 files) |
| `divideint` | `trunc(x / y)` rounds the quotient first, so it cannot reproduce `Division_impossible` |
| `maxmag`, `minmag`, `comparetotmag` | only `fmax`/`fmin`/`comparetotal` exist; a magnitude variant written in the test would be testing the test |
| `canonical`, `copy`, `copyabs`, `copynegate` | no user-facing spelling as quiet, non-arithmetic operations |
| `encode`, `decode` | `bid_conversion.hpp` and `dpd_conversion.hpp` exist but are not wired to dectest |
| `rescale` | `rescale(val, int)` saturates rather than signalling, and takes a digit count instead of a decimal |

Also unmodelled:

* **IEEE 754 status flags.** Nothing in `include/boost/decimal/` tracks
  `Inexact`, `Rounded`, `Invalid_operation`, `Division_by_zero`, `Overflow`,
  `Underflow`, `Subnormal` or `Clamped`. The harness parses the condition tokens
  and stores them, so a later flag model turns them on in one place, but today
  only values are asserted.
* **`half_down`, `up` and `05up` rounding.** Not IEEE 754 modes and absent from
  `rounding_mode`, so blocks under them are skipped and counted.
* **`extended: 0`.** The `dectest0/` files are legacy-arithmetic, which Python's
  `decimal` does not model either, so they stay hand-managed.

## Deliberate divergences

Marked inline as `# BOOST_DECIMAL_DIVERGENCE: <reason>` so the reason travels
with the line. The families:

* **NaN sign and signalling-ness.** IEEE 754 does not interpret the sign of a
  NaN, so Boost.Decimal neither propagates it nor quiets a signalling NaN
  through arithmetic, and it does not truncate payloads to the context
  precision.
* **No status flags.** A result the context would reject as
  `Invalid_operation` or `Division_impossible` overflows to infinity or
  underflows to zero instead. This covers an integer quotient too wide for the
  format, and a `scaleb` factor beyond `2 * (emax + precision)`.
* **Overflow yields infinity, not the clamped largest finite value**, under the
  rounding modes where decTest clamps.
* **decimal128_t `fma` is not fused.** There is no wider promoted type, so it
  rounds the product before adding; those cases differ by one ulp or overflow
  where a true fma would not.
* **`quax0a1`** states decTest's unclamped-context answer; the clamped-format
  answer is its `128#`-prefixed sibling `quax0a2`.

## Known defects

Marked `# BOOST_DECIMAL_KNOWN_BUG:`. Currently `dsadd71801` and `dsdiv3004`: a
directed rounding mode (`ceiling` here) drops the sticky bit when the discarded
part begins with several zeros, so the result rounds the wrong way. Reproduces
on `decimal32_t` and `decimal64_t`:

```
fesetround(rounding_mode::fe_dec_upward);
decimal32_t{1} / decimal32_t{999999}   // 1.000001e-06, should be 1.000002e-06
```

The same shape is what the note on `ddadd71801` in `ddAdd.decTest` suspected.
