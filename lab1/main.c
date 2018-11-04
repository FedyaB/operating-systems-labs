#include <syslog.h>
#include <sys/resource.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <string.h>

#define FILE_NAME_PID "run/daemon.pid"
#define FILE_NAME_CONFIG "config.txt"

#define MSG_CAUGHT_SIGTERM "SIGTERM has been caught! Exiting..."
#define MSG_CAUGHT_SIGHUP "SIGHUP has been caught!"
#define MSG_ERR_CATCH_SIGTERM "Error! Can't catch SIGTERM"
#define MSG_ERR_CATCH_SIGHUP "Error! Can't catch SIGHUP"

#define MSG_INFO_START "Starting daemonisation."
#define MSG_INFO_FORK_1_P "First fork successful (Parent)"
#define MSG_INFO_FORK_1_C "First fork successful (Child)"
#define MSG_INFO_SESSION "New session was created successfuly!"
#define MSG_INFO_FORK_2_P "Second fork successful (Parent)"
#define MSG_INFO_FORK_2_C "Second fork successful (Child)"
#define MSG_INFO_USAGE "USAGE: daemon [start|stop]"
#define MSG_ERR_DAEMON_NOT_RUNNING "The daemon is not running!"
#define MSG_ERR_PID_NAME_ALLOC "A problem with memory allocation for PID file name"
#define MSG_ERR_COPY_CONFIG_PATH "A problem with copying config path"
#define MSG_ERR_CONFIG_PARSE "Failed to parse config file"
#define MSG_ERR_CONFIG_NOT_EXIST "A config file was not found"

#define FORMAT_ERR_PID_RM "Failed to remove the pid file. Error number is %d"
#define FORMAT_ERR_PID_CREATE "Failed to create a pid file while daemonising. Error number is %d"
#define FORMAT_ERR_PID_WRITE "Failed to write pid to pid file while daemonising. Error number is %d, trying to remove file"
#define FORMAT_ERR_FORK_1 "Error occured in the first fork while daemonising. Error number is %d"
#define FORMAT_ERR_SESSION "Error occured in making a new session while daemonising. Error number is %d"
#define FORMAT_ERR_FORK_2 "Error occured in the second fork while daemonising. Error number is %d"
#define FORMAT_ERR_CH_WD "Failed to change working directory while daemonising. Error number is %d"
#define FORMAT_ERR_REOPEN_STDIN "Failed to reopen stdin while daemonising. Error number is %d"
#define FORMAT_ERR_REOPEN_STDOUT "Failed to reopen stdin while daemonising. Error number is %d"
#define FORMAT_ERR_REOPEN_STDERR "Failed to reopen stdin while daemonising. Error number is %d"
#define FORMAT_ERR_KILL_PID "Failed to terminate running daemon by pid. Error number is %d"

#define FORMAT_ERR_PID_READ "Failed to read a pid file. Error number is %d"
#define FORMAT_ERR_PID_OPEN "Failed to open a pid file. Error number is %d"
#define FORMAT_ERR_GET_CWD "Failed to copy current working directory. Error number is %d"
#define FORMAT_ERR_CONFIG_OPEN "Failed to open config file. Error number is %d"

#define ERRMODE_SIMPLE_MSG 0
#define ERRMODE_ERRNO_FORMAT 1
#define ERRMODE_TERMINAL_MSG 2

#define MAX_PATH_LENGTH 1024
#define MAX_EVENTS 10
#define MAX_TEXT_LENGTH 1024
#define DAEMON_SLEEP_DURATION 10

struct event_t
{
  time_t time;
  time_t repeat;
  char text[MAX_TEXT_LENGTH];
};

struct event_array_t
{
  struct event_t events[MAX_EVENTS];
  size_t size;
};

struct daemon_data_t 
{
  char config_path[MAX_PATH_LENGTH];
  char word[MAX_PATH_LENGTH];
  struct event_array_t events;
} g_daemon_data;

void on_error(const char* msg, int mode) 
{
  if (msg != NULL)
  {
    switch (mode)
    {
    case ERRMODE_ERRNO_FORMAT:
      syslog(LOG_ERR, msg, errno);
      break;
    case ERRMODE_SIMPLE_MSG:
      syslog(LOG_ERR, "%s", msg);
      break;
    case ERRMODE_TERMINAL_MSG:
      printf("%s\n", msg);
      break;
    }
  }
  exit(1);
}

