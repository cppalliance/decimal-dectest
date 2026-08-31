// Copyright 2025 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_DECTEST_TEST_HARNESS_HPP
#define BOOST_DECIMAL_DECTEST_TEST_HARNESS_HPP

#include <boost/decimal.hpp>
#include <boost/core/lightweight_test.hpp>
#include "where_file.hpp"
#include "dectest_parser.hpp"
#include <vector>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <cstddef>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <functional>
#include <utility>

template <typename T>
std::size_t ulp_distance(T lhs, T rhs) noexcept
{
    if (isinf(lhs) && isinf(rhs))
    {
        return 0u;
    }
    else if (lhs == 0U && rhs == 0U)
    {
        return 0u;
    }
    else if (isnan(lhs) || isnan(rhs))
    {
        return std::numeric_limits<std::size_t>::max();
    }

    return static_cast<std::size_t>(boost::decimal::abs((lhs - rhs) / std::min(lhs, rhs)) / std::numeric_limits<T>::epsilon());
}

template <typename T, typename U>
std::size_t ulp_distance(T, U) noexcept
{
    return std::numeric_limits<std::size_t>::max();
}

namespace boost {
namespace decimal {
namespace dectest {

// Bitwise equality. NaN never compares equal, so payload and sign need a raw check.
inline bool same_bits(const boost::decimal::decimal32_t lhs, const boost::decimal::decimal32_t rhs) noexcept
{
    std::uint32_t lhs_bits {};
    std::uint32_t rhs_bits {};
    std::memcpy(&lhs_bits, &lhs, sizeof(lhs_bits));
    std::memcpy(&rhs_bits, &rhs, sizeof(rhs_bits));

    return lhs_bits == rhs_bits;
}

inline bool same_bits(const boost::decimal::decimal64_t lhs, const boost::decimal::decimal64_t rhs) noexcept
{
    std::uint64_t lhs_bits {};
    std::uint64_t rhs_bits {};
    std::memcpy(&lhs_bits, &lhs, sizeof(lhs_bits));
    std::memcpy(&rhs_bits, &rhs, sizeof(rhs_bits));

    return lhs_bits == rhs_bits;
}

#if defined(__GNUC__) && !defined(__clang__)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wclass-memaccess"
#  pragma GCC diagnostic ignored "-Wstringop-overread"
#endif

inline bool same_bits(const boost::decimal::decimal128_t lhs, const boost::decimal::decimal128_t rhs) noexcept
{
    boost::int128::uint128_t lhs_bits {};
    boost::int128::uint128_t rhs_bits {};
    std::memcpy(&lhs_bits, &lhs, sizeof(lhs_bits));
    std::memcpy(&rhs_bits, &rhs, sizeof(rhs_bits));

    return lhs_bits == rhs_bits;
}

#if defined(__GNUC__) && !defined(__clang__)
#  pragma GCC diagnostic pop
#endif

// Anything else cannot be compared bitwise.
template <typename T, typename U>
bool same_bits(const T&, const U&) noexcept
{
    return false;
}

// Compare one computed result against the expected value from the test file.
template <typename Result, typename T>
void check_result(const std::string& id, const int precision, const Result& got, const T& expected,
                  const std::size_t ulp_tol, const bool strict_cohort)
{
    if (isnan(got) && isnan(expected))
    {
        if (!BOOST_TEST(same_bits(got, expected)))
        {
            std::cerr << "Failed test: " << id << " (precision: " << precision << ")\n"
                      << "Got: " << got << "\nExpected: " << expected << std::endl;
        }
    }
    else if (ulp_tol != 0U)
    {
        const auto dist {ulp_distance(got, expected)};
        if (!BOOST_TEST_LE(dist, ulp_tol))
        {
            std::cerr << "Failed test: " << id << " (precision: " << precision << ")\n"
                      << "Got: " << got << "\nExpected: " << expected << std::endl;
        }
    }
    else if (!BOOST_TEST_EQ(got, expected))
    {
        std::cerr << "Failed test: " << id << " (precision: " << precision << ")" << std::endl;
    }

    if (strict_cohort)
    {
        if (!BOOST_TEST(boost::decimal::samequantum(got, expected)))
        {
            std::cerr << "Failed cohort: " << id << " (precision: " << precision << ")\n"
                      << "Got: " << got << "\nExpected: " << expected << std::endl;
        }
    }
}

// Open a test file and hand back its lines, or report a hard failure.
inline bool open_test_file(const std::string& file_path, std::ifstream& in)
{
    const auto full_path {boost::decimal::dectest::where_file(file_path)};
    if (full_path.empty())
    {
        std::cerr << "Failed to find file: " << file_path << std::endl;
        BOOST_TEST(false);
        return false;
    }

    in.open(full_path.c_str());
    if (!in.is_open())
    {
        std::cerr << "Failed to open file: " << full_path << std::endl;
        BOOST_TEST(false);
        return false;
    }

    return true;
}

// Fold one line into the running state and decide whether it is ours to run.
// Returns true only for a test case of the requested operation that is ready to execute.
inline bool accept_line(const test_line& parsed, const std::string& op, const std::size_t arity,
                        scan_context& ctx, scan_counters& counters)
{
    if (parsed.kind == line_kind::directive)
    {
        apply_directive(parsed, ctx);
        return false;
    }

    if (parsed.kind == line_kind::ignored)
    {
        return false;
    }

    if (parsed.kind == line_kind::parse_error)
    {
        ++counters.parse_error;
        return false;
    }

    // Everything below is a test line, so only count it against the operation under test
    if (parsed.op != op || parsed.operands.size() != arity)
    {
        return false;
    }

    if (parsed.kind == line_kind::disabled)
    {
        ++counters.skip_disabled;
        return false;
    }

    if (parsed.kind == line_kind::hex_operand)
    {
        ++counters.skip_hex;
        return false;
    }

    if (parsed.kind == line_kind::undefined_result)
    {
        ++counters.skip_undefined;
        return false;
    }

    if (ctx.skip_block)
    {
        ++counters.skip_rounding;
        return false;
    }

    return true;
}

} // namespace dectest
} // namespace decimal
} // namespace boost

