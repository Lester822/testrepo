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

string check_string(string input) {
    // Function that returns the input string, unless the string is a system variable that should be replaced
    return input;
}

void echo_command(const vector<string>& args) {
    for (int i = 0; i < args.size() - 1; i++) {
        cout << args[i] << " ";
    }
    cout << args[args.size() - 1] << "\n";
}

void quexit_command() {
    exit(0);
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

void sigchld_handler(int sig) {
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

int main() {
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);

    // Set up the SIGCHLD handler
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART; // To restart interrupted system calls
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

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
            temp_vector.push_back(command[i]);
            if (command[i] == "|" || command[i] == ">" || command[i] == ">>") {
                commands.push_back(temp_vector);
                temp_vector.clear();
            }
        }
        temp_vector.push_back("");
        commands.push_back(temp_vector);

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
        

        // Basic Cases
        for (int j = 0; j < commands.size(); j++) { // For each command

            if (commands[j].size() != 0) { // Make sure it's not an empty command (EDGE CASE)
                
                string last_elem = commands[j][commands[j].size()-1] ;
                ofstream fileOut;
                streambuf* og_output = cout.rdbuf(); // Stores original cout (to terminal)
                // If last elem is a "" it means standard operation, and final command
                // If last elem is a | it means the output should be piped
                // If last elem is a > it means output should go to file stored in commands[j+1]
                // If last elem is a >> it means output should be appended to file stored in commands[j+1]

                if (last_elem == ">") {
                    fileOut.open(commands[j+1][0]);
                    // Redirecting cout to write to "output.txt"
                    cout.rdbuf(fileOut.rdbuf());
                }

                if (last_elem == ">>") {
                    fileOut.open(commands[j+1][0], ios::app);
                    // Redirecting cout to write to "output.txt"
                    cout.rdbuf(fileOut.rdbuf());
                }

                // QUASH Command Mode

                if (commands[j][0] == "echo") {
                    vector<string> temp_v(commands[j].begin() + 1, commands[j].end() - 1);
                    echo_command(temp_v);
                } else if (commands[j][0] == "export") {
                    // DO A DIFFERENT THING
                } else if (commands[j][0] == "cd") {
                    cd_command(commands[j]);
                } else if (commands[j][0] == "pwd") {
                    pwd_command();
                } else if (commands[j][0] == "quit" || command[0] == "exit") {
                    quexit_command();
                } else if (commands[j][0] == "jobs") {
                    
                } else {

                    // SOLUTION FOR ">" and ">>" for these commands

                    ofstream fileOut;
                    int output_fd = -1; // -1 indicates NO file output

                    if (last_elem == ">") { // Open files
                        output_fd = open(commands[j + 1][0].c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0644);
                    } else if (last_elem == ">>") {
                        output_fd = open(commands[j + 1][0].c_str(), O_CREAT | O_WRONLY | O_APPEND, 0644);
                    }

                    pid_t pid = fork(); // Fork process so that we can execvp the child

                    int has_and = 0;
                    if (commands[j].size() > 2) {
                        if (commands[j][commands[j].size()-2] == "&") {
                            has_and = 1;
                        }
                    }
                    
                    if (pid == 0) {

                        if (output_fd != -1) {
                            dup2(output_fd, STDOUT_FILENO);
                            close(output_fd);
                        }

                        vector<char*> args; // Vector to hold command args

                        for (int i = 0; i < commands[j].size()-(1 + has_and); i++) { // Assembles args argument by argument
                            args.push_back(const_cast<char*>(commands[j][i].c_str())); // Converts the vector of strings into a vector of char* one element at a time
                        }

                        args.push_back(nullptr); // Adds nullptr to back as needed

                        if (has_and == 1) {
                            pid_t child_pid = getpid();
                            setpgid(child_pid, child_pid); // Set child in its own process group

                            // Redirect stdin to /dev/null
                            int devNullIn = open("/dev/null", O_RDONLY);
                            dup2(devNullIn, STDIN_FILENO);
                            close(devNullIn);

                            // Redirect stdout and stderr to /dev/null
                            int devNullOut = open("/dev/null", O_WRONLY);
                            dup2(devNullOut, STDOUT_FILENO);
                            dup2(devNullOut, STDERR_FILENO);
                            close(devNullOut);
                        }

                        execvp(args[0], args.data()); // Replaces current program with execvp

                    } else {
                        if (has_and == 0) {
                            waitpid(pid, nullptr, 0);
                            if (output_fd != -1) {
                                close(output_fd);
                            }
                        } else {
                            cout << "Process running in background with PID: " << pid << endl;
                            setpgid(pid, pid); // Ensure child is in its own process group
                            tcsetpgrp(STDIN_FILENO, getpgrp()); // Set shell as foreground process group
                            if (output_fd != -1) {
                                close(output_fd);
                            }
                        }
                    }

                    if (last_elem == ">" || last_elem == ">>") {
                        cout.rdbuf(og_output); // Restores cout to terminal
                        fileOut.close();
                    }
                }

                // Execution Mode

                // Fork this process
                // Run execute command to replace it with what we want, passing in arguments with it


                if (last_elem != "|") {
                    break;
                }
                
                // Iterate through vector until we hit the end, a "|", or a ">"
                // These indicate end of current command.
                // If we hit the end of the vector then we just need to run the command and then return to QUASH
                // If we hit a "|" we need to pipe the output
                // If we hit a ">" we need to put the output in a file


            }

        }
    }
}
