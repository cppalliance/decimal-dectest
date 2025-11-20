// Copyright 2025 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/decimal.hpp>
#include <boost/core/lightweight_test.hpp>
#include "test_harness.hpp"

int main()
{
    test_two_arg_harness("dectest0/add0.decTest", "add", [](const auto x, const auto y) { return x + y; });

    // Requires rounding-mode changes
    #ifndef BOOST_DECIMAL_NO_CONSTEVAL_DETECTION

    boost::decimal::fesetround(boost::decimal::rounding_mode::fe_dec_default);
    std::cerr << std::setprecision(std::numeric_limits<boost::decimal::decimal64_t>::max_digits10);
    test_two_arg_harness<true>("dectest/ddAdd.decTest", "add", [](const auto x, const auto y) { return x + y; });

    #endif // BOOST_DECIMAL_NO_CONSTEVAL_DETECTION

    return boost::report_errors();
}
