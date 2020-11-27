#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"
#include <sstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <typeinfo>


using namespace std;

const std::string WHITESPACE = " \n\r\t\f\v";

#if 0
#define FUNC_ENTRY()  \
  cerr << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT()  \
  cerr << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif

#define MAX_ARGS_NUM 20
#define MAX_PWD_SIZE 4096 // TODO: check
#define ARGS_NUM_CD 1
#define ARGS_NUM_KILL 2
#define ARGS_NUM_BG 1
#define STDIN 0
#define STDOUT 1
#define STDERR 2
#define MAX_SIZE 80
#define DEBUG_PRINT cerr << "DEBUG: "
#define SIGSTOP 19
#define SIGCONT 18
#define EXEC(path, arg) \
  execvp((path), (arg));

//**************************************
// Auxiliary Functions
//**************************************
string _ltrim(const std::string& s)
{
  size_t start = s.find_first_not_of(WHITESPACE);
  return (start == std::string::npos) ? "" : s.substr(start);
}
string _rtrim(const std::string& s)
{
  size_t end = s.find_last_not_of(WHITESPACE);
  return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}
string _trim(const std::string& s)
{
  return _rtrim(_ltrim(s));
}
int _parseCommandLine(const char* cmd_line, char** args) {
  FUNC_ENTRY()
  int i = 0;
  std::istringstream iss(_trim(string(cmd_line)).c_str());
  for(std::string s; iss >> s; ) {
    args[i] = (char*)malloc(s.length()+1);
    memset(args[i], 0, s.length()+1);
    strcpy(args[i], s.c_str());
    args[++i] = NULL;
  }
  return i;

  FUNC_EXIT()
}
bool _isBackgroundComamnd(const char* cmd_line) {
  const string str(cmd_line);
  return str[str.find_last_not_of(WHITESPACE)] == '&';
}
void _removeBackgroundSign(char* cmd_line) {
  const string str(cmd_line);
  // find last character other than spaces
  unsigned int idx = str.find_last_not_of(WHITESPACE);
  // if all characters are spaces then return
  if (idx == string::npos) {
    return;
  }
  // if the command line does not end with & then return
  if (cmd_line[idx] != '&') {
    return;
  }
  // replace the & (background sign) with space and then remove all tailing spaces.
  cmd_line[idx] = ' ';
  // truncate the command line string up to the last non-space character
  cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}


//**************************************
// Command
//**************************************
Command::Command(const char *cmd_line): _pid(-2),isFinished(false){//, cmd_line(cmd_line) {

//    string tempCmd = string (cmd_line); // todo remove
//    cmd_line = tempCmd.c_str(); // todo remove

    this->cmd_line = (char*)malloc(strlen(cmd_line)+1); //tempCmdLine [strlen(cmd_line)+1];
    strcpy (this->cmd_line, cmd_line);
    //cmd_line = tempCmdLine;

    char** args_temp = (char**)malloc((MAX_ARGS_NUM+1)*sizeof(char*));
    int args_size = _parseCommandLine(cmd_line, args_temp);
    for (int i=0; i< args_size ; i++){
        args.push_back(args_temp[i]);
    }

}
Command::~Command() {}
pid_t Command::getPID(){return _pid;}
string Command::getCommandName() {return string(cmd_line); }
bool Command::getisFinished() {return isFinished;}

//**************************************
// ExternalCommand
//**************************************
ExternalCommand::ExternalCommand(const char* cmd_line, JobsList* jobs, bool toSetGrpPid): Command(cmd_line), jobs(jobs), toSetGrpPid(toSetGrpPid) {
    _wait = !_isBackgroundComamnd(cmd_line);
    clean_cmd_line= string(cmd_line);
    _removeBackgroundSign((char*)clean_cmd_line.c_str());

    JobState state = BG;
    if (_wait)
        state = FG;
    _jobID = jobs->addJob(this, state);
}
void ExternalCommand::execute(){
    pid_t pid = fork();

    if (pid < 0){
        perror("smash error: fork failed");
        return;
    }
    if (pid == 0){ // child process
        setpgrp();

        char* args_to_bash[] = {"/bin/bash",
                                "-c",
                                (char*)clean_cmd_line.c_str(),
                                NULL};
        //kill (getpid(), SIGTSTP); // debug
        execv(args_to_bash[0], args_to_bash);
        jobs->removeJobById(_jobID);
        jobs->maxJobID -- ;
        perror("smash error: execv failed");
        return;
    }
    else{ // shell
        _pid = pid;
        //kill (getpid(), SIGTSTP); // TODO: debug
        if (_wait){
            waitpid(pid,NULL,WUNTRACED);
        }
    }
}
void ExternalCommand::executePipe(){
    char* args_to_bash[] = {"/bin/bash",
                            "-c",
                            (char*)clean_cmd_line.c_str(),
                            NULL};
    execv(args_to_bash[0], args_to_bash);
    jobs->removeJobById(_jobID);
    jobs->maxJobID -- ;
    perror("smash error: execv failed");
    return;
    }
