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
#include <functional>
#include <utility>

template <typename T>
std::size_t ulp_distance(T lhs, T rhs, const std::size_t tol) noexcept
{
    if (lhs == rhs)
    {
        return 0;
    }
    else if (lhs < rhs)
    {
        std::swap(lhs, rhs);
    }


    for (std::size_t i {}; i < tol; ++i)
    {
        lhs = nextafter(lhs, rhs);
        if (lhs == rhs)
        {
            return i;
        }
    }

    return std::numeric_limits<std::size_t>::max();
}

template <typename Function>
void test_one_arg_harness(const std::string& file_path, const std::string& function_name, Function f)
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
    std::size_t invalid_tests {};
    std::string line;
    int current_precision = 16; // Default precision

    while (std::getline(in, line))
    {
        // Skip commented lines
        if (line.find("#") != std::string::npos)
        {
            continue;
        }

        // Check for precision specification
        auto precision_pos = line.find("precision:");
        if (precision_pos != std::string::npos)
        {
            auto precision_start = precision_pos + 10; // Skip "precision:"

            // Skip whitespace
            while (precision_start < line.length() && std::isspace(line[precision_start]))
            {
                precision_start++;
            }

            // Extract precision value
            std::string precision_str;
            while (precision_start < line.length() && std::isdigit(line[precision_start]))
            {
                precision_str += line[precision_start++];
            }

            if (!precision_str.empty())
            {
                current_precision = std::stoi(precision_str);
            }
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
            ++invalid_tests;
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

        // Select appropriate decimal type based on precision
        try
        {
            if (current_precision <= 9)
            {
                // Use decimal32_t
                const boost::decimal::decimal32_t lhs {lhs_value};
                const boost::decimal::decimal32_t rhs {rhs_value};

                if (isnan(lhs) && isnan(rhs))
                {
                    const auto f_lhs {f(lhs)};
                    std::uint32_t lhs_bits;
                    std::memcpy(&lhs_bits, &f_lhs, sizeof(std::uint32_t));

                    std::uint32_t rhs_bits;
                    std::memcpy(&rhs_bits, &rhs, sizeof(std::uint32_t));

                    if (!BOOST_TEST_EQ(lhs_bits, rhs_bits))
                    {
                        std::cerr << "Failed test: " << test_name << " (precision: " << current_precision << ")" << std::endl;
                    }
                }
                else if (!BOOST_TEST_EQ(f(lhs), rhs))
                {
                    std::cerr << "Failed test: " << test_name << " (precision: " << current_precision << ")" << std::endl;
                }
            }
            else if (current_precision <= 16)
            {
                // Use decimal64_t
                const boost::decimal::decimal64_t lhs {lhs_value};
                const boost::decimal::decimal64_t rhs {rhs_value};

                if (isnan(lhs) && isnan(rhs))
                {
                    const auto f_lhs {f(lhs)};
                    std::uint64_t lhs_bits;
                    std::memcpy(&lhs_bits, &f_lhs, sizeof(std::uint64_t));

                    std::uint64_t rhs_bits;
                    std::memcpy(&rhs_bits, &rhs, sizeof(std::uint64_t));

                    if (!BOOST_TEST_EQ(lhs_bits, rhs_bits))
                    {
                        std::cerr << "Failed test: " << test_name << " (precision: " << current_precision << ")" << std::endl;
                    }
                }
                else if (!BOOST_TEST_EQ(f(lhs), rhs))
                {
                    std::cerr << "Failed test: " << test_name << " (precision: " << current_precision << ")" << std::endl;
                }
            }
            else
            {
                #ifdef __GNUC__
                #  pragma GCC diagnostic push
                #  pragma GCC diagnostic ignored "-Wclass-memaccess"
                #endif

                // Use decimal128_t
                const boost::decimal::decimal128_t lhs {lhs_value};
                const boost::decimal::decimal128_t rhs {rhs_value};

                if (isnan(lhs) && isnan(rhs))
                {
                    const auto f_lhs {f(lhs)};
                    boost::int128::uint128_t lhs_bits;
                    std::memcpy(&lhs_bits, &f_lhs, sizeof(boost::int128::uint128_t));

                    boost::int128::uint128_t rhs_bits;
                    std::memcpy(&rhs_bits, &rhs, sizeof(boost::int128::uint128_t));

                    if (!BOOST_TEST_EQ(lhs_bits, rhs_bits))
                    {
                        std::cerr << "Failed test: " << test_name << " (precision: " << current_precision << ")" << std::endl;
                    }
                }
                else if (!BOOST_TEST_EQ(f(lhs), rhs))
                {
                    std::cerr << "Failed test: " << test_name << " (precision: " << current_precision << ")" << std::endl;
                }
                
                #ifdef __GNUC__
                #  pragma GCC diagnostic pop
                #endif
            }
        }
        catch (...)
        {
            // Invalid construction is supposed to throw
            ++invalid_tests;
        }
    }

    BOOST_TEST_GT(num_tests_found, 0U);
    BOOST_TEST_LT(invalid_tests, num_tests_found);
}

