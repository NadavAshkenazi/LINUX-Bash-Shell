#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>
#include <string>
#include <time.h>
#include <stdio.h>

using namespace std;

#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)
#define HISTORY_MAX_RECORDS (50)

enum JobState {STOPPED, FG, BG};

class Command {
 protected:
  char** args;
  int args_size;
  pid_t pid; // not zero only for built in commands
  bool isFinished; // TODO: should be updated by the dad- the shell after wait() finish. how to update bg process?
 public:
  Command(const char* cmd_line);
  virtual ~Command();
//  virtual void execute() = 0;
  virtual void execute(){};
  pid_t getPID();
//  virtual void prepare();
//  virtual void cleanup();
  string getCommandName(); // Todo: implement
  bool getisFinished(); // Todo: implement
};

class BuiltInCommand : public Command {
 public:
  BuiltInCommand(const char* cmd_line);
  virtual ~BuiltInCommand() {}
};

class ExternalCommand : public Command {
 public:
  ExternalCommand(const char* cmd_line);
  virtual ~ExternalCommand() {}
  void execute() override {}; //Todo: remove {} when implementing
};

class PipeCommand : public Command {
  // TODO: Add your data members
 public:
  PipeCommand(const char* cmd_line);
  virtual ~PipeCommand() {}
  void execute() override {}; //Todo: remove {} when implementing
};

class RedirectionCommand : public Command {
 // TODO: Add your data members
 public:
  explicit RedirectionCommand(const char* cmd_line);
  virtual ~RedirectionCommand() {}
  void execute() override {}; //Todo: remove {} when implementing
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

class JobsList;
class QuitCommand : public BuiltInCommand {
// TODO: Add your data members public
  JobsList* jobs;
  QuitCommand(const char* cmd_line, JobsList* jobs);
  virtual ~QuitCommand() {}
  void execute() override {}; //Todo: remove {} when implementing
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

private:
    int maxJobID;
    vector <JobEntry> jobList;
 public:
    JobsList();
    ~JobsList();
    void addJob(Command* cmd, JobState state);
    void printJobsList();
    void killAllJobs(); // why do we need?
    void removeFinishedJobs();
    JobEntry * getJobById(int jobId);
    void removeJobById(int jobId); // why do we need?
    JobEntry * getLastJob(int* lastJobId); // why do we need?
    JobEntry *getLastStoppedJob(int *jobId); // why do we need?
    void changeJobStatus (int jobId, JobState state);
    JobEntry* getFgJob();
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

class ForegroundCommand : public BuiltInCommand {
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

// TODO: add more classes if needed 
// maybe ls, timeout ?

class SmallShell {
 private:
  SmallShell();
  char* plastPwd;
public:
    JobsList* jobsList;
    string currentPrompt;
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