// Offer every line to all three IEEE types. fits_format decides which of them the test
// actually means something for, so a precision-independent case (most of the suite) is
// checked three times instead of once, and a precision-specific one still runs only
// against the format it was written for.
#define BOOST_DECIMAL_DECTEST_DISPATCH(body)                   \
    body(boost::decimal::decimal32_t{});                       \
    body(boost::decimal::decimal64_t{});                       \
    body(boost::decimal::decimal128_t{});

template <typename Function>
void test_one_arg_harness(const std::string& file_path, const std::string& function_name, Function f, const std::size_t ulp_tol = 0U)
{
    using namespace boost::decimal::dectest;

    std::ifstream in {};
    if (!open_test_file(file_path, in))
    {
        return;
    }

    boost::decimal::fesetround(boost::decimal::rounding_mode::fe_dec_default);

    scan_context ctx {};
    scan_counters counters {};
    std::string line {};

    while (std::getline(in, line))
    {
        const auto parsed {parse_line(line)};

        if (!accept_line(parsed, function_name, 1U, ctx, counters))
        {
            continue;
        }

        auto applied {false};
        const auto run = [&](auto tag)
        {
            using T = decltype(tag);

            if (!fits_format<T>(parsed, ctx))
            {
                return;
            }

            applied = true;

            try
            {
                const T lhs {parsed.operands[0]};
                const T expected {parsed.expected};
                const auto result {f(lhs)};

                check_result(parsed.id, ctx.precision, result, expected, ulp_tol, false);
                ++counters.verified;
            }
            catch (...)
            {
                ++counters.skip_unconstructible;
            }
        };

        BOOST_DECIMAL_DECTEST_DISPATCH(run)

        if (!applied)
        {
            ++counters.skip_wrong_format;
        }
    }

    report_counters(file_path, function_name, counters);
}

