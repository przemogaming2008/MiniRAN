#include "miniran/simulation/scenario_runner.h"

#include <optional>
#include <utility>
#include <vector>
#include <string>
#include "miniran/nodes/access_node.h"
#include "miniran/nodes/ue.h"
#include "miniran/protocol/protocol_message.h"
#include "miniran/traffic/traffic_generator.h"
#include "miniran/transport/virtual_network.h"
#include "miniran/core/core_network.h"

namespace miniran {

namespace {

void submitOutgoing(VirtualNetwork& network, std::vector<Datagram> datagrams, std::uint64_t nowMs) {
    for (auto& datagram : datagrams) {
        network.submit(std::move(datagram), nowMs);
    }
}

void submitOutgoingFromUes(VirtualNetwork& network, std::vector<Ue>& ues, std::uint64_t nowMs) {
    for (auto& ue : ues) {
        submitOutgoing(network, ue.flushOutgoing(), nowMs);
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

SessionEndReason mapUeEndReason(UeSessionEndReason reason) {
    switch (reason) {
        case UeSessionEndReason::None:
            return SessionEndReason::None;

        case UeSessionEndReason::CleanDetach:
            return SessionEndReason::CleanDetach;

        case UeSessionEndReason::AttachFailed:
            return SessionEndReason::AttachFailed;

        case UeSessionEndReason::DetachNotConfirmed:
            return SessionEndReason::DetachNotConfirmed;

        case UeSessionEndReason::InactivityTimeout:
            return SessionEndReason::InactivityTimeout;
    }

    return SessionEndReason::Error;
}

SessionEndReason mapCoreEndReason(CoreSessionEndReason reason) {
    switch (reason) {
        case CoreSessionEndReason::None:
            return SessionEndReason::None;

        case CoreSessionEndReason::CleanDetach:
            return SessionEndReason::CleanDetach;

        case CoreSessionEndReason::InactivityTimeout:
            return SessionEndReason::InactivityTimeout;

        case CoreSessionEndReason::ProtocolError:
        case CoreSessionEndReason::RetryLimitExceeded:
            return SessionEndReason::Error;
    }

    return SessionEndReason::Error;
}

const SessionRecord* findLatestSessionRecord(
    const CoreNetwork& coreNetwork,
    std::uint32_t ueId,
    std::uint32_t sessionId
) {
    const auto& history = coreNetwork.sessionHistory();
    const auto historyIt = history.find(ueId);

    if (historyIt == history.end() || historyIt->second.empty()) {
        return nullptr;
    }

    const auto& records = historyIt->second;

    if (sessionId != 0) {
        for (auto it = records.rbegin(); it != records.rend(); ++it) {
            if (it->sessionId == sessionId) {
                return &(*it);
            }
        }
    }

    return &records.back();
}
}  // namespace

ScenarioRunner::ScenarioRunner(ScenarioConfig config) : config_(std::move(config)) {}

SimulationResult ScenarioRunner::run() {

    SimulationResult result;
    result.scenarioName = config_.scenarioName;

    std::string validationError;
    if (!config_.validate(validationError)) {
        result.notes.push_back("Invalid scenario config: " + validationError);
        return result;
    }

    result.ueCount = config_.ueConfigs.size();

    std::vector<Ue> ues;

    std::vector<std::vector<TrafficEvent>> uplinkEventsPerUe;
    std::vector<std::vector<TrafficEvent>> downlinkEventsPerUe;

    std::vector<std::size_t> uplinkEventIndexes;
    std::vector<std::size_t> downlinkEventIndexes;

    std::vector<bool> attachStarted;
    std::vector<bool> detachRequested;
    std::vector<bool> detachStarted;

    for (const auto& ueConfig : config_.ueConfigs) {
        ues.emplace_back(
            ueConfig.nodeId,
            ueConfig.ueId,
            config_.accessNodeId,
            config_.transportMode,
            config_.timers
        );

        TrafficGenerator uplinkGenerator(
            ueConfig.uplinkTrafficProfile,
            config_.trafficSeed + ueConfig.ueId
        );

        auto uplinkEvents = uplinkGenerator.generate();

        std::vector<TrafficEvent> downlinkEvents;
        if (ueConfig.downlinkEnabled) {
            TrafficGenerator downlinkGenerator(
                ueConfig.downlinkTrafficProfile,
                config_.trafficSeed + ueConfig.ueId + 10000U
            );

            downlinkEvents = downlinkGenerator.generate();
        }

        UeSimulationResult ueResult;
        ueResult.nodeId = ueConfig.nodeId;
        ueResult.ueId = ueConfig.ueId;

        ueResult.uplinkPacketsGenerated = uplinkEvents.size();
        for (const auto& event : uplinkEvents) {
            ueResult.uplinkBytesGenerated += event.payload.size();
        }

        ueResult.downlinkPacketsGenerated = downlinkEvents.size();
        for (const auto& event : downlinkEvents) {
            ueResult.downlinkBytesGenerated += event.payload.size();
        }

        result.ueResults.push_back(ueResult);

        uplinkEventsPerUe.push_back(std::move(uplinkEvents));
        downlinkEventsPerUe.push_back(std::move(downlinkEvents));

        uplinkEventIndexes.push_back(0);
        downlinkEventIndexes.push_back(0);

        attachStarted.push_back(false);
        detachRequested.push_back(false);
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

                    const auto& event = events[eventIndex];

                    if (!ues[i].isAttached()) {
                        result.ueResults[i].uplinkPacketsGenerated += 1;
                        result.ueResults[i].uplinkBytesGenerated += event.payload.size();

                        result.ueResults[i].uplinkPacketsSkippedNoSession += 1;
                        result.ueResults[i].uplinkBytesSkippedNoSession += event.payload.size();

                        ++eventIndex;
                        continue;
                    }

                    ues[i].sendTraffic(event.payload, nowMs);
                    ++eventIndex;
                }
            }

            if (ueConfig.downlinkEnabled &&
                attachStarted[i] &&
                !detachStarted[i] &&
                nowMs >= ueConfig.trafficStartMs &&
                nowMs < ueConfig.trafficEndMs) {

                auto& events = downlinkEventsPerUe[i];
                auto& eventIndex = downlinkEventIndexes[i];

                while (eventIndex < events.size() &&
                       ueConfig.trafficStartMs + events[eventIndex].timestampMs <= nowMs) {

                    if (!ues[i].isAttached() ||
                        !accessNode.coreNetwork().hasActiveSession(ueConfig.ueId)) {
                        break;
                    }

                    auto downlinkMessage = accessNode.coreNetwork().makeDownlinkData(
                        ueConfig.ueId,
                        static_cast<std::uint32_t>(eventIndex + 1),
                        nowMs,
                        events[eventIndex].payload
                    );

                    if (!downlinkMessage) {
                        break;
                    }

                    if (accessNode.queueDownlinkToUe(*downlinkMessage, nowMs)) {
                        result.ueResults[i].downlinkPacketsSent += 1;
                        result.ueResults[i].downlinkBytesSent += events[eventIndex].payload.size();
                    } else {
                        result.ueResults[i].notes.push_back("Downlink packet could not be routed to UE.");
                    }

                    ++eventIndex;
                }
            }

            if (!detachRequested[i] && nowMs >= ueConfig.trafficEndMs) {
                detachRequested[i] = true;
            }

            if (detachRequested[i] && !detachStarted[i] && ues[i].isAttached()) {
                ues[i].startDetach(nowMs);
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

    result.totalDurationMs = config_.scenarioDurationMs;

    result.packetsDroppedInNetwork = network.metrics().packetsDropped;
    result.packetsDroppedByLoss = network.metrics().packetsDroppedByLoss;
    result.packetsDroppedByQueue = network.metrics().packetsDroppedByQueue;
    result.packetsDeliveredByNetwork = network.metrics().packetsDelivered;

    result.activeSessionsAtEnd = accessNode.coreNetwork().activeSessionCount();
    result.expiredSessions = accessNode.coreNetwork().expiredSessions();
    result.protocolRejectedPackets = accessNode.coreNetwork().protocolRejectedPackets();

    for (std::size_t i = 0; i < ues.size(); ++i) {
        const auto& ueConfig = config_.ueConfigs[i];

        result.ueResults[i].finalUeState = ues[i].state();
        result.ueResults[i].sessionId = ues[i].lastSessionId();
        result.ueResults[i].protocolMetrics = ues[i].protocolMetrics();

        result.ueResults[i].attachSucceeded =
            ues[i].state() == SessionState::Attached ||
            ues[i].state() == SessionState::Detaching ||
            ues[i].state() == SessionState::Released;

        result.ueResults[i].detachSucceeded =
            detachStarted[i] &&
            ues[i].detachConfirmed();

        result.ueResults[i].activeAtEnd = ues[i].isAttached();

        result.ueResults[i].uplinkPacketsSent = ues[i].uplinkMetrics().packetsSent;
        result.ueResults[i].uplinkBytesSent = ues[i].uplinkMetrics().bytesSent;

        result.ueResults[i].downlinkPacketsReceivedByUe =
            ues[i].downlinkMetrics().packetsDelivered;

        result.ueResults[i].downlinkBytesReceivedByUe =
            ues[i].downlinkMetrics().bytesDelivered;

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

        result.ueResults[i].downlinkThroughputMbps =
            (trafficDurationMs == 0)
                ? 0.0
                : (static_cast<double>(result.ueResults[i].downlinkBytesReceivedByUe) * 8.0) /
                      (static_cast<double>(trafficDurationMs) / 1000.0) / 1'000'000.0;

        result.uplinkPacketsAcceptedByCore += result.ueResults[i].uplinkPacketsAcceptedByCore;
        result.uplinkBytesAcceptedByCore += result.ueResults[i].uplinkBytesAcceptedByCore;

        result.downlinkPacketsDeliveredToUe += result.ueResults[i].downlinkPacketsReceivedByUe;
        result.downlinkBytesDeliveredToUe += result.ueResults[i].downlinkBytesReceivedByUe;

        const SessionRecord* coreRecord = findLatestSessionRecord(
            accessNode.coreNetwork(),
            ueConfig.ueId,
            result.ueResults[i].sessionId
        );

        const SessionEndReason coreEndReason =
            (coreRecord == nullptr)
                ? SessionEndReason::None
                : mapCoreEndReason(coreRecord->endReason);

        const SessionEndReason ueEndReason = mapUeEndReason(ues[i].endReason());

        if (result.ueResults[i].detachSucceeded) {
            ++result.cleanlyDetachedSessions;
            result.ueResults[i].endReason = SessionEndReason::CleanDetach;
        } else if (coreEndReason == SessionEndReason::InactivityTimeout ||
                ueEndReason == SessionEndReason::InactivityTimeout) {
            result.ueResults[i].endReason = SessionEndReason::InactivityTimeout;

            if (coreEndReason == SessionEndReason::InactivityTimeout &&
                ueEndReason == SessionEndReason::InactivityTimeout) {
                result.ueResults[i].notes.push_back(
                    "Session expired due to inactivity timeout on both UE and Core side."
                );
            } else if (coreEndReason == SessionEndReason::InactivityTimeout) {
                result.ueResults[i].notes.push_back(
                    "Core session expired due to inactivity timeout."
                );
            } else {
                result.ueResults[i].notes.push_back(
                    "UE session expired due to missing heartbeat ACK."
                );
            }
        } else if (coreEndReason == SessionEndReason::Error) {
            result.ueResults[i].endReason = SessionEndReason::Error;
            result.ueResults[i].notes.push_back("Core ended the session because of a protocol error.");
        } else if (ueEndReason == SessionEndReason::AttachFailed) {
            result.ueResults[i].endReason = SessionEndReason::AttachFailed;
            result.ueResults[i].notes.push_back("Attach did not succeed.");
        } else if (ueEndReason == SessionEndReason::DetachNotConfirmed) {
            result.ueResults[i].endReason = SessionEndReason::DetachNotConfirmed;
            result.ueResults[i].notes.push_back("Detach was not confirmed cleanly.");
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

    result.totalDownlinkThroughputMbps =
        (config_.scenarioDurationMs == 0)
            ? 0.0
            : (static_cast<double>(result.downlinkBytesDeliveredToUe) * 8.0) /
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
