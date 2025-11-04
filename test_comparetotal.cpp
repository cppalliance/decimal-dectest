// Copyright 2025 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/decimal.hpp>
#include <boost/core/lightweight_test.hpp>
#include "test_harness.hpp"

int main()
{
    test_comparetotal("dectest0/comparetotal0.decTest", "comparetotal");
    test_comparetotal("dectest/comparetotal.decTest", "comparetotal");

    return boost::report_errors();
}
