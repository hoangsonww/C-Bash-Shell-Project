#include "tests.hpp"

#include <cstdlib>

#include "main.c"
#include "shell.h"

const int EXECUTE_POINTS_PER_TEST_CASE = 2;

// Not easily possible to test that enough memory was allocated for argv or
// argv's char* elements See README section create_command
SAFE_TEST(CreateCommand, Basic, {
    for (int argc = 0; argc < 30; argc++) {
        command* cmd = create_command(argc);
        ASSERT_NE(nullptr, cmd->argv);

        for (int i = 0; i < argc; i++) {
            char* arg = cmd->argv[i];
            ASSERT_NE(nullptr, arg);
        }

        ASSERT_EQ(NULL, cmd->argv[argc]);

        cleanup(cmd);
    }
})

SAFE_TEST(Parse, cdNoArgs, {
    char input[] = "cd";
    command* rv = parse(input);
    EXPECT_EQ(1, rv->argc);
    EXPECT_STREQ("cd", rv->argv[0]);
    EXPECT_EQ(NULL, rv->argv[1]);
    cleanup(rv);
})

SAFE_TEST(Parse, cdOneArg, {
    char input[] = "cd /mnt/cdrom";
    command* rv = parse(input);
    EXPECT_EQ(2, rv->argc);
    EXPECT_STREQ("cd", rv->argv[0]);
    EXPECT_STREQ("/mnt/cdrom", rv->argv[1]);
    EXPECT_EQ(NULL, rv->argv[2]);
    cleanup(rv);
})

SAFE_TEST(Parse, lsTwoArgs, {
    char input[] = "ls -l -a";
    command* rv = parse(input);
    EXPECT_EQ(3, rv->argc);
    EXPECT_STREQ("ls", rv->argv[0]);
    EXPECT_STREQ("-l", rv->argv[1]);
    EXPECT_STREQ("-a", rv->argv[2]);
    EXPECT_EQ(NULL, rv->argv[3]);
    cleanup(rv);
})

SAFE_TEST(Parse, lsWithWhitespaces, {
    // Identical to above command but with insignificant extra whitespace
    // Should result in identical command struct after parse is called
    // strtok handles this case properly, if used correctly
    char input[] = "   ls   -l  -a   ";
    command* rv = parse(input);
    EXPECT_EQ(3, rv->argc);
    EXPECT_STREQ("ls", rv->argv[0]);
    EXPECT_STREQ("-l", rv->argv[1]);
    EXPECT_STREQ("-a", rv->argv[2]);
    EXPECT_EQ(NULL, rv->argv[3]);
    cleanup(rv);
})

SAFE_TEST(FindFullPath, mv, {
    command cmd;
    cmd.argc = 1;
    cmd.argv = new char*[cmd.argc + 1];
    cmd.argv[0] = new char[MAX_ARG_LEN];
    strncpy(cmd.argv[0], "mv", MAX_ARG_LEN);
    cmd.argv[1] = NULL;
    EXPECT_TRUE(find_full_path(&cmd));
    EXPECT_STREQ("/usr/bin/mv", cmd.argv[0]);
    delete[] cmd.argv[0];
    delete[] cmd.argv;
})

SAFE_TEST(FindFullPath, findfs, {
    command cmd;
    cmd.argc = 1;
    cmd.argv = new char*[cmd.argc + 1];
    cmd.argv[0] = new char[MAX_ARG_LEN];
    strncpy(cmd.argv[0], "findfs", MAX_ARG_LEN);
    cmd.argv[1] = NULL;
    EXPECT_TRUE(find_full_path(&cmd));
    EXPECT_STREQ("/usr/sbin/findfs", cmd.argv[0]);
    delete[] cmd.argv[0];
    delete[] cmd.argv;
})

SAFE_TEST(FindFullPath, objdumpWithArgs, {
    command cmd;
    cmd.argc = 3;
    cmd.argv = new char*[cmd.argc + 1];
    for (int i = 0; i < 3; i++) cmd.argv[i] = new char[MAX_ARG_LEN];
    strncpy(cmd.argv[0], "objdump", MAX_ARG_LEN);
    strncpy(cmd.argv[1], "-s", MAX_ARG_LEN);
    strncpy(cmd.argv[2], "file.o", MAX_ARG_LEN);
    cmd.argv[3] = NULL;
    EXPECT_TRUE(find_full_path(&cmd));
    EXPECT_STREQ("/usr/bin/objdump", cmd.argv[0]);
    for (int i = 0; i < 3; i++) delete[] cmd.argv[i];
    delete[] cmd.argv;
})

