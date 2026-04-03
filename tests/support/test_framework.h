#pragma once

#include <cmath>
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

inline void assertTrue(bool condition, const char* expression, const char* file, int line) {
    if (!condition) {
        std::ostringstream output;
        output << file << ':' << line << " ASSERT_TRUE failed: " << expression;
        throw TestFailure(output.str());
    }
}

template <typename Left, typename Right>
void assertEqual(const Left& left, const Right& right, const char* leftExpr, const char* rightExpr, const char* file, int line) {
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

inline int runAllTests() {
    int passed = 0;
    int failed = 0;

    for (const auto& test : registry()) {
        try {
            test.function();
            std::cout << "[PASS] " << test.name << '\n';
            ++passed;
        } catch (const std::exception& ex) {
            std::cout << "[FAIL] " << test.name << "\n       " << ex.what() << '\n';
            ++failed;
        }
    }

    std::cout << "\nSummary: passed=" << passed << ", failed=" << failed << '\n';
    return failed == 0 ? 0 : 1;
}

struct TestRegistrar {
    TestRegistrar(const std::string& name, std::function<void()> function) {
        registerTest(name, std::move(function));
    }
};

}  // namespace miniran::test

#define TEST_CASE(name)                                                           \
    static void name();                                                           \
    static ::miniran::test::TestRegistrar name##_registrar(#name, &name);        \
    static void name()

#define ASSERT_TRUE(expression) ::miniran::test::assertTrue((expression), #expression, __FILE__, __LINE__)
#define ASSERT_EQ(left, right) ::miniran::test::assertEqual((left), (right), #left, #right, __FILE__, __LINE__)
#define ASSERT_NE(left, right) ::miniran::test::assertNotEqual((left), (right), #left, #right, __FILE__, __LINE__)
#define ASSERT_NEAR(left, right, tolerance) \
    ::miniran::test::assertNear((left), (right), (tolerance), #left, #right, #tolerance, __FILE__, __LINE__)