// strict_cohort_compare: when true, in addition to mathematical equality the test verifies that
// the result has the same quantum (cohort) as the expected rhs. Use this for operations like
// quantize where IEEE 754-2008 specifies the exponent of the result, so a zero with the wrong
// cohort must not silently pass.
template <bool strict_cohort_compare = false, typename Function = std::minus<>>
void test_two_arg_harness(const std::string& file_path, const std::string& function_name, Function f, const std::size_t ulp_tol = 0U)
{
    using namespace boost::decimal::dectest;

    std::ifstream in {};
    if (!open_test_file(file_path, in))
    {
        return;
    }

    boost::decimal::fesetround(boost::decimal::rounding_mode::fe_dec_default);

    scan_context ctx {};
    scan_counters counters {};
    std::string line {};

    while (std::getline(in, line))
    {
        const auto parsed {parse_line(line)};

        if (!accept_line(parsed, function_name, 2U, ctx, counters))
        {
            continue;
        }

        auto applied {false};
        const auto run = [&](auto tag)
        {
            using T = decltype(tag);

            if (!fits_format<T>(parsed, ctx))
            {
                return;
            }

            applied = true;

            try
            {
                const T lhs {parsed.operands[0]};
                const T rhs {parsed.operands[1]};
                const T expected {parsed.expected};
                const auto result {f(lhs, rhs)};

                check_result(parsed.id, ctx.precision, result, expected, ulp_tol, strict_cohort_compare);
                ++counters.verified;
            }
            catch (...)
            {
                ++counters.skip_unconstructible;
            }
        };

        BOOST_DECIMAL_DECTEST_DISPATCH(run)

        if (!applied)
        {
            ++counters.skip_wrong_format;
        }
    }

    report_counters(file_path, function_name, counters);
}

template <typename Function>
void test_three_arg_harness(const std::string& file_path, const std::string& function_name, Function f, const std::size_t ulp_tol = 0U)
{
    using namespace boost::decimal::dectest;

    std::ifstream in {};
    if (!open_test_file(file_path, in))
    {
        return;
    }

    boost::decimal::fesetround(boost::decimal::rounding_mode::fe_dec_default);

    scan_context ctx {};
    scan_counters counters {};
    std::string line {};

    while (std::getline(in, line))
    {
        const auto parsed {parse_line(line)};

        if (!accept_line(parsed, function_name, 3U, ctx, counters))
        {
            continue;
        }

        auto applied {false};
        const auto run = [&](auto tag)
        {
            using T = decltype(tag);

            if (!fits_format<T>(parsed, ctx))
            {
                return;
            }

            applied = true;

            try
            {
                const T first {parsed.operands[0]};
                const T second {parsed.operands[1]};
                const T third {parsed.operands[2]};
                const T expected {parsed.expected};
                const auto result {f(first, second, third)};

                check_result(parsed.id, ctx.precision, result, expected, ulp_tol, false);
                ++counters.verified;
            }
            catch (...)
            {
                ++counters.skip_unconstructible;
            }
        };

        BOOST_DECIMAL_DECTEST_DISPATCH(run)

        if (!applied)
        {
            ++counters.skip_wrong_format;
        }
    }

    report_counters(file_path, function_name, counters);
}

