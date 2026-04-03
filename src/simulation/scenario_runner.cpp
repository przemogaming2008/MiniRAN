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

    Ue ue(config_.ueId, config_.accessNodeId, config_.transportMode, config_.timers);
    AccessNode accessNode(config_.accessNodeId, config_.ueId, CoreNetwork(config_.timers));
    VirtualNetwork network(config_.linkProfile, 1337);

    TrafficGenerator generator(config_.trafficProfile, 7);
    const auto trafficEvents = generator.generate();
    result.trafficStarted = !trafficEvents.empty();
    result.packetsGenerated = trafficEvents.size();
    for (const auto& event : trafficEvents) {
        result.bytesGenerated += event.payload.size();
    }

    std::uint64_t nowMs = 0;
    ue.startAttach(nowMs);
    submitOutgoing(network, ue.flushOutgoing(), nowMs);
    deliverReady(network, ue, accessNode, nowMs);

    while (nowMs <= config_.attachPhaseBudgetMs && !ue.isAttached()) {
        nowMs += config_.stepMs;
        ue.tick(nowMs);
        accessNode.tick(nowMs);
        submitOutgoing(network, ue.flushOutgoing(), nowMs);
        submitOutgoing(network, accessNode.flushOutgoing(), nowMs);
        deliverReady(network, ue, accessNode, nowMs);
    }
    result.attachSucceeded = ue.isAttached();
    if (!result.attachSucceeded) {
        result.notes.push_back("Attach phase ended without reaching Attached state.");
    }

    const std::uint64_t trafficStartMs = nowMs;
    std::size_t eventIndex = 0;
    while (nowMs < trafficStartMs + config_.trafficProfile.durationMs) {
        while (eventIndex < trafficEvents.size() && trafficStartMs + trafficEvents[eventIndex].timestampMs <= nowMs) {
            ue.sendTraffic(trafficEvents[eventIndex].payload, nowMs);
            ++eventIndex;
        }

        nowMs += config_.stepMs;
        ue.tick(nowMs);
        accessNode.tick(nowMs);
        submitOutgoing(network, ue.flushOutgoing(), nowMs);
        submitOutgoing(network, accessNode.flushOutgoing(), nowMs);
        deliverReady(network, ue, accessNode, nowMs);
    }

    ue.startDetach(nowMs);
    submitOutgoing(network, ue.flushOutgoing(), nowMs);
    deliverReady(network, ue, accessNode, nowMs);

    const std::uint64_t detachDeadlineMs = nowMs + config_.detachPhaseBudgetMs;
    while (nowMs <= detachDeadlineMs &&
           (ue.isAttached() || ue.state() == SessionState::Detaching || accessNode.coreNetwork().activeSessionCount() > 0)) {
        nowMs += config_.stepMs;
        ue.tick(nowMs);
        accessNode.tick(nowMs);
        submitOutgoing(network, ue.flushOutgoing(), nowMs);
        submitOutgoing(network, accessNode.flushOutgoing(), nowMs);
        deliverReady(network, ue, accessNode, nowMs);
    }

    result.detachSucceeded = !ue.isAttached() && accessNode.coreNetwork().activeSessionCount() == 0;
    if (!result.detachSucceeded) {
        result.notes.push_back("Detach phase did not close the session cleanly.");
    }

    result.totalDurationMs = nowMs;
    result.finalUeState = ue.state();
    result.packetsDeliveredToCore = accessNode.coreNetwork().deliveredPackets();
    result.bytesDeliveredToCore = accessNode.coreNetwork().deliveredBytes();
    result.packetsDroppedInNetwork = network.metrics().packetsDropped;
    result.packetsDeliveredByNetwork = network.metrics().packetsDelivered;
    result.activeSessionsAtEnd = accessNode.coreNetwork().activeSessionCount();
    result.throughputMbps = (config_.trafficProfile.durationMs == 0)
                                ? 0.0
                                : (static_cast<double>(result.bytesDeliveredToCore) * 8.0) /
                                      (static_cast<double>(config_.trafficProfile.durationMs) / 1000.0) / 1'000'000.0;

    if (result.bytesDeliveredToCore == 0) {
        result.notes.push_back("No user-plane payload reached the simplified core.");
    }

    return result;
}

}  // namespace miniran
