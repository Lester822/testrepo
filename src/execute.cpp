#include <iostream>
#include <sstream>
#include <iterator>
#include <string>
#include <vector>
#include <fstream>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
using namespace std;

// Helper functions:

// Remove quotes:
string remove_quotes(const string& str) {
    // If the string has ""s remove them
    if (str.front() == '"' && str.back() == '"') {
        return str.substr(1, str.size() - 2);
    }
    // If the string doesn't have ""s we don't change it
    return str;
}

vector<string> convert_env_vars(const vector<string>& args) {
    // Replaces environment variables in commands with their value as a string
    vector<string> converted_args;

    for (const string& arg : args) {
        if (arg[0] == '$') {

            string var_name = arg.substr(1);
            const char* var_value = getenv(var_name.c_str());

            if (var_value) {
                converted_args.push_back(string(var_value));
            } else {
                converted_args.push_back("");
            }
        } else {
            converted_args.push_back(arg);
        }
    }
    return converted_args;
}

void echo_command(const vector<string>& args) {
    for (int i = 0; i < args.size() - 1; i++) {
        cout << args[i] << " ";
    }
    cout << args[args.size() - 1] << "\n";
}

void export_command(const vector<string>& args) {

    // If the command is missing
    if (args.size() < 2) {
        cout << "not enough arguments" << endl;
        return;
    }

    string var = args[1];
    int pos_of_equ = var.find('=');

    string variable_name = var;
    string variable_value = "";

    if (pos_of_equ == string::npos) {
        variable_name = var;
        variable_value = "";
    } else {
        variable_name = var.substr(0, pos_of_equ);
        variable_value = var.substr(pos_of_equ + 1);
    }

    if (setenv(variable_name.c_str(), variable_value.c_str(), 1) != 0) {
        cout << "export: failed to set variable " << variable_name << endl;
    }
}

void cd_command(const vector<string>& args) {
    const char* home = getenv("HOME");

    if (args.size() > 1) {
        string path = remove_quotes(args[1]);
        const char* dir = path.c_str();

        if (chdir(dir) != 0) {
            cout << "cd to " << path << " failed." << endl;
        }
    } 
    else {
        if (chdir(home) != 0) {
            cout << "cd failed to change to home." << endl;
        }
    }

    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != nullptr) {
        setenv("PWD", cwd, 1);
    } else {
        cout << "failed to update PWD." << endl;
    }
}

void pwd_command() {
    char* cwd = get_current_dir_name();
    if (cwd != nullptr) {
        cout << cwd << endl;
        free(cwd);
    } else {
        cout << "getcwd failed" << endl;
    }
}

void quexit_command() {
    exit(0);
}

void jobs_command() {
    // Implement jobs functionality if needed
}

vector<pid_t> background_pids; // Global variable to store background PIDs

