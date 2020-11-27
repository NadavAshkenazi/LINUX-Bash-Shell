#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"
#include <unistd.h>

using namespace std;

void ctrlZHandler(int sig_num) { //todo: check with pipe and redirections
    cout << "smash: got ctrl-Z" << endl;
    SmallShell& smash = SmallShell::getInstance();
    smash.jobsList->removeFinishedJobs();
    if (smash.jobsList->hasPipeInFg){
        pid_t pipePid1 = smash.jobsList->pipePid1;
        pid_t pipePid2 = smash.jobsList->pipePid2;
        if (pipePid1 != 0){
            //cout << "pipe1" << pipePid1; //todo: debug
            if (kill(pipePid1, SIGSTOP) != 0){
                perror("smash error: kill failed"); // kill is the syscall that failed
                return;
            }
            else{
                JobsList::JobEntry* pipeJob1 = smash.jobsList->getJobByPid(pipePid1);
                pipeJob1->state = STOPPED;
                smash.jobsList->changeJobId(pipeJob1, smash.jobsList->maxJobID + 1);
                smash.jobsList->maxJobID++;
                cout << "smash: process " << pipePid1<< " was stopped" << endl;
            }
        }
        else{
            JobsList::JobEntry* pipeJob1 = smash.jobsList->getJobByPid(pipePid1);
            pipeJob1->state = STOPPED;
            if (pipeJob1->formerJobId == NOVALUE){
                smash.jobsList->changeJobId(pipeJob1, smash.jobsList->maxJobID + 1);
                smash.jobsList->maxJobID++;
            } else {
                smash.jobsList->changeJobId(pipeJob1, pipeJob1->formerJobId);
            }
            cout << "smash: process " << pipePid1<< " was stopped" << endl;
        }
        if (kill(pipePid2, SIGSTOP) != 0){
            perror("smash error: kill failed"); // kill is the syscall that failed
            return;
        }
        else{
            JobsList::JobEntry* pipeJob2 = smash.jobsList->getJobByPid(pipePid2);
            pipeJob2->state = STOPPED;
            if (pipeJob2->formerJobId == NOVALUE){
                smash.jobsList->changeJobId(pipeJob2, smash.jobsList->maxJobID + 1);
                smash.jobsList->maxJobID++;
            } else {
                smash.jobsList->changeJobId(pipeJob2, pipeJob2->formerJobId);
            }

            cout << "smash: process " << pipePid2 << " was stopped" << endl;
        }
        return;
    }
    JobsList::JobEntry* fgJob = smash.jobsList->getFgJob();
    if (fgJob == NULL){
       // cerr << "smash error: there is no foreground job to stop" << endl;
        return;
    }
//    cout << "fgJob jobID in ctrl z" << fgJob->jobID << endl; // todo debug
//    cout << "fgJob PID in ctrl z" << fgJob->command->getPID() << endl; // todo debug
    if (kill(fgJob->command->getPID(), SIGSTOP) != 0){
        perror("smash error: kill failed"); // kill is the syscall that failed
        return;
    }
    fgJob->state = STOPPED;
    if (fgJob->formerJobId == NOVALUE || fgJob->formerJobId == -1){
        smash.jobsList->changeJobId(fgJob, smash.jobsList->maxJobID +  1);
        smash.jobsList->maxJobID++;
    } else {
        smash.jobsList->changeJobId(fgJob, fgJob->formerJobId);
    }
    cout << "smash: process " << fgJob->command->getPID() << " was stopped" << endl;
    return;
}

void ctrlCHandler(int sig_num) { //todo: check with pipe and redirections
    cout << "smash: got ctrl-C" << endl;
    SmallShell& smash = SmallShell::getInstance();
    smash.jobsList->removeFinishedJobs();
    if (smash.jobsList->hasPipeInFg){
        pid_t pipePid1 = smash.jobsList->pipePid1;
        pid_t pipePid2 = smash.jobsList->pipePid2;
        if (kill(pipePid1, SIGKILL) != 0){
            perror("smash error: kill failed"); // kill is the syscall that failed
            return;
        }
        else{
            smash.jobsList->removeJobByPid(pipePid1);
            cout << "smash: process " << pipePid1 << " was killed" << endl;
        }
        if (kill(pipePid2, SIGKILL) != 0){
            perror("smash error: kill failed"); // kill is the syscall that failed
            return;
        }
        else{
            smash.jobsList->removeJobByPid(pipePid2);
            cout << "smash: process " << pipePid2 << " was killed" << endl;
        }
        return;
    }
    JobsList::JobEntry* fgJob = smash.jobsList->getFgJob();

    if (fgJob == NULL){
       // cerr << "smash error: there is no foreground job to kill" << endl; // no need to print according to the instructions
        //cout << "no fg job" << endl; // todo debug
        return;
    }
    //cout << "fgJob.jobID is: " << fgJob->jobID << endl; // todo debug
    //cout << "fgJob->command->getPID() is : " << fgJob->command->getPID() << endl; //todo debug
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
    cout << "smash: got an alarm" << endl;
    SmallShell& smash = SmallShell::getInstance();
//    smash.jobsList->_printTimeoutVector(); //todo: debug
    time_t* tempTime= NULL;
    time_t now = time(tempTime);
    for (int i = 0; i < smash.jobsList->timeoutJobs.size(); i++){
        JobsList::JobEntry* job;
//        cout << "cond 1: " << (smash.jobsList->timeoutJobs[i].pipe == PIPE1) << endl; // todo: debug
//        cout << "cond 2: " << (smash.jobsList->timeoutJobs[i].pipe == PIPE2) << endl; // todo: debug
        if(smash.jobsList->timeoutJobs[i].pipe == PIPE1){
            job = smash.jobsList->getJobByPid(smash.jobsList->pipePid1);
//            smash.jobsList->printJobsList(); // todo: debug
//            cout << "job? " << (job==NULL) << endl; // todo: debug
        }
        else if (smash.jobsList->timeoutJobs[i].pipe == PIPE2){
            job = smash.jobsList->getJobByPid(smash.jobsList->pipePid2);
        }
        else {
            job = smash.jobsList->getJobById(smash.jobsList->timeoutJobs[i].id);
        }
        if (job == NULL){
//            cout << "job is null" << endl;  // todo: debug
            smash.jobsList->removeTimeoutJob(smash.jobsList->timeoutJobs[i].id);
        }
        if (now - job->timeStamp >= smash.jobsList->timeoutJobs[i].sleepTime){
            if (kill(job->command->getPID(), SIGKILL) != 0){
                perror("smash error: kill failed"); // kill is the syscall that failed
                return;
            }
            else{
                string cmd_line1 = job->command->cmd_line;
                cout << "smash: process " << cmd_line1 << " timed out!" << endl;
//                smash.jobsList->removeFinishedJobs(); // todo :check
//                i--;// todo :check
            }
        }
    }
//    cout << "exit loop" << endl; //todo: debug
    smash.jobsList->removeFinishedJobs();
    return;
}

