/*
 * Catch2 v2.x single-header subset for LaprdusTTS tests
 *
 * This is a minimal subset of Catch2 for basic testing.
 * For full Catch2, download from: https://github.com/catchorg/Catch2
 *
 * Usage:
 *   #define CATCH_CONFIG_MAIN  // Only in one source file
 *   #include "catch.hpp"
 */

#ifndef CATCH_HPP_MINIMAL
#define CATCH_HPP_MINIMAL

#include <string>
#include <sstream>
#include <iostream>
#include <vector>
#include <functional>
#include <cmath>
#include <cstring>

namespace Catch {

// Approx class for floating point comparison
class Approx {
public:
    explicit Approx(double value) : m_value(value), m_epsilon(0.00001) {}

    Approx& epsilon(double eps) {
        m_epsilon = eps;
        return *this;
    }

    friend bool operator==(double lhs, Approx const& rhs) {
        return std::abs(lhs - rhs.m_value) < rhs.m_epsilon;
    }

    friend bool operator==(Approx const& lhs, double rhs) {
        return rhs == lhs;
    }

    friend bool operator!=(double lhs, Approx const& rhs) {
        return !(lhs == rhs);
    }

    friend std::ostream& operator<<(std::ostream& os, Approx const& a) {
        os << "Approx( " << a.m_value << " )";
        return os;
    }

private:
    double m_value;
    double m_epsilon;
};

// Test result tracking
struct TestResult {
    bool passed = true;
    std::string message;
};

// Simple test registry
class TestRegistry {
public:
    struct TestCase {
        std::string name;
        std::string tags;
        std::function<void()> func;
    };

    static TestRegistry& instance() {
        static TestRegistry reg;
        return reg;
    }

    void addTest(const std::string& name, const std::string& tags, std::function<void()> func) {
        tests.push_back({name, tags, func});
    }

    int run() {
        int passed = 0, failed = 0;

        for (auto& test : tests) {
            currentTest = &test;
            sectionDepth = 0;

            std::cout << "Running: " << test.name;
            if (!test.tags.empty()) {
                std::cout << " " << test.tags;
            }
            std::cout << std::endl;

            try {
                test.func();
                if (!currentFailed) {
                    passed++;
                    std::cout << "  PASSED" << std::endl;
                } else {
                    failed++;
                    std::cout << "  FAILED" << std::endl;
                }
            } catch (const SkipException&) {
                std::cout << "  SKIPPED" << std::endl;
            } catch (const std::exception& e) {
                failed++;
                std::cout << "  FAILED (exception): " << e.what() << std::endl;
            }

            currentFailed = false;
        }

        std::cout << "\n" << passed << " passed, " << failed << " failed\n";
        return failed > 0 ? 1 : 0;
    }

    void reportFailure(const std::string& file, int line, const std::string& expr) {
        currentFailed = true;
        std::cout << "\n  " << file << ":" << line << ": REQUIRE( " << expr << " )\n";
    }

    void enterSection(const std::string& name) {
        sectionDepth++;
        std::cout << "  Section: " << name << std::endl;
    }

    void exitSection() {
        sectionDepth--;
    }

    bool shouldRunSection(const std::string& name) {
        return true;  // Simplified - always run all sections
    }

    class SkipException : public std::exception {};

    void skip(const std::string& reason) {
        std::cout << "  SKIP: " << reason << std::endl;
        throw SkipException();
    }

    std::ostringstream& captureStream() { return captureBuffer; }
    void printCapture() {
        if (!captureBuffer.str().empty()) {
            std::cout << "  with: " << captureBuffer.str() << std::endl;
            captureBuffer.str("");
        }
    }

private:
    std::vector<TestCase> tests;
    TestCase* currentTest = nullptr;
    bool currentFailed = false;
    int sectionDepth = 0;
    std::ostringstream captureBuffer;
};

// Auto-registration helper
struct AutoReg {
    AutoReg(const char* name, const char* tags, void(*func)()) {
        TestRegistry::instance().addTest(name, tags, func);
    }
};

}  // namespace Catch

// Main test macros
#define TEST_CASE(name, tags) \
    static void CATCH_UNIQUE_NAME(test_func)(); \
    static Catch::AutoReg CATCH_UNIQUE_NAME(auto_reg)(name, tags, CATCH_UNIQUE_NAME(test_func)); \
    static void CATCH_UNIQUE_NAME(test_func)()

#define SECTION(name) \
    if (Catch::TestRegistry::instance().shouldRunSection(name)) \
        for (int _section_once = (Catch::TestRegistry::instance().enterSection(name), 1); \
             _section_once; \
             _section_once = (Catch::TestRegistry::instance().exitSection(), 0))

#define DYNAMIC_SECTION(name) SECTION(name)

#define REQUIRE(expr) \
    do { \
        if (!(expr)) { \
            Catch::TestRegistry::instance().reportFailure(__FILE__, __LINE__, #expr); \
        } \
    } while(0)

#define REQUIRE_FALSE(expr) REQUIRE(!(expr))

#define CHECK(expr) REQUIRE(expr)

#define INFO(msg) \
    do { Catch::TestRegistry::instance().captureStream() << msg; } while(0)

#define CAPTURE(var) INFO(#var << " := " << var)

#define SKIP(reason) Catch::TestRegistry::instance().skip(reason)

// Helper for unique names
#define CATCH_UNIQUE_NAME_LINE2(name, line) name##line
#define CATCH_UNIQUE_NAME_LINE(name, line) CATCH_UNIQUE_NAME_LINE2(name, line)
#define CATCH_UNIQUE_NAME(name) CATCH_UNIQUE_NAME_LINE(name, __LINE__)

// Main function
#ifdef CATCH_CONFIG_MAIN
int main(int argc, char* argv[]) {
    return Catch::TestRegistry::instance().run();
}
#endif

#endif // CATCH_HPP_MINIMAL
