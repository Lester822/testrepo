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
        cout << "not enough arguements" << endl;
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
            cout << "cd to " << path << " failed: " << endl;
        }
    } 
    else {
        if (chdir(home) != 0) {
            cout << "cd failed to change to home: " << endl;
        }
    }

    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != nullptr) {
        setenv("PWD", cwd, 1);
    } else {
        cout << "failed to update PWD: " << endl;
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
    exit(0);
}

int main() {
    cout << "Welcome..." << endl;
    while (1) {

        // Get user input

        string input_string;
        cout << "[QUASH] $ ";
        getline(cin, input_string);

        // Split inputted string in vector

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

        // Vector to Vector of Vectors

        vector<vector<string>> commands;
        vector<string> temp_vector;

        // cout << "PRESPLIT VECTOR\n\n";
        // for (int i = 0; i < command.size(); i++) {
        //     cout << command[i] << "\n";
        // }
        // cout << "\nPRESPLIT VECTOR\n\n";

        for (int i = 0; i < command.size(); i++) {
            if (i == command.size()-1 && (command[i] == "&")) {
                temp_vector.push_back("");
                commands.push_back(temp_vector);
                commands.push_back({"&"});
            } else {
                temp_vector.push_back(command[i]);
                if (command[i] == "|" || command[i] == ">" || command[i] == ">>") {
                    commands.push_back(temp_vector);
                    temp_vector.clear();
                }
            }
            
        }
        if (commands.empty()) {
            temp_vector.push_back("");
            commands.push_back(temp_vector);
        } else if (commands[commands.size()-1] != vector<string>{"&"}) {
            temp_vector.push_back("");
            commands.push_back(temp_vector);
        }
        

        // cout << "COMMAND LIST START\n\n";
        // for (int j = 0; j < commands.size(); j++) {
        //     cout << "START VECTOR\n\n";
        //     for (int i = 0; i < commands[j].size(); i++) {
        //         cout << commands[j][i] << "\n";
        //     }
        //     cout << "\nEND VECTOR\n\n";
        // }
        // cout << "COMMAND LIST END\n\n";
        // Need to combine terms that are part of a single set of quotation marks
        // Need to merge "<" with next term
        vector<pid_t> child_pids;

        // Prepare previous pipe file descriptors
        int prev_pipe_fd[2] = { -1, -1 };

        // Basic Cases
        for (int j = 0; j < commands.size(); j++) { // For each command

            if (commands[j].size() != 0) { // Make sure it's not an empty command (EDGE CASE)

                string last_elem = commands[j][commands[j].size() - 1];
                bool has_next_pipe = (last_elem == "|");
                int pipe_fd[2];

                // Handle built-in commands without piping
                if (commands[j][0] == "echo" || commands[j][0] == "export" || commands[j][0] == "cd" ||
                    commands[j][0] == "pwd" || commands[j][0] == "quit" || commands[j][0] == "exit" ||
                    commands[j][0] == "jobs") {

                    if (prev_pipe_fd[0] != -1 || has_next_pipe) {
                        cout << "Error: Built-in commands cannot be used with pipes." << endl;
                        break;
                    }

                    if (commands[j][0] == "echo") {
                        vector<string> temp_v(commands[j].begin() + 1, commands[j].end() - 1);
                        echo_command(convert_env_vars(temp_v));
                    } else if (commands[j][0] == "export") {
                        export_command(convert_env_vars(commands[j]));
                    } else if (commands[j][0] == "cd") {
                        cd_command(convert_env_vars(commands[j]));
                    } else if (commands[j][0] == "pwd") {
                        pwd_command();
                    } else if (commands[j][0] == "quit" || commands[j][0] == "exit") {
                        quexit_command();
                    } else if (commands[j][0] == "jobs") {
                        jobs_command();
                    }

                } else {
                    // External Commands

                    if (has_next_pipe) {
                        pipe(pipe_fd);
                    }

                    pid_t pid = fork();

                    if (pid == 0) {
                        // Child process

                        // Handle input from previous pipe
                        if (prev_pipe_fd[0] != -1) {
                            dup2(prev_pipe_fd[0], STDIN_FILENO);
                        }

                        // Handle output to next pipe
                        if (has_next_pipe) {
                            dup2(pipe_fd[1], STDOUT_FILENO);
                        }

                        // Close unused FDs
                        if (prev_pipe_fd[0] != -1) {
                            close(prev_pipe_fd[0]);
                            close(prev_pipe_fd[1]);
                        }
                        if (has_next_pipe) {
                            close(pipe_fd[0]);
                            close(pipe_fd[1]);
                        }

                        // Handle output redirection to file if necessary
                        int output_fd = -1;
                        if (last_elem == ">" || last_elem == ">>") {
                            string outfile = commands[j + 1][0];
                            if (last_elem == ">") {
                                output_fd = open(outfile.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0644);
                            } else if (last_elem == ">>") {
                                output_fd = open(outfile.c_str(), O_CREAT | O_WRONLY | O_APPEND, 0644);
                            }
                            if (output_fd == -1) {
                                perror("open");
                                exit(EXIT_FAILURE);
                            }
                            dup2(output_fd, STDOUT_FILENO);
                        }

                        // Prepare arguments
                        vector<string> cmd_args = commands[j];
                        // Remove last element if it's "|", ">", ">>"
                        if (last_elem == "|" || last_elem == ">" || last_elem == ">>") {
                            cmd_args.pop_back();
                        }
                        vector<char*> args;
                        for (size_t i = 0; i < cmd_args.size(); ++i) {
                            args.push_back(const_cast<char*>(cmd_args[i].c_str()));
                        }
                        args.push_back(nullptr);

                        execvp(args[0], args.data());
                        perror("execvp");
                        exit(EXIT_FAILURE);
                    } else {
                        // Parent process
                        child_pids.push_back(pid);

                        // Close unused FDs
                        if (prev_pipe_fd[0] != -1) {
                            close(prev_pipe_fd[0]);
                            close(prev_pipe_fd[1]);
                        }

                        // Update prev_pipe_fd
                        if (has_next_pipe) {
                            prev_pipe_fd[0] = pipe_fd[0];
                            prev_pipe_fd[1] = pipe_fd[1];
                        } else {
                            prev_pipe_fd[0] = -1;
                            prev_pipe_fd[1] = -1;
                        }
                    }
                }
            }
        }

        // After setting up all pipelines, wait for all child processes
        for (pid_t pid : child_pids) {
            waitpid(pid, nullptr, 0);
        }

    }
}