template <bool allow_rounding_changes = false, typename Function = std::minus<>()>
void test_two_arg_harness(const std::string& file_path, const std::string& function_name, Function f, const std::size_t ulp_tol = 0)
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
    std::size_t invalid_tests {};
    std::string line;
    int current_precision = 16;
    bool skip = false;
    BOOST_DECIMAL_ATTRIBUTE_UNUSED unsigned skip_counter {};
    BOOST_DECIMAL_ATTRIBUTE_UNUSED unsigned total_skipped_tests {};

    while (std::getline(in, line))
    {
        // Skip commented lines
        if (line.find("#") != std::string::npos)
        {
            continue;
        }

        // Check for precision specification
        // When the precision is specified we ust that one until it is specified again
        // Some of these test sets assume precisions we don't offer so this is the best effort
        auto precision_pos = line.find("precision:");
        if (precision_pos != std::string::npos)
        {
            auto precision_start = precision_pos + 10; // Skip "precision:"

            // Skip whitespace
            while (precision_start < line.length() && std::isspace(line[precision_start]))
            {
                precision_start++;
            }

            // Extract precision value
            std::string precision_str;
            while (precision_start < line.length() && std::isdigit(line[precision_start]))
            {
                precision_str += line[precision_start++];
            }

            if (!precision_str.empty())
            {
                current_precision = std::stoi(precision_str);
            }
            continue;
        }

        // Check for rounding mode changes
        BOOST_DECIMAL_IF_CONSTEXPR (allow_rounding_changes)
        {
            auto rounding_pos = line.find("rounding:");
            if (rounding_pos != std::string::npos)
            {
                auto rounding_start = rounding_pos + 10;
                auto rounding_end = line.length();

                while (rounding_start < line.length() && std::isspace(line[rounding_start]))
                {
                    rounding_start++;
                }

                // Extract the rounding mode
                const std::string rounding_str {line.substr(rounding_start, rounding_end - rounding_start - 1u)};

                if (rounding_str == "floor" || rounding_str == "down")
                {
                    boost::decimal::fesetround(boost::decimal::rounding_mode::fe_dec_downward);
                    skip = false;
                }
                else if (rounding_str == "ceiling" || rounding_str == "up")
                {
                    boost::decimal::fesetround(boost::decimal::rounding_mode::fe_dec_upward);
                    skip = false;
                }
                else if (rounding_str == "half_up")
                {
                    boost::decimal::fesetround(boost::decimal::rounding_mode::fe_dec_to_nearest_from_zero);
                    skip = false;
                }
                else if (rounding_str == "half_even")
                {
                    boost::decimal::fesetround(boost::decimal::rounding_mode::fe_dec_to_nearest);
                    skip = false;
                }
                else
                {
                    std::cerr << "\nInvalid rounding mode: " << rounding_str << std::endl;
                    skip = true;
                }

                if (!skip && skip_counter > 0U)
                {
                    std::cerr << "Skipped: " << skip_counter << " due to invalid rounding mode.\n" << std::endl;
                    num_tests_found += skip_counter;
                    total_skipped_tests += skip_counter;
                    skip_counter = 0U;
                }

                continue;
            }
        }

        if (skip)
        {
            // Testing of unsupported rounding modes should be completely skipped
            ++skip_counter;
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
            ++invalid_tests;
            continue;
        }

        // Extract the substring containing both LHS values
        auto operands_start = pos_test + function_name.length() + 1;
        auto operands_end = arrow_pos;

        // Trim trailing whitespace
        while (operands_end > operands_start && std::isspace(line[operands_end - 1]))
        {
            operands_end--;
        }

        std::string operands_str = line.substr(operands_start, operands_end - operands_start);

        // Parse the two operands from the string
        std::string lhs1_value, lhs2_value;
        std::size_t i = 0;

        // Skip leading whitespace
        while (i < operands_str.length() && std::isspace(operands_str[i]))
        {
            i++;
        }

        // Parse first operand
        if (i < operands_str.length() && operands_str[i] == '\'')
        {
            // Quoted value
            i++; // Skip opening quote
            std::size_t quote_end = operands_str.find('\'', i);
            if (quote_end != std::string::npos)
            {
                lhs1_value = operands_str.substr(i, quote_end - i);
                i = quote_end + 1;
            }
        }
        else
        {
            // Unquoted value
            std::size_t start = i;
            while (i < operands_str.length() && !std::isspace(operands_str[i]))
            {
                i++;
            }
            lhs1_value = operands_str.substr(start, i - start);
        }

        // Skip whitespace between operands
        while (i < operands_str.length() && std::isspace(operands_str[i]))
        {
            i++;
        }

        // Parse second operand
        if (i < operands_str.length() && operands_str[i] == '\'')
        {
            // Quoted value
            i++; // Skip opening quote
            std::size_t quote_end = operands_str.find('\'', i);
            if (quote_end != std::string::npos)
            {
                lhs2_value = operands_str.substr(i, quote_end - i);
                i = quote_end + 1;
            }
        }
        else
        {
            // Unquoted value
            std::size_t start = i;
            while (i < operands_str.length() && !std::isspace(operands_str[i]))
            {
                i++;
            }
            lhs2_value = operands_str.substr(start, i - start);
        }

        // Extract RHS value (after ->)
        auto rhs_start = arrow_pos + 2; // Skip "->"
        auto rhs_end = line.length();

        // Trim whitespace
        while (rhs_start < rhs_end && std::isspace(line[rhs_start]))
        {
            rhs_start++;
        }
        while (rhs_end > rhs_start && std::isspace(line[rhs_end - 1]))
        {
            rhs_end--;
        }

        std::string rhs_value = line.substr(rhs_start, rhs_end - rhs_start);

        // Remove quotes if present
        if (!rhs_value.empty() && rhs_value.front() == '\'' && rhs_value.back() == '\'')
        {
            rhs_value = rhs_value.substr(1, rhs_value.length() - 2);
        }

        // Select appropriate decimal type based on precision
        try
        {
            if (current_precision <= 9)
            {
                // Use decimal32_t
                const boost::decimal::decimal32_t lhs1 {lhs1_value};
                const boost::decimal::decimal32_t lhs2 {lhs2_value};
                const boost::decimal::decimal32_t rhs {rhs_value};
                const auto f_result {f(lhs1, lhs2)};  // Generic lambda works here

                if ((isnan(lhs1) && isnan(lhs2)) || isnan(rhs))
                {
                    std::uint32_t result_bits;
                    std::memcpy(&result_bits, &f_result, sizeof(std::uint32_t));

                    std::uint32_t rhs_bits;
                    std::memcpy(&rhs_bits, &rhs, sizeof(std::uint32_t));

                    if (!BOOST_TEST_EQ(result_bits, rhs_bits))
                    {
                        std::cerr << "Failed test: " << test_name << " (precision: " << current_precision << ")" << std::endl;
                    }
                }
                else if (ulp_tol != 0)
                {
                    const auto dist {ulp_distance(f_result, rhs, ulp_tol)};
                    if (!BOOST_TEST_LE(dist, ulp_tol))
                    {
                        std::cerr << "Failed test: " << test_name << " (precision: " << current_precision << ")" << "\n"
                                  << "Got: " << f_result << "\nExpected: " << rhs;
                    }
                }
                else if (!BOOST_TEST_EQ(f_result, rhs))  // Generic lambda works here
                {
                    std::cerr << "Failed test: " << test_name << " (precision: " << current_precision << ")" << std::endl;
                }
            }
            else if (current_precision <= 16)
            {
                // Use decimal64_t
                const boost::decimal::decimal64_t lhs1 {lhs1_value};
                const boost::decimal::decimal64_t lhs2 {lhs2_value};
                const boost::decimal::decimal64_t rhs {rhs_value};
                const auto f_result {f(lhs1, lhs2)};

                if ((isnan(lhs1) && isnan(lhs2)) || isnan(rhs))
                {
                    std::uint64_t result_bits;
                    std::memcpy(&result_bits, &f_result, sizeof(std::uint64_t));

                    std::uint64_t rhs_bits;
                    std::memcpy(&rhs_bits, &rhs, sizeof(std::uint64_t));

                    if (!BOOST_TEST_EQ(result_bits, rhs_bits))
                    {
                        std::cerr << "Failed test: " << test_name << " (precision: " << current_precision << ")" << std::endl;
                    }
                }
                else if (ulp_tol != 0)
                {
                    const auto dist {ulp_distance(f_result, rhs, ulp_tol)};
                    if (!BOOST_TEST_LE(dist, ulp_tol))
                    {
                        std::cerr << "Failed test: " << test_name << " (precision: " << current_precision << ")" << "\n"
                                  << "Got: " << f_result << "\nExpected: " << rhs;
                    }
                }
                else if (!BOOST_TEST_EQ(f_result, rhs))
                {
                    std::cerr << "Failed test: " << test_name << " (precision: " << current_precision << ")" << std::endl;
                }
            }
            else
            {
                #ifdef __GNUC__
                #  pragma GCC diagnostic push
                #  pragma GCC diagnostic ignored "-Wclass-memaccess"
                #endif

                // Use decimal128_t
                const boost::decimal::decimal128_t lhs1 {lhs1_value};
                const boost::decimal::decimal128_t lhs2 {lhs2_value};
                const boost::decimal::decimal128_t rhs {rhs_value};
                const auto f_result {f(lhs1, lhs2)};

                if ((isnan(lhs1) && isnan(lhs2)) || isnan(rhs))
                {
                    boost::int128::uint128_t result_bits;
                    std::memcpy(&result_bits, &f_result, sizeof(boost::int128::uint128_t));

                    boost::int128::uint128_t rhs_bits;
                    std::memcpy(&rhs_bits, &rhs, sizeof(boost::int128::uint128_t));

                    if (!BOOST_TEST_EQ(result_bits, rhs_bits))
                    {
                        std::cerr << "Failed test: " << test_name << " (precision: " << current_precision << ")" << std::endl;
                    }
                }
                else if (ulp_tol != 0)
                {
                    const auto dist {ulp_distance(f_result, rhs, ulp_tol)};
                    if (!BOOST_TEST_LE(dist, ulp_tol))
                    {
                        std::cerr << "Failed test: " << test_name << " (precision: " << current_precision << ")" << "\n"
                                  << "Got: " << f_result << "\nExpected: " << rhs;
                    }
                }
                else if (!BOOST_TEST_EQ(f_result, rhs))
                {
                    std::cerr << "Failed test: " << test_name << " (precision: " << current_precision << ")" << std::endl;
                }

                #ifdef __GNUC__
                #  pragma GCC diagnostic pop
                #endif
            }
        }
        catch (...)
        {
            // Invalid construction is supposed to throw
            ++invalid_tests;
        }
    }

    if (skip_counter > 0U)
    {
        std::cerr << "Skipped: " << skip_counter << " due to invalid rounding mode.\n";
        total_skipped_tests += skip_counter;
    }

    std::cerr << "\nTotal number of tests: " << num_tests_found << "\n";
    std::cerr << "Total number of skipped tests: " << total_skipped_tests << "\n" << std::endl;

    BOOST_TEST_GT(num_tests_found, 0U);
    BOOST_TEST_LT(invalid_tests, num_tests_found);
}

