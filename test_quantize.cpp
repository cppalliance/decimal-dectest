// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/decimal.hpp>
#include <boost/core/lightweight_test.hpp>
#include "test_harness.hpp"

int main()
{
    std::cerr << std::setprecision(std::numeric_limits<boost::decimal::decimal128_t>::max_digits10);

    // Quantize must produce a result whose cohort matches the rhs (IEEE 754-2008 5.3.2),
    // so enable strict_cohort_compare alongside the value check.
    test_two_arg_harness<true>("dectest0/quantize0.decTest", "quantize", [](const auto x, const auto y) { return boost::decimal::quantize(x, y); });
    test_two_arg_harness<true>("dectest/quantize.decTest", "quantize", [](const auto x, const auto y) { return boost::decimal::quantize(x, y); });
    test_two_arg_harness<true>("dectest/ddQuantize.decTest", "quantize", [](const auto x, const auto y) { return boost::decimal::quantize(x, y); });
    test_two_arg_harness<true>("dectest/dqQuantize.decTest", "quantize", [](const auto x, const auto y) { return boost::decimal::quantize(x, y); });

    return boost::report_errors();
}
