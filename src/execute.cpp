#include <iostream>
#include <sstream>
#include <iterator>
#include <string>
#include <vector>
using namespace std;

int main() {
    while (1) {
        string input_string;
        cout << "[QUASH] $ ";
        getline(cin, input_string);
        cout << input_string << "\n";

        // Split Inputted String in Vector

        vector<string> command;
        string buffer;

        istringstream stream(input_string);

        while (getline(stream, buffer, ' ')) {
            command.push_back(buffer);
        }
        
    }
}