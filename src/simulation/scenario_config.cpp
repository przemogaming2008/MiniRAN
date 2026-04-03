#include "miniran/simulation/scenario_config.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <unordered_map>

namespace miniran {

namespace {

std::string trim(std::string value) {
    auto notSpace = [](unsigned char ch) { return !std::isspace(ch); };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), notSpace));
    value.erase(std::find_if(value.rbegin(), value.rend(), notSpace).base(), value.end());
    return value;
}

std::optional<std::string> readValue(const std::unordered_map<std::string, std::string>& values, const std::string& key) {
    const auto it = values.find(key);
    if (it == values.end()) {
        return std::nullopt;
    }
    return it->second;
}

}  // namespace

std::optional<ScenarioConfig> ScenarioConfig::fromFile(const std::string& path, std::string& error) {
    std::ifstream input(path);
    if (!input) {
        error = "Cannot open config file: " + path;
        return std::nullopt;
    }

    std::unordered_map<std::string, std::string> values;
    std::string line;
    while (std::getline(input, line)) {
        const auto commentPos = line.find('#');
        if (commentPos != std::string::npos) {
            line = line.substr(0, commentPos);
        }
        line = trim(line);
        if (line.empty()) {
            continue;
        }
        const auto separatorPos = line.find('=');
        if (separatorPos == std::string::npos) {
            error = "Invalid config line: " + line;
            return std::nullopt;
        }
        const std::string key = trim(line.substr(0, separatorPos));
        const std::string value = trim(line.substr(separatorPos + 1));
        values[key] = value;
    }

    auto parseUnsigned = [&](const std::string& key, std::uint64_t& target) {
        if (const auto value = readValue(values, key)) {
            target = std::stoull(*value);
        }
    };
    auto parseUnsigned32 = [&](const std::string& key, std::uint32_t& target) {
        if (const auto value = readValue(values, key)) {
            target = static_cast<std::uint32_t>(std::stoul(*value));
        }
    };
    auto parseSize = [&](const std::string& key, std::size_t& target) {
        if (const auto value = readValue(values, key)) {
            target = static_cast<std::size_t>(std::stoull(*value));
        }
    };
    auto parseDouble = [&](const std::string& key, double& target) {
        if (const auto value = readValue(values, key)) {
            target = std::stod(*value);
        }
    };

    ScenarioConfig config;

    if (const auto value = readValue(values, "scenario_name")) {
        config.scenarioName = *value;
    }
    if (const auto value = readValue(values, "transport_mode")) {
        const auto parsed = parseTransportMode(*value);
        if (!parsed) {
            error = "Invalid transport_mode: " + *value;
            return std::nullopt;
        }
        config.transportMode = *parsed;
        config.linkProfile.mode = *parsed;
    }
    if (const auto value = readValue(values, "traffic_pattern")) {
        const auto parsed = parseTrafficPattern(*value);
        if (!parsed) {
            error = "Invalid traffic_pattern: " + *value;
            return std::nullopt;
        }
        config.trafficProfile.pattern = *parsed;
    }

    parseUnsigned32("ue_id", config.ueId);
    parseUnsigned32("access_node_id", config.accessNodeId);
    parseUnsigned("step_ms", config.stepMs);
    parseUnsigned("attach_phase_budget_ms", config.attachPhaseBudgetMs);
    parseUnsigned("detach_phase_budget_ms", config.detachPhaseBudgetMs);

    parseUnsigned32("attach_timeout_ms", config.timers.attachTimeoutMs);
    parseUnsigned32("detach_timeout_ms", config.timers.detachTimeoutMs);
    parseUnsigned32("heartbeat_interval_ms", config.timers.heartbeatIntervalMs);
    parseUnsigned32("inactivity_timeout_ms", config.timers.inactivityTimeoutMs);
    parseUnsigned32("max_attach_retries", config.timers.maxAttachRetries);
    parseUnsigned32("max_detach_retries", config.timers.maxDetachRetries);

    parseUnsigned32("latency_ms", config.linkProfile.latencyMs);
    parseUnsigned32("jitter_ms", config.linkProfile.jitterMs);
    parseDouble("loss_percent", config.linkProfile.lossPercent);
    parseDouble("reorder_percent", config.linkProfile.reorderPercent);
    if (const auto value = readValue(values, "bandwidth_kbps")) {
        config.linkProfile.bandwidthKbps = std::stoull(*value);
    }
    parseSize("queue_limit_packets", config.linkProfile.queueLimitPackets);

    parseUnsigned("traffic_duration_ms", config.trafficProfile.durationMs);
    parseSize("packet_size_bytes", config.trafficProfile.packetSizeBytes);
    parseUnsigned32("packets_per_second", config.trafficProfile.packetsPerSecond);
    parseUnsigned32("burst_packets", config.trafficProfile.burstPackets);
    parseUnsigned32("burst_interval_ms", config.trafficProfile.burstIntervalMs);
    parseUnsigned32("ramp_start_pps", config.trafficProfile.rampStartPps);
    parseUnsigned32("ramp_end_pps", config.trafficProfile.rampEndPps);

    if (!config.linkProfile.isValid()) {
        error = "Invalid LinkProfile values.";
        return std::nullopt;
    }
    if (!config.trafficProfile.isValid()) {
        error = "Invalid TrafficProfile values.";
        return std::nullopt;
    }
    if (config.stepMs == 0) {
        error = "step_ms must be > 0.";
        return std::nullopt;
    }

    error.clear();
    return config;
}

}  // namespace miniran
