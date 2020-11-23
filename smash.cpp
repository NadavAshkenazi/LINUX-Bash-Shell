#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include "signals.h"
#include <sys/wait.h>
#include "Commands.h"


int main(int argc, char* argv[]) {

    if(signal(SIGTSTP , ctrlZHandler)==SIG_ERR) {
        perror("smash error: failed to set ctrl-Z handler");
    }
    if(signal(SIGINT , ctrlCHandler)==SIG_ERR) {
        perror("smash error: failed to set ctrl-C handler");
    }
    //TODO: setup sig alarm handler - with sigaction as instructed
    SmallShell& smash = SmallShell::getInstance();

    while(true) {
        std::cout << smash.currentPrompt;
        std::string cmd_line;
        std::getline(std::cin, cmd_line);
        char * plastPwd = NULL;
        const char *cstr = cmd_line.c_str();
        smash.executeCommand(cmd_line.c_str());

    }
    return 0;
}