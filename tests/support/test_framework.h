#pragma once

#include <chrono>
#include <cmath>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace miniran::test {

class TestFailure : public std::runtime_error {
public:
    explicit TestFailure(const std::string& message) : std::runtime_error(message) {}
};

struct TestCase {
    std::string name;
    std::function<void()> function;
};

struct TestResult {
    std::string name;
    bool passed = false;
    std::string message;
    double timeSeconds = 0.0;
};

inline std::vector<TestCase>& registry() {
    static std::vector<TestCase> tests;
    return tests;
}

inline void registerTest(std::string name, std::function<void()> function) {
    registry().push_back({std::move(name), std::move(function)});
}

template <typename T>
std::string toDebugString(const T& value) {
    std::ostringstream output;
    if constexpr (std::is_enum_v<T>) {
        output << static_cast<std::underlying_type_t<T>>(value);
    } else {
        output << value;
    }
    return output.str();
}

inline std::string toDebugString(const std::string& value) {
    return value;
}

inline std::string toDebugString(const char* value) {
    return value ? std::string(value) : std::string("<null>");
}

inline std::string escapeXml(const std::string& value) {
    std::string escaped;

    for (const char ch : value) {
        switch (ch) {
            case '&':
                escaped += "&amp;";
                break;
            case '<':
                escaped += "&lt;";
                break;
            case '>':
                escaped += "&gt;";
                break;
            case '"':
                escaped += "&quot;";
                break;
            case '\'':
                escaped += "&apos;";
                break;
            default:
                escaped += ch;
                break;
        }
    }

    return escaped;
}

inline void writeJUnit(const std::vector<TestResult>& results, const std::string& path) {
    if (path.empty()) {
        return;
    }

    int failures = 0;
    double totalTime = 0.0;

    for (const auto& result : results) {
        if (!result.passed) {
            ++failures;
        }
        totalTime += result.timeSeconds;
    }

    std::ofstream output(path);

    if (!output) {
        throw std::runtime_error("Could not write JUnit report: " + path);
    }

    output << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    output << "<testsuite name=\"MiniRAN\" tests=\"" << results.size() << "\" failures=\"" << failures
           << "\" errors=\"0\" time=\"" << std::fixed << std::setprecision(6) << totalTime << "\">\n";

    for (const auto& result : results) {
        output << "  <testcase name=\"" << escapeXml(result.name) << "\" time=\"" << std::fixed << std::setprecision(6)
               << result.timeSeconds << "\">";

        if (!result.passed) {
            output << "\n";
            output << "    <failure message=\"" << escapeXml(result.message) << "\">" << escapeXml(result.message)
                   << "</failure>\n";
            output << "  </testcase>\n";
        } else {
            output << "</testcase>\n";
        }
    }

    output << "</testsuite>\n";
}

inline void assertTrue(bool condition, const char* expression, const char* file, int line) {
    if (!condition) {
        std::ostringstream output;
        output << file << ':' << line << " ASSERT_TRUE failed: " << expression;
        throw TestFailure(output.str());
    }
}

inline void assertTrueMessage(bool condition, const std::string& message, const char* file, int line) {
    if (!condition) {
        std::ostringstream output;
        output << file << ':' << line << " ASSERT_TRUE failed: " << message;
        throw TestFailure(output.str());
    }
}

template <typename Left, typename Right>
void assertEqual(const Left& left, const Right& right, const char* leftExpr, const char* rightExpr, const char* file,
                 int line) {
    if (!(left == right)) {
        std::ostringstream output;
        output << file << ':' << line << " ASSERT_EQ failed: " << leftExpr << " != " << rightExpr << " ("
               << toDebugString(left) << " vs " << toDebugString(right) << ')';
        throw TestFailure(output.str());
    }
}

template <typename Left, typename Right>
void assertNotEqual(const Left& left, const Right& right, const char* leftExpr, const char* rightExpr, const char* file,
                    int line) {
    if (left == right) {
        std::ostringstream output;
        output << file << ':' << line << " ASSERT_NE failed: " << leftExpr << " == " << rightExpr;
        throw TestFailure(output.str());
    }
}

