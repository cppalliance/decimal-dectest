// Copyright 2025 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/decimal.hpp>
#include <boost/core/lightweight_test.hpp>
#include "test_harness.hpp"

template <typename T>
void test_edges(const char* lhs_str, const char* rhs_str, const char* res_str, const boost::decimal::rounding_mode round)
{
    const auto current_round {boost::decimal::fesetround(round)};
    BOOST_TEST(current_round == round);

    const T lhs {lhs_str};
    const T rhs {rhs_str};
    const T expected_res {res_str};

    const T res {lhs + rhs};

    BOOST_TEST_EQ(res, expected_res);
}

int main()
{
    #ifndef BOOST_DECIMAL_NO_CONSTEVAL_DETECTION

    std::cerr << std::setprecision(std::numeric_limits<boost::decimal::decimal128_t>::max_digits10);
    test_edges<boost::decimal::decimal64_t>("1E16", "-0.51", "9999999999999999", boost::decimal::rounding_mode::fe_dec_to_nearest);
    test_edges<boost::decimal::decimal128_t>("1E34", "-0.51", "9999999999999999999999999999999999", boost::decimal::rounding_mode::fe_dec_to_nearest);

    #endif

    test_two_arg_harness("dectest0/add0.decTest", "add", [](const auto x, const auto y) { return x + y; });

    // Requires rounding-mode changes
    #ifndef BOOST_DECIMAL_NO_CONSTEVAL_DETECTION

    boost::decimal::fesetround(boost::decimal::rounding_mode::fe_dec_default);
    std::cerr << std::setprecision(std::numeric_limits<boost::decimal::decimal64_t>::max_digits10);
    test_two_arg_harness<true>("dectest/ddAdd.decTest", "add", [](const auto x, const auto y) { return x + y; });

    boost::decimal::fesetround(boost::decimal::rounding_mode::fe_dec_default);
    std::cerr << std::setprecision(std::numeric_limits<boost::decimal::decimal128_t>::max_digits10);
    test_two_arg_harness<true>("dectest/dqAdd.decTest", "add", [](const auto x, const auto y) { return x + y; });

    #endif // BOOST_DECIMAL_NO_CONSTEVAL_DETECTION

    return boost::report_errors();
}
