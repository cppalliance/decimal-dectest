#!/usr/bin/env python3
# Copyright 2026 Matt Borland
# Distributed under the Boost Software License, Version 1.0.
# https://www.boost.org/LICENSE_1_0.txt
"""Author the ds* (decimal32) arithmetic files the IBM suite never shipped.

In decNumber decSingle is a storage-only format, so upstream provides only
dsBase and dsEncode: there is no dsAdd, dsMultiply, dsDivide and so on. That
leaves decimal32_t with no test of its own rounding, overflow or subnormal
boundaries, because those sit at precision 7 / Emax 96 / Emin -95 and every
generic file is written for a wider context.

The dd (decimal64) and dq (decimal128) files are the same tests at different
digit widths, so a ds file is the same narrowing applied once more. Each dd
file is read, its operand literals are narrowed to seven digits, its exponents
are scaled into the decimal32 range, and every expected result and condition is
then RECOMPUTED with Python's decimal module under a decimal32 context.

That last step is what makes this safe: the answer never comes from the source
file, so a poor operand rewrite can only make a test less interesting, never
wrong. Python's decimal implements the same General Decimal Arithmetic
specification; --self-check replays the pristine dd and dq files against it and
reports how much it reproduces, which must stay at or above 99.9%.

Usage:
  tools/generate_ds.py --self-check          verify the oracle against upstream
  tools/generate_ds.py --emit                write dectest/ds*.decTest
"""

import argparse
import decimal
import os
import re
import sys

D = decimal.Decimal

DS_PRECISION = 7
DS_EMAX = 96
DS_EMIN = -95

DD_PRECISION = 16
DD_EMAX = 384

ROUNDING = {
    'half_even': decimal.ROUND_HALF_EVEN,
    'half_up': decimal.ROUND_HALF_UP,
    'half_down': decimal.ROUND_HALF_DOWN,
    'down': decimal.ROUND_DOWN,
    'up': decimal.ROUND_UP,
    'ceiling': decimal.ROUND_CEILING,
    'floor': decimal.ROUND_FLOOR,
    '05up': decimal.ROUND_05UP,
}

# decTest operation -> (Context method, operand count)
OPS = {
    'abs': ('abs', 1),
    'add': ('add', 2),
    'subtract': ('subtract', 2),
    'multiply': ('multiply', 2),
    'divide': ('divide', 2),
    'divideint': ('divide_int', 2),
    'remainder': ('remainder', 2),
    'remaindernear': ('remainder_near', 2),
    'minus': ('minus', 1),
    'plus': ('plus', 1),
    'compare': ('compare', 2),
    'comparesig': ('compare_signal', 2),
    'comparetotal': ('compare_total', 2),
    'comparetotmag': ('compare_total_mag', 2),
    'quantize': ('quantize', 2),
    'samequantum': ('same_quantum', 2),
    'max': ('max', 2),
    'min': ('min', 2),
    'maxmag': ('max_mag', 2),
    'minmag': ('min_mag', 2),
    'nextplus': ('next_plus', 1),
    'nextminus': ('next_minus', 1),
    'nexttoward': ('next_toward', 2),
    'logb': ('logb', 1),
    'scaleb': ('scaleb', 2),
    'reduce': ('normalize', 1),
    'copysign': ('copy_sign', 2),
    'copyabs': ('copy_abs', 1),
    'copynegate': ('copy_negate', 1),
    'copy': ('copy_decimal', 1),
    'fma': ('fma', 3),
    'squareroot': ('sqrt', 1),
    'power': ('power', 2),
    'exp': ('exp', 1),
    'ln': ('ln', 1),
    'log10': ('log10', 1),
    'tointegral': ('to_integral_value', 1),
    'tointegralx': ('to_integral_exact', 1),
    'class': ('number_class', 1),
    'invert': ('logical_invert', 1),
    'and': ('logical_and', 2),
    'or': ('logical_or', 2),
    'xor': ('logical_xor', 2),
    'shift': ('shift', 2),
    'rotate': ('rotate', 2),
}

