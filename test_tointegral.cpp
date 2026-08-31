// Copyright 2025 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/decimal.hpp>
#include <boost/core/lightweight_test.hpp>
#include "test_harness.hpp"

int main()
{
    std::cerr << std::setprecision(std::numeric_limits<boost::decimal::decimal128_t>::max_digits10);

    // Both decTest spellings round to an integral value under the context rounding mode
    // and differ only in whether Inexact is signalled, which we do not model. rint
    // follows the current mode, unlike a cast to an integer type which always truncates.
    const auto op = [](const auto x) { return boost::decimal::rint(x); };

    test_one_arg_harness("dectest0/tointegral0.decTest", "tointegral", op);
    test_one_arg_harness("dectest/tointegral.decTest", "tointegral", op);

    // The dd/dq files spell every case tointegralx, which differs only in signalling Inexact
    test_one_arg_harness("dectest/tointegralx.decTest", "tointegralx", op);
    test_one_arg_harness("dectest/ddToIntegral.decTest", "tointegralx", op);
    test_one_arg_harness("dectest/dqToIntegral.decTest", "tointegralx", op);

    return boost::report_errors();
}
