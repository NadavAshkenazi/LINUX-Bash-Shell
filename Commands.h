#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>
#include <string>
#include <time.h>
#include <stdio.h>
#include <map>
#include <dirent.h>


using namespace std;

#define PIPE1 1
#define PIPE2 2
#define NOVALUE -5
#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)
#define HISTORY_MAX_RECORDS (50)


enum JobState {STOPPED, FG, BG, NOPIPE};

class JobsList;

class Command {
 protected:
  vector<string> args;
  pid_t _pid; // not zero only for built in commands
  bool isFinished;
public:
    char* cmd_line;
    Command(const char* cmd_line);
  virtual ~Command();
//  virtual void execute() = 0;
  virtual void execute(){};
  virtual pid_t getPID();
//  virtual void prepare();
//  virtual void cleanup();
  string getCommandName();
  bool getisFinished();
  void cleanArgs();
  virtual void executePipe(){};
  virtual void setPid(pid_t pid){};
};

class BuiltInCommand : public Command {
 public:
  BuiltInCommand(const char* cmd_line);
  virtual ~BuiltInCommand() {}
  void executePipe();
  void setPid(pid_t pid){};
};

class ExternalCommand : public Command {
private:
    bool _wait;
    JobsList* jobs;
    int _jobID;
    string clean_cmd_line;
    bool toSetGrpPid;
public:
  ExternalCommand(const char* cmd_line, JobsList* jobs, bool toSetGrpPid=false);
  virtual ~ExternalCommand() {}
  void execute() override;
  void executePipe();
  void setPid(pid_t pid);
};

class PipeCommand : public Command {
private:
    pid_t _pid1;
    pid_t _pid2;
    string _cmd1;
    string _cmd2;
    bool regularPipe; // true if "|", false is "|&"
    JobsList* jobs;

public:
  PipeCommand(const char* cmd_line, JobsList* jobs);
  virtual ~PipeCommand() {}
  void execute() override;
  pid_t getPID();
};

class RedirectionCommand : public Command {
private:
    bool override; // true only if ">"
    string cmd;
    string fileName;
 public:
  explicit RedirectionCommand(const char* cmd_line);
  virtual ~RedirectionCommand() {}
  void execute() override;
  //void prepare() override;
  //void cleanup() override;
};

class ChangeDirCommand : public BuiltInCommand {
 private:
    char** plastPwd;
 public:
   ChangeDirCommand(const char* cmd_line, char** plastPwd);
  virtual ~ChangeDirCommand() {}
  void execute() override;
};

class GetCurrDirCommand : public BuiltInCommand {
 public:
  GetCurrDirCommand(const char* cmd_line);
  virtual ~GetCurrDirCommand() {}
  void execute() override;
};

class ShowPidCommand : public BuiltInCommand {
 public:
  ShowPidCommand(const char* cmd_line);
  virtual ~ShowPidCommand() {}
  void execute() override;
};

class ChangePromptCommand : public BuiltInCommand {
public:
    string* currentPromp;
    ChangePromptCommand(const char* cmd_line, string* currentPrompt);
    virtual ~ChangePromptCommand() {}
    void execute() override;
};

class QuitCommand : public BuiltInCommand {
private:
    bool killAllJobs;
    JobsList* jobs;
public:
  QuitCommand(const char* cmd_line);
  virtual ~QuitCommand() {}
  void execute() override;
};

class TimeoutCommand : public Command {
// TODO: Add your data members public
    JobsList* jobs;
public:
    TimeoutCommand(const char* cmd_line, JobsList* jobs);
    virtual ~TimeoutCommand() {}
    void execute() override;
};


class cpCommand : public BuiltInCommand {
// TODO: Add your data members public
    bool _wait;
    JobsList* jobs;
    int _jobID;
    string clean_cmd_line;
public:
    cpCommand(const char* cmd_line, JobsList* jobs);
    virtual ~cpCommand() {}
    void execute() override;
};

class JobsList {

 public:
  class JobEntry {
  public:
      time_t timeStamp;
      int jobID;
      int formerJobId;
      JobState state;
      Command* command;
      JobEntry(Command* cmd, int jobID, JobState state);
      ~JobEntry(){};
  };

  class timeoutJob{
    public:
        int id;
        int sleepTime;
        int pipe;
        timeoutJob(int id, int sleepTime, int pipe = 0) : id(id),sleepTime(sleepTime), pipe(pipe) {};
      ~timeoutJob(){};
    };

 public:
    bool hasPipeInFg;
    pid_t pipePid1;
    pid_t pipePid2;
    int maxJobID;
    vector <JobEntry> jobList;
    vector <timeoutJob> timeoutJobs;
    JobsList();
    ~JobsList();
    int addJob(Command* cmd, JobState state);
    void printJobsList();
    void killAllJobs();
    void removeFinishedJobs();
    JobEntry* getJobById(int jobId);
    void removeJobById(int jobId);
    JobEntry* getLastJob(int* lastJobId);
    int getLastStoppedJobId();
    void changeJobStatus (int jobId, JobState state);
    JobEntry* getFgJob();
    void printFirstJobs();
    void addTimeoutJob(int jobId, int sleepTime, int pipe = 0);
    JobEntry* getTimeoutJob(int pipe = 0);
    void removeTimeoutJob(int jobId, int pipe = 0);
    void resetJobTimerById(int jobId);
    void changeJobId(JobsList::JobEntry* job, int newId);
    JobEntry* getJobByPid(pid_t pid);
    void removeJobByPid(pid_t pid);
    int getLastJobId();
    void _printTimeoutVector();
    void calcMaxJobId();
    int getNumOfBgJobs();
};


class JobsCommand : public BuiltInCommand {
 // TODO: Add your data members
  JobsList* jobs;
public:
  JobsCommand(const char* cmd_line, JobsList* jobs);
  virtual ~JobsCommand() {}
  void execute() override;
};

class KillCommand : public BuiltInCommand {
private:
    JobsList* jobs;
    int signum;
    int jobIdToKill;
    pid_t pidToKill;
    bool print;
public:
  KillCommand(const char* cmd_line, JobsList* jobs ,bool print=true);
  virtual ~KillCommand() {}
  void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
private:
    int JobIdToResume;
    JobsList* jobs;
public:
  ForegroundCommand(const char* cmd_line);
  virtual ~ForegroundCommand() {}
  void execute() override;
};

class BackgroundCommand : public BuiltInCommand {
 int JobIdToResume;
 JobsList* jobs;
public:
  BackgroundCommand(const char* cmd_line);
  virtual ~BackgroundCommand() {}
  void execute() override;
};

class lsCommand : public BuiltInCommand{
public:
    lsCommand(const char* cmd_line);
    virtual ~lsCommand() {};
    void execute() override;
};


class SmallShell {
 private:
  SmallShell();
  char* plastPwd;
public:
    JobsList* jobsList;
    string currentPrompt;
    pid_t smashPid;
    Command *CreateCommand(const char* cmd_line);
    SmallShell(SmallShell const&)      = delete; // disable copy ctor
    void operator=(SmallShell const&)  = delete; // disable = operator
    static SmallShell& getInstance(){// make SmallShell singleton
        static SmallShell instance; // Guaranteed to be destroyed.
        // Instantiated on first use.
        return instance;
      }
    ~SmallShell();
    void executeCommand(const char* cmd_line);
};

#endif //SMASH_COMMAND_H_
