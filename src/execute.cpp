#include <iostream>
#include <sstream>
#include <iterator>
#include <string>
#include <vector>
using namespace std;

string check_string(string input) {
    // Function that returns the input string, unless the string is a system variable that should be replaced
    return input;
}

void echo_command(const vector<string> &args) {
    for (int i = 0; i < args.size() - 1; i++) {
        cout << args[i] << " ";
    }
    cout << args[args.size() - 1] << "\n";
}

int main() {
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

        // Simplify vector

        // Need to combine terms that are part of a single set of quotation marks
        // Need to merge "<" with next term
        cout << "START VECTOR\n\n";
        for (int i = 0; i < command.size(); i++) {
            cout << command[i] << "\n";
        }
        cout << "\nEND VECTOR\n\n";

        // Basic Cases

        if (command.size() != 0) { // Make sure it's not an empty command (EDGE CASE)
            
            // QUASH Command Mode

            if (command[0] == "echo") {
                //echo_command();
            } else if (command[0] == "export") {
                // DO A DIFFERENT THING
            } else if (command[0] == "cd") {

            } else if (command[0] == "pwd") {
                
            } else if (command[0] == "quit" || command[0] == "exit") {
                
            } else if (command[0] == "jobs") {
                
            }

            // Execution Mode

            // Calculate path
            string path;

            // Fork this process
            // Run execute command to replace it with what we want, passing in arguments with it
            
            // Iterate through vector until we hit the end, a "|", or a ">"
            // These indicate end of current command.
            // If we hit the end of the vector then we just need to run the command and then return to QUASH
            // If we hit a "|" we need to pipe the output
            // If we hit a ">" we need to put the output in a file


        }


        
    }
}