void ExternalCommand::setPid(pid_t pid){
    this->_pid = pid;
}
//**************************************
// PipeCommand
//**************************************

void trimForPipe(const char* cmd_line, string* cmd1, string* cmd2, bool* regularPipe){
    string s_cmd = string(cmd_line);
    int pipeLocation;

    if (s_cmd.find("|&") != string::npos) {
        *regularPipe = false;
        pipeLocation = s_cmd.find("|&");
    } else if (s_cmd.find("|") != string::npos) {
        //cout << "in | if" << endl; // todo debug
        *regularPipe = true;
        pipeLocation = s_cmd.find("|");
    }

    bool isBackground = _isBackgroundComamnd(cmd_line);
    *cmd1 = _trim(s_cmd.substr(0, pipeLocation)) + (isBackground ? " &" : "");
    int sizeOfPipe = regularPipe ? 1 : 2;
    *cmd2 = _trim(s_cmd.substr(pipeLocation + sizeOfPipe));
}

PipeCommand::PipeCommand(const char* cmd_line, JobsList* jobs): Command(cmd_line), _pid1(-1), _pid2(-1), jobs(jobs){
    trimForPipe(cmd_line, &_cmd1, &_cmd2, &regularPipe);
    jobs->hasPipeInFg = true;
};
void PipeCommand::execute() {
    int fd[2];
    if (pipe(fd) == -1) {
        perror("smash error: pipe failed");
        return;
    }

    SmallShell &smash = SmallShell::getInstance();
    Command* firstCommand= smash.CreateCommand(_cmd1.c_str());
    Command* secondCommand= smash.CreateCommand(_cmd2.c_str());

    pid_t pid1 = fork();
    if (pid1 < 0) {
        perror("smash error: fork failed");
        return;
    }else if (pid1 == 0) {  // first command
        setpgrp();

        int fdEntry = regularPipe ? STDOUT : STDERR;
        if (dup2(fd[1], fdEntry) == -1) {
            perror("smash error: dup2 failed");
            return;
        }
        if (close(fd[0]) == -1) {
            perror("smash error: close failed");
            return;
        }
        if (close(fd[1]) == -1) {
            perror("smash error: close failed");
            return;
        }

        firstCommand->executePipe();
        exit(0);
    } else { // shell code
        _pid1 = pid1;
        firstCommand->setPid(_pid1);

        pid_t pid2 = fork();
        if (pid2 < 0) {
            perror("smash error: fork failed");
            return;
        }
        if (pid2 == 0) { // second command
            setpgrp();

            if (dup2(fd[0], STDIN) == -1) {
                perror("smash error: dup2 failed");
                return;
            }
            if (close(fd[0]) == -1) {
                perror("smash error: close failed");
                return;
            }
            if (close(fd[1]) == -1) {
                perror("smash error: close failed");
                return;
            }
            secondCommand->executePipe();
            exit(0);
        } else { // shell code
            if (close(fd[0]) == -1) {
                perror("smash error: close failed");
                return;
            }
            if (close(fd[1]) == -1) {
                perror("smash error: close failed");
                return;
            }
            _pid2 = pid2;
            secondCommand->setPid(_pid2);
            jobs->pipePid1= _pid1;
            jobs->pipePid2= _pid2;
            cout << "pid1 from pipe = " << pid1 << endl;// todo debug
            cout << "pid2 from pipe = " << pid2 << endl;// todo debug

            if (!_isBackgroundComamnd(cmd_line)){
//                kill (getpid(), SIGALRM); // TODO: debug
                waitpid( _pid1, NULL, WUNTRACED);
                waitpid( _pid2, NULL, WUNTRACED);
            }
            jobs->hasPipeInFg = false;
//            jobs->pipePid1= 0;
//            jobs->pipePid2= 0;
        }
    }
    return;
}
pid_t PipeCommand::getPID() {
    //cout << "enter pipe pid " << endl; // todo debug
    return _pid2;
}