void memorize_config_path() {
  size_t needed_length, wd_length;
  char* result;
  
  result = getcwd(g_daemon_data.config_path, MAX_PATH_LENGTH);
  if (result == NULL)
    on_error(FORMAT_ERR_GET_CWD, ERRMODE_ERRNO_FORMAT);
  wd_length = strlen(g_daemon_data.config_path);
  needed_length = wd_length + strlen(FILE_NAME_CONFIG) + 2;
  if (needed_length > MAX_PATH_LENGTH || 
      !strcat(g_daemon_data.config_path, "/") ||
      !strcat(g_daemon_data.config_path + wd_length, FILE_NAME_CONFIG))
    on_error(MSG_ERR_COPY_CONFIG_PATH, ERRMODE_SIMPLE_MSG);
}

int parse_config_data(FILE* config) //!!
{
  return fscanf(config, "%s", g_daemon_data.word); 
}

void read_config()
{
  FILE* config;
  int scan_status;

  if (!file_exists(g_daemon_data.config_path))
    on_error(MSG_ERR_CONFIG_NOT_EXIST, ERRMODE_SIMPLE_MSG);
  if ((config = fopen(g_daemon_data.config_path, "r")) == NULL)
    on_error(FORMAT_ERR_CONFIG_OPEN, ERRMODE_ERRNO_FORMAT);
  scan_status = parse_config_data(config);
  fclose(config);
  if (scan_status == -1)
    on_error(MSG_ERR_CONFIG_PARSE, ERRMODE_SIMPLE_MSG);
}

void on_sigterm()
{
  syslog(LOG_INFO, MSG_CAUGHT_SIGTERM);
  if (remove(FILE_NAME_PID) != 0)
    on_error(FORMAT_ERR_PID_RM, ERRMODE_ERRNO_FORMAT);
  exit(0);
}

void on_sighup() 
{
  syslog(LOG_INFO, MSG_CAUGHT_SIGHUP);
  read_config();
}

void sig_handler(int signo)
{
  switch (signo)
  {
  case(SIGTERM):
    on_sigterm();
    break;
  case(SIGHUP):
    on_sighup();
    break;
  }
}

void handle_signals()
{
  if(signal(SIGTERM, sig_handler) == SIG_ERR)
    on_error(MSG_ERR_CATCH_SIGTERM, ERRMODE_SIMPLE_MSG);
  if(signal(SIGHUP, sig_handler) == SIG_ERR)
    on_error(MSG_ERR_CATCH_SIGHUP, ERRMODE_SIMPLE_MSG);
}

void daemonise()
{
  pid_t pid, sid;
  FILE *pid_fp;

  syslog(LOG_INFO, MSG_INFO_START);

  //First fork
  pid = fork();
  if (pid < 0)
    on_error(FORMAT_ERR_FORK_1, ERRMODE_ERRNO_FORMAT);

  if(pid > 0)
  {
    syslog(LOG_INFO, MSG_INFO_FORK_1_P);
    exit(0);
  }
  syslog(LOG_INFO, MSG_INFO_FORK_1_C);

  //Create a new session
  sid = setsid();
  if (sid < 0)
    on_error(FORMAT_ERR_SESSION, ERRMODE_ERRNO_FORMAT);
  syslog(LOG_INFO, MSG_INFO_SESSION);

  //Second fork
  pid = fork();
  if (pid < 0)
    on_error(FORMAT_ERR_FORK_2, ERRMODE_ERRNO_FORMAT);

  if(pid > 0)
  {
    syslog(LOG_INFO, MSG_INFO_FORK_2_P);
    exit(0);
  }
  syslog(LOG_INFO, MSG_INFO_FORK_2_C);

  pid = getpid();

  //Change working directory to Home directory
  if (chdir(getenv("HOME")) == -1)
    on_error(FORMAT_ERR_CH_WD, ERRMODE_ERRNO_FORMAT);

  //Grant all permisions for all files and directories created by the daemon
  umask(0);

  //Redirect std IO
  close(STDIN_FILENO);
  close(STDOUT_FILENO);
  close(STDERR_FILENO);
  if (open("/dev/null", O_RDONLY) == -1)
    on_error(FORMAT_ERR_REOPEN_STDIN, ERRMODE_ERRNO_FORMAT);
  if(open("/dev/null",O_WRONLY) == -1)
    on_error(FORMAT_ERR_REOPEN_STDOUT, ERRMODE_ERRNO_FORMAT);
  if(open("/dev/null",O_RDWR) == -1)
    on_error(FORMAT_ERR_REOPEN_STDERR, ERRMODE_ERRNO_FORMAT);

  //Create a pid file
  mkdir("run/", 0777);
  pid_fp = fopen(FILE_NAME_PID, "w");
  if (pid_fp == NULL)
    on_error(FORMAT_ERR_PID_CREATE, ERRMODE_ERRNO_FORMAT);
  if(fprintf(pid_fp, "%d\n", pid) < 0)
  {
    syslog(LOG_ERR, FORMAT_ERR_PID_WRITE, errno);
    fclose(pid_fp);
    if(remove(FILE_NAME_PID) != 0)
    {
      syslog(LOG_ERR, FORMAT_ERR_PID_RM, errno);
    }
    exit(1);
  }
  fclose(pid_fp);
}

