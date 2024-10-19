#include <iostream>
#include <sstream>
using namespace std;

int main() {
    while (1) {
        string input_string;
        cout << "[QUASH] $ ";
        getline(cin, input_string);
        cout << input_string << "\n";
    }
}