//**************************************
// RedirectionCommand
//**************************************
RedirectionCommand::RedirectionCommand(const char* cmd_line): Command(cmd_line){
    int signLocation;
    bool isBackground = _isBackgroundComamnd(cmd_line);
    _removeBackgroundSign((char*)cmd_line);
    string s_cmd = string(cmd_line);

    if (s_cmd.find(">>") != string::npos) {
        override = false;
        signLocation = s_cmd.find(">>");
    } else if (s_cmd.find(">") != string::npos) {
        override = true;
        signLocation = s_cmd.find(">");
    }
    //cout << "signLocation = "<<signLocation << endl; // todo debug
    cmd = _trim(s_cmd.substr(0, signLocation)) + (isBackground ? " &" : "");
    int sizeOfSign = override ? 1 : 2;
    fileName = _trim(s_cmd.substr(signLocation + sizeOfSign));
//    cout << "cmd= "<< cmd << endl; // todo debug
//    cout << "fileName= "<< fileName << endl; // todo debug
}
void RedirectionCommand::execute() {

    int stdoutFd = dup(STDOUT);
    //cout <<  "stdoutFd is " << stdoutFd << endl; //todo debug
    close(STDOUT);
    int fd;

    if (override) {
        if ((fd = open(fileName.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRWXU)) == -1) {
            perror("smash error: open failed");
            return;
        }
    }
    else {
        if ((fd = open(fileName.c_str(), O_CREAT | O_RDWR | O_APPEND, S_IRWXU)) == -1){
            perror("smash error: open failed");
            return;
        }
    }

    SmallShell::getInstance().executeCommand(cmd.c_str());

    close(fd);
    dup2(stdoutFd,STDOUT);
    close(stdoutFd);
    return;
}

//**************************************
// BuiltInCommand
//**************************************
BuiltInCommand::BuiltInCommand(const char *cmd_line) :Command(cmd_line)  {
    vector<string> ignoreArgs{">", "<", "<<", ">>", "|","&"};

    for (int i = 0; i < args.size(); i++){
        for (vector<string>::iterator it_ignore = ignoreArgs.begin(); it_ignore != ignoreArgs.end(); ++it_ignore) {
            if (args[i] == *it_ignore) {
                args.erase(args.begin() + i);
                i--;
            }
        }
    }
}
void BuiltInCommand::executePipe(){
    this->execute();
}

//**************************************
// ChangeDirCommand
//**************************************
ChangeDirCommand::ChangeDirCommand(const char* cmd_line, char** plastPwd) :BuiltInCommand(cmd_line), plastPwd(plastPwd){}
void ChangeDirCommand::execute() {
    if (args.size() - 1 > ARGS_NUM_CD) {
        cerr << "smash error: cd: too many arguments" << endl;
        return;
    }
    if (args.size() - 1 < ARGS_NUM_CD) {
        cerr <<"smash error: cd: too few arguments" << endl;
        return;
    }

    if (string(args[ARGS_NUM_CD]) != "-") {
        char *temp = *plastPwd; // just in case cd failed
        char *pwd = (char *) malloc(MAX_PWD_SIZE);
        getcwd(pwd, MAX_PWD_SIZE);
        *plastPwd = pwd;

        if (chdir((const char *) (args[ARGS_NUM_CD]).c_str()) != 0) { // cd failed
            perror("smash error: chdir failed");
            *plastPwd = temp;
            return;
        }
    } else {
        if (*plastPwd == NULL) {
            cerr <<"smash error: cd: OLDPWD not set" << endl;
            return;
        } else {
            args[ARGS_NUM_CD] = *plastPwd;
            ChangeDirCommand::execute();
        }
    }
}

//**************************************
// GetCurrDirCommand
//**************************************
GetCurrDirCommand::GetCurrDirCommand(const char* cmd_line) :BuiltInCommand(cmd_line) {}
void GetCurrDirCommand::execute() {
    char pwd[MAX_PWD_SIZE];
    getcwd(pwd, MAX_PWD_SIZE);
    string s_pwd= string(pwd);
    cout << s_pwd << endl;
}

