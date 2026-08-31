// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/decimal.hpp>
#include <boost/core/lightweight_test.hpp>
#include "test_harness.hpp"

int main()
{
    test_predicate_harness("dectest0/samequantum0.decTest", "samequantum");
    test_predicate_harness("dectest/samequantum.decTest", "samequantum");
    test_predicate_harness("dectest/ddSameQuantum.decTest", "samequantum");
    test_predicate_harness("dectest/dqSameQuantum.decTest", "samequantum");

    return boost::report_errors();
}