inline void assertNear(double left, double right, double tolerance, const char* leftExpr, const char* rightExpr,
                       const char* toleranceExpr, const char* file, int line) {
    if (std::fabs(left - right) > tolerance) {
        std::ostringstream output;
        output << file << ':' << line << " ASSERT_NEAR failed: |" << leftExpr << " - " << rightExpr << "| > "
               << toleranceExpr << " (" << std::setprecision(10) << left << " vs " << right << ")";
        throw TestFailure(output.str());
    }
}

inline int runAllTests(const std::string& junitPath = {}) {
    int passed = 0;
    int failed = 0;
    std::vector<TestResult> results;

    for (const auto& test : registry()) {
        TestResult result;
        result.name = test.name;

        const auto start = std::chrono::steady_clock::now();

        try {
            test.function();
            result.passed = true;
            std::cout << "[PASS] " << test.name << '\n';
            ++passed;
        } catch (const std::exception& ex) {
            result.passed = false;
            result.message = ex.what();
            std::cout << "[FAIL] " << test.name << "\n       " << ex.what() << '\n';
            ++failed;
        } catch (...) {
            result.passed = false;
            result.message = "Unknown non-std exception";
        }

        const auto end = std::chrono::steady_clock::now();
        result.timeSeconds = std::chrono::duration<double>(end - start).count();

        results.push_back(std::move(result));
    }

    writeJUnit(results, junitPath);

    std::cout << "\nSummary: passed=" << passed << ", failed=" << failed << '\n';
    return failed == 0 ? 0 : 1;
}

struct TestRegistrar {
    TestRegistrar(const std::string& name, std::function<void()> function) {
        registerTest(name, std::move(function));
    }
};

}  // namespace miniran::test

#define TEST_CASE(name)                                                    \
    static void name();                                                    \
    static ::miniran::test::TestRegistrar name##_registrar(#name, &name); \
    static void name()

#define ASSERT_TRUE(expression) ::miniran::test::assertTrue((expression), #expression, __FILE__, __LINE__)
#define ASSERT_EQ(left, right) ::miniran::test::assertEqual((left), (right), #left, #right, __FILE__, __LINE__)
#define ASSERT_NE(left, right) ::miniran::test::assertNotEqual((left), (right), #left, #right, __FILE__, __LINE__)
#define ASSERT_NEAR(left, right, tolerance)                                    \
    do {                                                                      \
        const auto actualLeft = static_cast<double>(left);                     \
        const auto actualRight = static_cast<double>(right);                   \
        const auto actualTolerance = static_cast<double>(tolerance);           \
                                                                              \
        if (!std::isfinite(actualLeft) ||                                      \
            !std::isfinite(actualRight) ||                                     \
            !std::isfinite(actualTolerance) ||                                 \
            actualTolerance < 0.0) {                                           \
            std::ostringstream oss;                                            \
            oss << "ASSERT_NEAR(" #left ", " #right ", " #tolerance          \
                ") failed: non-finite value or negative tolerance. "           \
                << "left=" << actualLeft                                       \
                << ", right=" << actualRight                                   \
                << ", tolerance=" << actualTolerance;                          \
            throw ::miniran::test::AssertionFailure(                           \
                __FILE__,                                                      \
                __LINE__,                                                      \
                oss.str());                                                    \
        }                                                                     \
                                                                              \
        if (std::fabs(actualLeft - actualRight) > actualTolerance) {           \
            std::ostringstream oss;                                            \
            oss << "ASSERT_NEAR(" #left ", " #right ", " #tolerance          \
                ") failed: "                                                  \
                << "left=" << actualLeft                                       \
                << ", right=" << actualRight                                   \
                << ", tolerance=" << actualTolerance;                          \
            throw ::miniran::test::AssertionFailure(                           \
                __FILE__,                                                      \
                __LINE__,                                                      \
                oss.str());                                                    \
        }                                                                     \
    } while (false)