//**************************************
// ShowPidCommand
//**************************************
ShowPidCommand::ShowPidCommand(const char* cmd_line) :BuiltInCommand(cmd_line) {}
void ShowPidCommand::execute() {
    pid_t currPid = getpid();
    cout << "smash pid is " << currPid << endl;
}

//**************************************
// QuitCommand
//**************************************
QuitCommand::QuitCommand(const char* cmd_line) :BuiltInCommand(cmd_line) {
    jobs = SmallShell::getInstance().jobsList;
    killAllJobs = false;
    if (args.size() > 1){
        killAllJobs = (args[1] == "kill");
    }
}
void QuitCommand::execute() {
    if (killAllJobs){
        jobs->killAllJobs();
    }
    delete this;
    exit(0);
}

//**************************************
// JobsCommand
//**************************************
JobsCommand::JobsCommand(const char* cmd_line, JobsList* jobs) :BuiltInCommand(cmd_line), jobs(jobs) {}
void JobsCommand::execute() {
    this->jobs->printJobsList();
}

//**************************************
// KillCommand
//**************************************
KillCommand::KillCommand(const char* cmd_line, JobsList* jobs, bool print) :BuiltInCommand(cmd_line), print(print), jobs(jobs) {
    //jobs = SmallShell::getInstance().jobsList;
}
void KillCommand::execute() {
    if (args.size()-1 < ARGS_NUM_KILL){
        cerr <<"smash error: kill: invalid arguments" << endl;
        return;
    }
    if (args[1].find("-") == -1){
        cerr <<"smash error: kill: invalid arguments" << endl;
        return;
    }
    try{
        jobIdToKill = stoi(args[ARGS_NUM_KILL]);
    } catch (invalid_argument) {
        cerr <<"smash error: kill: invalid arguments" << endl;
        return;
    }
    try{
        signum = stoi(args[1].substr(args[1].find("-")+1));
    } catch (invalid_argument) {
        cerr <<"smash error: kill: invalid arguments" << endl;
        return;
    }
    if ((signum < 1) | (signum > 31)){
        cerr <<"smash error: kill: invalid arguments" << endl;
        return;
    }
    JobsList::JobEntry* jobToKill = jobs->getJobById(jobIdToKill);
    if (jobToKill == NULL){
        cerr << "smash error: kill: job-id " + args[ARGS_NUM_KILL] + " does not exist" << endl;
        return;
    }
    pidToKill = jobToKill->command->getPID();

//    cout << "signum is " << signum << endl; // todo debug
//      cout << "pidToKill: " << pidToKill << endl; // todo debug
//      cout << "this pid " << getpid() << endl; // todo debug

    if (print)
        cout << "signal number " << signum << " was sent to pid " << pidToKill << endl;
    if(kill(pidToKill, signum) == -1){
        perror("smash error: kill failed");
        return;
    }
    waitpid(pidToKill,NULL,WUNTRACED);
    jobs->removeFinishedJobs(); // TODO: check why need to kill twice
    if (signum == SIGSTOP){
        jobs->changeJobStatus(jobIdToKill, STOPPED);
    }
    if (signum == SIGCONT){
        jobs->changeJobStatus(jobIdToKill, BG);
    }
    return;
}

