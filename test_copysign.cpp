// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/decimal.hpp>
#include <boost/core/lightweight_test.hpp>
#include "test_harness.hpp"

int main()
{
    std::cerr << std::setprecision(std::numeric_limits<boost::decimal::decimal128_t>::max_digits10);

    const auto op = [](const auto x, const auto y) { return boost::decimal::copysign(x, y); };

    test_two_arg_harness("dectest/copysign.decTest", "copysign", op);
    test_two_arg_harness("dectest/ddCopySign.decTest", "copysign", op);
    test_two_arg_harness("dectest/dqCopySign.decTest", "copysign", op);

    return boost::report_errors();
}
