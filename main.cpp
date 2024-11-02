#include <iostream>
#include <stdio.h>
#include <string.h>
#include <vector>
#include <unistd.h>

using namespace std;

vector<const char*> splitString(string input, string delim) {
    int totalArgs = 0;
    std::vector<const char*> strings;

    int pos = 0;
    string token;
    while ((pos = input.find(delim)) != string::npos) {
        token = input.substr(0, pos);
        strings.push_back(token.c_str());
        input = input.substr(pos + 1);
    }
    strings.push_back(input.c_str());

    return strings;
}

void printArgs(vector<const char*> argv, int len) {
    for(int i = 0; i < len; i++){
        cout << argv[i] <<  "";
    }
    cout << endl;
}
void tryExecute(vector<const char*> argv) {
    cout << "PATH" << getenv("PATH") << "PATH" << endl;
    vector<const char*> paths = splitString(getenv("PATH"), ":");
    printArgs(paths, paths.size());
    //execvp(, char *const argv[]);
}
void acceptCommands() {
    string input = "";

    while (true) {
        getline(cin,input); // later split &&
        vector<const char*> argv = splitString(input, " ");
        int argc = argv.size();
        printArgs(argv, argc);

        if (argv[0] == "cd") {
            cout << "cd stuff";
        } else if (argv[0] == "exit") {
            break;
        }

        int pid = 1;//fork();
        if (pid == -1) {
            perror("fork");
            exit(0);
        }
        if (pid > 0) {
            cout << "parent";
            exit(0);
        } else {
            tryExecute(argv);
        }
        //
    }
    exit(0);
}

int main() {

    acceptCommands();
}