//**************************************
// ForegroundCommand
//**************************************
ForegroundCommand::ForegroundCommand(const char* cmd_line) :BuiltInCommand(cmd_line) {
    jobs = SmallShell::getInstance().jobsList;
}
void ForegroundCommand::execute(){
    if (args.size()-1 > ARGS_NUM_BG){
        cerr <<"smash error: fg: invalid arguments" << endl;
        return;
    }
    if (args.size() == 1){ // no specific jobId to resume
        if (jobs->jobList.size() == 0){
            cerr <<"smash error: fg: jobs list is empty" << endl;
            return;
        }
        JobIdToResume= jobs->getLastJobId();
        if (JobIdToResume == -1){
            cerr <<"smash error: fg: jobs list is empty" << endl;
            return;
        }
    } else {
        JobIdToResume= stoi(_trim(args[ARGS_NUM_BG]));
        if (jobs->getJobById(JobIdToResume) == NULL){
            cerr <<"smash error: fg: job-id "+to_string(JobIdToResume)+" does not exist" << endl;
            return;
        }
    }

    string killCmd = "kill -"+to_string(SIGCONT)+" "+to_string(JobIdToResume);
    KillCommand* killCommand= new KillCommand(killCmd.c_str(), false);
    JobsList::JobEntry* jobToResume= jobs->getJobById(JobIdToResume);
    cout << jobToResume->command->getCommandName() << ":" << jobs->getJobById(JobIdToResume)->command->getPID() << endl;
    jobs->resetJobTimerById(JobIdToResume);
    jobs->changeJobId(jobToResume, -1);
    jobs->changeJobStatus(jobToResume->jobID, FG);
//    cout << "the job id is now: " << jobToResume->jobID << endl; // todo debug
//    cout << "the former job id is now: " << jobToResume->formerJobId << endl; // todo debug
//    cout << "state is now: " << jobToResume->state << endl; // todo debug
    killCommand->execute();
    waitpid(jobToResume->command->getPID(),NULL,WUNTRACED);
    return;
}

//**************************************
// BackgroundCommand
//**************************************
BackgroundCommand::BackgroundCommand(const char* cmd_line) :BuiltInCommand(cmd_line) {
    jobs = SmallShell::getInstance().jobsList;
}
void BackgroundCommand::execute() {
    if (args.size()-1 > ARGS_NUM_BG){
        cerr <<"smash error: bg: invalid arguments" << endl;
        return;
    }
    if (args.size() == 1){ // no specific jobId to resume
        JobIdToResume= jobs->getLastStoppedJobId();
        if (JobIdToResume == -1){
            cerr <<"smash error: bg: there is no stopped jobs to resume" << endl;
            return;
        }
    } else {
        JobIdToResume= stoi(_trim(args[ARGS_NUM_BG]));
        if (jobs->getJobById(JobIdToResume) == NULL){
            cerr << "smash error: bg: job-id "+to_string(JobIdToResume)+" does not exist" << endl;
            return;
        }
        if (jobs->getJobById(JobIdToResume)->state != STOPPED){
            cerr << "smash error: bg: job-id "+to_string(JobIdToResume)+" is already running in the background" << endl;
            return;
        }
    }

    string killCmd = "kill -"+to_string(SIGCONT)+" "+to_string(JobIdToResume);
    KillCommand* killCommand= new KillCommand(killCmd.c_str(), false);
    JobsList::JobEntry* jobToResume= jobs->getJobById(JobIdToResume);
    cout << jobToResume->command->getCommandName() << ":" << jobs->getJobById(JobIdToResume)->command->getPID() << endl;
    jobs->resetJobTimerById(JobIdToResume);
    jobs->changeJobStatus(jobToResume->jobID, BG);
    //jobs->changeJobId(jobToResume, jobToResume->formerJobId); // todo: maybe remove?
    killCommand->execute();
    return;
}

//**************************************
// lsCommand
//**************************************
lsCommand::lsCommand(const char* cmd_line):BuiltInCommand(cmd_line){}
void lsCommand::execute(){
    struct dirent **namelist;
    int n;

    n = scandir(".", &namelist, NULL, alphasort);
    if (n == -1) {
        perror("smash error: scandir failed");
        return;
    }

    for (int i = 0; i < n; i++) {
        cout << namelist[i]->d_name << endl; // TODO: change so it won't print "." and ".."
        free(namelist[i]);
    }
    free(namelist);
}

//**************************************
// ChangePromptCommand
//**************************************
ChangePromptCommand::ChangePromptCommand(const char *cmd_line, string* currentPrompt) :BuiltInCommand(cmd_line),
                                                                                        currentPromp(currentPrompt) {}
void ChangePromptCommand::execute() {
    if (this->args.size() == 1){
        *currentPromp = "smash> "
                        "";
    }
    else{
        *currentPromp = this->args[1] + "> ";
    }
}

//**************************************
// Timeout Command
//**************************************

