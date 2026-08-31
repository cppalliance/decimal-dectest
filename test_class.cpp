// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/decimal.hpp>
#include <boost/core/lightweight_test.hpp>
#include "test_harness.hpp"

int main()
{
    test_class_harness("dectest/class.decTest", "class");
    test_class_harness("dectest/ddClass.decTest", "class");
    test_class_harness("dectest/dqClass.decTest", "class");
    test_class_harness("dectest/dsClass.decTest", "class");

    return boost::report_errors();
}
