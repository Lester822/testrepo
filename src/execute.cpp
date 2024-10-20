#include <iostream>
#include <sstream>
#include <iterator>
#include <string>
#include <vector>
#include <fstream>
#include <unistd.h>
#include <stdlib.h>
using namespace std;

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
    }
    // Potential error handling location
}

void cd_command(const vector<string>& args) {
    const char* home = getenv("HOME");

    // If the number of args provided are more than 1 (not just cd )
    if (args.size() > 1) {
        const char* path = args[1].c_str();

        if (chdir(path) != 0) {
            cout << "cd to" << path << " failed"; // Handle error locations
        }
        else {
            if (chdir(home) != 0) {
                cout << "cd failed to change to home"; // Handle error locations
            }
        }
    }
    // Need an update to PWD?
    char cwd[1024];
    if (getcwd(cwd, size(cwd)) != nullptr) {
        setenv("PWD", cwd, 1);
    }
    else {
        cout << "failed to update PWD";
    }
}

int main() {
    cout << "Welcome...";
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
                    
                }

                // Execution Mode

                // Calculate path
                string path;

                // Fork this process
                // Run execute command to replace it with what we want, passing in arguments with it
                if (last_elem == ">") {
                    cout.rdbuf(og_output); // Restores cout to terminal
                    fileOut.close();
                }

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