bool _isBuiltIn(string cmd_s){
    if (cmd_s.find("bg") != std::string::npos){
        return true;
    }
    else if (cmd_s.find("fg") != std::string::npos){
        return true;
    }
    else if (cmd_s.find("cd") != std::string::npos){
        return true;
    }
    else if (cmd_s.find("pwd") != std::string::npos){
        return true;
    }
    else if (cmd_s.find("showpid") != std::string::npos){
        return true;
    }
    else if (cmd_s.find("jobs") != std::string::npos){
        return true;
    }
    else if (cmd_s.find("quit") != std::string::npos){
        return true;
    }
    else if (cmd_s.find("chprompt") != std::string::npos) {
        return true;
    }
    else if (cmd_s.find("kill") != std::string::npos){
        return true;
    }
    else if (cmd_s.find("ls") != std::string::npos){
        return true;
    }
    return false;
}

TimeoutCommand::TimeoutCommand(const char* cmd_line, JobsList* jobs):Command(cmd_line), jobs(jobs){}
void TimeoutCommand::execute() {
    string command = "";
    for (int i = 2; i < args.size(); i++){
        command += args[i];
        if (i + 1 < args.size())
            command += " ";
    }
    SmallShell& smash = SmallShell::getInstance();
    pid_t pid = fork();
    if (pid < 0){
        perror("smash error: fork failed");
        return;
    }

    int sleep = stoi(args[1]);

    if (pid == 0){ // child process
        setpgrp();
        time_t* tempTime= NULL;
        time_t start = time(tempTime);
        time_t now = start;
        while (now < start + sleep){
            now = time(tempTime);
        }
        kill(smash.smashPid, SIGALRM);
        exit(0);
    }
    else{ // shell
        if (string(cmd_line).find("|") != std::string::npos){
            string cmd1;
            string cmd2;
            bool temp = false;
            trimForPipe(command.c_str(), &cmd1, &cmd2, &temp);
            if (!_isBuiltIn(cmd1)){
                if (_isBackgroundComamnd(cmd1.c_str())){
                    smash.jobsList->addTimeoutJob(smash.jobsList->maxJobID+1, sleep, PIPE1);
                }
                else{
                    smash.jobsList->addTimeoutJob(-1, sleep, PIPE1);
                }
            }
            if (!_isBuiltIn(cmd2)){
                if (_isBackgroundComamnd(cmd2.c_str())){
                    smash.jobsList->addTimeoutJob(smash.jobsList->maxJobID+1, sleep, PIPE2);
                }
                else{
                    smash.jobsList->addTimeoutJob(-1, sleep, PIPE2);
                }
            }
        }
        else{
            if (_isBackgroundComamnd(command.c_str())){
                smash.jobsList->addTimeoutJob(smash.jobsList->maxJobID+1, sleep);
            }
            else{
                smash.jobsList->addTimeoutJob(-1, sleep);
            }
        }
        smash.executeCommand(command.c_str());
        return;
    }
}

//**************************************
// JobsList
//**************************************

