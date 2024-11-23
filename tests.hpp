#include <fcntl.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include <filesystem>
#include <fstream>
#include <functional>
#include <ios>
#include <random>

#include "gtest/gtest.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"

void run_with_signal_catching(const std::function<void()>& test_body);

#define SAFE_TEST(TestSuiteName, TestName, TestBody) \
    TEST(TestSuiteName, TestName) {                  \
        run_with_signal_catching([]() { TestBody }); \
    }

#define SAFE_TEST_F(TestSuiteName, TestName, TestBody)   \
    TEST_F(TestSuiteName, TestName) {                    \
        run_with_signal_catching([this]() { TestBody }); \
    }

namespace fs = std::filesystem;
const fs::path DATA_DIR("data");

const unsigned long BUFSIZE_FOR_CAPTURING_STDOUT = 1 << 16;

/**
 * For test cases with "random" in the full name, use this number to generate
 * this many (sets of) random inputs
 */
constexpr int NUM_RANDOM_TRIALS = 5;

/**
 * Read at most dest_len characters from /dev/urandom into dest
 */
static void sys_random(void* dest, size_t dest_len) {
    char* buffer = reinterpret_cast<char*>(dest);
    std::ifstream stream("/dev/urandom",
                         std::ios_base::binary | std::ios_base::in);
    stream.read(buffer, dest_len);
}

/**
 * @note Singleton, seeded once when first called
 * @return mt19937 generator used to provide randomness when generating a number
 * from a distribution
 */
static std::mt19937& get_rng() {
    static std::mt19937 gen([] {
        std::uint_least32_t seed;
        sys_random(&seed, sizeof(seed));
        return std::mt19937(seed);
    }());
    return gen;
}

void run_with_signal_catching(const std::function<void()>& test_body) {
    int pipefd[2];

    if (pipe(pipefd) != 0) {
        FAIL() << "run_with_signal_handling: pipe call failed";
        return;
    }

    pid_t pid = fork();
    if (pid == 0) {
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        dup2(pipefd[1], STDERR_FILENO);
        close(pipefd[1]);
        test_body();
        if (::testing::Test::HasFailure()) _Exit(1);
        _Exit(0);
    } else {
        close(pipefd[1]);
        std::ostringstream output;
        char buffer[1024];
        ssize_t count;
        while ((count = read(pipefd[0], buffer, sizeof(buffer))) > 0)
            output.write(buffer, count);
        close(pipefd[0]);
        std::string raw_output = output.str();
        std::string trimmed_output =
            raw_output.substr(0, raw_output.find_last_not_of("\n") + 1);
        int status;
        waitpid(pid, &status, 0);
        if (WIFSIGNALED(status)) {
            int signal = WTERMSIG(status);
            std::string error_message = trimmed_output;
            if (!trimmed_output.empty()) error_message.append("\n");
            error_message.append("Test crashed with signal: " +
                                 std::string(strsignal(signal)));
            FAIL() << error_message;
        } else if (WIFEXITED(status)) {
            if (WEXITSTATUS(status) != 0) FAIL() << trimmed_output;
        }
    }
}

int main(int argc, char* argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

#pragma GCC diagnostic pop