inline void test_comparisons(const std::string& file_path, const std::string& function_name)
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
    std::size_t invalid_tests {};
    std::string line;
    int current_precision = 16;

    while (std::getline(in, line))
    {
        // Skip commented lines
        if (line.find("#") != std::string::npos)
        {
            continue;
        }

        // Check for precision specification
        // When the precision is specified we ust that one until it is specified again
        // Some of these test sets assume precisions we don't offer so this is the best effort
        auto precision_pos = line.find("precision:");
        if (precision_pos != std::string::npos)
        {
            auto precision_start = precision_pos + 10; // Skip "precision:"

            // Skip whitespace
            while (precision_start < line.length() && std::isspace(line[precision_start]))
            {
                precision_start++;
            }

            // Extract precision value
            std::string precision_str;
            while (precision_start < line.length() && std::isdigit(line[precision_start]))
            {
                precision_str += line[precision_start++];
            }

            if (!precision_str.empty())
            {
                current_precision = std::stoi(precision_str);
            }
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
            ++invalid_tests;
            continue;
        }

        // Extract the substring containing both LHS values
        auto operands_start = pos_test + function_name.length() + 1;
        auto operands_end = arrow_pos;

        // Trim trailing whitespace
        while (operands_end > operands_start && std::isspace(line[operands_end - 1]))
        {
            operands_end--;
        }

        std::string operands_str = line.substr(operands_start, operands_end - operands_start);

        // Parse the two operands from the string
        std::string lhs1_value, lhs2_value;
        std::size_t i = 0;

        // Skip leading whitespace
        while (i < operands_str.length() && std::isspace(operands_str[i]))
        {
            i++;
        }

        // Parse first operand
        if (i < operands_str.length() && operands_str[i] == '\'')
        {
            // Quoted value
            i++; // Skip opening quote
            std::size_t quote_end = operands_str.find('\'', i);
            if (quote_end != std::string::npos)
            {
                lhs1_value = operands_str.substr(i, quote_end - i);
                i = quote_end + 1;
            }
        }
        else
        {
            // Unquoted value
            std::size_t start = i;
            while (i < operands_str.length() && !std::isspace(operands_str[i]))
            {
                i++;
            }
            lhs1_value = operands_str.substr(start, i - start);
        }

        // Skip whitespace between operands
        while (i < operands_str.length() && std::isspace(operands_str[i]))
        {
            i++;
        }

        // Parse second operand
        if (i < operands_str.length() && operands_str[i] == '\'')
        {
            // Quoted value
            i++; // Skip opening quote
            std::size_t quote_end = operands_str.find('\'', i);
            if (quote_end != std::string::npos)
            {
                lhs2_value = operands_str.substr(i, quote_end - i);
                i = quote_end + 1;
            }
        }
        else
        {
            // Unquoted value
            std::size_t start = i;
            while (i < operands_str.length() && !std::isspace(operands_str[i]))
            {
                i++;
            }
            lhs2_value = operands_str.substr(start, i - start);
        }

        // Extract RHS value (after ->)
        auto rhs_start = arrow_pos + 2; // Skip "->"
        auto rhs_end = line.length();

        // Trim whitespace
        while (rhs_start < rhs_end && std::isspace(line[rhs_start]))
        {
            rhs_start++;
        }
        while (rhs_end > rhs_start && std::isspace(line[rhs_end - 1]))
        {
            rhs_end--;
        }

        std::string rhs_value = line.substr(rhs_start, rhs_end - rhs_start);

        // Remove quotes if present
        if (!rhs_value.empty() && rhs_value.front() == '\'' && rhs_value.back() == '\'')
        {
            rhs_value = rhs_value.substr(1, rhs_value.length() - 2);
        }

        // Select appropriate decimal type based on precision
        try
        {
            const std::string eq = "0";
            const std::string gt = "1";
            const std::string lt = "-1";

            if (current_precision <= 9)
            {
                // Use decimal32_t
                const boost::decimal::decimal32_t lhs1 {lhs1_value};
                const boost::decimal::decimal32_t lhs2 {lhs2_value};

                if (rhs_value == eq)
                {
                    if (!BOOST_TEST_EQ(lhs1, lhs2))
                    {
                        std::cerr << "Failed test: " << test_name << " (precision: " << current_precision << ")" << std::endl;
                    }
                }
                else if (rhs_value == gt)
                {
                    if (!BOOST_TEST_GT(lhs1, lhs2))
                    {
                        std::cerr << "Failed test: " << test_name << " (precision: " << current_precision << ")" << std::endl;
                    }
                }
                else if (rhs_value == lt)
                {
                    if (!BOOST_TEST_LT(lhs1, lhs2))
                    {
                        std::cerr << "Failed test: " << test_name << " (precision: " << current_precision << ")" << std::endl;
                    }
                }
                else
                {
                    throw std::logic_error("Invalid comparison");
                }
            }
            else if (current_precision <= 16)
            {
                // Use decimal64_t
                const boost::decimal::decimal64_t lhs1 {lhs1_value};
                const boost::decimal::decimal64_t lhs2 {lhs2_value};

                if (rhs_value == eq)
                {
                    if (!BOOST_TEST_EQ(lhs1, lhs2))
                    {
                        std::cerr << "Failed test: " << test_name << " (precision: " << current_precision << ")" << std::endl;
                    }
                }
                else if (rhs_value == gt)
                {
                    if (!BOOST_TEST_GT(lhs1, lhs2))
                    {
                        std::cerr << "Failed test: " << test_name << " (precision: " << current_precision << ")" << std::endl;
                    }
                }
                else if (rhs_value == lt)
                {
                    if (!BOOST_TEST_LT(lhs1, lhs2))
                    {
                        std::cerr << "Failed test: " << test_name << " (precision: " << current_precision << ")" << std::endl;
                    }
                }
                else
                {
                    throw std::logic_error("Invalid comparison");
                }
            }
            else
            {
                // Use decimal128_t
                const boost::decimal::decimal128_t lhs1 {lhs1_value};
                const boost::decimal::decimal128_t lhs2 {lhs2_value};

                if (rhs_value == eq)
                {
                    if (!BOOST_TEST_EQ(lhs1, lhs2))
                    {
                        std::cerr << "Failed test: " << test_name << " (precision: " << current_precision << ")" << std::endl;
                    }
                }
                else if (rhs_value == gt)
                {
                    if (!BOOST_TEST_GT(lhs1, lhs2))
                    {
                        std::cerr << "Failed test: " << test_name << " (precision: " << current_precision << ")" << std::endl;
                    }
                }
                else if (rhs_value == lt)
                {
                    if (!BOOST_TEST_LT(lhs1, lhs2))
                    {
                        std::cerr << "Failed test: " << test_name << " (precision: " << current_precision << ")" << std::endl;
                    }
                }
                else
                {
                    throw std::logic_error("Invalid comparison");
                }
            }
        }
        catch (...)
        {
            // Invalid construction is supposed to throw
            ++invalid_tests;
        }
    }

    BOOST_TEST_GT(num_tests_found, 0U);
    BOOST_TEST_LT(invalid_tests, num_tests_found);
}