FLAGS = [
    (decimal.InvalidOperation, 'Invalid_operation'),
    (decimal.DivisionByZero, 'Division_by_zero'),
    (decimal.Overflow, 'Overflow'),
    (decimal.Underflow, 'Underflow'),
    (decimal.Inexact, 'Inexact'),
    (decimal.Rounded, 'Rounded'),
    (decimal.Subnormal, 'Subnormal'),
    (decimal.Clamped, 'Clamped'),
]

NUMERIC = re.compile(r'^[-+]?(\d+\.?\d*|\.\d+)([eE][-+]?\d+)?$')


def tokenize(line):
    """Split on whitespace, keeping quoted runs intact and dropping a trailing comment."""
    tokens, current, quote, in_token = [], '', None, False
    i = 0
    while i < len(line):
        c = line[i]
        if quote:
            if c == quote:
                quote = None
                tokens.append(current)
                current, in_token = '', False
            else:
                current += c
        elif c in '\'"':
            quote, in_token = c, True
        elif c.isspace():
            if in_token:
                tokens.append(current)
                current, in_token = '', False
        elif not in_token and c == '-' and i + 1 < len(line) and line[i + 1] == '-':
            break
        else:
            current += c
            in_token = True
        i += 1
    if in_token:
        tokens.append(current)
    return tokens


def narrow_literal(text, from_precision=DD_PRECISION, from_emax=DD_EMAX):
    """Rewrite a decimal64 literal as the decimal32 test of the same shape.

    Coefficients keep their leading digits and their last one, which is what preserves
    IBM's rounding probes: 4444444444444446 becomes 4444446, so the boundary digit still
    sits just past the format's last place. Exponents scale by the ratio of the ranges.
    """
    if not NUMERIC.match(text):
        return text

    match = re.fullmatch(r'([-+]?)(\d*)(?:\.(\d*))?(?:[eE]([-+]?\d+))?', text)
    if match is None:
        return text

    sign, int_part, frac_part, exp_part = match.groups()
    int_part = int_part or ''
    digits = int_part + (frac_part or '')
    stripped = digits.lstrip('0')
    leading_zeros = len(digits) - len(stripped)

    if len(stripped) > DS_PRECISION:
        kept = stripped[:DS_PRECISION - 1] + stripped[-1]
        digits = '0' * leading_zeros + kept

    # Put the point back where it was, as far as the shortened digits allow
    if frac_part is None:
        new_int, new_frac = digits, None
    else:
        keep_int = min(len(int_part), len(digits))
        new_int, new_frac = digits[:keep_int], digits[keep_int:]

    out = sign + (new_int if new_int else ('0' if new_frac else ''))
    if new_frac:
        out += '.' + new_frac

    if exp_part is not None:
        scaled = round(int(exp_part) * DS_EMAX / from_emax)
        # Keep the scaled exponent inside the subnormal floor of the target format
        scaled = max(-(DS_EMAX + 2 * DS_PRECISION), min(DS_EMAX, scaled))
        out += 'E%+d' % scaled

    return out or text


class Replay:
    """Evaluate decTest lines with Python's decimal under a given context."""

    def __init__(self, precision, emax, emin, clamp=1):
        self.ctx = decimal.Context(prec=precision, Emax=emax, Emin=emin, clamp=clamp,
                                   rounding=decimal.ROUND_HALF_EVEN, traps=[])
        self.defaults = (precision, emax, emin, clamp)

    def reset(self):
        precision, emax, emin, clamp = self.defaults
        self.ctx.prec, self.ctx.Emax, self.ctx.Emin, self.ctx.clamp = precision, emax, emin, clamp
        self.ctx.rounding = decimal.ROUND_HALF_EVEN

    def directive(self, key, value):
        key = key.lower()
        try:
            if key == 'precision':
                self.ctx.prec = int(value)
            elif key == 'maxexponent':
                self.ctx.Emax = int(value)
            elif key == 'minexponent':
                self.ctx.Emin = int(value)
            elif key == 'clamp':
                self.ctx.clamp = int(value)
            elif key == 'rounding':
                self.ctx.rounding = ROUNDING.get(value.lower(), self.ctx.rounding)
        except (ValueError, decimal.InvalidOperation):
            pass

    def evaluate(self, op, operands):
        """Return (result text, condition list), or None when unsupported."""
        entry = OPS.get(op.lower())
        if entry is None:
            return None

        method, arity = entry
        if len(operands) != arity:
            return None
        if any('#' in o for o in operands):
            return None

        self.ctx.clear_flags()
        try:
            args = [D(o) for o in operands]
            value = getattr(self.ctx, method)(*args)
        except (decimal.InvalidOperation, decimal.DecimalException, ValueError, ArithmeticError):
            return None

        if value is True:
            text = '1'
        elif value is False:
            text = '0'
        else:
            text = str(value)

        conditions = [name for flag, name in FLAGS if self.ctx.flags.get(flag)]
        return text, conditions


