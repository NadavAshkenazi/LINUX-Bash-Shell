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
Command::Command(const char *cmd_line) {
    // TODO: parse line
    //
}
Command::~Command() {
    //
}

pid_t Command::getPID(){return 0;} // TODO: really implement
string Command::getCommandName() {return "shahar"; } // TODO: really implement
bool Command::getisFinished() {return false;} // TODO: really implement


//**************************************
// JobsList
//**************************************
JobsList::JobEntry::JobEntry(Command* command, int jobID): isStopped(false), jobID(jobID) {

    time_t* temp_time;
    time(temp_time);
    timeStamp = *(temp_time);
    delete temp_time;

    command = command;
}
JobsList::JobsList(): maxJobID(0){
    vector <JobEntry*> jobList;
}
void JobsList::addJob(Command* cmd, bool isStopped){

    JobEntry* newJob = new JobEntry(cmd, maxJobID++);
    jobList.push_back(newJob);
}
void JobsList::printJobsList(){

    //removeFinishedJobs();
    for (vector<JobEntry*>::iterator it = jobList.begin() ; it != jobList.end(); ++it){

        cout << "[" << (*it)->jobID << "]" ;
        cout << (*it)->command->getCommandName() << ":" ;
        cout << (*it)->command->getPID();

        time_t* current_time;
        time(current_time);
        double timeElapsed = difftime (*current_time, (*it)->timeStamp);
        cout << timeElapsed;
        delete current_time;

        if ((*it)->isStopped){
            cout << "(stopped)";
        }

        cout << endl;
    }

}
void JobsList::removeFinishedJobs(){ // TODO: how do we know

    for (vector<JobEntry*>::iterator it = jobList.begin() ; it != jobList.end(); ++it){
        if ((*it)->command->getisFinished()){
            jobList.erase(it);
            delete (*it)->command;
            delete (*it);
        }
    }
}
JobsList::JobEntry* JobsList::getJobById(int jobId){
    for (vector<JobEntry*>::iterator it = jobList.begin() ; it != jobList.end(); ++it){
        if ((*it)->jobID == jobId){
            return (*it);
        }
    }
}
void JobsList::changeJobStatus (int jobId, bool isStopped){
    JobEntry* jobToChange = getJobById(jobId);
    jobToChange->isStopped = isStopped;
}

//**************************************
// SmallShell
//**************************************
SmallShell::SmallShell() {
// TODO: add your implementation
}
SmallShell::~SmallShell() {
// TODO: add your implementation
}
// Creates and returns a pointer to Command class which matches the given command line (cmd_line)
Command * SmallShell::CreateCommand(const char* cmd_line) {
    // For example:
/*
  string cmd_s = string(cmd_line);
  if (cmd_s.find("pwd") == 0) {
    return new GetCurrDirCommand(cmd_line);
  }
  else if ...
  .....
  else {
    return new ExternalCommand(cmd_line);
  }
  */
    return nullptr;
}
void SmallShell::executeCommand(const char *cmd_line) {
    // TODO: Add your implementation here
    // for example:
    // Command* cmd = CreateCommand(cmd_line);
    // cmd->execute();
    // Please note that you must fork smash process for some commands (e.g., external commands....)
}