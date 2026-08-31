// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/decimal.hpp>
#include <boost/core/lightweight_test.hpp>
#include "test_harness.hpp"

int main()
{
    std::cerr << std::setprecision(std::numeric_limits<boost::decimal::decimal128_t>::max_digits10);

    const auto op = [](const auto x, const auto y, const auto z) { return boost::decimal::fma(x, y, z); };

    test_three_arg_harness("dectest0/fma0.decTest", "fma", op);
    test_three_arg_harness("dectest/fma.decTest", "fma", op);
    test_three_arg_harness("dectest/ddFMA.decTest", "fma", op);
    test_three_arg_harness("dectest/dqFMA.decTest", "fma", op);

    return boost::report_errors();
}
