// Copyright 2025 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/decimal.hpp>
#include <boost/core/lightweight_test.hpp>
#include "test_harness.hpp"

int main()
{
    test_comparisons("dectest0/compare0.decTest", "compare");
    test_comparisons("dectest/compare.decTest", "compare");
    test_comparisons("dectest/ddCompare.decTest", "compare");

    test_comparisons("dectest/comparesig.decTest", "comparesig");
    test_comparisons("dectest/ddComparesig.decTest", "comparesig");

    return boost::report_errors();
}
