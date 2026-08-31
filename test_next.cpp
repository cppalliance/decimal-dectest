// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/decimal.hpp>
#include <boost/core/lightweight_test.hpp>
#include "test_harness.hpp"

int main()
{
    std::cerr << std::setprecision(std::numeric_limits<boost::decimal::decimal128_t>::max_digits10);

    // nextplus/nextminus are nextafter toward the respective infinity
    const auto up = [](const auto x)
    {
        using T = decltype(x);
        return boost::decimal::nextafter(x, std::numeric_limits<T>::infinity());
    };
    const auto down = [](const auto x)
    {
        using T = decltype(x);
        return boost::decimal::nextafter(x, -std::numeric_limits<T>::infinity());
    };
    const auto toward = [](const auto x, const auto y) { return boost::decimal::nextafter(x, y); };

    test_one_arg_harness<true>("dectest/nextplus.decTest", "nextplus", up);
    test_one_arg_harness<true>("dectest/ddNextPlus.decTest", "nextplus", up);
    test_one_arg_harness<true>("dectest/dqNextPlus.decTest", "nextplus", up);
    test_one_arg_harness<true>("dectest/dsNextPlus.decTest", "nextplus", up);

    test_one_arg_harness<true>("dectest/nextminus.decTest", "nextminus", down);
    test_one_arg_harness<true>("dectest/ddNextMinus.decTest", "nextminus", down);
    test_one_arg_harness<true>("dectest/dqNextMinus.decTest", "nextminus", down);
    test_one_arg_harness<true>("dectest/dsNextMinus.decTest", "nextminus", down);

    test_two_arg_harness<false, true>("dectest/nexttoward.decTest", "nexttoward", toward);
    test_two_arg_harness<false, true>("dectest/ddNextToward.decTest", "nexttoward", toward);
    test_two_arg_harness<false, true>("dectest/dqNextToward.decTest", "nexttoward", toward);
    test_two_arg_harness<false, true>("dectest/dsNextToward.decTest", "nexttoward", toward);

    return boost::report_errors();
}
