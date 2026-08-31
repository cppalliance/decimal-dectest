// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/decimal.hpp>
#include <boost/core/lightweight_test.hpp>
#include "test_harness.hpp"

int main()
{
    std::cerr << std::setprecision(std::numeric_limits<boost::decimal::decimal128_t>::max_digits10);

    // decTest remaindernear is the IEEE 754 remainder: the quotient is rounded to nearest,
    // which is what boost::decimal::remainder does (operator% truncates instead).
    //
    // Marked format-specific: the answer needs digits(quotient) + digits(y) of precision to
    // come out exact, so a 16-digit expectation says nothing about a 7-digit type even when
    // every literal on the line fits it.
    const auto op = [](const auto x, const auto y) { return boost::decimal::remainder(x, y); };

    test_two_arg_harness<false, true>("dectest0/remaindernear0.decTest", "remaindernear", op);
    test_two_arg_harness<false, true>("dectest/remaindernear.decTest", "remaindernear", op);
    test_two_arg_harness<false, true>("dectest/ddRemainderNear.decTest", "remaindernear", op);
    test_two_arg_harness<false, true>("dectest/dqRemainderNear.decTest", "remaindernear", op);

    return boost::report_errors();
}
