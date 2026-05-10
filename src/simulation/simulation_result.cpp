#include "miniran/simulation/simulation_result.h"

#include <iomanip>
#include <sstream>

namespace miniran {

std::string SimulationResult::summary() const {
    std::ostringstream output;

    output << "Scenario: " << scenarioName << '\n';
    output << "Duration [ms]: " << totalDurationMs << '\n';

    output << "Packets dropped in network: " << packetsDroppedInNetwork << '\n';
    output << "Packets delivered by network: " << packetsDeliveredByNetwork << '\n';
    output << "Active sessions at end: " << activeSessionsAtEnd << '\n';
    output << "Expired sessions: " << expiredSessions << '\n';

    if (!ueResults.empty()) {
        output << '\n';
        output << "UE results:" << '\n';

        for (const auto& ueResult : ueResults) {
            output << "UE " << ueResult.ueId << ":" << '\n';
            output << "  Attach succeeded: " << (ueResult.attachSucceeded ? "yes" : "no") << '\n';
            output << "  Traffic started: " << (ueResult.trafficStarted ? "yes" : "no") << '\n';
            output << "  Detach succeeded: " << (ueResult.detachSucceeded ? "yes" : "no") << '\n';
            output << "  Final UE state: " << toString(ueResult.finalUeState) << '\n';
            output << "  Packets generated: " << ueResult.packetsGenerated << '\n';
            output << "  Bytes generated: " << ueResult.bytesGenerated << '\n';
            output << "  Packets delivered to core: " << ueResult.packetsDeliveredToCore << '\n';
            output << "  Bytes delivered to core: " << ueResult.bytesDeliveredToCore << '\n';
            output << std::fixed << std::setprecision(3)
                   << "  Throughput [Mbps]: " << ueResult.throughputMbps << '\n';

            if (!ueResult.notes.empty()) {
                output << "  Notes:" << '\n';
                for (const auto& note : ueResult.notes) {
                    output << "    - " << note << '\n';
                }
            }
        }
    }

    if (!notes.empty()) {
        output << '\n';
        output << "Global notes:" << '\n';
        for (const auto& note : notes) {
            output << "  - " << note << '\n';
        }
    }

    return output.str();
}

}  // namespace miniran
