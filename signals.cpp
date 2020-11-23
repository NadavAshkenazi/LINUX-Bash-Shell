#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"

using namespace std;

void ctrlZHandler(int sig_num) { //todo check with pipe and redirections
    cout << "smash got ctrl-Z" << endl;
    SmallShell& smash = SmallShell::getInstance();
    smash.jobsList->printFirstJobs();
    JobsList::JobEntry* fgJob = smash.jobsList->getFgJob();
    if (kill(fgJob->command->getPID(), SIGSTOP) != 0){
        perror("smash error: kill failed"); // kill is the syscall that failed
        return;
    }
    cout << "smash: process" << fgJob->command->getPID() << "was stopped" << endl;
}

void ctrlCHandler(int sig_num) {
  // TODO: Add your implementation
}

void alarmHandler(int sig_num) {
  // TODO: Add your implementation
}

