#include "miniran/simulation/scenario_runner.h"

#include <string>
#include <utility>

#include "miniran/nodes/access_node.h"
#include "miniran/nodes/ue.h"
#include "miniran/traffic/traffic_generator.h"
#include "miniran/transport/virtual_network.h"

namespace miniran {

namespace {

std::size_t submitOutgoing(VirtualNetwork& network, std::vector<Datagram> datagrams, std::uint64_t nowMs) {
    std::size_t rejected = 0;

    for (auto& datagram : datagrams) {
        if (!network.submit(std::move(datagram), nowMs)) {
            ++rejected;
        }
    }

    return rejected;
}

void addRejectedNetworkSubmissionsNote(SimulationResult& result, std::size_t rejectedNetworkSubmissions) {
    if (rejectedNetworkSubmissions > 0) {
        result.notes.push_back(
            "VirtualNetwork rejected " + std::to_string(rejectedNetworkSubmissions) +
            " outgoing datagram submissions."
        );
    }
}

void deliverReady(VirtualNetwork& network, Ue& ue, AccessNode& accessNode, std::uint64_t nowMs) {
    auto ready = network.pollReady(nowMs);
    for (const auto& datagram : ready) {
        if (datagram.toNodeId == ue.nodeId()) {
            ue.onDatagram(datagram, nowMs);
        } else if (datagram.toNodeId == accessNode.nodeId()) {
            accessNode.onDatagram(datagram, nowMs);
        }
    }
}

}  // namespace

ScenarioRunner::ScenarioRunner(ScenarioConfig config) : config_(std::move(config)) {}

SimulationResult ScenarioRunner::run() {
    SimulationResult result;
    result.scenarioName = config_.scenarioName;

    Ue ue(config_.ueId, config_.accessNodeId,config_.timers);
    AccessNode accessNode(config_.accessNodeId, config_.ueId, CoreNetwork(config_.timers));
    VirtualNetwork network(config_.linkProfile, 1337);

    TrafficGenerator generator(config_.trafficProfile, 7);
    const auto trafficEvents = generator.generate();
    result.trafficStarted = false;
    result.packetsGenerated = trafficEvents.size();
    for (const auto& event : trafficEvents) {
        result.bytesGenerated += event.payload.size();
    }

    std::uint64_t nowMs = 0;
    std::size_t rejectedNetworkSubmissions = 0;

    ue.startAttach(nowMs);
    rejectedNetworkSubmissions += submitOutgoing(network, ue.flushOutgoing(), nowMs);
    deliverReady(network, ue, accessNode, nowMs);

    const std::uint64_t attachDeadlineMs = config_.attachPhaseBudgetMs;

    while (!ue.isAttached()) {
        if (nowMs >= attachDeadlineMs || config_.stepMs > attachDeadlineMs - nowMs) {
            break;
        }

        nowMs += config_.stepMs;

        ue.tick(nowMs);
        accessNode.tick(nowMs);
        rejectedNetworkSubmissions += submitOutgoing(network, ue.flushOutgoing(), nowMs);
        rejectedNetworkSubmissions += submitOutgoing(network, accessNode.flushOutgoing(), nowMs);
        deliverReady(network, ue, accessNode, nowMs);
    }

    result.attachSucceeded = ue.isAttached();
    if (!result.attachSucceeded) {
        result.notes.push_back("Attach phase ended without reaching Attached state.");
        result.notes.push_back("Traffic and detach phases skipped because no session was established.");

        result.totalDurationMs = nowMs;
        result.finalUeState = ue.state();
        result.packetsDroppedInNetwork = network.metrics().packetsDropped;
        result.packetsDeliveredByNetwork = network.metrics().packetsDelivered;
        result.activeSessionsAtEnd = accessNode.coreNetwork().activeSessionCount();
        result.expiredSessions = accessNode.coreNetwork().expiredSessions();

        addRejectedNetworkSubmissionsNote(result, rejectedNetworkSubmissions);

        return result;
    }

    const std::uint64_t trafficStartMs = nowMs;
    std::size_t eventIndex = 0;

    while (nowMs < trafficStartMs + config_.trafficProfile.durationMs) {
        while (eventIndex < trafficEvents.size() &&
               trafficStartMs + trafficEvents[eventIndex].timestampMs <= nowMs) {
            if (ue.isAttached()) {
                result.trafficStarted = true;
            }

            ue.sendTraffic(trafficEvents[eventIndex].payload, nowMs);
            ++eventIndex;
        }

        nowMs += config_.stepMs;

        ue.tick(nowMs);
        accessNode.tick(nowMs);
        rejectedNetworkSubmissions += submitOutgoing(network, ue.flushOutgoing(), nowMs);
        rejectedNetworkSubmissions += submitOutgoing(network, accessNode.flushOutgoing(), nowMs);
        deliverReady(network, ue, accessNode, nowMs);
    }

    ue.startDetach(nowMs);
    rejectedNetworkSubmissions += submitOutgoing(network, ue.flushOutgoing(), nowMs);
    deliverReady(network, ue, accessNode, nowMs);

    const std::uint64_t detachDeadlineMs = nowMs + config_.detachPhaseBudgetMs;

    while (ue.isAttached() ||
           ue.state() == SessionState::Detaching ||
           accessNode.coreNetwork().activeSessionCount() > 0) {
        if (nowMs >= detachDeadlineMs || config_.stepMs > detachDeadlineMs - nowMs) {
            break;
        }

        nowMs += config_.stepMs;

        ue.tick(nowMs);
        accessNode.tick(nowMs);
        rejectedNetworkSubmissions += submitOutgoing(network, ue.flushOutgoing(), nowMs);
        rejectedNetworkSubmissions += submitOutgoing(network, accessNode.flushOutgoing(), nowMs);
        deliverReady(network, ue, accessNode, nowMs);
    }

    result.detachSucceeded =
        ue.state() == SessionState::Released &&
        accessNode.coreNetwork().activeSessionCount() == 0 &&
        accessNode.coreNetwork().expiredSessions() == 0;

    if (!result.detachSucceeded) {
        if (ue.state() != SessionState::Released) {
            result.notes.push_back("Detach was not confirmed by DetachAccept.");
        }

        if (accessNode.coreNetwork().activeSessionCount() > 0) {
            result.notes.push_back("CoreNetwork still has an active session after detach phase.");
        }

        if (accessNode.coreNetwork().expiredSessions() > 0) {
            result.notes.push_back("CoreNetwork session expired instead of being detached cleanly.");
        }

        if (result.notes.empty()) {
            result.notes.push_back("Detach phase did not close the session cleanly.");
        }
    }

    result.totalDurationMs = nowMs;
    result.finalUeState = ue.state();
    result.packetsDeliveredToCore = accessNode.coreNetwork().deliveredPackets();
    result.bytesDeliveredToCore = accessNode.coreNetwork().deliveredBytes();
    result.packetsDroppedInNetwork = network.metrics().packetsDropped;
    result.packetsDeliveredByNetwork = network.metrics().packetsDelivered;
    result.activeSessionsAtEnd = accessNode.coreNetwork().activeSessionCount();
    result.expiredSessions = accessNode.coreNetwork().expiredSessions();

    result.throughputMbps = (config_.trafficProfile.durationMs == 0)
                                ? 0.0
                                : (static_cast<double>(result.bytesDeliveredToCore) * 8.0) /
                                      (static_cast<double>(config_.trafficProfile.durationMs) / 1000.0) /
                                      1'000'000.0;

    addRejectedNetworkSubmissionsNote(result, rejectedNetworkSubmissions);

    if (result.expiredSessions > 0) {
        result.notes.push_back("At least one CoreNetwork session expired unexpectedly.");
    }

    if (result.bytesGenerated > 0 && result.bytesDeliveredToCore == 0) {
        result.notes.push_back("No user-plane payload reached the simplified core.");
    }

    if (result.packetsGenerated > 0 && result.packetsDeliveredToCore == 0) {
        result.notes.push_back("No user-plane packet reached the simplified core.");
    }

    if (result.finalUeState != SessionState::Released) {
        result.notes.push_back("Final UE state is not Released.");
    }

    if (result.activeSessionsAtEnd != 0) {
        result.notes.push_back("CoreNetwork still has active sessions at the end.");
    }

    if (!result.isHealthy()) {
        result.notes.push_back("Scenario result is unhealthy.");
    }

    return result;
}

}  // namespace miniran