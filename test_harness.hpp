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
inline void test_one_arg_harness(const std::string& file_path, const std::string& function_name, Function f)
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
        // Skip commented lines
        if (line.find("#") != std::string::npos)
        {
            continue;
        }

        const auto first_space = line.find(" ");
        const auto test_name = line.substr(0, first_space);

        // Check if this line contains our function
        const auto pos_test = line.find(function_name + " ");
        if (pos_test == std::string::npos)
        {
            continue;
        }

        ++num_tests_found;

        // Find the arrow separator
        const auto arrow_pos = line.find("->");
        if (arrow_pos == std::string::npos)
        {
            std::cerr << "Invalid format: missing '->' in line: " << line << std::endl;
            continue;
        }

        // Extract LHS value (between function name and ->)
        auto lhs_start = pos_test + function_name.length() + 1; // Skip function name and space
        auto lhs_end = arrow_pos;

        // Trim whitespace and extract value
        while (lhs_start < lhs_end && std::isspace(line[lhs_start]))
        {
            lhs_start++;
        }

        while (lhs_end > lhs_start && std::isspace(line[lhs_end - 1]))
        {
            lhs_end--;
        }

        std::string lhs_value = line.substr(lhs_start, lhs_end - lhs_start);

        // Remove quotes if present
        if (!lhs_value.empty() && lhs_value.front() == '\'' && lhs_value.back() == '\'')
        {
            lhs_value = lhs_value.substr(1, lhs_value.length() - 2);
        }

        // Extract RHS value (after ->)
        auto rhs_start = arrow_pos + 2; // Skip "->"
        auto rhs_end = line.length();

        // Trim whitespace
        while (rhs_start < rhs_end && std::isspace(line[rhs_start])) rhs_start++;
        while (rhs_end > rhs_start && std::isspace(line[rhs_end - 1])) rhs_end--;

        std::string rhs_value = line.substr(rhs_start, rhs_end - rhs_start);

        // Remove quotes if present
        if (!rhs_value.empty() && rhs_value.front() == '\'' && rhs_value.back() == '\'')
        {
            rhs_value = rhs_value.substr(1, rhs_value.length() - 2);
        }

        // Now we can utilize the string constructors to form the values
        try
        {
            const boost::decimal::decimal64_t lhs {lhs_value};
            const boost::decimal::decimal64_t rhs {rhs_value};
            if (!BOOST_TEST_EQ(f(lhs), rhs))
            {
                std::cerr << "Failed test: " << test_name << std::endl;
            }
        }
        catch (...)
        {
            // Invalid construction is supposed to throw
            BOOST_TEST(true);
        }
    }

    BOOST_TEST_GT(num_tests_found, 0U);
}

#endif // BOOST_DECIMAL_DECTEST_TEST_HARNESS_HPP
