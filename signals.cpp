#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"
#include <unistd.h>

using namespace std;

void ctrlZHandler(int sig_num) { //todo: check with pipe and redirections
    cout << "smash: got ctrl-Z" << endl;
    SmallShell& smash = SmallShell::getInstance();
    JobsList::JobEntry* fgJob = smash.jobsList->getFgJob();
    if (kill(fgJob->command->getPID(), SIGSTOP) != 0){
        perror("smash error: kill failed"); // kill is the syscall that failed
        return;
    }
    fgJob->state = STOPPED;
    smash.jobsList->changeJobId(fgJob);
    cout << "smash: process " << fgJob->command->getPID() << " was stopped" << endl;
    return;
}

void ctrlCHandler(int sig_num) { //todo: check with pipe and redirections
    cout << "smash got ctrl-c" << endl;
    SmallShell& smash = SmallShell::getInstance();
    JobsList::JobEntry* fgJob = smash.jobsList->getFgJob();
    if (kill(fgJob->command->getPID(), SIGKILL) != 0){
        perror("smash error: kill failed"); // kill is the syscall that failed
        return;
    }
    pid_t deletedPid = fgJob->command->getPID();
    smash.jobsList->removeJobById(fgJob->jobID);
    cout << "smash: process " << deletedPid << " was killed" << endl;
    return;
}

void alarmHandler(int sig_num) {
  // TODO: Add your implementation
    cout << "smash got alarm" << endl;
    SmallShell& smash = SmallShell::getInstance();
    JobsList::JobEntry* timeoutJob = smash.jobsList->getTimeoutJob();
    if (timeoutJob != NULL){
        if (kill(timeoutJob->command->getPID(), SIGKILL) != 0){
            perror("smash error: kill failed"); // kill is the syscall that failed
            return;
        }
    }
    string cmd_line = timeoutJob->command->cmd_line;
    smash.jobsList->removeJobById(timeoutJob->jobID);
    cout << "smash: process " << cmd_line << " timed out!" << endl;
    return;
}

