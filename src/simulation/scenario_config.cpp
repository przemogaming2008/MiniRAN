#include "miniran/simulation/scenario_config.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <exception>
#include <limits>

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
std::optional<std::string> requireKey(
    const std::unordered_map<std::string, std::string>& values,
    const std::string& key,
    std::string& error
) {
    const auto it = values.find(key);

    if (it == values.end()) {
        error = "Missing required key: " + key;
        return std::nullopt;
    }

    return it->second;
}
}  // namespace
bool ScenarioConfig::validate(std::string& error) const {
    if (stepMs == 0) {
        error = "step_ms must be greater than 0.";
        return false;
    }

    if (scenarioDurationMs == 0) {
        error = "scenario_duration_ms must be greater than 0.";
        return false;
    }

    if (!linkProfile.isValid()) {
        error = "Invalid link profile.";
        return false;
    }

    if (ueConfigs.empty()) {
        error = "At least one UE must be configured.";
        return false;
    }

    std::unordered_set<std::uint32_t> usedNodeIds;
    std::unordered_set<std::uint32_t> usedUeIds;

    for (std::size_t i = 0; i < ueConfigs.size(); ++i) {
        const auto& ueConfig = ueConfigs[i];

        if (ueConfig.nodeId == 0) {
            error = "UE node_id must be non-zero.";
            return false;
        }

        if (ueConfig.ueId == 0) {
            error = "UE ue_id must be non-zero.";
            return false;
        }

        if (ueConfig.nodeId == accessNodeId) {
            error = "UE node_id must be different from access_node_id.";
            return false;
        }

        if (!usedNodeIds.insert(ueConfig.nodeId).second) {
            error = "Duplicate UE node_id.";
            return false;
        }

        if (!usedUeIds.insert(ueConfig.ueId).second) {
            error = "Duplicate UE ue_id.";
            return false;
        }

        if (ueConfig.trafficEndMs <= ueConfig.trafficStartMs) {
            error = "UE traffic_end_ms must be greater than traffic_start_ms.";
            return false;
        }

        if (!ueConfig.uplinkTrafficProfile.isValid()) {
            error = "Invalid uplink traffic profile.";
            return false;
        }

        if (ueConfig.downlinkEnabled &&
            !ueConfig.downlinkTrafficProfile.isValid()) {
            error = "Invalid downlink traffic profile.";
            return false;
        }
    }

    error.clear();
    return true;
}

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

    auto parseUnsigned = [&](const std::string& key, std::uint64_t& target) -> bool {
        if (const auto value = readValue(values, key)) {
            if (value->empty()) {
                error = "Empty unsigned integer value for key: " + key;
                return false;
            }

            if ((*value)[0] == '-') {
                error = "Negative value is not allowed for key: " + key;
                return false;
            }

            try {
                std::size_t pos = 0;
                const auto parsed = std::stoull(*value, &pos);

                if (pos != value->size()) {
                    error = "Invalid unsigned integer suffix for key: " + key;
                    return false;
                }

                target = parsed;
            } catch (const std::exception&) {
                error = "Invalid unsigned integer value for key: " + key;
                return false;
            }
        }

        return true;
    };
    auto parseUnsigned32 = [&](const std::string& key, std::uint32_t& target) -> bool {
        if (const auto value = readValue(values, key)) {
            if (value->empty()) {
                error = "Empty uint32 value for key: " + key;
                return false;
            }

            if ((*value)[0] == '-') {
                error = "Negative value is not allowed for key: " + key;
                return false;
            }

            try {
                std::size_t pos = 0;
                const auto parsed = std::stoull(*value, &pos);

                if (pos != value->size()) {
                    error = "Invalid uint32 suffix for key: " + key;
                    return false;
                }

                if (parsed > std::numeric_limits<std::uint32_t>::max()) {
                    error = "Value is too large for uint32 key: " + key;
                    return false;
                }

                target = static_cast<std::uint32_t>(parsed);
            } catch (const std::exception&) {
                error = "Invalid uint32 value for key: " + key;
                return false;
            }
        }

        return true;
    };
    auto parseSize = [&](const std::string& key, std::size_t& target) -> bool {
        if (const auto value = readValue(values, key)) {
            if (value->empty()) {
                error = "Empty size value for key: " + key;
                return false;
            }

            if ((*value)[0] == '-') {
                error = "Negative value is not allowed for key: " + key;
                return false;
            }

            try {
                std::size_t pos = 0;
                const auto parsed = std::stoull(*value, &pos);

                if (pos != value->size()) {
                    error = "Invalid size suffix for key: " + key;
                    return false;
                }

                if (parsed > std::numeric_limits<std::size_t>::max()) {
                    error = "Value is too large for size key: " + key;
                    return false;
                }

                target = static_cast<std::size_t>(parsed);
            } catch (const std::exception&) {
                error = "Invalid size value for key: " + key;
                return false;
            }
        }

        return true;
    };
    auto parseDouble = [&](const std::string& key, double& target) -> bool {
        if (const auto value = readValue(values, key)) {
            if (value->empty()) {
                error = "Empty double value for key: " + key;
                return false;
            }

            try {
                std::size_t pos = 0;
                const auto parsed = std::stod(*value, &pos);

                if (pos != value->size()) {
                    error = "Invalid double suffix for key: " + key;
                    return false;
                }

                target = parsed;
            } catch (const std::exception&) {
                error = "Invalid double value for key: " + key;
                return false;
            }
        }

        return true;
    };
    auto parseBool = [&](const std::string& key, bool& target) -> bool {
        if (const auto value = readValue(values, key)) {
            if (*value == "true" || *value == "1" || *value == "yes") {
                target = true;
            } else if (*value == "false" || *value == "0" || *value == "no") {
                target = false;
            } else {
                error = "Invalid bool value for key: " + key;
                return false;
            }
        }
        return true;
    };
    auto parseTraffic = [&](const std::string& prefix, TrafficProfile& profile) -> bool {
        if (const auto value = readValue(values, prefix + "traffic_pattern")) {
            const auto parsed = parseTrafficPattern(*value);
            if (!parsed) {
                error = "Invalid traffic_pattern: " + *value;
                return false;
            }
            profile.pattern = *parsed;
        }

        if (!parseSize(prefix + "packet_size_bytes", profile.packetSizeBytes)) return false;
        if (!parseUnsigned32(prefix + "packets_per_second", profile.packetsPerSecond)) return false;
        if (!parseUnsigned32(prefix + "burst_packets", profile.burstPackets)) return false;
        if (!parseUnsigned32(prefix + "burst_interval_ms", profile.burstIntervalMs)) return false;
        if (!parseUnsigned32(prefix + "ramp_start_pps", profile.rampStartPps)) return false;
        if (!parseUnsigned32(prefix + "ramp_end_pps", profile.rampEndPps)) return false;

        if (!profile.isValid()) {
            error = "Invalid TrafficProfile values for prefix: " + prefix;
            return false;
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
     if (!parseUnsigned32("access_node_id", config.accessNodeId)) return std::nullopt;
    if (!parseUnsigned("step_ms", config.stepMs)) return std::nullopt;
    if (!parseUnsigned("scenario_duration_ms", config.scenarioDurationMs)) return std::nullopt;

    if (!parseUnsigned32("network_seed", config.networkSeed)) return std::nullopt;
    if (!parseUnsigned32("traffic_seed", config.trafficSeed)) return std::nullopt;

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

    if (!config.linkProfile.isValid()) {
        error = "Invalid LinkProfile values.";
        return std::nullopt;
    }

    if (config.stepMs == 0) {
        error = "step_ms must be > 0.";
        return std::nullopt;
    }

    if (config.scenarioDurationMs == 0) {
        error = "scenario_duration_ms must be > 0.";
        return std::nullopt;
    }

    std::uint32_t ueCount = 1;
    if (!parseUnsigned32("ue_count", ueCount)) return std::nullopt;

    if (ueCount == 0) {
        error = "ue_count must be > 0.";
        return std::nullopt;
    }

    config.ueConfigs.clear();

    std::unordered_set<std::uint32_t> usedNodeIds;
    std::unordered_set<std::uint32_t> usedUeIds;

    for (std::uint32_t i = 0; i < ueCount; ++i) {
        const std::string prefix = "ue." + std::to_string(i) + ".";

        UeConfig ueConfig;

        if (!requireKey(values, prefix + "node_id", error)) {
            return std::nullopt;
        }

        if (!requireKey(values, prefix + "ue_id", error)) {
            return std::nullopt;
        }

        if (!parseUnsigned32(prefix + "node_id", ueConfig.nodeId)) return std::nullopt;
        if (!parseUnsigned32(prefix + "ue_id", ueConfig.ueId)) return std::nullopt;
        if (!parseUnsigned(prefix + "attach_start_ms", ueConfig.attachStartMs)) return std::nullopt;
        if (!parseUnsigned(prefix + "traffic_start_ms", ueConfig.trafficStartMs)) return std::nullopt;
        if (!parseUnsigned(prefix + "traffic_end_ms", ueConfig.trafficEndMs)) return std::nullopt;

        if (ueConfig.nodeId == config.accessNodeId) {
            error = "UE node_id cannot be equal to access_node_id.";
            return std::nullopt;
        }

        if (!usedNodeIds.insert(ueConfig.nodeId).second) {
            error = "Duplicate UE node_id.";
            return std::nullopt;
        }

        if (!usedUeIds.insert(ueConfig.ueId).second) {
            error = "Duplicate ue_id.";
            return std::nullopt;
        }

        if (ueConfig.trafficEndMs <= ueConfig.trafficStartMs) {
            error = "traffic_end_ms must be greater than traffic_start_ms.";
            return std::nullopt;
        }

        if (ueConfig.attachStartMs > config.scenarioDurationMs ||
            ueConfig.trafficStartMs > config.scenarioDurationMs ||
            ueConfig.trafficEndMs > config.scenarioDurationMs) {
            error = "UE timing exceeds scenario_duration_ms.";
            return std::nullopt;
        }

        ueConfig.uplinkTrafficProfile.durationMs = ueConfig.trafficEndMs - ueConfig.trafficStartMs;

        if (!parseTraffic(prefix + "uplink.", ueConfig.uplinkTrafficProfile)) {
            return std::nullopt;
        }

        if (!parseBool(prefix + "downlink_enabled", ueConfig.downlinkEnabled)) {
            return std::nullopt;
        }

        if (ueConfig.downlinkEnabled) {
            ueConfig.downlinkTrafficProfile.durationMs = ueConfig.trafficEndMs - ueConfig.trafficStartMs;

            if (!parseTraffic(prefix + "downlink.", ueConfig.downlinkTrafficProfile)) {
                return std::nullopt;
            }
        }

        config.ueConfigs.push_back(ueConfig);
    }

    error.clear();
    if (!config.validate(error)) {
        return std::nullopt;
    }

    return config;
}

}  // namespace miniran
