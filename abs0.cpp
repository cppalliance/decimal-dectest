// Copyright 2025 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/decimal.hpp>
#include <boost/core/lightweight_test.hpp>
#include "test_harness.hpp"
#include <functional>

int main()
{
    test_harness("dectest0/abs0.decTest", "abs", [](const auto x) { return boost::decimal::abs(x); });
    return boost::report_errors();
}