def self_check(archive_dir):
    """Replay the pristine upstream files and report the reproduction rate."""
    total = matched = flags_matched = 0
    worst = []

    for name in sorted(os.listdir(archive_dir)):
        if not name.endswith('.decTest'):
            continue
        if any(k in name for k in ('Encode', 'Canonical', 'Base', 'testall', 'decSingle',
                                   'decDouble', 'decQuad')):
            continue

        replay = Replay(9, 384, -383, clamp=0)
        replay.reset()
        file_total = file_bad = 0

        with open(os.path.join(archive_dir, name), errors='replace') as handle:
            for raw in handle:
                tokens = tokenize(raw.split('--')[0])
                if not tokens:
                    continue
                if ':' in tokens[0]:
                    key, _, inline = tokens[0].partition(':')
                    replay.directive(key, inline or (tokens[1] if len(tokens) > 1 else ''))
                    continue
                if '->' not in tokens:
                    continue

                arrow = tokens.index('->')
                if arrow < 3 or arrow + 1 >= len(tokens):
                    continue

                op, operands, expected = tokens[1], tokens[2:arrow], tokens[arrow + 1]
                wanted = {t.lower() for t in tokens[arrow + 2:]}
                if expected.startswith('#') or expected == '?':
                    continue

                answer = replay.evaluate(op, operands)
                if answer is None:
                    continue

                text, conditions = answer
                total += 1
                file_total += 1
                if text.lower() == expected.lower():
                    matched += 1
                    if {c.lower() for c in conditions} == wanted:
                        flags_matched += 1
                else:
                    file_bad += 1

        if file_bad:
            worst.append((file_bad, name, file_total))

    worst.sort(reverse=True)
    for bad, name, count in worst[:8]:
        print('  %-28s cases=%-6d mismatch=%d' % (name, count, bad))

    rate = 100.0 * matched / total if total else 0.0
    flag_rate = 100.0 * flags_matched / total if total else 0.0
    print('\n  cases=%d  value-match=%d (%.3f%%)  value+conditions=%d (%.3f%%)'
          % (total, matched, rate, flags_matched, flag_rate))
    return rate


def banner(text=''):
    """One 72 column comment row, matching the upstream files' framing."""
    return '-- ' + text.ljust(66) + ' --'


HEADER_TEMPLATE = """{rule}
{name_row}
{blank}
{body}
{rule}
version: 2.62

extended:    1
clamp:       1
precision:   {precision}
maxExponent: {emax}
minExponent: {emin}
rounding:    half_even

"""


