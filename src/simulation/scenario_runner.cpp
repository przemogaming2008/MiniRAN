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

    std::vector<Ue> ues;
    std::vector<std::vector<TrafficEvent>> trafficEventsPerUe;
    std::vector<std::size_t> eventIndexes;

    for (const auto& ueConfig : config_.ueConfigs) {
        ues.emplace_back(
            ueConfig.ueId,
            config_.accessNodeId,
            config_.transportMode,
            config_.timers
        );

        TrafficGenerator generator(ueConfig.trafficProfile, ueConfig.ueId);
        auto trafficEvents = generator.generate();

        UeSimulationResult ueResult;
        ueResult.ueId = ueConfig.ueId;
        ueResult.trafficStarted = false;
        ueResult.packetsGenerated = trafficEvents.size();

        for (const auto& event : trafficEvents) {
            ueResult.bytesGenerated += event.payload.size();
        }

        result.ueResults.push_back(ueResult);
        trafficEventsPerUe.push_back(std::move(trafficEvents));
        eventIndexes.push_back(0);
    }

    AccessNode accessNode(config_.accessNodeId, CoreNetwork(config_.timers));
    VirtualNetwork network(config_.linkProfile, 1337);

    std::uint64_t nowMs = 0;

    for (auto& ue : ues) {
        ue.startAttach(nowMs);
    }

    submitOutgoingFromUes(network, ues, nowMs);
    submitOutgoing(network, accessNode.flushOutgoing(), nowMs);
    deliverReady(network, ues, accessNode, nowMs);

    auto allAttached = [&]() {
        for (const auto& ue : ues) {
            if (!ue.isAttached()) {
                return false;
            }
        }
        return !ues.empty();
    };

    while (nowMs <= config_.attachPhaseBudgetMs && !allAttached()) {
        nowMs += config_.stepMs;

        for (auto& ue : ues) {
            ue.tick(nowMs);
        }

        accessNode.tick(nowMs);

        submitOutgoingFromUes(network, ues, nowMs);
        submitOutgoing(network, accessNode.flushOutgoing(), nowMs);
        deliverReady(network, ues, accessNode, nowMs);
    }

    bool anyAttached = false;

    for (std::size_t i = 0; i < ues.size(); ++i) {
        result.ueResults[i].attachSucceeded = ues[i].isAttached();
        result.ueResults[i].finalUeState = ues[i].state();

        if (ues[i].isAttached()) {
            anyAttached = true;
        } else {
            result.ueResults[i].notes.push_back("Attach phase ended without reaching Attached state.");
            result.ueResults[i].notes.push_back("Traffic and detach phases skipped because no session was established.");
        }
    }

    if (!anyAttached) {
        result.notes.push_back("No UE reached Attached state.");

        result.totalDurationMs = nowMs;
        result.packetsDroppedInNetwork = network.metrics().packetsDropped;
        result.packetsDeliveredByNetwork = network.metrics().packetsDelivered;
        result.activeSessionsAtEnd = accessNode.coreNetwork().activeSessionCount();
        result.expiredSessions = accessNode.coreNetwork().expiredSessions();

        return result;
    }

    std::uint64_t maxTrafficDurationMs = 0;

    for (const auto& ueConfig : config_.ueConfigs) {
        if (ueConfig.trafficProfile.durationMs > maxTrafficDurationMs) {
            maxTrafficDurationMs = ueConfig.trafficProfile.durationMs;
        }
    }

    const std::uint64_t trafficStartMs = nowMs;

    while (nowMs < trafficStartMs + maxTrafficDurationMs) {
        for (std::size_t i = 0; i < ues.size(); ++i) {
            auto& events = trafficEventsPerUe[i];
            auto& eventIndex = eventIndexes[i];

            while (eventIndex < events.size() &&
                   trafficStartMs + events[eventIndex].timestampMs <= nowMs) {
                const auto packetsBefore = ues[i].metrics().packetsSent;

                ues[i].sendTraffic(events[eventIndex].payload, nowMs);

                if (ues[i].metrics().packetsSent > packetsBefore) {
                    result.ueResults[i].trafficStarted = true;
                }

                ++eventIndex;
            }
        }

        nowMs += config_.stepMs;

        for (auto& ue : ues) {
            ue.tick(nowMs);
        }

        accessNode.tick(nowMs);

        submitOutgoingFromUes(network, ues, nowMs);
        submitOutgoing(network, accessNode.flushOutgoing(), nowMs);
        deliverReady(network, ues, accessNode, nowMs);
    }

    for (auto& ue : ues) {
        if (ue.isAttached()) {
            ue.startDetach(nowMs);
        }
    }

    submitOutgoingFromUes(network, ues, nowMs);
    submitOutgoing(network, accessNode.flushOutgoing(), nowMs);
    deliverReady(network, ues, accessNode, nowMs);

    auto anyUeStillActive = [&]() {
        for (const auto& ue : ues) {
            if (ue.isAttached() || ue.state() == SessionState::Detaching) {
                return true;
            }
        }
        return false;
    };

    const std::uint64_t detachDeadlineMs = nowMs + config_.detachPhaseBudgetMs;

    while (nowMs <= detachDeadlineMs &&
           (anyUeStillActive() || accessNode.coreNetwork().activeSessionCount() > 0)) {
        nowMs += config_.stepMs;

        for (auto& ue : ues) {
            ue.tick(nowMs);
        }

        accessNode.tick(nowMs);

        submitOutgoingFromUes(network, ues, nowMs);
        submitOutgoing(network, accessNode.flushOutgoing(), nowMs);
        deliverReady(network, ues, accessNode, nowMs);
    }

    for (std::size_t i = 0; i < ues.size(); ++i) {
        result.ueResults[i].finalUeState = ues[i].state();

        if (!result.ueResults[i].attachSucceeded) {
            continue;
        }

        result.ueResults[i].detachSucceeded =
            !ues[i].isAttached() &&
            ues[i].state() != SessionState::Detaching;

        if (!result.ueResults[i].detachSucceeded) {
            result.ueResults[i].notes.push_back("Detach phase did not close the UE session cleanly.");
        }
    }

    result.totalDurationMs = nowMs;
    result.packetsDroppedInNetwork = network.metrics().packetsDropped;
    result.packetsDeliveredByNetwork = network.metrics().packetsDelivered;
    result.activeSessionsAtEnd = accessNode.coreNetwork().activeSessionCount();
    result.expiredSessions = accessNode.coreNetwork().expiredSessions();

    if (accessNode.coreNetwork().deliveredBytes() == 0) {
        result.notes.push_back("No user-plane payload reached the simplified core.");
    }

    if (result.activeSessionsAtEnd > 0) {
        result.notes.push_back("Some sessions are still active at the end of the scenario.");
    }

    return result;
}

}  // namespace miniran