// compare / comparesig: the expected result is -1, 0 or 1. Anything else (NaN with an
// Invalid_operation condition) needs a signalling compare we do not model.
inline void test_comparisons(const std::string& file_path, const std::string& function_name)
{
    using namespace boost::decimal::dectest;

    std::ifstream in {};
    if (!open_test_file(file_path, in))
    {
        return;
    }

    boost::decimal::fesetround(boost::decimal::rounding_mode::fe_dec_default);

    scan_context ctx {};
    scan_counters counters {};
    std::string line {};

    while (std::getline(in, line))
    {
        const auto parsed {parse_line(line)};

        if (!accept_line(parsed, function_name, 2U, ctx, counters))
        {
            continue;
        }

        if (parsed.expected != "0" && parsed.expected != "1" && parsed.expected != "-1")
        {
            ++counters.skip_unsupported;
            continue;
        }

        auto applied {false};
        const auto run = [&](auto tag)
        {
            using T = decltype(tag);

            if (!fits_format<T>(parsed, ctx))
            {
                return;
            }

            applied = true;

            try
            {
                const T lhs {parsed.operands[0]};
                const T rhs {parsed.operands[1]};

                bool ok {};
                if (parsed.expected == "0")
                {
                    ok = BOOST_TEST_EQ(lhs, rhs);
                }
                else if (parsed.expected == "1")
                {
                    ok = BOOST_TEST_GT(lhs, rhs);
                }
                else
                {
                    ok = BOOST_TEST_LT(lhs, rhs);
                }

                if (!ok)
                {
                    std::cerr << "Failed test: " << parsed.id << " (precision: " << ctx.precision << ")" << std::endl;
                }

                ++counters.verified;
            }
            catch (...)
            {
                ++counters.skip_unconstructible;
            }
        };

        BOOST_DECIMAL_DECTEST_DISPATCH(run)

        if (!applied)
        {
            ++counters.skip_wrong_format;
        }
    }

    report_counters(file_path, function_name, counters);
}

// comparetotal: boost::decimal::comparetotal(lhs, rhs) is true when lhs is ordered before rhs.
inline void test_comparetotal(const std::string& file_path, const std::string& function_name)
{
    using namespace boost::decimal::dectest;

    std::ifstream in {};
    if (!open_test_file(file_path, in))
    {
        return;
    }

    boost::decimal::fesetround(boost::decimal::rounding_mode::fe_dec_default);

    scan_context ctx {};
    scan_counters counters {};
    std::string line {};

    while (std::getline(in, line))
    {
        const auto parsed {parse_line(line)};

        if (!accept_line(parsed, function_name, 2U, ctx, counters))
        {
            continue;
        }

        if (parsed.expected != "0" && parsed.expected != "1" && parsed.expected != "-1")
        {
            ++counters.skip_unsupported;
            continue;
        }

        auto applied {false};
        const auto run = [&](auto tag)
        {
            using T = decltype(tag);

            if (!fits_format<T>(parsed, ctx))
            {
                return;
            }

            applied = true;

            try
            {
                const T lhs {parsed.operands[0]};
                const T rhs {parsed.operands[1]};

                const auto forward {boost::decimal::comparetotal(lhs, rhs)};
                const auto reverse {boost::decimal::comparetotal(rhs, lhs)};

                bool ok {};
                if (parsed.expected == "0")
                {
                    // Infinities of one sign and NaNs are indistinguishable to comparetotal,
                    // so only require that it answers the same way in both directions
                    if ((isinf(lhs) && isinf(rhs) && (signbit(lhs) == signbit(rhs))) || (isnan(lhs) && isnan(rhs)))
                    {
                        ok = BOOST_TEST(forward == reverse);
                    }
                    else
                    {
                        ok = BOOST_TEST(forward) && BOOST_TEST(reverse);
                    }
                }
                else if (parsed.expected == "1")
                {
                    ok = BOOST_TEST(reverse);
                }
                else
                {
                    ok = BOOST_TEST(forward);
                }

                if (!ok)
                {
                    std::cerr << "Failed test: " << parsed.id << " (precision: " << ctx.precision << ")" << std::endl;
                }

                ++counters.verified;
            }
            catch (...)
            {
                ++counters.skip_unconstructible;
            }
        };

        BOOST_DECIMAL_DECTEST_DISPATCH(run)

        if (!applied)
        {
            ++counters.skip_wrong_format;
        }
    }

    report_counters(file_path, function_name, counters);
}

#endif // BOOST_DECIMAL_DECTEST_TEST_HARNESS_HPP