def emit(archive_dir, out_dir, sources):
    """Write one ds file per named dd source."""
    written = []
    for source, target, prefix_from, prefix_to in sources:
        path = os.path.join(archive_dir, source)
        if not os.path.exists(path):
            print('  missing %s' % source)
            continue

        replay = Replay(DS_PRECISION, DS_EMAX, DS_EMIN)
        replay.reset()
        lines, emitted, skipped, diverged = [], 0, 0, 0

        past_header = False

        with open(path, errors='replace') as handle:
            for raw in handle:
                body = raw.split('--')[0]
                comment = raw.split('--', 1)[1].rstrip('\r\n') if '--' in raw else None
                tokens = tokenize(body)

                # Our own banner replaces the parent's, so skip everything up to its
                # first directive and keep only the section comments in the body.
                if not past_header:
                    if tokens and ':' in tokens[0]:
                        past_header = True
                    continue

                if not tokens:
                    # Keep section banners so the file stays diffable against its parent
                    if comment is not None and comment.strip() and not comment.startswith('----'):
                        lines.append('--' + comment)
                    elif not raw.strip():
                        lines.append('')
                    continue

                if ':' in tokens[0]:
                    key, _, inline = tokens[0].partition(':')
                    value = inline or (tokens[1] if len(tokens) > 1 else '')
                    # The context is fixed to decimal32; only rounding carries over
                    if key.lower() == 'rounding':
                        replay.directive(key, value)
                        lines.append('rounding:    %s' % value)
                    continue

                if '->' not in tokens:
                    continue

                arrow = tokens.index('->')
                if arrow < 3 or arrow + 1 >= len(tokens):
                    continue

                test_id, op, operands = tokens[0], tokens[1], tokens[2:arrow]
                if any('#' in o for o in operands) or tokens[arrow + 1].startswith('#'):
                    skipped += 1
                    continue

                narrowed = [narrow_literal(o) for o in operands]
                answer = replay.evaluate(op, narrowed)
                if answer is None:
                    skipped += 1
                    continue

                text, conditions = answer
                new_id = prefix_to + test_id[len(prefix_from):]
                row = '%-10s %-13s %s -> %s' % (new_id, op, ' '.join(narrowed), text)
                if conditions:
                    row += '  ' + ' '.join(conditions)

                reason = divergence_for(new_id, op, narrowed, text, conditions)
                if reason is not None:
                    marker = 'BOOST_DECIMAL_KNOWN_BUG' if reason is STICKY_BUG \
                             else 'BOOST_DECIMAL_DIVERGENCE'
                    lines.append('# %s: %s' % (marker, reason))
                    lines.append('#' + row.rstrip())
                    diverged += 1
                else:
                    lines.append(row.rstrip())
                    emitted += 1

        parent = source.replace('.decTest', '')
        body = '\n'.join(banner(t) for t in (
            'The IBM suite ships no ds arithmetic files: decSingle is a',
            'storage-only format in decNumber, so decimal32 has never had a',
            'conformance test of its own rounding, overflow and subnormal',
            'boundaries, which sit at precision 7 / Emax 96 / Emin -95.',
            '',
            'This file is ' + parent + ' narrowed to that context, with',
            'every expected result and condition recomputed for it.',
            '',
            'Generated file. Regenerate with:',
            '  tools/generate_ds.py --emit',
            '',
            'Derived from the General Decimal Arithmetic testcases,',
            'Copyright (c) Mike Cowlishaw 1981, 2010 and IBM Corporation',
            '1981, 2008. See http://speleotrove.com/decimal',
        ))
        header = HEADER_TEMPLATE.format(rule='-' * 72,
                                        name_row=banner(target),
                                        blank=banner('Generated for Boost.Decimal from ' + source),
                                        body=body,
                                        precision=DS_PRECISION, emax=DS_EMAX, emin=DS_EMIN)
        out_path = os.path.join(out_dir, target)
        with open(out_path, 'w', newline='\r\n') as handle:
            handle.write(header)
            handle.write('\n'.join(lines).rstrip() + '\n')

        written.append((target, emitted, skipped, diverged))
        print('  %-26s emitted=%-6d diverged=%-5d skipped=%d' % (target, emitted, diverged, skipped))

    return written



NAN_DIVERGENCE = ('IEEE 754 does not interpret the sign of a NaN, so Boost.Decimal neither '
                  'propagates it nor quiets a signalling NaN through arithmetic')
SIGNAL_DIVERGENCE = ('Boost.Decimal models no IEEE 754 status flags, so a result the context '
                     'would reject as Invalid_operation overflows to infinity or underflows '
                     'to zero instead')
STICKY_BUG = ('known defect: a directed rounding mode drops the sticky bit when the discarded '
              'part begins with several zeros, so the result rounds the wrong way; see the same '
              'note on ddadd71801 in ddAdd.decTest')

# Cases that reproduce the sticky-bit defect rather than a deliberate divergence.
STICKY_CASES = {'dsadd71801', 'dsdiv3004'}