JobsList::JobEntry::JobEntry(Command* cmd, int jobID, JobState state): command(cmd), state(state), jobID(jobID), formerJobId(NOVALUE) {
    time_t* temp_time= NULL;
    timeStamp = time(temp_time);
}
JobsList::JobsList(): maxJobID(0){
    vector <JobEntry> jobList;
    vector<timeoutJob> timeoutJobs;
}
JobsList::~JobsList(){ // TODO: think
//    for (vector<JobEntry>::iterator it = jobList.begin() ; it != jobList.end(); ++it){
//        delete (*it);
//    }
}
int JobsList::addJob(Command* cmd, JobState state){

    removeFinishedJobs();
    if (state==BG){
        JobEntry newJob = JobEntry(cmd, maxJobID+1, state);
        maxJobID++;
        jobList.push_back(newJob);
    } else if (state==FG){
        if ((getJobById(-1) != NULL) && !hasPipeInFg){
            removeJobById(-1);
        }
        JobEntry newJob = JobEntry(cmd, -1, state);
        jobList.push_back(newJob);
    }

    return maxJobID;

}
void JobsList::printJobsList(){

    removeFinishedJobs();
    //cout << "pipePid1 " << pipePid1 << endl; //todo: debug
    //cout << "pipePid2 " << pipePid2 << endl; //todo: debug
    for (vector<JobEntry>::iterator it = jobList.begin() ; it != jobList.end(); ++it){

        if ((*it).state == FG)
            continue;

        cout << "[" << (*it).jobID << "]" ;
        cout << (*it).command->getCommandName() << " : " ;
        cout << (*it).command->getPID() << " ";

        time_t* current_time =  NULL;
        double timeElapsed = difftime (time(current_time), (*it).timeStamp);
        cout << timeElapsed << " secs";

        if ((*it).state == STOPPED){
            cout << " (stopped)";
        }

        cout << endl;
    }

}
void JobsList::removeFinishedJobs(){
    for(int i=0 ; i < jobList.size() ; i ++ ){
        //cout << "check if finishedsize: " << jobList[i].command->getPID(); // todo debug
        if (jobList[i].command->getPID() == -2) // init value
            continue;
        if (waitpid(jobList[i].command->getPID(), NULL, WNOHANG) != 0){//== jobList[i].command->getPID()){
            //cout << waitpid(jobList[i].command->getPID(), NULL, WNOHANG) << endl; // todo debug
            delete jobList[i].command;
            jobList.erase(jobList.begin() +i);
            removeTimeoutJob(jobList[i].jobID);
//            cout << "timeout to remove " << jobList[i].jobID << endl; // todo:debug
            i--; // to conform with joblist size
        }
    }
}
JobsList::JobEntry* JobsList::getJobById(int jobId){
    for (vector<JobEntry>::iterator it = jobList.begin() ; it != jobList.end(); ++it){
//        cout << "[" << (*it).jobID  << "]"<< (*it).command->getCommandName() << endl; //todo: debug
        if ((*it).jobID == jobId){
            return &(*it);
        }
    }
    return NULL;
}
JobsList::JobEntry* JobsList::getJobByPid(pid_t pid){
    for (vector<JobEntry>::iterator it = jobList.begin() ; it != jobList.end(); ++it){
        if ((*it).command->getPID() == pid){
            return &(*it);
        }
    }
    return NULL;
}
void JobsList::changeJobStatus (int jobId, JobState state){
   JobEntry* jobToChange = getJobById(jobId);
   if (jobToChange != NULL)
       jobToChange->state = state;
}
JobsList::JobEntry* JobsList::getFgJob() {
    for (vector<JobEntry>::iterator it = jobList.begin() ; it != jobList.end(); ++it){
        if ((*it).state == FG){
            return &(*it);
        }
    }
    return NULL;
}
void JobsList::printFirstJobs(){
   cout << jobList.begin()->command->getCommandName() <<  jobList.begin()->state << endl;
}
void JobsList::removeJobById(int jobId){
    for(int i=0 ; i < jobList.size() ; i ++ ){
        if (jobList[i].jobID == jobId){
            delete jobList[i].command;
            jobList.erase(jobList.begin() +i);
            removeTimeoutJob(jobId);
        }
    }
    return;
}
void JobsList::removeJobByPid(pid_t pid){
    for(int i=0 ; i < jobList.size() ; i ++ ){
        if (jobList[i].command->getPID() == pid){
            int jobId = jobList[i].jobID;
            delete jobList[i].command;
            jobList.erase(jobList.begin() +i);
            removeTimeoutJob(jobId);
        }
    }
    return;
}
void JobsList:: changeJobId(JobsList::JobEntry* job, int newId){
    job->formerJobId = job->jobID;
    job->jobID = newId;
}
void JobsList::addTimeoutJob(int jobId, int sleepTime, int pipe) {
    timeoutJob newJob = timeoutJob(jobId, sleepTime, pipe);
    timeoutJobs.push_back(newJob);
}
JobsList::JobEntry* JobsList::getTimeoutJob(int pipe){
    for (vector<timeoutJob>::iterator it = timeoutJobs.begin() ; it != timeoutJobs.end(); ++it){
        JobEntry* job = getJobById((it->id));
        time_t* tempTime= NULL;
        time_t now = time(tempTime);
        if (now - job->timeStamp >= it->sleepTime){
            if (pipe == 1){
                if (job->command->getPID() == pipePid1){
                    return job;
                }
            }
            if (pipe == 2){
                if (job->command->getPID() == pipePid2){
                    return job;
                }
            }
            else{
                return job;
            }
        }
    }
    return NULL;
}
void JobsList::removeTimeoutJob(int jobId){
    for (int i = 0; i< timeoutJobs.size(); i++){
        if (timeoutJobs[i].id == jobId) {
            timeoutJobs.erase(timeoutJobs.begin() + i);
            return;
        }
    }
    return;
}
void JobsList::killAllJobs(){
    cout << "sending SIGKILL signal to " << jobList.size() << " jobs:" << endl;
    for(int i=0 ; i < jobList.size() ; i ++ ){
        cout << jobList[i].command->getPID() << ": " << jobList[i].command->getCommandName() << endl;
        kill(jobList[i].command->getPID(), 9);
        delete jobList[i].command;
        jobList.erase(jobList.begin() + i);
        i--;
    }
};
int JobsList::getLastStoppedJobId() {
    for (int i = jobList.size() - 1; i > -1; i--) { // going over jobList from last to first
        if (jobList[i].state == STOPPED)
            return jobList[i].jobID;
    }
    return -1;
}
void JobsList::resetJobTimerById(int jobId){
    for(int i=0 ; i < jobList.size() ; i ++ ){
        if (jobList[i].jobID == jobId){
            jobList[i].timeStamp= time(NULL);
            return;
        }
    }
    return;
}
int JobsList::getLastJobId(){
    for (int i = jobList.size() - 1; i > -1; i--) { // going over jobList from last to first
        if (jobList[i].jobID > -1)
            return jobList[i].jobID;
    }
    return -1;
}

