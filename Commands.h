#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>
#include <string>
#include <time.h>

using namespace std;

#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)
#define HISTORY_MAX_RECORDS (50)

enum JobState {STOPPED, FG, BG};

class Command {
 private:
  string* args;
  pid_t pid; // not zero only for built in commands
  bool isFinished; // TODO: should be updated by the dad- the shell after wait() finish. how to update bg process?
 public:
  Command(const char* cmd_line);
  virtual ~Command();
  virtual void execute() = 0; // TODO: should update the pid
  pid_t getPID();
  //virtual void prepare();
  //virtual void cleanup();
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
  void execute() override;
};

class PipeCommand : public Command {
  // TODO: Add your data members
 public:
  PipeCommand(const char* cmd_line);
  virtual ~PipeCommand() {}
  void execute() override;
};

class RedirectionCommand : public Command {
 // TODO: Add your data members
 public:
  explicit RedirectionCommand(const char* cmd_line);
  virtual ~RedirectionCommand() {}
  void execute() override;
  //void prepare() override;
  //void cleanup() override;
};

class ChangeDirCommand : public BuiltInCommand {
// TODO: Add your data members public:
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

class JobsList;
class QuitCommand : public BuiltInCommand {
// TODO: Add your data members public:
  QuitCommand(const char* cmd_line, JobsList* jobs);
  virtual ~QuitCommand() {}
  void execute() override;
};

class CommandsHistory {
 protected:
  class CommandHistoryEntry {
	  // TODO: Add your data members
  };
 // TODO: Add your data members
 public:
  CommandsHistory();
  ~CommandsHistory() {}
  void addRecord(const char* cmd_line);
  void printHistory();
};

class HistoryCommand : public BuiltInCommand {
 // TODO: Add your data members
 public:
  HistoryCommand(const char* cmd_line, CommandsHistory* history);
  virtual ~HistoryCommand() {}
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
      JobEntry(Command* command, int jobID, JobState state);
      ~JobEntry(){};
  };

private:
    int maxJobID;
    vector <JobEntry*> jobList;
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
}; // TODO: test the whole class after implementing command class

class JobsCommand : public BuiltInCommand {
 // TODO: Add your data members
 public:
  JobsCommand(const char* cmd_line, JobsList* jobs);
  virtual ~JobsCommand() {}
  void execute() override;
};

class KillCommand : public BuiltInCommand {
 // TODO: Add your data members
 public:
  KillCommand(const char* cmd_line, JobsList* jobs);
  virtual ~KillCommand() {}
  void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
 // TODO: Add your data members
 public:
  ForegroundCommand(const char* cmd_line, JobsList* jobs);
  virtual ~ForegroundCommand() {}
  void execute() override;
};

class BackgroundCommand : public BuiltInCommand {
 // TODO: Add your data members
 public:
  BackgroundCommand(const char* cmd_line, JobsList* jobs);
  virtual ~BackgroundCommand() {}
  void execute() override;
};

// TODO: add more classes if needed 
// maybe ls, timeout ?

class SmallShell {
 private:
  // TODO: Add your data members
  SmallShell();
  string currentPrompt = "smash>";
  JobsList* jobsList;
  string prevWDir;
 public:
  Command *CreateCommand(const char* cmd_line);
  SmallShell(SmallShell const&)      = delete; // disable copy ctor
  void operator=(SmallShell const&)  = delete; // disable = operator
  static SmallShell& getInstance() // make SmallShell singleton
  {
    static SmallShell instance; // Guaranteed to be destroyed.
    // Instantiated on first use.
    return instance;
  }
  ~SmallShell();
  void executeCommand(const char* cmd_line);
  // TODO: add extra methods as needed
};

#endif //SMASH_COMMAND_H_