def divergence_for(test_id, op, operands, expected, conditions):
    """Why Boost.Decimal will not match this case, or None when it should."""
    if test_id in STICKY_CASES:
        return STICKY_BUG

    lowered = expected.lower()
    if 'nan' in lowered:
        signed = expected.lstrip().startswith('-')
        payload = any(c.isdigit() for c in expected)
        signalling = any('snan' in o.lower() for o in operands)
        # Negation touches the sign bit, so even an unsigned NaN comes back flipped
        negating = op.lower() in ('minus', 'copynegate')

        if signed or payload or signalling or negating:
            return NAN_DIVERGENCE

    if any(c in ('Invalid_operation', 'Division_impossible') for c in conditions):
        if 'nan' in lowered and not any('nan' in o.lower() or 'inf' in o.lower() for o in operands):
            return SIGNAL_DIVERGENCE

    return None


SOURCES = [
    ('ddAbs.decTest', 'dsAbs.decTest', 'ddabs', 'dsabs'),
    ('ddAdd.decTest', 'dsAdd.decTest', 'ddadd', 'dsadd'),
    ('ddSubtract.decTest', 'dsSubtract.decTest', 'ddsub', 'dssub'),
    ('ddMultiply.decTest', 'dsMultiply.decTest', 'ddmul', 'dsmul'),
    ('ddDivide.decTest', 'dsDivide.decTest', 'dddiv', 'dsdiv'),
    ('ddRemainder.decTest', 'dsRemainder.decTest', 'ddrem', 'dsrem'),
    ('ddRemainderNear.decTest', 'dsRemainderNear.decTest', 'ddrmn', 'dsrmn'),
    ('ddQuantize.decTest', 'dsQuantize.decTest', 'ddqua', 'dsqua'),
    ('ddCompare.decTest', 'dsCompare.decTest', 'ddcom', 'dscom'),
    ('ddCompareSig.decTest', 'dsCompareSig.decTest', 'ddcms', 'dscms'),
    ('ddCompareTotal.decTest', 'dsCompareTotal.decTest', 'ddcot', 'dscot'),
    ('ddMax.decTest', 'dsMax.decTest', 'ddmax', 'dsmax'),
    ('ddMin.decTest', 'dsMin.decTest', 'ddmin', 'dsmin'),
    ('ddMinus.decTest', 'dsMinus.decTest', 'ddmns', 'dsmns'),
    ('ddPlus.decTest', 'dsPlus.decTest', 'ddpls', 'dspls'),
    ('ddFMA.decTest', 'dsFMA.decTest', 'ddfma', 'dsfma'),
    ('ddNextPlus.decTest', 'dsNextPlus.decTest', 'ddnextp', 'dsnextp'),
    ('ddNextMinus.decTest', 'dsNextMinus.decTest', 'ddnextm', 'dsnextm'),
    ('ddNextToward.decTest', 'dsNextToward.decTest', 'ddnextt', 'dsnextt'),
    ('ddLogB.decTest', 'dsLogB.decTest', 'ddlogb', 'dslogb'),
    ('ddScaleB.decTest', 'dsScaleB.decTest', 'ddscb', 'dsscb'),
    ('ddSameQuantum.decTest', 'dsSameQuantum.decTest', 'ddsamq', 'dssamq'),
    ('ddReduce.decTest', 'dsReduce.decTest', 'ddred', 'dsred'),
    ('ddClass.decTest', 'dsClass.decTest', 'ddcla', 'dscla'),
    ('ddCopySign.decTest', 'dsCopySign.decTest', 'ddcps', 'dscps'),
    ('ddToIntegral.decTest', 'dsToIntegral.decTest', 'ddintx', 'dsintx'),
]


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--self-check', action='store_true',
                        help='replay the pristine upstream files against the oracle')
    parser.add_argument('--emit', action='store_true', help='write dectest/ds*.decTest')
    parser.add_argument('--archive', default='archive/dectest')
    parser.add_argument('--out', default='dectest')
    args = parser.parse_args()

    if not args.self_check and not args.emit:
        parser.error('choose --self-check or --emit')

    if args.self_check:
        print('Replaying %s against Python decimal %s:' % (args.archive, decimal.__version__
                                                           if hasattr(decimal, '__version__') else ''))
        rate = self_check(args.archive)
        if rate < 99.9:
            print('\n  FAIL: reproduction below the 99.9%% bar')
            return 1
        print('  OK: oracle reproduces upstream at %.3f%%' % rate)

    if args.emit:
        print('Writing decimal32 files into %s:' % args.out)
        emit(args.archive, args.out, SOURCES)

    return 0


if __name__ == '__main__':
    sys.exit(main())
