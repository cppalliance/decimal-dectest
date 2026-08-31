// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/decimal.hpp>
#include <boost/core/lightweight_test.hpp>
#include "test_harness.hpp"

int main()
{
    std::cerr << std::setprecision(std::numeric_limits<boost::decimal::decimal128_t>::max_digits10);

    // decTest reduce strips trailing zeros to the shortest cohort member
    const auto op = [](const auto x) { return boost::decimal::normalize(x); };

    test_one_arg_harness("dectest0/reduce0.decTest", "reduce", op);
    test_one_arg_harness("dectest/reduce.decTest", "reduce", op);
    test_one_arg_harness("dectest/ddReduce.decTest", "reduce", op);
    test_one_arg_harness("dectest/dqReduce.decTest", "reduce", op);

    return boost::report_errors();
}
