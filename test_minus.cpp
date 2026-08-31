// Copyright 2025 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/decimal.hpp>
#include <boost/core/lightweight_test.hpp>
#include "test_harness.hpp"

int main()
{
    std::cerr << std::setprecision(std::numeric_limits<boost::decimal::decimal128_t>::max_digits10);

    test_one_arg_harness("dectest0/minus0.decTest", "minus", [](const auto x) { return -x; });
    test_one_arg_harness("dectest/minus.decTest", "minus", [](const auto x) { return -x; });
    test_one_arg_harness("dectest/ddMinus.decTest", "minus", [](const auto x) { return -x; });
    test_one_arg_harness("dectest/dqMinus.decTest", "minus", [](const auto x) { return -x; });
    test_one_arg_harness("dectest/dsMinus.decTest", "minus", [](const auto x) { return -x; });

    return boost::report_errors();
}
