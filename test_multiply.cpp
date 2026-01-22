// Copyright 2025 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/decimal.hpp>
#include <boost/core/lightweight_test.hpp>
#include "test_harness.hpp"

int main()
{
    std::cerr << std::setprecision(17);
    test_two_arg_harness("dectest0/multiply0.decTest", "multiply", [](const auto x, const auto y) { return x * y; });
    test_two_arg_harness("dectest0/inexact0.decTest", "multiply", [](const auto x, const auto y) { return x * y; });

    std::cerr << std::setprecision(34);
    test_two_arg_harness("dectest/multiply.decTest", "multiply", [](const auto x, const auto y) { return x * y; });

    #ifndef BOOST_DECIMAL_NO_CONSTEVAL_DETECTION

    test_two_arg_harness<true>("dectest/ddMultiply.decTest", "multiply", [](const auto x, const auto y) { return x * y; });
    test_two_arg_harness<true>("dectest/dqMultiply.decTest", "multiply", [](const auto x, const auto y) { return x * y; });

    #endif

    return boost::report_errors();
}
