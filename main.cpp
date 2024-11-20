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

void tryExecute(char** argv);

//Global Variables------------------
const int PIPE_READ = 0;
const int PIPE_WRITE = 1;
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
void pipeCommand(char** cmd1, char** cmd2) { // https://stackoverflow.com/questions/1461331/writing-my-own-shell-stuck-on-pipes
  int fds[2]; // file descriptors
  pipe(fds);
  // child process #1
  if (fork() == 0) {
    // Reassign stdin to fds[0] end of pipe.
    dup2(fds[0], PIPE_READ);
    close(fds[1]);
    close(fds[0]);
    // Execute the second command.
    // child process #2
    if (fork() == 0) {
        // Reassign stdout to fds[1] end of pipe.
        dup2(fds[1], PIPE_WRITE);
        close(fds[0]);
        close(fds[1]);
        // Execute the first command.
        tryExecute(cmd1);
    }
    wait(NULL); // wait for any child to die
    tryExecute(cmd2);
    }
    close(fds[1]);
    close(fds[0]);
    wait(NULL);
}
void splitArgsAt(char** in, int splitPos, int totalLen, char** outleft, char** outright) {// splits the array at pos into left and right arrays
    int len1 = splitPos + 1;
    int len2 = totalLen - splitPos;
    
    outleft = new char*[len1]; // remove the split position [1, 2, x, 3] // i=2
    outright = new char*[len2]; // 4 - 2 - 1 = 1
    for (int i = 0; i < len1; i++) {
        outleft[i] = in[i];
    }
    outleft[len1] = nullptr;
    for (int i = len1; i < totalLen; i++) {
        outright[i-(splitPos+1)] = in[i];
    }
    outright[len2] = nullptr;
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
    cout << "not a valid command" << endl;
    exit (0);
}
void tryExecute(char** argv) {
    
    // if there arent any weird delimiters, just execute as normal
    auto paths = splitString(getenv("PATH"), ":");
    tryExecuteFromPaths(paths, argv);
    exit(1); // problem
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
void smile() {
    cout << "\n\e[1m\033[96m:)\n \033[0m$\033[30m ";
}
int smile(int a, int b) {
    smile();
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
    rl_command_func_t smile;
    rl_bind_key ('\x02', smile);//ctrl b
}
int getSize(char** arr) {
    int i = 0;
    char* ar = arr[0];
    while (ar != nullptr) {
        ar = arr[++i];
    }
    return i+1;
}
int findFirst(char** arr, const char* findIt) {
    int i = 0;
    char* ar = arr[0];
    while (ar != nullptr) {
        if (ar == findIt) {
            return i;
        }
        ar = arr[++i];
    }
    return -1;
}
char** readInput(char** in) {
    char cwd[1024];
    string cc = string(getcwd(cwd, sizeof(cwd)));
    string hd = getHomeDir();
    replace(cc, hd, "~");
    string linehead =  "\033[32m" + cc + " \033[0m$\033[30m ";
    *in = readline(linehead.c_str()); // later split &&
    if (**in) add_history(*in);
    auto argv = splitString(*in, " "); // this is so sad, we wasted our time

    return argv;
}
void parseAllCommands(char** allCommands, const char* hDir) {
    char** command1;

    int totalLen = getSize(allCommands);
    int splitPos = findFirst(allCommands, "&&");
    cout << splitPos << " is the split" << endl;
    if (splitPos > 0)
        splitArgsAt(allCommands, 0, totalLen, command1, allCommands); // command1 is the command to be executed, allCommands are anything in queue
    else {
        command1 = allCommands;
        allCommands = nullptr;
    }
    printArgs(command1);
    if (!command1[0]) return;
    string command = string(command1[0]);

    if (command == "cd") {
        int c = chdir(command1[1]);
        if (c == -1) {
            cout << "no such directory" << endl;
        }
    } else if (command == "exit") {
        write_history(hDir);
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
        if (terminated_pid == 0) {
            cout << "command successfully executed" << endl;
        }
        cout << "SAfafsfsa";
        //if 0 command success
    } else { // child go do the work
        cout << "things" << endl;
        tryExecute(command1);
        // problem?
        exit(0);
    }

    //if (allCommands != nullptr) parseAllCommands(allCommands, hDir);
    //command1 = nullptr;
}
void acceptCommands() {
    char* in;

    wordexp_t * wordsP;

    struct passwd *pw = getpwuid(getuid());
    const char* homedir = pw->pw_dir;
    string hd = (addTwoStrings(homedir,"/.dotsh_history"));
    const char* historyDir = hd.c_str();
    read_history(historyDir);

    while (true) {
        auto argv = readInput(&in);

        cout << "\e[A\033[0m" << endl; // for some reason readline cant print unless its on a new line. Workaround: replace the last line with a color I want to use

        parseAllCommands(argv, historyDir);
        
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