inline void test_comparetotal(const std::string& file_path, const std::string& function_name)
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
    std::size_t invalid_tests {};
    std::string line;
    int current_precision = 16;

    while (std::getline(in, line))
    {
        // Skip commented lines
        if (line.find("#") != std::string::npos)
        {
            continue;
        }

        // Check for precision specification
        // When the precision is specified we ust that one until it is specified again
        // Some of these test sets assume precisions we don't offer so this is the best effort
        auto precision_pos = line.find("precision:");
        if (precision_pos != std::string::npos)
        {
            auto precision_start = precision_pos + 10; // Skip "precision:"

            // Skip whitespace
            while (precision_start < line.length() && std::isspace(line[precision_start]))
            {
                precision_start++;
            }

            // Extract precision value
            std::string precision_str;
            while (precision_start < line.length() && std::isdigit(line[precision_start]))
            {
                precision_str += line[precision_start++];
            }

            if (!precision_str.empty())
            {
                current_precision = std::stoi(precision_str);
            }
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
            ++invalid_tests;
            continue;
        }

        // Extract the substring containing both LHS values
        auto operands_start = pos_test + function_name.length() + 1;
        auto operands_end = arrow_pos;

        // Trim trailing whitespace
        while (operands_end > operands_start && std::isspace(line[operands_end - 1]))
        {
            operands_end--;
        }

        std::string operands_str = line.substr(operands_start, operands_end - operands_start);

        // Parse the two operands from the string
        std::string lhs1_value, lhs2_value;
        std::size_t i = 0;

        // Skip leading whitespace
        while (i < operands_str.length() && std::isspace(operands_str[i]))
        {
            i++;
        }

        // Parse first operand
        if (i < operands_str.length() && operands_str[i] == '\'')
        {
            // Quoted value
            i++; // Skip opening quote
            std::size_t quote_end = operands_str.find('\'', i);
            if (quote_end != std::string::npos)
            {
                lhs1_value = operands_str.substr(i, quote_end - i);
                i = quote_end + 1;
            }
        }
        else
        {
            // Unquoted value
            std::size_t start = i;
            while (i < operands_str.length() && !std::isspace(operands_str[i]))
            {
                i++;
            }
            lhs1_value = operands_str.substr(start, i - start);
        }

        // Skip whitespace between operands
        while (i < operands_str.length() && std::isspace(operands_str[i]))
        {
            i++;
        }

        // Parse second operand
        if (i < operands_str.length() && operands_str[i] == '\'')
        {
            // Quoted value
            i++; // Skip opening quote
            std::size_t quote_end = operands_str.find('\'', i);
            if (quote_end != std::string::npos)
            {
                lhs2_value = operands_str.substr(i, quote_end - i);
                i = quote_end + 1;
            }
        }
        else
        {
            // Unquoted value
            std::size_t start = i;
            while (i < operands_str.length() && !std::isspace(operands_str[i]))
            {
                i++;
            }
            lhs2_value = operands_str.substr(start, i - start);
        }

        // Extract RHS value (after ->)
        auto rhs_start = arrow_pos + 2; // Skip "->"
        auto rhs_end = line.length();

        // Trim whitespace
        while (rhs_start < rhs_end && std::isspace(line[rhs_start]))
        {
            rhs_start++;
        }
        while (rhs_end > rhs_start && std::isspace(line[rhs_end - 1]))
        {
            rhs_end--;
        }

        std::string rhs_value = line.substr(rhs_start, rhs_end - rhs_start);

        // Remove quotes if present
        if (!rhs_value.empty() && rhs_value.front() == '\'' && rhs_value.back() == '\'')
        {
            rhs_value = rhs_value.substr(1, rhs_value.length() - 2);
        }

        // Select appropriate decimal type based on precision
        try
        {
            const std::string eq = "0";
            const std::string gt = "1";
            const std::string lt = "-1";

            if (current_precision <= 9)
            {
                // Use decimal32_t
                const boost::decimal::decimal32_t lhs1 {lhs1_value};
                const boost::decimal::decimal32_t lhs2 {lhs2_value};

                if (rhs_value == eq)
                {
                    if (!(BOOST_TEST(boost::decimal::comparetotal(lhs1, lhs2)) && BOOST_TEST(boost::decimal::comparetotal(lhs2, lhs1))))
                    {
                        std::cerr << "Failed test: " << test_name << " (precision: " << current_precision << ")" << std::endl;
                    }
                }
                else if (rhs_value == gt)
                {
                    if (!BOOST_TEST(boost::decimal::comparetotal(lhs2, lhs1)))
                    {
                        std::cerr << "Failed test: " << test_name << " (precision: " << current_precision << ")" << std::endl;
                    }
                }
                else if (rhs_value == lt)
                {
                    if (!BOOST_TEST(boost::decimal::comparetotal(lhs1, lhs2)))
                    {
                        std::cerr << "Failed test: " << test_name << " (precision: " << current_precision << ")" << std::endl;
                    }
                }
                else
                {
                    throw std::logic_error("Invalid comparison");
                }
            }
            else if (current_precision <= 16)
            {
                // Use decimal64_t
                const boost::decimal::decimal64_t lhs1 {lhs1_value};
                const boost::decimal::decimal64_t lhs2 {lhs2_value};

                if (rhs_value == eq)
                {
                    if (!(BOOST_TEST(boost::decimal::comparetotal(lhs1, lhs2)) && BOOST_TEST(boost::decimal::comparetotal(lhs2, lhs1))))
                    {
                        std::cerr << "Failed test: " << test_name << " (precision: " << current_precision << ")" << std::endl;
                    }
                }
                else if (rhs_value == gt)
                {
                    if (!BOOST_TEST(boost::decimal::comparetotal(lhs2, lhs1)))
                    {
                        std::cerr << "Failed test: " << test_name << " (precision: " << current_precision << ")" << std::endl;
                    }
                }
                else if (rhs_value == lt)
                {
                    if (!BOOST_TEST(boost::decimal::comparetotal(lhs1, lhs2)))
                    {
                        std::cerr << "Failed test: " << test_name << " (precision: " << current_precision << ")" << std::endl;
                    }
                }
                else
                {
                    throw std::logic_error("Invalid comparison");
                }
            }
            else
            {
                // Use decimal128_t
                const boost::decimal::decimal128_t lhs1 {lhs1_value};
                const boost::decimal::decimal128_t lhs2 {lhs2_value};

                if (rhs_value == eq)
                {
                    if (!(BOOST_TEST(boost::decimal::comparetotal(lhs1, lhs2)) && BOOST_TEST(boost::decimal::comparetotal(lhs2, lhs1))))
                    {
                        std::cerr << "Failed test: " << test_name << " (precision: " << current_precision << ")" << std::endl;
                    }
                }
                else if (rhs_value == gt)
                {
                    if (!BOOST_TEST(boost::decimal::comparetotal(lhs2, lhs1)))
                    {
                        std::cerr << "Failed test: " << test_name << " (precision: " << current_precision << ")" << std::endl;
                    }
                }
                else if (rhs_value == lt)
                {
                    if (!BOOST_TEST(boost::decimal::comparetotal(lhs1, lhs2)))
                    {
                        std::cerr << "Failed test: " << test_name << " (precision: " << current_precision << ")" << std::endl;
                    }
                }
                else
                {
                    throw std::logic_error("Invalid comparison");
                }
            }
        }
        catch (...)
        {
            // Invalid construction is supposed to throw
            ++invalid_tests;
        }
    }

    BOOST_TEST_GT(num_tests_found, 0U);
    BOOST_TEST_LT(invalid_tests, num_tests_found);
}

#endif // BOOST_DECIMAL_DECTEST_TEST_HARNESS_HPP
