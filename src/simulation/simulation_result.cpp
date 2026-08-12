#include "miniran/simulation/simulation_result.h"

#include <iomanip>
#include <sstream>

namespace miniran {

namespace
{

const char* transportModeName(TransportMode mode)
{
    switch (mode) {
    case TransportMode::Tcp:
        return "Tcp";
    case TransportMode::Udp:
        return "Udp";
    }

    return "Unknown";
}

}  // namespace

double SimulationResult::deliveryRatio() const {
    if (packetsGenerated == 0) {
        return 1.0;
    }

    return static_cast<double>(packetsDeliveredToCore) /
           static_cast<double>(packetsGenerated);
}

bool SimulationResult::isHealthy() const {
    if (!attachSucceeded) {
        return false;
    }

    if (!trafficStarted) {
        return false;
    }

    if (!detachSucceeded) {
        return false;
    }

    if (finalUeState != SessionState::Released) {
        return false;
    }

    if (activeSessionsAtEnd != 0) {
        return false;
    }

    if (expiredSessions != 0) {
        return false;
    }

    if (transportMode == TransportMode::Tcp &&
        rejectedNetworkSubmissions != 0)
    {
        return false;
    }

    if (transportMode == TransportMode::Tcp &&
        packetsGenerated != packetsDeliveredToCore)
    {
        return false;
    }

    if (transportMode == TransportMode::Tcp &&
        bytesGenerated != bytesDeliveredToCore)
    {
        return false;
    }

    if (transportMode == TransportMode::Udp &&
        packetsGenerated > 0 &&
        deliveryRatio() < minDeliveryRatio)
    {
        return false;
    }

    if (bytesGenerated > 0 && bytesDeliveredToCore == 0) {
        return false;
    }

    if (packetsGenerated > 0 && packetsDeliveredToCore == 0) {
        return false;
    }

    return true;
}

std::string SimulationResult::summary() const {
    std::ostringstream output;
    output << "Scenario: " << scenarioName << '\n';
    output << "Transport mode: " << transportModeName(transportMode) << '\n';
    output << std::fixed << std::setprecision(3);
    output << "Delivery ratio: " << deliveryRatio() << '\n';
    output << "Required min delivery ratio: " << minDeliveryRatio << '\n';
    output << "Duration [ms]: " << totalDurationMs << '\n';
    output << "Attach succeeded: " << (attachSucceeded ? "yes" : "no") << '\n';
    output << "Traffic started: " << (trafficStarted ? "yes" : "no") << '\n';
    output << "Detach succeeded: " << (detachSucceeded ? "yes" : "no") << '\n';
    output << "Healthy result: " << (isHealthy() ? "yes" : "no") << '\n';
    output << "Final UE state: " << toString(finalUeState) << '\n';
    output << "Packets generated: " << packetsGenerated << '\n';
    output << "Bytes generated: " << bytesGenerated << '\n';
    output << "Packets delivered to core: " << packetsDeliveredToCore << '\n';
    output << "Bytes delivered to core: " << bytesDeliveredToCore << '\n';
    output << "Packets dropped in network: " << packetsDroppedInNetwork << '\n';
    output << "Rejected network submissions: " << rejectedNetworkSubmissions << '\n';
    output << "Packets delivered by network: " << packetsDeliveredByNetwork << '\n';
    output << "Active sessions at end: " << activeSessionsAtEnd << '\n';
    output << "Expired sessions: " << expiredSessions << '\n';
    output << "Throughput [Mbps]: " << throughputMbps << '\n';
    if (!notes.empty()) {
        output << "Notes:" << '\n';
        for (const auto& note : notes) {
            output << "  - " << note << '\n';
        }
    }
    return output.str();
}

}  // namespace miniran
