#include "support/test_framework.h"

#include <string>

int main(int argc, char** argv) {
    std::string junitPath;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];

        if (arg == "--junit" && i + 1 < argc) {
            junitPath = argv[++i];
        }
    }

    return miniran::test::runAllTests(junitPath);
}