#include <unity.h>
#include "utils/utils.hpp"

TEST_CASE("transform_range_single", "[module]")
{

    TEST_ASSERT_EQUAL(0, utils::transform_range(0, 100, 0, 10, 0));
    TEST_ASSERT_EQUAL(5, utils::transform_range(0, 100, 0, 10, 50));
    TEST_ASSERT_EQUAL(10, utils::transform_range(0, 100, 0, 10, 100));

    TEST_ASSERT_EQUAL(0, utils::transform_range(50, 100, 0, 10, 50));
    TEST_ASSERT_EQUAL(0, utils::transform_range(50, 100, 0, 10, 5));
    TEST_ASSERT_EQUAL(10, utils::transform_range(50, 100, 0, 10, 115));
}

TEST_CASE("transform_range_multi", "[module]")
{
    std::vector<std::pair<uint16_t, uint8_t>> ranges;
    ranges.push_back({500, 2});
    ranges.push_back({700, 5});
    ranges.push_back({2000, 10});

    TEST_ASSERT_EQUAL(2, utils::transform_ranges(ranges, (uint16_t)50));
    TEST_ASSERT_EQUAL(2, utils::transform_ranges(ranges, (uint16_t)500));
    TEST_ASSERT_EQUAL(5, utils::transform_ranges(ranges, (uint16_t)700));
    TEST_ASSERT_EQUAL(10, utils::transform_ranges(ranges, (uint16_t)2000));
    TEST_ASSERT_EQUAL(10, utils::transform_ranges(ranges, (uint16_t)2001));

    TEST_ASSERT_EQUAL(3, utils::transform_ranges(ranges, (uint16_t)600));
    TEST_ASSERT_EQUAL(6, utils::transform_ranges(ranges, (uint16_t)1000));
}

void do_tests_utils()
{
    UNITY_BEGIN();
    unity_run_all_tests();
    UNITY_END();
}