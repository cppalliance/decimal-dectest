// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_DECTEST_PARSER_HPP
#define BOOST_DECIMAL_DECTEST_PARSER_HPP

#include <boost/decimal.hpp>
#include <boost/core/lightweight_test.hpp>
#include <string>
#include <vector>
#include <iostream>
#include <cctype>
#include <limits>
#include <cstddef>

namespace boost {
namespace decimal {
namespace dectest {

// How a single line of a .decTest file was classified.
enum class line_kind
{
    ignored,            // Blank, banner, or a "--" comment
    directive,          // "key: value"
    test_case,          // A usable test
    disabled,           // Manually disabled by a leading '#'
    hex_operand,        // Carries a directly encoded "#..." value we cannot parse
    undefined_result,   // Expected result is "?"
    parse_error         // Looked like a test but could not be read
};

// One parsed line. Only the members relevant to "kind" are populated.
struct test_line
{
    line_kind kind {line_kind::ignored};
    std::string id {};
    std::string op {};
    std::vector<std::string> operands {};
    std::string expected {};
    std::vector<std::string> conditions {};
    std::string key {};
    std::string value {};
};

// Why lines were not verified, so a silent drop cannot masquerade as a pass.
struct scan_counters
{
    std::size_t verified {};
    std::size_t skip_disabled {};        // Leading '#', disabled by a maintainer
    std::size_t skip_hex {};             // Directly encoded "#..." operand or result
    std::size_t skip_undefined {};       // Expected result is "?"
    std::size_t skip_rounding {};        // Inside an unsupported rounding mode block
    std::size_t skip_unsupported {};     // Expected result we cannot express
    std::size_t skip_unconstructible {}; // A value string Boost.Decimal refuses to build
    std::size_t skip_wrong_format {};    // Needs more digits or range than the type has
    std::size_t parse_error {};          // Looked like a test but could not be read
};

namespace detail {

inline bool is_quote(const char c) noexcept
{
    return c == '\'' || c == '"';
}

inline std::string to_lower(std::string s)
{
    for (auto& c : s)
    {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }

    return s;
}

} // namespace detail

// Split a line into whitespace separated tokens, keeping quoted runs intact and
// dropping everything from a trailing "--" comment. Quotes are stripped.
inline std::vector<std::string> tokenize(const std::string& line)
{
    std::vector<std::string> tokens {};
    std::string current {};
    bool in_token {false};
    char quote {'\0'};

    for (std::size_t i {}; i < line.size(); ++i)
    {
        const auto c {line[i]};

        if (quote != '\0')
        {
            if (c == quote)
            {
                quote = '\0';
            }
            else
            {
                current += c;
            }
            continue;
        }

        if (detail::is_quote(c))
        {
            quote = c;
            in_token = true;
            continue;
        }

        if (std::isspace(static_cast<unsigned char>(c)))
        {
            if (in_token)
            {
                tokens.push_back(current);
                current.clear();
                in_token = false;
            }
            continue;
        }

        // A token beginning with "--" starts a comment that runs to end of line
        if (!in_token && c == '-' && i + 1U < line.size() && line[i + 1U] == '-')
        {
            break;
        }

        current += c;
        in_token = true;
    }

    if (in_token)
    {
        tokens.push_back(current);
    }

    return tokens;
}

// Classify one raw line and pull out whatever it carries.
inline test_line parse_line(const std::string& raw);

inline test_line parse_line(const std::string& raw)
{
    test_line result {};

    std::size_t first {};
    while (first < raw.size() && std::isspace(static_cast<unsigned char>(raw[first])))
    {
        ++first;
    }

    if (first == raw.size())
    {
        return result;
    }

    // A leading '#' marks a test a maintainer disabled on purpose. Re-read it without
    // the marker so a disabled test can still be attributed to its operation, and so a
    // plain "# note to self" comment is not mistaken for one.
    if (raw[first] == '#')
    {
        auto stripped = raw.substr(first + 1U);
        auto inner = parse_line(stripped);

        if (inner.kind == line_kind::test_case || inner.kind == line_kind::hex_operand ||
            inner.kind == line_kind::undefined_result)
        {
            inner.kind = line_kind::disabled;
            return inner;
        }

        return result;
    }

    const auto tokens {tokenize(raw)};
    if (tokens.empty())
    {
        return result;
    }

    // "key: value" directive
    const auto colon {tokens[0].find(':')};
    if (colon != std::string::npos)
    {
        result.kind = line_kind::directive;
        result.key = detail::to_lower(tokens[0].substr(0, colon));

        auto value = tokens[0].substr(colon + 1U);
        if (value.empty() && tokens.size() > 1U)
        {
            value = tokens[1];
        }

        result.value = value;
        return result;
    }

    std::size_t arrow {tokens.size()};
    for (std::size_t i {}; i < tokens.size(); ++i)
    {
        if (tokens[i] == "->")
        {
            arrow = i;
            break;
        }
    }

    if (arrow == tokens.size())
    {
        // Not a test line at all (banner text, section header)
        return result;
    }

    // Needs at least an id, an operation, one operand, and a result
    if (arrow < 3U || arrow + 1U >= tokens.size())
    {
        result.kind = line_kind::parse_error;
        return result;
    }

    result.id = tokens[0];
    result.op = tokens[1];
    result.operands.assign(tokens.begin() + 2, tokens.begin() + static_cast<std::ptrdiff_t>(arrow));
    result.expected = tokens[arrow + 1U];
    result.conditions.assign(tokens.begin() + static_cast<std::ptrdiff_t>(arrow) + 2, tokens.end());

    // "?" means the spec leaves the result undefined
    if (result.expected == "?")
    {
        result.kind = line_kind::undefined_result;
        return result;
    }

    // '#' introduces a directly encoded operand or result, which we cannot construct from
    for (const auto& operand : result.operands)
    {
        if (operand.find('#') != std::string::npos)
        {
            result.kind = line_kind::hex_operand;
            return result;
        }
    }

    if (result.expected.find('#') != std::string::npos)
    {
        result.kind = line_kind::hex_operand;
        return result;
    }

    result.kind = line_kind::test_case;
    return result;
}

// Running state carried across the lines of one file.
struct scan_context
{
    int precision {16};
    int max_exponent {384};
    int min_exponent {-383};
    bool skip_block {false};
};

// What a numeric literal needs from a format: how many significant digits it carries
// and where its leading digit sits. Specials (NaN, Infinity) need nothing.
struct value_profile
{
    bool special {false};
    int significant_digits {};
    int adjusted_exponent {};
};

// Measure a decTest numeric literal without constructing anything, so the decision to
// run a test does not depend on the behavior being tested.
inline value_profile profile_value(const std::string& text)
{
    value_profile profile {};

    std::size_t i {};
    if (i < text.size() && (text[i] == '+' || text[i] == '-'))
    {
        ++i;
    }

    const auto rest {detail::to_lower(text.substr(i))};
    if (rest.find("nan") != std::string::npos || rest.find("inf") != std::string::npos)
    {
        profile.special = true;
        return profile;
    }

    std::string digits {};
    int exponent {};
    int fractional {};
    bool seen_point {false};

    for (; i < text.size(); ++i)
    {
        const auto c {text[i]};

        if (c == '.')
        {
            seen_point = true;
        }
        else if (c == 'e' || c == 'E')
        {
            try
            {
                exponent = std::stoi(text.substr(i + 1U));
            }
            catch (...)
            {
                exponent = 0;
            }
            break;
        }
        else if (std::isdigit(static_cast<unsigned char>(c)))
        {
            digits += c;
            if (seen_point)
            {
                ++fractional;
            }
        }
    }

    // Leading zeros are not significant
    std::size_t first_significant {};
    while (first_significant + 1U < digits.size() && digits[first_significant] == '0')
    {
        ++first_significant;
    }
    const auto significant {digits.substr(first_significant)};

    profile.significant_digits = significant.empty() ? 1 : static_cast<int>(significant.size());
    profile.adjusted_exponent = exponent - fractional + profile.significant_digits - 1;

    return profile;
}

// True when the test line carries the named condition.
inline bool has_condition(const test_line& parsed, const std::string& name)
{
    for (const auto& condition : parsed.conditions)
    {
        if (detail::to_lower(condition) == name)
        {
            return true;
        }
    }

    return false;
}

// Decide whether a test is meaningful for type T.
//
// Two things have to hold. Every literal must land in T unchanged, or the operands are
// already different numbers before the operation runs. And where the file's context
// shaped the expected result, that context has to be T's own: an expected value rounded
// at precision 9 says nothing about a type carrying 7 digits, and a Clamped or Subnormal
// result depends on the exponent range it was produced in.
template <typename T>
bool fits_format(const test_line& parsed, const scan_context& ctx, const bool format_specific = false)
{
    // Some operations are defined by the format itself rather than by the operand values:
    // one step of nextplus is one ulp of the format, so a precision-9 answer says nothing
    // about a 16-digit type even though every literal on the line fits it.
    if (format_specific &&
        (ctx.precision != std::numeric_limits<T>::digits ||
         ctx.max_exponent != std::numeric_limits<T>::max_exponent ||
         ctx.min_exponent != std::numeric_limits<T>::min_exponent))
    {
        return false;
    }

    auto representable = [](const std::string& text)
    {
        const auto profile {profile_value(text)};

        if (profile.special)
        {
            return true;
        }

        return profile.significant_digits <= std::numeric_limits<T>::digits &&
               profile.adjusted_exponent <= std::numeric_limits<T>::max_exponent &&
               profile.adjusted_exponent >= std::numeric_limits<T>::min_exponent;
    };

    for (const auto& operand : parsed.operands)
    {
        if (!representable(operand))
        {
            return false;
        }
    }

    if (!representable(parsed.expected))
    {
        return false;
    }

    if (has_condition(parsed, "rounded") || has_condition(parsed, "inexact") ||
        has_condition(parsed, "lost_digits"))
    {
        if (ctx.precision != std::numeric_limits<T>::digits)
        {
            return false;
        }
    }

    if (has_condition(parsed, "clamped") || has_condition(parsed, "subnormal") ||
        has_condition(parsed, "underflow") || has_condition(parsed, "overflow"))
    {
        if (ctx.max_exponent != std::numeric_limits<T>::max_exponent ||
            ctx.min_exponent != std::numeric_limits<T>::min_exponent)
        {
            return false;
        }
    }

    // A NaN that the context produced because the answer did not fit says nothing about a
    // wider format, where the same operation succeeds. An operation invalid in its own
    // right (a signalling operand, an infinity) is not capacity driven and carries over.
    if ((has_condition(parsed, "invalid_operation") || has_condition(parsed, "division_impossible")) &&
        profile_value(parsed.expected).special)
    {
        auto special_operand {false};
        for (const auto& operand : parsed.operands)
        {
            special_operand = special_operand || profile_value(operand).special;
        }

        if (!special_operand &&
            (ctx.precision != std::numeric_limits<T>::digits ||
             ctx.max_exponent != std::numeric_limits<T>::max_exponent ||
             ctx.min_exponent != std::numeric_limits<T>::min_exponent))
        {
            return false;
        }
    }

    return true;
}

// Apply a "rounding:" directive. Returns false when the mode cannot be honored, in
// which case the caller skips the block that follows rather than running it under
// whatever mode happens to be current.
inline bool apply_rounding(const std::string& mode)
{
    #ifdef BOOST_DECIMAL_NO_CONSTEVAL_DETECTION

    // fesetround cannot take effect here, so only the default mode is honest
    return mode == "half_even";

    #else

    if (mode == "half_even")
    {
        boost::decimal::fesetround(boost::decimal::rounding_mode::fe_dec_to_nearest);
    }
    else if (mode == "half_up")
    {
        boost::decimal::fesetround(boost::decimal::rounding_mode::fe_dec_to_nearest_from_zero);
    }
    else if (mode == "down")
    {
        // dectest "down" truncates toward zero, unlike "floor"
        boost::decimal::fesetround(boost::decimal::rounding_mode::fe_dec_toward_zero);
    }
    else if (mode == "floor")
    {
        boost::decimal::fesetround(boost::decimal::rounding_mode::fe_dec_downward);
    }
    else if (mode == "ceiling")
    {
        boost::decimal::fesetround(boost::decimal::rounding_mode::fe_dec_upward);
    }
    else
    {
        // "up", "half_down" and "05up" are not IEEE 754 modes and are not modelled
        return false;
    }

    return true;

    #endif // BOOST_DECIMAL_NO_CONSTEVAL_DETECTION
}

// Fold a directive into the running context. Returns false when the directive
// selects a rounding mode we cannot honor.
inline bool apply_directive(const test_line& parsed, scan_context& ctx)
{
    if (parsed.key == "precision" || parsed.key == "maxexponent" || parsed.key == "minexponent")
    {
        try
        {
            const auto value {std::stoi(parsed.value)};

            if (parsed.key == "precision")
            {
                ctx.precision = value;
            }
            else if (parsed.key == "maxexponent")
            {
                ctx.max_exponent = value;
            }
            else
            {
                ctx.min_exponent = value;
            }
        }
        catch (...)
        {
            // Leave the previous value in place
        }
    }
    else if (parsed.key == "rounding")
    {
        ctx.skip_block = !apply_rounding(detail::to_lower(parsed.value));
        return !ctx.skip_block;
    }

    return true;
}

// Print the breakdown and assert that nothing was dropped for a reason we do not understand.
inline void report_counters(const std::string& file_path, const std::string& op, const scan_counters& counters)
{
    std::cerr << file_path << " [" << op << "]"
              << "  verified: " << counters.verified
              << "  disabled: " << counters.skip_disabled
              << "  hex: " << counters.skip_hex
              << "  undefined: " << counters.skip_undefined
              << "  rounding: " << counters.skip_rounding
              << "  unsupported: " << counters.skip_unsupported
              << "  wrong_format: " << counters.skip_wrong_format
              << "  unconstructible: " << counters.skip_unconstructible
              << "  parse_error: " << counters.parse_error
              << std::endl;

    if (!BOOST_TEST_EQ(counters.parse_error, std::size_t {0}))
    {
        std::cerr << "Unparseable lines in " << file_path << std::endl;
    }

    // The operation has to appear in the file at all, or the driver is pointed at the wrong one
    const auto recognized {counters.verified + counters.skip_disabled + counters.skip_hex +
                           counters.skip_undefined + counters.skip_rounding + counters.skip_unsupported +
                           counters.skip_wrong_format + counters.skip_unconstructible};

    if (!BOOST_TEST_GT(recognized, std::size_t {0}))
    {
        std::cerr << "Operation " << op << " does not appear in " << file_path << std::endl;
    }
    else if (counters.verified == 0U)
    {
        std::cerr << "Note: nothing runnable in " << file_path << " for operation " << op << std::endl;
    }
}

} // namespace dectest
} // namespace decimal
} // namespace boost

#endif // BOOST_DECIMAL_DECTEST_PARSER_HPP
