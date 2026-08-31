// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/decimal.hpp>
#include <boost/core/lightweight_test.hpp>
#include "test_harness.hpp"

int main()
{
    std::cerr << std::setprecision(std::numeric_limits<boost::decimal::decimal128_t>::max_digits10);

    test_scaleb_harness("dectest/scaleb.decTest", "scaleb");
    test_scaleb_harness("dectest/ddScaleB.decTest", "scaleb");
    test_scaleb_harness("dectest/dqScaleB.decTest", "scaleb");
    test_scaleb_harness("dectest/dsScaleB.decTest", "scaleb");

    return boost::report_errors();
}
