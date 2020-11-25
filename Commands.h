#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>
#include <string>
#include <time.h>
#include <stdio.h>
#include <map>

using namespace std;

#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)
#define HISTORY_MAX_RECORDS (50)


enum JobState {STOPPED, FG, BG};

class JobsList;

class Command {
 protected:
  vector<string> args;
  pid_t _pid; // not zero only for built in commands
  bool isFinished; // TODO: should be updated by the dad- the shell after wait() finish. how to update bg process?
public:
    char* cmd_line;
    Command(const char* cmd_line);
  virtual ~Command();
//  virtual void execute() = 0;
  virtual void execute(){};
  pid_t getPID();
//  virtual void prepare();
//  virtual void cleanup();
  string getCommandName(); // Todo: implement
  bool getisFinished(); // Todo: implement
  void cleanArgs();
};

class BuiltInCommand : public Command {
 public:
  BuiltInCommand(const char* cmd_line);
  virtual ~BuiltInCommand() {}
};

class ExternalCommand : public Command {
private:
    bool _wait;
    JobsList* jobs;
public:
  ExternalCommand(const char* cmd_line, JobsList* jobs);
  virtual ~ExternalCommand() {}
  void execute() override;
};

class PipeCommand : public Command {
private:
    pid_t _pid1;
    pid_t _pid2;
    JobsList* jobs;

public:
  PipeCommand(const char* cmd_line, JobsList* jobs);
  virtual ~PipeCommand() {}
  void execute() override;
};

class RedirectionCommand : public Command {
 // TODO: Add your data members
 public:
  explicit RedirectionCommand(const char* cmd_line);
  virtual ~RedirectionCommand() {}
  void execute() override; //Todo: remove {} when implementing
  //void prepare() override;
  //void cleanup() override;
};

class ChangeDirCommand : public BuiltInCommand {
// TODO: Add your data members public:
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
    virtual ~ChangePromptCommand() {} //todo: =default?
    void execute() override;
};

class QuitCommand : public BuiltInCommand {
// TODO: Add your data members public
  JobsList* jobs;
  QuitCommand(const char* cmd_line, JobsList* jobs);
  virtual ~QuitCommand() {}
  void execute() override {}; //Todo: remove {} when implementing
};

class TimeoutCommand : public BuiltInCommand {
// TODO: Add your data members public
    JobsList* jobs;
public:
    TimeoutCommand(const char* cmd_line, JobsList* jobs);
    virtual ~TimeoutCommand() {}
    void execute() override;
};

class JobsList {

 public:
  class JobEntry {
  public:
      time_t timeStamp;
      int jobID;
      JobState state; // TODO: change everthing according to state
      Command* command;
      JobEntry(Command* cmd, int jobID, JobState state);
      ~JobEntry(){};
  };

  class timeoutJob{
    public:
        int id;
        int sleepTime;
        timeoutJob(int id, int sleepTime): id(id),sleepTime(sleepTime) {};
      ~timeoutJob(){};
    };
private:
    vector <JobEntry> jobList;
 public:
    int maxJobID;
    vector <timeoutJob> timeoutJobs; //TODO: can built in get timeout
    JobsList();
    ~JobsList();
    int addJob(Command* cmd, JobState state);
    void printJobsList();
    void killAllJobs(); // why do we need?
    void removeFinishedJobs();
    JobEntry* getJobById(int jobId);
    void removeJobById(int jobId);
    JobEntry* getLastJob(int* lastJobId); // why do we need?
    JobEntry*getLastStoppedJob(int *jobId); // why do we need?
    void changeJobStatus (int jobId, JobState state);
    JobEntry* getFgJob();
    void printFirstJobs();
    void addTimeoutJob(int jobId, int sleepTime);
    JobEntry* getTimeoutJob();
    void removeTimeoutJob(int jobId);
    };

class JobsCommand : public BuiltInCommand {
 // TODO: Add your data members
  JobsList* jobs;
public:
  JobsCommand(const char* cmd_line, JobsList* jobs);
  virtual ~JobsCommand() {}
  void execute() override; //Todo: remove {} when implementing
};

class KillCommand : public BuiltInCommand {
 // TODO: Add your data members
 JobsList* jobs;
public:
  KillCommand(const char* cmd_line, JobsList* jobs);
  virtual ~KillCommand() {}
  void execute() override {}; //Todo: remove {} when implementing
};

class ForegroundCommand : public BuiltInCommand { // TODO: reset timer in jobs list when
 // TODO: Add your data members
 JobsList* jobs;
public:
  ForegroundCommand(const char* cmd_line, JobsList* jobs);
  virtual ~ForegroundCommand() {}
  void execute() override {}; //Todo: remove {} when implementing
};

class BackgroundCommand : public BuiltInCommand {
 // TODO: Add your data members
 JobsList* jobs;
public:
  BackgroundCommand(const char* cmd_line, JobsList* jobs);
  virtual ~BackgroundCommand() {}
  void execute() override {}; //Todo: remove {} when implementing
};

class lsCommand : public BuiltInCommand{
public:
    lsCommand(const char* cmd_line);
    virtual ~lsCommand() {};
    void execute() override{};
};

// TODO: add more classes if needed 
// maybe ls, timeout ?

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
