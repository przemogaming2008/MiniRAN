#include "miniran/simulation/scenario_runner.h"
#include "support/test_framework.h"

using namespace miniran;

TEST_CASE(component_rejects_duplicate_ue_node_id) {
    ScenarioConfig config;
    config.scenarioName = "component_duplicate_node_id";

    config.ueConfigs.resize(2);

    config.ueConfigs[0].nodeId = 7;
    config.ueConfigs[0].ueId = 7;

    config.ueConfigs[1].nodeId = 7;
    config.ueConfigs[1].ueId = 8;

    ScenarioRunner runner(config);
    const auto result = runner.run();

    ASSERT_TRUE(!result.succeeded());
    ASSERT_TRUE(!result.notes.empty());
    ASSERT_EQ(result.activeSessionsAtEnd, 0U);
}

TEST_CASE(component_rejects_duplicate_ue_id) {
    ScenarioConfig config;
    config.scenarioName = "component_duplicate_ue_id";

    config.ueConfigs.resize(2);

    config.ueConfigs[0].nodeId = 7;
    config.ueConfigs[0].ueId = 7;

    config.ueConfigs[1].nodeId = 8;
    config.ueConfigs[1].ueId = 7;

    ScenarioRunner runner(config);
    const auto result = runner.run();

    ASSERT_TRUE(!result.succeeded());
    ASSERT_TRUE(!result.notes.empty());
    ASSERT_EQ(result.activeSessionsAtEnd, 0U);
}

TEST_CASE(component_rejects_ue_node_id_equal_to_access_node_id) {
    ScenarioConfig config;
    config.scenarioName = "component_node_id_collides_with_access_node";

    config.accessNodeId = 1000;
    config.ueConfigs.resize(1);

    config.ueConfigs[0].nodeId = 1000;
    config.ueConfigs[0].ueId = 7;

    ScenarioRunner runner(config);
    const auto result = runner.run();

    ASSERT_TRUE(!result.succeeded());
    ASSERT_TRUE(!result.notes.empty());
    ASSERT_EQ(result.activeSessionsAtEnd, 0U);
}