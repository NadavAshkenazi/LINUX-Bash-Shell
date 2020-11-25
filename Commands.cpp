#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"
#include <sstream>

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
#define MAX_PWD_SIZE 1000000 // TODO: check
#define ARGS_NUM_CD 1
#define STDIN 0
#define STDOUT 1
#define STDERR 2
#define MAX_SIZE 80
#define DEBUG_PRINT cerr << "DEBUG: "

#define EXEC(path, arg) \
  execvp((path), (arg));

//#define _printSysCallErr(sysCall) "perror(\"a\" ); return" // todo debug

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
Command::Command(const char *cmd_line): _pid(0),isFinished(false){//, cmd_line(cmd_line) {

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

ExternalCommand::ExternalCommand(const char* cmd_line, JobsList* jobs): Command(cmd_line), jobs(jobs) {
    _wait = true;
    for (vector<string>::iterator it = args.begin(); it != args.end(); it++) {
        if (*it == "&"){
            _wait = false;
        }
    }

    JobState state = BG;
    if (_wait)
        state = FG;
    int jobID = jobs->addJob(this, state);
}

void ExternalCommand::execute(){
    char clean_cmd_line[strlen(cmd_line) + 1];
    strcpy(clean_cmd_line,cmd_line);
    _removeBackgroundSign(clean_cmd_line);

//    JobState state = BG;
//    if (_wait)
//        state = FG;
//    int jobID = jobs->addJob(this, state);
    //cout << "added job" << this->args[0] << state << endl; // todo : not print these jobs- maybe not really added?
    //jobs->printFirstJobs();  // TODO: debug

    pid_t pid = fork();

    if (pid < 0){
        perror("smash error: fork failed");
        return;
    }
    if (pid == 0){ // child process
        setpgrp();

        //kill (getpid(), SIGTSTP); // debug

        char* args_to_bash[] = {"/bin/bash",
                                "-c",
                                clean_cmd_line,
                                NULL};
        //kill (getpid(), SIGTSTP); // debug
        execv(args_to_bash[0], args_to_bash);
        //jobs->removeJobById(jobID); // todo fix
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


//**************************************
// PipeCommand
//**************************************
PipeCommand::PipeCommand(const char* cmd_line, JobsList* jobs): Command(cmd_line), _pid1(-1), _pid2(-1), jobs(jobs){}; // todo how should we print the name of pipe command
void PipeCommand::execute() { // todo need to update real proccess pid

 //   bool isBackground = _isBackgroundComamnd(cmd_line);

//    JobState state = BG;
//    if (!isBackground)
//        state = FG;
//    int pipeJobID = jobs->addJob(this, state);

    bool regularPipe = false;
    bool errorPipe = false;

    string s_cmd = string(cmd_line);
    int pipeLocation;

    if (s_cmd.find("|&") != string::npos) {
        errorPipe = true;
        pipeLocation = s_cmd.find("|&");
    } else if (s_cmd.find("|") != string::npos) {
        //cout << "in | if" << endl; // todo debug
        regularPipe = true;
        pipeLocation = s_cmd.find("|");
    }

    string cmd_1 = s_cmd.substr(0, pipeLocation);
    int sizeOfPipe = regularPipe ? 1 : 2;
    string cmd_2 = s_cmd.substr(pipeLocation + sizeOfPipe);
    //cout << "cmd_1 = " << cmd_1 << endl; // todo debug
    //cout << "cmd_2 = " << cmd_2 << endl; // todo debug


    int fd[2];
    if (pipe(fd) == -1) {
        perror("smash error: pipe failed");
        return;
    }

    SmallShell &smash = SmallShell::getInstance();
    Command* firstCommand= smash.CreateCommand(cmd_1.c_str());
    Command* secondCommand= smash.CreateCommand(cmd_2.c_str());

    pid_t pid1 = fork();

    if (pid1 < 0) {
        perror("smash error: fork failed");
        return;
    }

    if (pid1 == 0) {  // first command
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

        //smash.executeCommand(cmd_1.c_str());
        firstCommand->execute();
        exit(0);

    } else { // shell code
        _pid1 = pid1;

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

            //smash.executeCommand(cmd_2.c_str());
            secondCommand->execute();
            exit(0);
        } else {

            // shell code
            if (close(fd[0]) == -1) {
                perror("smash error: close failed");
                return;
            }
            if (close(fd[1]) == -1) {
                perror("smash error: close failed");
                return;
            }
            _pid2 = pid2;

            cout << "pid1 = " << pid1 << endl;
            cout << "pid2 = " << pid2 << endl;

            waitpid( pid1, NULL, 0);
            waitpid( pid2, NULL, 0);
//            waitpid( pid1, NULL, WUNTRACED);
//            waitpid( pid2, NULL, WUNTRACED);
            //cout << "pid2 finished: " << waitpid( pid2, NULL, WUNTRACED) << endl; // todo debug
            //cout << "pid2 finished2: " << waitpid( pid2, NULL, WUNTRACED) << endl; // todo debug
            //cout << "pid2 in remove : " << waitpid( pid2, NULL, WNOHANG ) <<endl; // todo debug
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
RedirectionCommand::RedirectionCommand(const char* cmd_line): Command(cmd_line){}
void RedirectionCommand::execute() {
//    bool regularRedirection = false;
//    bool contRedirection = false;
//
//    int signLocation;
//    string s_cmd = string(cmd_line);
//
//    if (s_cmd.find(">>") != string::npos) {
//        contRedirection = true;
//        signLocation = s_cmd.find("|&");
//    } else if (s_cmd.find(">") != string::npos) {
//        regularRedirection = true;
//        signLocation = s_cmd.find("|");
//    }
//
//    string cmd = s_cmd.substr(0, signLocation);
//    int sizeOfSign = regularRedirection ? 1 : 2;
//    string fileName = s_cmd.substr(signLocation + sizeOfSign);
//
//    pid_t pid = fork();
//
//    if (pid < 0 ){
//        perror("smash error: fork failed");
//        return;
//    }
//    else if (pid == 0){ // child
//        if (close(STDOUT) == -1){
//            perror("smash error: close failed");
//            return;
//        }
//        if (open(fileName,O_CREAT)){ // todo think if its for >> or >
//            perror("smash error: open failed");
//            return;
//        }
//
//        SmallShell &smash = SmallShell::getInstance();
//        smash.CreateCommand(cmd.c_str());
//    }
//    else{ // shell
//
//    }


}


//**************************************
// BuiltInCommand
//**************************************
BuiltInCommand::BuiltInCommand(const char *cmd_line) :Command(cmd_line)  {
    vector<string> ignoreArgs{">", "<", "<<", ">>", "|","&"}; // TODO: check if need to do redirection

    for (vector<string>::iterator it_args = args.begin(); it_args != args.end(); it_args++) {
        for (vector<string>::iterator it_ignore = ignoreArgs.begin(); it_ignore != ignoreArgs.end(); ++it_ignore) {
            if (*it_args == *it_ignore)
                it_args= args.erase(it_args); // TODO: check
        }
    }
}


//**************************************
// ChangeDirCommand
//**************************************

ChangeDirCommand::ChangeDirCommand(const char* cmd_line, char** plastPwd) :BuiltInCommand(cmd_line), plastPwd(plastPwd){}
void ChangeDirCommand::execute(){
    if (args.size()-1 > ARGS_NUM_CD){
        perror("smash error: cd: too many arguments");
        return;
    }
    if (args.size()-1 < ARGS_NUM_CD){
        perror("smash error: cd: too few arguments");
        return;
    }

    if (string(args[ARGS_NUM_CD]) != "-"){
        char* temp = *plastPwd; // just in case cd failed
        char* pwd = (char*)malloc(MAX_PWD_SIZE);
        getcwd(pwd, MAX_PWD_SIZE);
        *plastPwd = pwd;

        if (chdir((const char *) (args[ARGS_NUM_CD]).c_str()) != 0){ // cd failed
            perror("smash error: chdir failed");
            *plastPwd = temp;
            return;
        }
    }
    else{
        if (*plastPwd == NULL){
            perror("smash error: cd: OLDPWD not set");
            return;
        }
        else{
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

QuitCommand::QuitCommand(const char* cmd_line, JobsList* jobs) :BuiltInCommand(cmd_line), jobs(jobs) {}

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

KillCommand::KillCommand(const char* cmd_line, JobsList* jobs) :BuiltInCommand(cmd_line), jobs(jobs) {}

//**************************************
// ForegroundCommand
//**************************************

ForegroundCommand::ForegroundCommand(const char* cmd_line, JobsList* jobs) :BuiltInCommand(cmd_line), jobs(jobs) {}

//**************************************
// BackgroundCommand
//**************************************

BackgroundCommand::BackgroundCommand(const char* cmd_line, JobsList* jobs) :BuiltInCommand(cmd_line), jobs(jobs) {}

//**************************************
// lsCommand
//**************************************
lsCommand::lsCommand(const char* cmd_line):BuiltInCommand(cmd_line){}
//void lsCommand::execute{}

//**************************************
// ChangePromptCommand
//**************************************

ChangePromptCommand::ChangePromptCommand(const char *cmd_line, string* currentPrompt) :BuiltInCommand(cmd_line),
                                                                                        currentPromp(currentPrompt) {}

void ChangePromptCommand::execute() {
    if (this->args.size() == 1){
        *currentPromp = "smash>";
    }
    else{
        string test = this->args[1] + ">";
        *currentPromp = this->args[1] + ">";
    }
}


//**************************************
// Timeout Command
//**************************************

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
        smash.jobsList->addTimeoutJob(smash.jobsList->maxJobID+1, sleep);
        smash.executeCommand(command.c_str());
        return;
    }
    //todo: fork - parent - execute, child - send signal
}

//**************************************
// JobsList
//**************************************

JobsList::JobEntry::JobEntry(Command* cmd, int jobID, JobState state): command(cmd), state(state), jobID(jobID) {
    time_t* temp_time= NULL;
    timeStamp = time(temp_time); //todo: check time format
}
JobsList::JobsList(): maxJobID(-1){
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
    JobEntry newJob = JobEntry(cmd, maxJobID+1, state);
    maxJobID++;
    jobList.push_back(newJob);
    return maxJobID;

}
void JobsList::printJobsList(){

    removeFinishedJobs();
    for (vector<JobEntry>::iterator it = jobList.begin() ; it != jobList.end(); ++it){

        if ((*it).state == FG)
            continue;

        cout << "[" << (*it).jobID << "]" ;
        cout << (*it).command->getCommandName() << ": " ;
        cout << (*it).command->getPID() << " ";

        time_t* current_time =  NULL;
        double timeElapsed = difftime (time(current_time), (*it).timeStamp);
        cout << timeElapsed;

        if ((*it).state == STOPPED){
            cout << "(stopped)";
        }

        cout << endl;
    }

}
void JobsList::removeFinishedJobs(){

    for(int i=0 ; i < jobList.size() ; i ++ ){
        //cout << "check if finished: " << jobList[i].command->getPID(); // todo debug
        if (waitpid(jobList[i].command->getPID(), NULL, WNOHANG)  == jobList[i].command->getPID()){
            cout << waitpid(jobList[i].command->getPID(), NULL, WNOHANG) << endl;
            delete jobList[i].command;
            jobList.erase(jobList.begin() +i);
            i--; // to conform with joblist size
        }
    }
}
JobsList::JobEntry* JobsList::getJobById(int jobId){
    for (vector<JobEntry>::iterator it = jobList.begin() ; it != jobList.end(); ++it){
        if ((*it).jobID == jobId){
            return &(*it);
        }
    }
    return NULL;
}
void JobsList::changeJobStatus (int jobId, JobState state){
    JobEntry* jobToChange = getJobById(jobId);
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

void JobsList::removeJobById(int jobId){ // todo remove also from timeout vector
    for(int i=0 ; i < jobList.size() ; i ++ ){
        if (jobList[i].jobID == jobId){
            delete jobList[i].command;
            jobList.erase(jobList.begin() +i);
            removeTimeoutJob(jobId);
            return;
        }
    }
    return;
}

void JobsList::addTimeoutJob(int jobId, int sleepTime) {
    timeoutJob newJob = timeoutJob(jobId, sleepTime);
    timeoutJobs.push_back(newJob);
}

JobsList::JobEntry* JobsList::getTimeoutJob(){
    for (vector<timeoutJob>::iterator it = timeoutJobs.begin() ; it != timeoutJobs.end(); ++it){
        JobEntry* job = getJobById((it->id));
        time_t* tempTime= NULL;
        time_t now = time(tempTime);
        if (now - job->timeStamp >= it->sleepTime){
            return job;
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
    cout << "size: " << timeoutJobs.size() << endl;
    return;
}

//**************************************
// SmallShell
//**************************************
SmallShell::SmallShell(): currentPrompt("smash>"),plastPwd(NULL) {
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
    else if (cmd_s.find(">") != std::string::npos || cmd_s.find("<") != std::string::npos){
//        return new RedirectionCommand(cmd_line);
    }
    else if (cmd_s.find("|") != std::string::npos){
        return new PipeCommand(cmd_line, this->jobsList);
    }
    else if (cmd_s.find("bg") != std::string::npos){
//        return new BackgroundCommand(cmd_line); //todo: add jobslist
    }
    else if (cmd_s.find("fg") != std::string::npos){
//        return new ForegroundCommand(cmd_line); //todo: add jobslist
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
//        return new QuitCommand(cmd_line); //todo: add jobslist
    }
    else if (cmd_s.find("kill") != std::string::npos){
//        return new KillCommand(cmd_line); //todo: add jobslist
    }
    else if (cmd_s.find("chprompt") != std::string::npos){
        return new ChangePromptCommand(cmd_line, &this->currentPrompt);
    }
//todo: add ls

//    else if (cmd_s.find("pwd") == 0) {
//    return new GetCurrDirCommand(cmd_line);
//    }
//
//  else if ...
//  .....
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

