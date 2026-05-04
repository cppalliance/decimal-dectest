// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/decimal.hpp>
#include <boost/core/lightweight_test.hpp>
#include "test_harness.hpp"

int main()
{
    test_two_arg_harness<true>("dectest0/quantize0.decTest", "quantize", [](const auto x, const auto y) { return boost::decimal::quantize(x, y); });

    return boost::report_errors();
}
