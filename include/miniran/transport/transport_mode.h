#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace miniran {

enum class TransportMode {
    Tcp,
    Udp,
};

inline std::string toString(TransportMode mode) {
    switch (mode) {
        case TransportMode::Tcp:
            return "tcp";
        case TransportMode::Udp:
            return "udp";
    }
    return "unknown";
}

inline std::optional<TransportMode> parseTransportMode(std::string_view value) {
    if (value == "tcp" || value == "TCP") {
        return TransportMode::Tcp;
    }
    if (value == "udp" || value == "UDP") {
        return TransportMode::Udp;
    }
    return std::nullopt;
}

}  // namespace miniran
