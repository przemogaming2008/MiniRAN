#include <limits>
#include <string>

#include "support/test_framework.h"

TEST_CASE(assert_near_passes_for_values_inside_tolerance)
{
    ASSERT_NEAR(1.0, 1.001, 0.01);
}

TEST_CASE(assert_near_reports_values_outside_tolerance)
{
    bool caught = false;

    try {
        ASSERT_NEAR(1.0, 2.0, 0.01);
    } catch (const ::miniran::test::TestFailure& failure) {
        caught = true;
        const std::string message = failure.what();
        ASSERT_TRUE(message.find("ASSERT_NEAR failed") != std::string::npos);
    }

    ASSERT_TRUE(caught);
}

TEST_CASE(assert_near_rejects_nan)
{
    bool caught = false;

    try {
        ASSERT_NEAR(std::numeric_limits<double>::quiet_NaN(), 1.0, 0.01);
    } catch (const ::miniran::test::TestFailure& failure) {
        caught = true;
        const std::string message = failure.what();
        ASSERT_TRUE(message.find("non-finite") != std::string::npos);
    }

    ASSERT_TRUE(caught);
}

TEST_CASE(assert_near_rejects_negative_tolerance)
{
    bool caught = false;

    try {
        ASSERT_NEAR(1.0, 1.0, -0.01);
    } catch (const ::miniran::test::TestFailure& failure) {
        caught = true;
        const std::string message = failure.what();
        ASSERT_TRUE(message.find("negative tolerance") != std::string::npos);
    }

    ASSERT_TRUE(caught);
}