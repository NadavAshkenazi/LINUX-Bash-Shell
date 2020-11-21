#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"

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
#define MAX_SIZE 80
#define DEBUG_PRINT cerr << "DEBUG: "

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
Command::Command(const char *cmd_line): pid(0),isFinished(false) {

    args = (char**)malloc((MAX_ARGS_NUM+1)*sizeof(char*));
    args_size = _parseCommandLine(cmd_line, args);
}
Command::~Command() {

    for (int i=0; i< args_size ; i++){
        free(args[i]);
    }
    free(args);
}

pid_t Command::getPID(){return pid;}
string Command::getCommandName() {return args[0]; }
bool Command::getisFinished() {return isFinished;}

BuiltInCommand::BuiltInCommand(const char *cmd_line) :Command(cmd_line)  {
    vector<string> ignoreArgs{">", "<", "<<", ">>", "|","&"};
    for (int i =0; i < this->args_size; i++) {
        string arg_s = string(this->args[i]);
        for (vector<string>::iterator it = ignoreArgs.begin(); it != ignoreArgs.end(); ++it) {
            if (arg_s == *it)
                free(this->args[i]);
        }
    }
    char** clean_args = (char**)malloc((MAX_ARGS_NUM+1)*sizeof(char*));
    int j =0;
    for (int i = 0; i < this->args_size; i++) {
        if (this->args[i] != NULL)
            clean_args[j++] = this->args[i];
    }
    free(this->args);
    this->args = clean_args;
    this->args_size = j;
}

//**************************************
// ChangeDirCommand
//**************************************

ChangeDirCommand::ChangeDirCommand(const char* cmd_line, char** plastPwd) :BuiltInCommand(cmd_line), plastPwd(*plastPwd){}

//**************************************
// GetCurrDirCommand
//**************************************

GetCurrDirCommand::GetCurrDirCommand(const char* cmd_line) :BuiltInCommand(cmd_line) {}

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
// JobsList
//**************************************
JobsList::JobEntry::JobEntry(Command* cmd, int jobID, JobState state): command(cmd), state(state), jobID(jobID) {
    time_t* temp_time= NULL;
    timeStamp = time(temp_time);
    //delete temp_time;
}

JobsList::JobsList(): maxJobID(0){
    vector <JobEntry> jobList;
}

JobsList::~JobsList(){ // TODO: think
//    for (vector<JobEntry>::iterator it = jobList.begin() ; it != jobList.end(); ++it){
//        delete (*it);
//    }
}
void JobsList::addJob(Command* cmd, JobState state){

    JobEntry newJob = JobEntry(cmd, maxJobID+1, state);
    maxJobID++;
    jobList.push_back(newJob);
}
void JobsList::printJobsList(){

    //removeFinishedJobs();
    for (vector<JobEntry>::iterator it = jobList.begin() ; it != jobList.end(); ++it){

        if ((*it).state == FG)
            continue;

        cout << "[" << (*it).jobID << "]" ;
        cout << (*it).command->getCommandName() << ":" ;
        cout << (*it).command->getPID();

        time_t* current_time =  NULL;
        double timeElapsed = difftime (time(current_time), (*it).timeStamp);
        cout << timeElapsed;

        if ((*it).state == STOPPED){
            cout << "(stopped)";
        }

        cout << endl;
    }

}
void JobsList::removeFinishedJobs(){ // TODO: how do we know

    for (vector<JobEntry>::iterator it = jobList.begin() ; it != jobList.end(); ++it){
        if ((*it).command->getisFinished()){
            jobList.erase(it);
            delete (*it).command;
            //delete (*it);
        }
    }
}
JobsList::JobEntry* JobsList::getJobById(int jobId){
    for (vector<JobEntry>::iterator it = jobList.begin() ; it != jobList.end(); ++it){
        if ((*it).jobID == jobId){
            return &(*it);
        }
    }
}

void JobsList::changeJobStatus (int jobId, JobState state){
    JobEntry* jobToChange = getJobById(jobId);
    jobToChange->state = state;
}


//**************************************
// SmallShell
//**************************************
SmallShell::SmallShell(): currentPrompt("smash>"),prevWDir(NULL) {
    jobsList = new JobsList();
}
SmallShell::~SmallShell() {
    delete jobsList;
}

// Creates and returns a pointer to Command class which matches the given command line (cmd_line)
Command * SmallShell::CreateCommand(const char* cmd_line) {
    string cmd_s = string(cmd_line);
    if (cmd_s.find(">") != std::string::npos || cmd_s.find("<") != std::string::npos){
//        return new RedirectionCommand(cmd_line);
    }

    else if (cmd_s.find("|") != std::string::npos){
//        return new PipeCommand(cmd_line);
    }
    else if (cmd_s.find("bg") != std::string::npos){
//        return new BackgroundCommand(cmd_line); //todo: add jobslist
    }
    else if (cmd_s.find("fg") != std::string::npos){
//        return new ForegroundCommand(cmd_line); //todo: add jobslist
    }
    else if (cmd_s.find("cd") != std::string::npos){
//        return new ChangeDirCommand(cmd_line); //todo: add plastPwd
    }
    else if (cmd_s.find("pwd") != std::string::npos){
//        return new GetCurrDirCommand(cmd_line);
    }
    else if (cmd_s.find("showpid") != std::string::npos){
        return new ShowPidCommand(cmd_line);
    }
    else if (cmd_s.find("jobs") != std::string::npos){
        return new JobsCommand(cmd_line, this->jobsList); //todo: add jobslist
    }
    else if (cmd_s.find("quit") != std::string::npos){
//        return new QuitCommand(cmd_line); //todo: add jobslist
    }
    else if (cmd_s.find("kill") != std::string::npos){
//        return new KillCommand(cmd_line); //todo: add jobslist
    }
//todo: add ls

//    else if (cmd_s.find("pwd") == 0) {
//    return new GetCurrDirCommand(cmd_line);
//    }
//
//  else if ...
//  .....
    else {
//        return new ExternalCommand(cmd_line);
    }

    return nullptr;
}
void SmallShell::executeCommand(const char *cmd_line) {
    Command* cmd = CreateCommand(cmd_line);
    cmd->execute();
    // Please note that you must fork smash process for some commands (e.g., external commands....)
}

