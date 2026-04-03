#include "miniran/transport/link_profile.h"
#include "support/test_framework.h"

using namespace miniran;

TEST_CASE(link_profile_validation_accepts_reasonable_values) {
    LinkProfile profile;
    profile.bandwidthKbps = 1000;
    profile.lossPercent = 2.5;
    profile.reorderPercent = 10.0;
    profile.queueLimitPackets = 64;

    ASSERT_TRUE(profile.isValid());
}

TEST_CASE(link_profile_validation_rejects_invalid_values) {
    LinkProfile profile;
    profile.bandwidthKbps = 0;
    profile.lossPercent = 101.0;
    ASSERT_TRUE(!profile.isValid());
}
