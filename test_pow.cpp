// Copyright 2025 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/decimal.hpp>
#include <boost/core/lightweight_test.hpp>
#include "test_harness.hpp"

int main()
{
    std::cerr << std::setprecision(17);
    test_two_arg_harness("dectest0/power0.decTest", "power", [](const auto x, const auto y) { return pow(x, y); }, 10U);

    return boost::report_errors();
}
