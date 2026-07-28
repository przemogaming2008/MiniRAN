#include "miniran/simulation/scenario_config.h"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <cctype>
#include <fstream>
#include <limits>
#include <sstream>
#include <system_error>
#include <unordered_map>
#include <unordered_set>

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

const std::unordered_set<std::string>& knownConfigKeys() {
    static const std::unordered_set<std::string> keys = {
        "scenario_name",
        "transport_mode",
        "traffic_pattern",

        "ue_id",
        "access_node_id",
        "step_ms",
        "attach_phase_budget_ms",
        "detach_phase_budget_ms",

        "attach_timeout_ms",
        "detach_timeout_ms",
        "heartbeat_interval_ms",
        "inactivity_timeout_ms",
        "max_attach_retries",
        "max_detach_retries",

        "latency_ms",
        "jitter_ms",
        "loss_percent",
        "reorder_percent",
        "bandwidth_kbps",
        "queue_limit_packets",

        "traffic_duration_ms",
        "packet_size_bytes",
        "packets_per_second",
        "burst_packets",
        "burst_interval_ms",
        "ramp_start_pps",
        "ramp_end_pps"
    };

    return keys;
}

bool parseUInt64Strict(const std::string& text, std::uint64_t& out) {
    if (text.empty()) {
        return false;
    }

    if (text.front() == '-' || text.front() == '+') {
        return false;
    }

    std::uint64_t value = 0;
    const char* begin = text.data();
    const char* end = text.data() + text.size();

    const auto [ptr, ec] = std::from_chars(begin, end, value, 10);

    if (ec != std::errc{} || ptr != end) {
        return false;
    }

    out = value;
    return true;
}

bool parseUInt32Strict(const std::string& text, std::uint32_t& out) {
    std::uint64_t value = 0;

    if (!parseUInt64Strict(text, value)) {
        return false;
    }

    if (value > std::numeric_limits<std::uint32_t>::max()) {
        return false;
    }

    out = static_cast<std::uint32_t>(value);
    return true;
}

bool parseSizeStrict(const std::string& text, std::size_t& out) {
    std::uint64_t value = 0;

    if (!parseUInt64Strict(text, value)) {
        return false;
    }

    if (value > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
        return false;
    }

    out = static_cast<std::size_t>(value);
    return true;
}