void JobsList::_printTimeoutVector(){
    for (vector<timeoutJob>::iterator it = timeoutJobs.begin() ; it != timeoutJobs.end(); ++it){
        cout << "[" << it->id << "]" << getJobById(it->id)->command->getCommandName() <<  " " << it->pipe << endl;
    }
};

//**************************************
// SmallShell
//**************************************
SmallShell::SmallShell(): currentPrompt("smash> "),plastPwd(NULL) {
    jobsList = new JobsList();
    smashPid = getpid();
    if (smashPid <0){
        perror("smash error: getpid failed");
    }
}
SmallShell::~SmallShell() {
    delete jobsList;
    free(plastPwd);
}
// Creates and returns a pointer to Command class which matches the given command line (cmd_line)
Command * SmallShell::CreateCommand(const char* cmd_line) {
    string cmd_s = string(cmd_line);
    if (cmd_s.find("timeout") != std::string::npos) {
        return new TimeoutCommand(cmd_line, this->jobsList);
    }
    else if (cmd_s.find(">") != std::string::npos){
        return new RedirectionCommand(cmd_line);
    }
    else if (cmd_s.find("|") != std::string::npos){
        return new PipeCommand(cmd_line, this->jobsList);
    }
    else if (cmd_s.find("bg") != std::string::npos){
        return new BackgroundCommand(cmd_line);
    }
    else if (cmd_s.find("fg") != std::string::npos){
        return new ForegroundCommand(cmd_line);
    }
    else if (cmd_s.find("cd") != std::string::npos){
        return new ChangeDirCommand(cmd_line, &plastPwd);
    }
    else if (cmd_s.find("pwd") != std::string::npos){
        return new GetCurrDirCommand(cmd_line);
    }
    else if (cmd_s.find("showpid") != std::string::npos){
        return new ShowPidCommand(cmd_line);
    }
    else if (cmd_s.find("jobs") != std::string::npos){
        return new JobsCommand(cmd_line, this->jobsList);
    }
    else if (cmd_s.find("quit") != std::string::npos){
        return new QuitCommand(cmd_line);
    }
    else if (cmd_s.find("chprompt") != std::string::npos) {
        return new ChangePromptCommand(cmd_line, &this->currentPrompt);
    }
    else if (cmd_s.find("kill") != std::string::npos){
        return new KillCommand(cmd_line, this->jobsList);
    }
    else if (cmd_s.find("ls") != std::string::npos){
        return new lsCommand(cmd_line);
    }
    else {
        return new ExternalCommand(cmd_line, jobsList);
    }

    return nullptr;
}
void SmallShell::executeCommand(const char *cmd_line) {
    Command* cmd = CreateCommand(cmd_line);
    cmd->execute();
//    Command* cmd = new Command(cmd_line); // todo debug
//    jobsList->addJob(cmd, FG);
//    JobsList::JobEntry* FG_job =  jobsList->getFgJob();
//    cout << FG_job->state << endl;


    // Please note that you must fork smash process for some commands (e.g., external commands....)

    //char** args;
    //Command* cmd = _parseCommandLine(cmd_line, args);

}