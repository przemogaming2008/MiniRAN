#include "miniran/simulation/scenario_runner.h"

#include <utility>

#include "miniran/nodes/access_node.h"
#include "miniran/nodes/ue.h"
#include "miniran/traffic/traffic_generator.h"
#include "miniran/transport/virtual_network.h"

namespace miniran {

namespace {

void submitOutgoing(VirtualNetwork& network, std::vector<Datagram> datagrams, std::uint64_t nowMs) {
    for (auto& datagram : datagrams) {
        network.submit(std::move(datagram), nowMs);
    }
}

void deliverReady(VirtualNetwork& network, std::vector<Ue>& ues, AccessNode& accessNode, std::uint64_t nowMs) {
    auto ready = network.pollReady(nowMs);

    for (const auto& datagram : ready) {
        if (datagram.toNodeId == accessNode.nodeId()) {
            accessNode.onDatagram(datagram, nowMs);
            continue;
        }

        for (auto& ue : ues) {
            if (datagram.toNodeId == ue.nodeId()) {
                ue.onDatagram(datagram, nowMs);
                break;
            }
        }
    }
}

void submitOutgoingFromUes(VirtualNetwork& network, std::vector<Ue>& ues, std::uint64_t nowMs) {
    for (auto& ue : ues) {
        submitOutgoing(network, ue.flushOutgoing(), nowMs);
    }
}

}  // namespace

ScenarioRunner::ScenarioRunner(ScenarioConfig config) : config_(std::move(config)) {}

SimulationResult ScenarioRunner::run() {
    SimulationResult result;
    result.scenarioName = config_.scenarioName;
    result.ueCount = config_.ueConfigs.size();

    std::vector<Ue> ues;
    std::vector<std::vector<TrafficEvent>> uplinkEventsPerUe;
    std::vector<std::size_t> uplinkEventIndexes;
    std::vector<bool> attachStarted;
    std::vector<bool> detachStarted;

    for (const auto& ueConfig : config_.ueConfigs) {
        ues.emplace_back(
            ueConfig.nodeId,
            config_.accessNodeId,
            config_.transportMode,
            config_.timers
        );

        TrafficGenerator generator(
            ueConfig.uplinkTrafficProfile,
            config_.trafficSeed + ueConfig.ueId
        );

        auto uplinkEvents = generator.generate();

        UeSimulationResult ueResult;
        ueResult.nodeId = ueConfig.nodeId;
        ueResult.ueId = ueConfig.ueId;
        ueResult.uplinkPacketsGenerated = uplinkEvents.size();

        for (const auto& event : uplinkEvents) {
            ueResult.uplinkBytesGenerated += event.payload.size();
        }

        result.ueResults.push_back(ueResult);
        uplinkEventsPerUe.push_back(std::move(uplinkEvents));
        uplinkEventIndexes.push_back(0);
        attachStarted.push_back(false);
        detachStarted.push_back(false);
    }

    AccessNode accessNode(config_.accessNodeId, CoreNetwork(config_.timers));
    VirtualNetwork network(config_.linkProfile, config_.networkSeed);

    std::uint64_t nowMs = 0;

    while (nowMs <= config_.scenarioDurationMs) {
        for (std::size_t i = 0; i < ues.size(); ++i) {
            const auto& ueConfig = config_.ueConfigs[i];

            if (!attachStarted[i] && nowMs >= ueConfig.attachStartMs) {
                ues[i].startAttach(nowMs);
                attachStarted[i] = true;
            }

            if (attachStarted[i] &&
                !detachStarted[i] &&
                nowMs >= ueConfig.trafficStartMs &&
                nowMs < ueConfig.trafficEndMs) {

                auto& events = uplinkEventsPerUe[i];
                auto& eventIndex = uplinkEventIndexes[i];

                while (eventIndex < events.size() &&
                       ueConfig.trafficStartMs + events[eventIndex].timestampMs <= nowMs) {
                    const auto packetsBefore = ues[i].metrics().packetsSent;

                    ues[i].sendTraffic(events[eventIndex].payload, nowMs);

                    if (ues[i].metrics().packetsSent > packetsBefore) {
                        result.ueResults[i].trafficStarted = true;
                    }

                    ++eventIndex;
                }
            }

            if (!detachStarted[i] && nowMs >= ueConfig.trafficEndMs) {
                if (ues[i].isAttached()) {
                    ues[i].startDetach(nowMs);
                }
                detachStarted[i] = true;
            }
        }

        for (auto& ue : ues) {
            ue.tick(nowMs);
        }

        accessNode.tick(nowMs);

        submitOutgoingFromUes(network, ues, nowMs);
        submitOutgoing(network, accessNode.flushOutgoing(), nowMs);
        deliverReady(network, ues, accessNode, nowMs);

        nowMs += config_.stepMs;
    }

    result.totalDurationMs = nowMs;
    result.packetsDroppedInNetwork = network.metrics().packetsDropped;
    result.packetsDeliveredByNetwork = network.metrics().packetsDelivered;
    result.activeSessionsAtEnd = accessNode.coreNetwork().activeSessionCount();
    result.expiredSessions = accessNode.coreNetwork().expiredSessions();

    for (std::size_t i = 0; i < ues.size(); ++i) {
        const auto& ueConfig = config_.ueConfigs[i];

        result.ueResults[i].finalUeState = ues[i].state();
        result.ueResults[i].attachSucceeded =
            ues[i].state() == SessionState::Attached ||
            ues[i].state() == SessionState::Detaching ||
            ues[i].state() == SessionState::Released;

        result.ueResults[i].detachSucceeded =
            detachStarted[i] &&
            !ues[i].isAttached() &&
            ues[i].state() != SessionState::Detaching;

        result.ueResults[i].activeAtEnd = ues[i].isAttached();

        result.ueResults[i].uplinkPacketsSent = ues[i].metrics().packetsSent;
        result.ueResults[i].uplinkBytesSent = ues[i].metrics().bytesSent;

        result.ueResults[i].uplinkPacketsAcceptedByCore =
            accessNode.coreNetwork().deliveredPacketsForUe(ueConfig.ueId);

        result.ueResults[i].uplinkBytesAcceptedByCore =
            accessNode.coreNetwork().deliveredBytesForUe(ueConfig.ueId);

        const auto trafficDurationMs =
            (ueConfig.trafficEndMs > ueConfig.trafficStartMs)
                ? (ueConfig.trafficEndMs - ueConfig.trafficStartMs)
                : 0;

        result.ueResults[i].uplinkThroughputMbps =
            (trafficDurationMs == 0)
                ? 0.0
                : (static_cast<double>(result.ueResults[i].uplinkBytesAcceptedByCore) * 8.0) /
                      (static_cast<double>(trafficDurationMs) / 1000.0) / 1'000'000.0;

        result.uplinkPacketsAcceptedByCore += result.ueResults[i].uplinkPacketsAcceptedByCore;
        result.uplinkBytesAcceptedByCore += result.ueResults[i].uplinkBytesAcceptedByCore;

        if (result.ueResults[i].detachSucceeded) {
            ++result.cleanlyDetachedSessions;
            result.ueResults[i].endReason = SessionEndReason::CleanDetach;
        } else if (!result.ueResults[i].attachSucceeded) {
            result.ueResults[i].endReason = SessionEndReason::AttachFailed;
            result.ueResults[i].notes.push_back("Attach did not succeed.");
        } else if (result.ueResults[i].activeAtEnd) {
            result.ueResults[i].endReason = SessionEndReason::StillActiveAtEnd;
            result.ueResults[i].notes.push_back("UE session is still active at the end of the scenario.");
        } else {
            result.ueResults[i].endReason = SessionEndReason::DetachNotConfirmed;
            result.ueResults[i].notes.push_back("Detach was not confirmed cleanly.");
        }

        if (result.ueResults[i].uplinkBytesAcceptedByCore == 0) {
            result.ueResults[i].notes.push_back("No uplink user-plane payload was accepted by core.");
        }
    }

    result.totalUplinkThroughputMbps =
        (config_.scenarioDurationMs == 0)
            ? 0.0
            : (static_cast<double>(result.uplinkBytesAcceptedByCore) * 8.0) /
                  (static_cast<double>(config_.scenarioDurationMs) / 1000.0) / 1'000'000.0;

    if (result.uplinkBytesAcceptedByCore == 0) {
        result.notes.push_back("No uplink user-plane payload reached the simplified core.");
    }

    if (result.activeSessionsAtEnd > 0) {
        result.notes.push_back("Some sessions are still active at the end of the scenario.");
    }

    return result;
}

}  // namespace miniran
