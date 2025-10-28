// Copyright 2025 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_DECTEST_TEST_HARNESS_HPP
#define BOOST_DECIMAL_DECTEST_TEST_HARNESS_HPP

#include <boost/decimal.hpp>
#include <boost/core/lightweight_test.hpp>
#include "where_file.hpp"
#include <vector>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <cstddef>
#include <cstdio>
#include <cstdint>

template <typename Function>
inline void test_harness(const std::string& file_path, const std::string& function_name, Function f)
{
    const auto full_path {boost::decimal::dectest::where_file(file_path)};
    if (full_path.empty())
    {
        std::cerr << "Failed to find file: " << file_path << std::endl;
        BOOST_TEST(false);
        return;
    }

    std::ifstream in(full_path.c_str());
    if (!in.is_open())
    {
        std::cerr << "Failed to open file: " << full_path << std::endl;
        BOOST_TEST(false);
        return;
    }

    std::size_t num_tests_found {};
    std::string line;
    while (std::getline(in, line))
    {
        const auto pos_test {line.find(function_name, 0U)};
        if (pos_test)
        {
            ++num_tests_found;
        }
    }

    BOOST_TEST_GT(num_tests_found, 0U);
}

#endif // BOOST_DECIMAL_DECTEST_TEST_HARNESS_HPP