enum start_mode_t {
  MODE_STOP,
  MODE_START,
};

void daemon_do() //!!
{
  FILE* tell = fopen("tell.txt", "a");
  if (!tell)
    exit(1);
  fprintf(tell, "%s\n", g_daemon_data.word);
  fclose(tell);
}

enum start_mode_t parse_run_mode(const char* arg) 
{ 
  if ((strcmp(arg, "start") == 0) || (strcmp(arg, "stop") == 0))
    return strcmp(arg, "start") == 0 ? MODE_START : MODE_STOP;
  else
    on_error(MSG_INFO_USAGE, ERRMODE_TERMINAL_MSG);
} 

enum start_mode_t handle_args(int argc, char** argv) 
{
  if (argc > 2)
    on_error(MSG_INFO_USAGE, ERRMODE_TERMINAL_MSG);
  //argc is either less than 2 or equal to 2 in the following condition
  return argc < 2 ? MODE_START : parse_run_mode(argv[1]); //If no additional args -- start mode by default
}

int file_exists(char* file_name) 
{
  struct stat st;
  return !(stat(file_name, &st) == -1 && errno == ENOENT);
}

int is_pid_running(int pid) 
{
  return !(kill(pid, 0) == -1 && errno == ESRCH);
}

int terminate_existing_daemon()
{
  FILE* pid_file;
  int pid, scan_status;

  if (file_exists(FILE_NAME_PID)) 
  {
    if ((pid_file = fopen(FILE_NAME_PID, "r")) == NULL)
      on_error(FORMAT_ERR_PID_OPEN, ERRMODE_ERRNO_FORMAT);
    scan_status = fscanf(pid_file, "%d", &pid);
    fclose(pid_file);
    if (scan_status == -1)
      on_error(FORMAT_ERR_PID_READ, ERRMODE_ERRNO_FORMAT);
    printf("PID:%d\n", pid); //DBG
    if (is_pid_running(pid) && (kill(pid, SIGTERM) != 0))
      on_error(FORMAT_ERR_KILL_PID, ERRMODE_ERRNO_FORMAT);
    return 0;
  }
  return -1;
}

void on_mode_start() 
{
  terminate_existing_daemon();
  daemonise();
  read_config();
}

void on_mode_stop()
{
  if (terminate_existing_daemon() != 0)
    on_error(MSG_ERR_DAEMON_NOT_RUNNING, ERRMODE_SIMPLE_MSG);
  exit(0);
}

void handle_start(enum start_mode_t mode) 
{
  memorize_config_path();
  if (chdir(getenv("HOME")) == -1)
    on_error(FORMAT_ERR_CH_WD, ERRMODE_ERRNO_FORMAT);

  switch (mode)
  {
  case MODE_START:
    on_mode_start();
    break;
  case MODE_STOP:
    on_mode_stop();
    break;
  }
}

int main(int argc, char** argv)
{
  enum start_mode_t mode;
  
  mode = handle_args(argc, argv);
  handle_start(mode);
  handle_signals();

  while(1)
  {
    daemon_do();
    sleep(DAEMON_SLEEP_DURATION);
  }
  return 0;
}
