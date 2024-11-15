#include <iostream>
#include <stdio.h>
#include <string.h>
#include <vector>
#include <unistd.h>
#include <sys/wait.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <sys/types.h>
#include <pwd.h>
#include <wordexp.h>

using namespace std;

//Global Variables------------------

// END Global Variables-------------

void makeArrayBigger(char*** fullArray, int* totalArrLen) {
    int newLen = *totalArrLen * 2; // double the size
    char** newArray = new char*[newLen];

    for (int i = 0; i < *totalArrLen; i++) {
        newArray[i] = (*fullArray)[i]; // copy
    }
    
    delete[] *fullArray;
    *fullArray = newArray; // point the array to the new stuff
    *totalArrLen = newLen;
}

void printArgs(char** argv) {
    cout << "args: " << endl;
    int len = 0;
    char* arg = argv[0];
    while (arg != nullptr){
        cout << string(arg) <<  " ";
        arg = argv[++len];
    }
    cout << endl;
}
void printArgs(char** argv, int len) {
    cout << "args of len: " << endl;
    for (int i = 0; i < len; i++){
        cout << string(argv[i]) <<  " ";
    }
    cout << endl;
}
char** splitString(string input, string delim) { // LEGACY (kinda)
    if (input == "") return NULL;

    int totalArrLen = 8; // assume 1 arg, then 1 spot for nullptr
    int pos = 0;
    int totalArgs = 0;

    char** fullArray = new char*[totalArrLen]; // copy the array to a new array with an extra entry
    string token;
    while ((pos = input.find(delim)) != string::npos) {
        token = input.substr(0, pos); // find whatever string is next

        if (totalArgs+2 >= totalArrLen) { // +2 because you need to add the last token and the nullptr
            makeArrayBigger(&fullArray, &totalArrLen); // if there are ever too many elements make the array bigger
        }
        //cout << "adding " << token << " to " << totalArgs << endl;
        fullArray[totalArgs] = new char[token.size() + 1];
        strcpy(fullArray[totalArgs], token.c_str()); // add it to the array
        totalArgs++;
        input = string(input.substr(pos + 1));
    }
    token = input.substr(pos+1);
    fullArray[totalArgs] = new char[token.size() + 1];
    strcpy(fullArray[totalArgs++], token.c_str());
    fullArray[totalArgs] = nullptr;
    return fullArray;
}
void tryExecuteFromPaths(char** paths, char** argv) {
    int len = 0;
    char* arg = paths[0];
    while (arg != nullptr){
        //cout << (string(arg) + "/" + string(argv[0])).c_str() << endl;
        execv((string(arg) + "/" + string(argv[0])).c_str(), argv);
        arg = paths[++len];
    }
}
void tryExecute(char** argv) {
    auto paths = splitString(getenv("PATH"), ":");
    tryExecuteFromPaths(paths, argv);
    exit(0);
}
std::string addTwoStrings(const std::string& a, const std::string& b)
{
    return a + b;
}
void clearConsole() {
    system("clear"); // I dont think this is legal unfortunatly
    cout << "\e[1m\033[96m[DOTSH] \033[0m$\033[30m ";
}
int clearConsole(int a, int b) {
    clearConsole();
    return 0;
}
string getHomeDir() {
    return getenv("HOME");
}
bool replace(std::string& str, const std::string& from, const std::string& to) {
    size_t start_pos = str.find(from);
    if(start_pos == std::string::npos)
        return false;
    str.replace(start_pos, from.length(), to);
    return true;
}
void setupHotkeys() {
    rl_initialize();
    rl_command_func_t clearConsole; // declare it as a certain type of function so bind_key can use it properly
    rl_bind_key ('\x0C', clearConsole);//ctrl l
}
char** readInput(char** in, wordexp_t* wordsP) {
    char cwd[1024];
    string cc = string(getcwd(cwd, sizeof(cwd)));
    string hd = getHomeDir();
    replace(cc, hd, "~");
    string linehead =  "\033[32m" + cc + " \033[0m$\033[30m ";
    *in = readline(linehead.c_str()); // later split &&
    if (**in) add_history(*in);
    //auto argv = splitString(input, " "); // this is so sad, we wasted our time
    int wexp = wordexp(*in, wordsP, 0);
    return wordsP->we_wordv;
}
void acceptCommands() {
    char* in;
    //rl_attempted_completion_function = some_tab_completion_function_override;

    wordexp_t * wordsP;

    struct passwd *pw = getpwuid(getuid());
    const char* homedir = pw->pw_dir;
    string hd = (addTwoStrings(homedir,"/.dotsh_history"));
    const char* historyDir = hd.c_str();
    read_history(historyDir);

    while (true) {

        auto argv = readInput(&in, wordsP);
        cout << "\e[A\033[0m" << endl; // for some reason readline cant print unless its on a new line. Workaround: replace the last line with a color I want to use

        //printArgs(argv);
        if (!argv[0]) continue;
        string command = string(argv[0]);
        
        if (command == "cd") {
            chdir(argv[1]);
            continue;
        } else if (command == "exit") {
            write_history(historyDir);
            exit(0);
        }

        // Forking stuff --------------------

        int pid = fork();
        if (pid == -1) {
            perror("fork");
            exit(0);
        }
        if (pid > 0) {
            // this is a parent :)
            int status;
            pid_t terminated_pid = waitpid(pid, &status, 0);
        } else { // child go do the work
            tryExecute(argv);
        }
        wordfree(wordsP);
        free(in); // free the memory for each command
    }
}
void initConsole() {
    //TODO: maybe add some
}

int main() {
    initConsole();
    setupHotkeys();
    acceptCommands();
}