bool parseDoubleStrict(const std::string& text, double& out) {
    if (text.empty()) {
        return false;
    }

    if (text.front() == '-' || text.front() == '+') {
        return false;
    }

    double value = 0.0;
    const char* begin = text.data();
    const char* end = text.data() + text.size();

    const auto [ptr, ec] = std::from_chars(begin, end, value);

    if (ec != std::errc{} || ptr != end) {
        return false;
    }

    if (!std::isfinite(value)) {
        return false;
    }

    out = value;
    return true;
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
    std::size_t lineNumber = 0;

    while (std::getline(input, line)) {
        ++lineNumber;

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
            error = "Invalid config line " + std::to_string(lineNumber) + ": " + line;
            return std::nullopt;
        }

        const std::string key = trim(line.substr(0, separatorPos));
        const std::string value = trim(line.substr(separatorPos + 1));

        if (key.empty()) {
            error = "Empty config key at line " + std::to_string(lineNumber);
            return std::nullopt;
        }

        if (knownConfigKeys().find(key) == knownConfigKeys().end()) {
            error = "Unknown config key at line " + std::to_string(lineNumber) + ": " + key;
            return std::nullopt;
        }

        if (values.find(key) != values.end()) {
            error = "Duplicate config key at line " + std::to_string(lineNumber) + ": " + key;
            return std::nullopt;
        }

        values[key] = value;
    }

    auto parseUnsigned = [&](const std::string& key, std::uint64_t& target) -> bool {
        if (const auto value = readValue(values, key)) {
            if (!parseUInt64Strict(*value, target)) {
                error = "Invalid unsigned integer value for key: " + key + ", value: " + *value;
                return false;
            }
        }
        return true;
    };

    auto parseUnsigned32 = [&](const std::string& key, std::uint32_t& target) -> bool {
        if (const auto value = readValue(values, key)) {
            if (!parseUInt32Strict(*value, target)) {
                error = "Invalid uint32 value for key: " + key + ", value: " + *value;
                return false;
            }
        }
        return true;
    };

    auto parseSize = [&](const std::string& key, std::size_t& target) -> bool {
        if (const auto value = readValue(values, key)) {
            if (!parseSizeStrict(*value, target)) {
                error = "Invalid size value for key: " + key + ", value: " + *value;
                return false;
            }
        }
        return true;
    };

    auto parseDouble = [&](const std::string& key, double& target) -> bool {
        if (const auto value = readValue(values, key)) {
            if (!parseDoubleStrict(*value, target)) {
                error = "Invalid double value for key: " + key + ", value: " + *value;
                return false;
            }
        }
        return true;
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

    if (!parseUnsigned32("ue_id", config.ueId)) return std::nullopt;
    if (!parseUnsigned32("access_node_id", config.accessNodeId)) return std::nullopt;
    if (!parseUnsigned("step_ms", config.stepMs)) return std::nullopt;
    if (!parseUnsigned("attach_phase_budget_ms", config.attachPhaseBudgetMs)) return std::nullopt;
    if (!parseUnsigned("detach_phase_budget_ms", config.detachPhaseBudgetMs)) return std::nullopt;

    if (!parseUnsigned32("attach_timeout_ms", config.timers.attachTimeoutMs)) return std::nullopt;
    if (!parseUnsigned32("detach_timeout_ms", config.timers.detachTimeoutMs)) return std::nullopt;
    if (!parseUnsigned32("heartbeat_interval_ms", config.timers.heartbeatIntervalMs)) return std::nullopt;
    if (!parseUnsigned32("inactivity_timeout_ms", config.timers.inactivityTimeoutMs)) return std::nullopt;
    if (!parseUnsigned32("max_attach_retries", config.timers.maxAttachRetries)) return std::nullopt;
    if (!parseUnsigned32("max_detach_retries", config.timers.maxDetachRetries)) return std::nullopt;

    if (!parseUnsigned32("latency_ms", config.linkProfile.latencyMs)) return std::nullopt;
    if (!parseUnsigned32("jitter_ms", config.linkProfile.jitterMs)) return std::nullopt;
    if (!parseDouble("loss_percent", config.linkProfile.lossPercent)) return std::nullopt;
    if (!parseDouble("reorder_percent", config.linkProfile.reorderPercent)) return std::nullopt;
    if (!parseUnsigned("bandwidth_kbps", config.linkProfile.bandwidthKbps)) return std::nullopt;
    if (!parseSize("queue_limit_packets", config.linkProfile.queueLimitPackets)) return std::nullopt;

    if (!parseUnsigned("traffic_duration_ms", config.trafficProfile.durationMs)) return std::nullopt;
    if (!parseSize("packet_size_bytes", config.trafficProfile.packetSizeBytes)) return std::nullopt;
    if (!parseUnsigned32("packets_per_second", config.trafficProfile.packetsPerSecond)) return std::nullopt;
    if (!parseUnsigned32("burst_packets", config.trafficProfile.burstPackets)) return std::nullopt;
    if (!parseUnsigned32("burst_interval_ms", config.trafficProfile.burstIntervalMs)) return std::nullopt;
    if (!parseUnsigned32("ramp_start_pps", config.trafficProfile.rampStartPps)) return std::nullopt;
    if (!parseUnsigned32("ramp_end_pps", config.trafficProfile.rampEndPps)) return std::nullopt;

    constexpr std::uint64_t maxReasonableDurationMs = 24ull * 60ull * 60ull * 1000ull;
    constexpr std::uint64_t maxReasonableStepMs = 60ull * 1000ull;
    constexpr std::uint32_t maxReasonableRetries = 1000u;
    constexpr std::size_t maxReasonablePacketSizeBytes = 1024ull * 1024ull;
    constexpr std::size_t maxReasonableQueueLimitPackets = 1'000'000ull;

    if (config.ueId == 0) {
        error = "ue_id must be > 0.";
        return std::nullopt;
    }

    if (config.accessNodeId == 0) {
        error = "access_node_id must be > 0.";
        return std::nullopt;
    }

    if (config.ueId == config.accessNodeId) {
        error = "ue_id and access_node_id must be different.";
        return std::nullopt;
    }

    if (config.stepMs == 0) {
        error = "step_ms must be > 0.";
        return std::nullopt;
    }

    if (config.stepMs > maxReasonableStepMs) {
        error = "step_ms is unreasonably large.";
        return std::nullopt;
    }

    if (config.attachPhaseBudgetMs == 0) {
        error = "attach_phase_budget_ms must be > 0.";
        return std::nullopt;
    }

    if (config.detachPhaseBudgetMs == 0) {
        error = "detach_phase_budget_ms must be > 0.";
        return std::nullopt;
    }

    if (config.attachPhaseBudgetMs > maxReasonableDurationMs) {
        error = "attach_phase_budget_ms is unreasonably large.";
        return std::nullopt;
    }

    if (config.detachPhaseBudgetMs > maxReasonableDurationMs) {
        error = "detach_phase_budget_ms is unreasonably large.";
        return std::nullopt;
    }

    if (config.timers.attachTimeoutMs == 0) {
        error = "attach_timeout_ms must be > 0.";
        return std::nullopt;
    }

    if (config.timers.detachTimeoutMs == 0) {
        error = "detach_timeout_ms must be > 0.";
        return std::nullopt;
    }

    if (config.timers.heartbeatIntervalMs == 0) {
        error = "heartbeat_interval_ms must be > 0.";
        return std::nullopt;
    }

    if (config.timers.inactivityTimeoutMs == 0) {
        error = "inactivity_timeout_ms must be > 0.";
        return std::nullopt;
    }

    if (config.timers.maxAttachRetries > maxReasonableRetries) {
        error = "max_attach_retries is unreasonably large.";
        return std::nullopt;
    }

    if (config.timers.maxDetachRetries > maxReasonableRetries) {
        error = "max_detach_retries is unreasonably large.";
        return std::nullopt;
    }

    if (config.trafficProfile.durationMs > maxReasonableDurationMs) {
        error = "traffic_duration_ms is unreasonably large.";
        return std::nullopt;
    }

    if (config.trafficProfile.packetSizeBytes > maxReasonablePacketSizeBytes) {
        error = "packet_size_bytes is unreasonably large.";
        return std::nullopt;
    }

    if (config.linkProfile.queueLimitPackets > maxReasonableQueueLimitPackets) {
        error = "queue_limit_packets is unreasonably large.";
        return std::nullopt;
    }

    if (!config.linkProfile.isValid()) {
        error = "Invalid LinkProfile values.";
        return std::nullopt;
    }

    if (!config.trafficProfile.isValid()) {
        error = "Invalid TrafficProfile values.";
        return std::nullopt;
    }

    error.clear();
    return config;
}

}  // namespace miniran