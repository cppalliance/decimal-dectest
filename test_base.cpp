// Copyright 2025 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/decimal.hpp>
#include <boost/core/lightweight_test.hpp>
#include "test_harness.hpp"

int main()
{
    // Effectively NOOP since we are already performing this string manipulation in the constructors
    test_one_arg_harness("dectest0/base0.decTest", "toSci", [](const auto x) { return x; });

    return boost::report_errors();
}
