// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/decimal.hpp>
#include <boost/core/lightweight_test.hpp>
#include "test_harness.hpp"

int main()
{
    std::cerr << std::setprecision(std::numeric_limits<boost::decimal::decimal128_t>::max_digits10);

    const auto op = [](const auto x) { return boost::decimal::logb(x); };

    test_one_arg_harness("dectest/logb.decTest", "logb", op);
    test_one_arg_harness("dectest/ddLogB.decTest", "logb", op);
    test_one_arg_harness("dectest/dqLogB.decTest", "logb", op);
    test_one_arg_harness("dectest/dsLogB.decTest", "logb", op);

    return boost::report_errors();
}