SAFE_TEST(FindFullPath, nonexistentcommand, {
    command cmd;
    cmd.argc = 1;
    cmd.argv = new char*[cmd.argc + 1];
    cmd.argv[0] = new char[MAX_ARG_LEN];
    strncpy(cmd.argv[0], "nonexistentcommand", MAX_ARG_LEN);
    cmd.argv[1] = NULL;
    EXPECT_FALSE(find_full_path(&cmd));
    EXPECT_STREQ("nonexistentcommand", cmd.argv[0]);
    delete[] cmd.argv[0];
    delete[] cmd.argv;
})

/**
 * Checks whether the stdout of ./main < data/in*.txt is the same as the
 * contents of data/out*.txt
 *
 * This test case and the in*.txt and out*.txt files assume that this code is
 * run in an Ubuntu environment from the root of this repo
 */
TEST(Execute, RunMainFuncOnDataDirFilesAndCompareOutput) {
    // Doesn't have to be mmapped because the child doesn't write to this, only
    // reads
    std::vector<std::string> input_files;
    // Must be mmapped because the child writes to this variable
    // Without mmap, the child would operate on its copy of this, so the parent
    // wouldn't see the modifcation
    int* shared_pass = (int*)mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE,
                                  MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    for (const fs::directory_entry& dir_entry :
         fs::recursive_directory_iterator(DATA_DIR))
        if (dir_entry.is_regular_file() &&
            dir_entry.path().string().find("in") != std::string::npos)
            input_files.push_back(dir_entry.path().string());
    RecordProperty("total", size(input_files) * EXECUTE_POINTS_PER_TEST_CASE);
    run_with_signal_catching([&input_files, &shared_pass]() {
        const int argc = 1;
        for (auto& input_file : input_files) {
            const char* argv[] = {"./main", NULL};

            // Redirect stdin from input file
            int input_fd = open(input_file.c_str(), O_RDONLY);
            int original_stdin_fd = dup(STDIN_FILENO);
            dup2(input_fd, STDIN_FILENO);
            close(input_fd);

            // Since _main prints to stdout, capture stdout
            int pipe_fd[2];
            pipe(pipe_fd);
            int original_stdout_fd = dup(STDOUT_FILENO);
            dup2(pipe_fd[1], STDOUT_FILENO);
            close(pipe_fd[1]);

            // Since _main calls exit, need to fork
            pid_t pid = fork();
            if (pid == 0) {
                // Child process, run _main
                _main(argc, argv);
                // Shouldn't go past this point
                perror("Child process failed to exit!");
                exit(-errno);
            } else {
                // Parent process, wait for child to finish
                int status;
                waitpid(pid, &status, 0);

                fflush(stdout);
                dup2(original_stdout_fd, STDOUT_FILENO);
                close(original_stdout_fd);

                char buffer[BUFSIZE_FOR_CAPTURING_STDOUT];
                ssize_t bytes_read =
                    read(pipe_fd[0], buffer, sizeof(buffer) - 1);
                buffer[bytes_read] = '\0';
                close(pipe_fd[0]);

                std::string captured_stdout = buffer;
                std::string expected_output_path = input_file;
                size_t pos = expected_output_path.find("in");
                if (pos != std::string::npos)
                    expected_output_path.replace(pos, 2, "out");
                std::ifstream expected_output_file(expected_output_path);
                std::string expected(
                    (std::istreambuf_iterator<char>(expected_output_file)),
                    std::istreambuf_iterator<char>());
                std::string actual = captured_stdout;
                if (expected.compare(actual) == 0)
                    *shared_pass += EXECUTE_POINTS_PER_TEST_CASE;

                EXPECT_STREQ(expected.c_str(), actual.c_str())
                    << "after calling _main with\n\targc: " << argc
                    << "\n\targv: {\"" << argv[0]
                    << "\", NULL}\n\tstdin from: " << input_file
                    << "\nfile with expected stdout: " << expected_output_path;

                // Restore stdin
                dup2(original_stdin_fd, STDIN_FILENO);
                close(original_stdin_fd);
            }
        }
    });
    RecordProperty("pass", *shared_pass);
    munmap(shared_pass, sizeof(int));
}
