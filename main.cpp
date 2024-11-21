// Holden Ernest - some date :(

// recreate a shell experience from scratch
// Damage Over Time shell - DOTsh

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
#include <fcntl.h>

using namespace std;

// 1 exec
// 3 execv
// 2 &&
// 5 |
// 1 |+
// 3 <
// 3 >
// 3 >>
// 1 | >
// 5 RL stuff
// 1 chdir
// TOTAL 16


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
// https://stackoverflow.com/questions/1461331/writing-my-own-shell-stuck-on-pipes
void pipeCommand(char** cmd1, char** cmd2) {
    int fds[2]; // file descriptors
    pipe(fds);
    int status;
    int pid = -1;
    if (pid = fork() == 0) { // dup command 2 to pipe it to
        dup2(fds[1], 1);
        close(fds[0]);
        close(fds[1]);
        tryExecute(cmd1);
    }
    pid_t terminated_pid = waitpid(pid, &status, 0);
    if (status != 0) {
        cerr << "cannot pipe from '" << cmd1[0] << "'" << endl;
    }
    if (status == 0 && (pid = fork() == 0)) { // dup command 1 to recieve it when exec
        dup2(fds[0], 0);
        close(fds[0]);
        close(fds[1]);
        tryExecute(cmd2);
    }
    close(fds[0]);
    close(fds[1]);
    terminated_pid = waitpid(pid, &status, 0);
}
int getSize(char** arr) {
    int i = 0;
    char* ar = arr[0];
    while (ar != nullptr) {
        ar = arr[++i];
    }
    return i;
}
int findFirst(char** arr, const char* findIt) {
    int i = 0;
    char* ar = arr[0];
    while (ar != nullptr) {
        if (strcmp(ar, findIt) == 0) {
            return i;
        }
        ar = arr[++i];
    }
    return -1;
}
void splitArgsAt(char** in, int splitPos, int totalLen, char*** outleft, char*** outright) {// splits the array at pos into left and right arrays
    int len1 = splitPos;
    int len2 = totalLen - splitPos - 1;
    *outleft = new char*[len1+1]; // remove the split position [1, 2, x, 3] // i=2
    *outright = new char*[len2+1]; // 4 - 2 - 1 = 1
    for (int i = 0; i < len1; i++) {
        (*outleft)[i] = in[i];
    }
    (*outleft)[len1] = nullptr;
    for (int i = len1+1; i < totalLen; i++) {
        int index = i-(splitPos+1);
        (*outright)[index] = in[i];
    }
    (*outright)[len2] = nullptr;
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
string concatArgs(char** args) {
    int i = 0;
    
    char* arg = args[0];
    string ans = "";
    while (arg != nullptr) {
        if (i > 0) {
            ans += " ";  // Add a space before appending the next string
        }
        ans = ans + arg;
        arg = args[++i];
    }
    return ans;
}
void expandAllWords(char** args, wordexp_t* cc) {
    string ca = concatArgs(args);
    int wexp = wordexp(ca.c_str(), cc, 0);
}
void tryRedirectExec(char** left, char** right,  bool app) {
    if (fork() == 0) {
        int fd = -1;
        if (app)
            fd = open(right[0], O_WRONLY | O_CREAT | O_APPEND, 0644);
        else fd = open(right[0], O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd == -1) {
            std::cerr << "Error opening file!" << std::endl;
            exit(1);
        }

        if (dup2(fd, STDOUT_FILENO) == -1) {
            std::cerr << "Error redirecting stdout!" << std::endl;
            exit(1);
        }

        // Step 4: Close the file
        close(fd);
        tryExecute(left);
    }
    wait(NULL);
    exit (0);
}
void tryRedirectFromFile(char** left, char** right) {
    if (fork() == 0) {
        int fd = open(right[0], O_RDONLY, 0644);
        if (fd == -1) {
            std::cerr << "Error opening file!" << std::endl;
            exit(1);
        }

        if (dup2(fd, STDIN_FILENO) == -1) {
            std::cerr << "Error redirecting stdout!" << std::endl;
            exit(1);
        }

        // Step 4: Close the file
        close(fd);
        tryExecute(left);
    }
    wait(NULL);
    exit (0);
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
    exit (1);
}
void tryExecute(char** argv) {
    
    char** left;
    char** right;
    int pipeC = findFirst(argv, "|");
    int ltC = findFirst(argv, "<");
    int gtC = findFirst(argv, ">");
    bool appGT = false;
    if (gtC == -1) {
        gtC = findFirst(argv, ">>");
        if (gtC > 0) {
            appGT = true;
        }
    }
    if (pipeC > 0) {
        splitArgsAt(argv, pipeC, getSize(argv), &left, &right);
        pipeCommand(left, right); // exec of some kind
    } else if (gtC > 0) {
        splitArgsAt(argv, gtC, getSize(argv), &left, &right);
        tryRedirectExec(left,right, appGT);
    } else if (ltC > 0) {
        splitArgsAt(argv, ltC, getSize(argv), &left, &right);
        tryRedirectFromFile(left,right);
    } else {
        //! TODO: use wordExp() now
        // if there arent any weird delimiters, just execute as normal
        auto paths = splitString(getenv("PATH"), ":");
        tryExecuteFromPaths(paths, argv);
    }
    cerr << "GOT TO THINGS" << endl;
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
    if (allCommands == nullptr)
        return;
    
    char** command1 = nullptr;
    char** moreCommands = nullptr;

    int totalLen = getSize(allCommands);
    int splitPos = findFirst(allCommands, "&&");

    bool ignoreExecResult = false;
    if (splitPos > 0) {
        splitArgsAt(allCommands, splitPos, totalLen, &command1, &moreCommands); // command1 is the command to be executed, allCommands are anything in queue
    } else {
        command1 = allCommands;
        moreCommands = nullptr;
    }
    // from this command split off the < and > and >>
    //cout << "printing current command: " << endl;
    //printArgs(command1);
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
    } else {
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
            if (status == 0 || ignoreExecResult) {
                if (moreCommands != nullptr) parseAllCommands(moreCommands, hDir);
            } else {
                //cout << "ignored other half" << endl;
            }
        } else { // child go do the work
            tryExecute(command1);
            // problem?
            exit(1);
        }
    }
}
void acceptCommands() {
    char* in;

    struct passwd *pw = getpwuid(getuid());
    const char* homedir = pw->pw_dir;
    string hd = (addTwoStrings(homedir,"/.dotsh_history"));
    const char* historyDir = hd.c_str();
    read_history(historyDir);

    while (true) {
        auto argv = readInput(&in);

        cout << "\e[A\033[0m" << endl; // for some reason readline cant print unless its on a new line. Workaround: replace the last line with a color I want to use

        parseAllCommands(argv, historyDir);
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
