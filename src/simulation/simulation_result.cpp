#include "miniran/simulation/simulation_result.h"

#include <iomanip>
#include <sstream>

namespace miniran {

std::string SimulationResult::summary() const {
    std::ostringstream output;
    output << "Scenario: " << scenarioName << '\n';
    output << "Duration [ms]: " << totalDurationMs << '\n';
    output << "Attach succeeded: " << (attachSucceeded ? "yes" : "no") << '\n';
    output << "Traffic started: " << (trafficStarted ? "yes" : "no") << '\n';
    output << "Detach succeeded: " << (detachSucceeded ? "yes" : "no") << '\n';
    output << "Final UE state: " << toString(finalUeState) << '\n';
    output << "Packets generated: " << packetsGenerated << '\n';
    output << "Bytes generated: " << bytesGenerated << '\n';
    output << "Packets delivered to core: " << packetsDeliveredToCore << '\n';
    output << "Bytes delivered to core: " << bytesDeliveredToCore << '\n';
    output << "Packets dropped in network: " << packetsDroppedInNetwork << '\n';
    output << "Packets delivered by network: " << packetsDeliveredByNetwork << '\n';
    output << "Active sessions at end: " << activeSessionsAtEnd << '\n';
    output << std::fixed << std::setprecision(3) << "Throughput [Mbps]: " << throughputMbps << '\n';
    if (!notes.empty()) {
        output << "Notes:" << '\n';
        for (const auto& note : notes) {
            output << "  - " << note << '\n';
        }
    }
    return output.str();
}

}  // namespace miniran