int main() {
    cout << "Welcome..." << endl;
    while (1) {

        // Get user input

        string input_string;
        cout << "[QUASH] $ ";
        getline(cin, input_string);

        // Split inputted string into vector

        vector<string> command;
        string buffer;

        istringstream stream(input_string);

        string prev = "";
        while (getline(stream, buffer, ' ')) {
            if (prev == "") {
                if (buffer[0] == '"') {
                    prev = buffer;
                } else if (buffer == "<") {
                    prev = "<";
                } else {
                    command.push_back(buffer);
                }

            } else if (prev == "<") {
                command.push_back("<" + buffer);
                prev = "";
            } else {
                prev = prev + " " + buffer;
                if (buffer.back() == '"') {
                    command.push_back(prev);
                    prev = "";
                }
            }
        }

        if (prev != "") {
            command.push_back(prev);
        }

        // Convert command vector to vector of vectors

        vector<vector<string>> commands;
        vector<string> temp_vector;

        int pipeline_count = 0;

        for (int i = 0; i < command.size(); i++) {
            if (command[i] == "&") {
                // Handle background process indicator
                temp_vector.push_back("&");
                commands.push_back(temp_vector);
                temp_vector.clear();
            } else {
                temp_vector.push_back(command[i]);
                if (command[i] == "|" || command[i] == ">" || command[i] == ">>") {
                    commands.push_back(temp_vector);
                    temp_vector.clear();
                    if (command[i] == "|") {
                        pipeline_count++;
                    }
                }
            }
        }
        if (!temp_vector.empty()) {
            commands.push_back(temp_vector);
        }

        // Check if last command is "&" for background processing
        bool is_background = false;
        if (!commands.empty() && commands.back().size() == 1 && commands.back()[0] == "&") {
            is_background = true;
            commands.pop_back(); // Remove "&" from commands
        }

        // Creating Our Pipes
        int num_pipes = pipeline_count;
        int pipe_fds[num_pipes][2];

        for (int k = 0; k < num_pipes; k++) {
            if (pipe(pipe_fds[k]) == -1) {
                perror("pipe");
                exit(EXIT_FAILURE);
            }
        }

        // Command execution loop
        int pipe_index = 0;
        int num_commands = commands.size();
        for (int j = 0; j < num_commands; j++) {

            if (commands[j].size() != 0) { // Ensure it's not an empty command

                string last_elem = commands[j][commands[j].size() - 1];
                int output_fd = -1; // Initialize output_fd to -1

                // Handle output redirection to files
                if (last_elem == ">") {
                    output_fd = open(commands[j + 1][0].c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0644);
                    if (output_fd == -1) {
                        perror("open");
                        continue;
                    }
                    commands[j].pop_back(); // Remove the operator
                    commands.erase(commands.begin() + j + 1); // Remove the filename from commands
                    num_commands -= 1;
                } else if (last_elem == ">>") {
                    output_fd = open(commands[j + 1][0].c_str(), O_CREAT | O_WRONLY | O_APPEND, 0644);
                    if (output_fd == -1) {
                        perror("open");
                        continue;
                    }
                    commands[j].pop_back(); // Remove the operator
                    commands.erase(commands.begin() + j + 1); // Remove the filename from commands
                    num_commands -= 1;
                }

                // Remove the last element if it's an operator (already handled)
                if (last_elem == "" || last_elem == "|" || last_elem == ">" || last_elem == ">>") {
                    commands[j].pop_back();
                }

                // Handle built-in commands
                if (commands[j][0] == "echo") {
                    vector<string> temp_v(commands[j].begin() + 1, commands[j].end());
                    echo_command(convert_env_vars(temp_v));
                    continue;
                } else if (commands[j][0] == "export") {
                    export_command(convert_env_vars(commands[j]));
                    continue;
                } else if (commands[j][0] == "cd") {
                    cd_command(convert_env_vars(commands[j]));
                    continue;
                } else if (commands[j][0] == "pwd") {
                    pwd_command();
                    continue;
                } else if (commands[j][0] == "quit" || commands[j][0] == "exit") {
                    quexit_command();
                } else if (commands[j][0] == "jobs") {
                    jobs_command();
                    continue;
                }

                // External commands and commands that need piping
                pid_t pid = fork();

                if (pid == 0) { // Child process

                    // If not the first command and previous command outputs to a pipe, redirect stdin
                    if (j > 0 && commands[j - 1].back() == "|") {
                        if (dup2(pipe_fds[pipe_index - 1][0], STDIN_FILENO) == -1) {
                            perror("dup2");
                            exit(EXIT_FAILURE);
                        }
                    }

                    // If current command outputs to a pipe, redirect stdout
                    if (last_elem == "|") {
                        if (dup2(pipe_fds[pipe_index][1], STDOUT_FILENO) == -1) {
                            perror("dup2");
                            exit(EXIT_FAILURE);
                        }
                    }

                    // If output redirection to a file
                    if (output_fd != -1) {
                        if (dup2(output_fd, STDOUT_FILENO) == -1) {
                            perror("dup2");
                            exit(EXIT_FAILURE);
                        }
                        close(output_fd);
                    }

                    // Close all pipe file descriptors in the child
                    for (int k = 0; k < num_pipes; k++) {
                        close(pipe_fds[k][0]);
                        close(pipe_fds[k][1]);
                    }

                    // Prepare arguments for execvp
                    vector<char*> args;
                    for (const auto& arg : commands[j]) {
                        args.push_back(const_cast<char*>(arg.c_str()));
                    }
                    args.push_back(nullptr);

                    // Execute the command
                    execvp(args[0], args.data());
                    perror("execvp");
                    exit(EXIT_FAILURE);

                } else if (pid > 0) { // Parent process

                    if (!is_background) {
                        // Wait for the child process if not background
                        waitpid(pid, nullptr, 0);
                    } else {
                        // Background process
                        background_pids.push_back(pid);
                        cout << "Process running in background with PID: " << pid << endl;
                    }

                    // Close pipe ends that are no longer needed
                    if (j > 0 && commands[j - 1].back() == "|") {
                        close(pipe_fds[pipe_index - 1][0]);
                        close(pipe_fds[pipe_index - 1][1]);
                    }

                    // If output redirection to file, close the file descriptor
                    if (output_fd != -1) {
                        close(output_fd);
                    }

                    // Move to next pipe if current command outputs to a pipe
                    if (last_elem == "|") {
                        pipe_index++;
                    }

                } else {
                    // Fork failed
                    perror("fork");
                    exit(EXIT_FAILURE);
                }
            }
        }

        // Close any remaining pipes in the parent process
        for (int k = 0; k < num_pipes; k++) {
            close(pipe_fds[k][0]);
            close(pipe_fds[k][1]);
        }

        // Reap any finished background processes
        for (auto it = background_pids.begin(); it != background_pids.end(); ) {
            pid_t pid = *it;
            int status;
            pid_t result = waitpid(pid, &status, WNOHANG);
            if (result == 0) {
                // Process still running
                ++it;
            } else if (result == -1) {
                // Error occurred
                perror("waitpid");
                it = background_pids.erase(it);
            } else {
                // Process finished
                cout << "Background process with PID: " << pid << " finished" << endl;
                it = background_pids.erase(it);
            }
        }
    }
}