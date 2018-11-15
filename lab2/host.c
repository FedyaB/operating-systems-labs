#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <string.h>
#include <fcntl.h>
#include <syslog.h>
#include <time.h>
#include <math.h>
#include <errno.h>

#include "connector.h"

//Semaphores parameters
#define SEMAPHORE_NAME_H "/sem_host"
#define SEMAPHORE_NAME_C "/sem_client"
#define SEMAPHORE_WAIT_LIMIT 5

//Number of goats
#define N 1

//Error messages
#define MSG_ERR_SEMAPHORE_CREATE "Semaphore creation error"
#define MSG_ERR_SEMAPHORE_OPEN "Semaphore opening error"
#define MSG_ERR_FORK_FAILED "Fork failed"
#define MSG_ERR_CONN_CREATE "Communication: connector creation fail"
#define MSG_ERR_CONN_READ "Communication: reading data fail"
#define MSG_ERR_CONN_WRITE "Communication: writing data fail"
#define MSG_ERR_TIME "System time access fail"
#define MSG_ERR_TIMED_WAIT "Timed wait error"

//Info messages
#define MSG_INFO_TIMEOUT "Connection timeout exceeded"

//Game info messages
#define GAME_START "*****\nThe game has started!\n*****"
#define GAME_END "The game has ended!"
#define GAME_GOAT_DIED "The goat has died"
#define GAME_GOAT_RESURRECTED "The goat has resurrected"
#define GAME_FORMAT_TURN "Turn: %d\n"
#define GAME_FORMAT_WOLF_NUMBER "Wolf number: %d\n"
#define GAME_FORMAT_GOAT_NUMBER "Goat number: %d\n"
#define GAME_FORMAT_NUM_DIFF "An absolute difference between numbers is: %d\n"

//Game data structure
struct game_data_t
{
  int wolf_number, goat_number; //Current wolf and goat numbers
  int goat_alive, goat_death_turns; //Current goat's metadata
};

//Semaphores
static sem_t* p_sem_host = NULL;
static sem_t* p_sem_client = NULL;

//Close semaphore with provided name and unlink if needed
void close_semaphore(sem_t* p_sem, int unlink, const char* name)
{
  sem_close(p_sem);
  if (unlink)
    sem_unlink(name);
}

//Clean global connections
void clean()
{
  if (p_sem_host != NULL && p_sem_host != SEM_FAILED)
    close_semaphore(p_sem_host, 1, SEMAPHORE_NAME_H);
  if (p_sem_client != NULL && p_sem_client != SEM_FAILED)
    close_semaphore(p_sem_client, 1, SEMAPHORE_NAME_C);
  if (connector_is_created() == CONNECTOR_SUCCEEDED)
    connector_destruct();
}

//Error handler
void on_error(const char* msg)
{
  if (msg != NULL)
    syslog(LOG_ERR, "%s", msg);
  clean();
  exit(1);
}

//Semaphore wait with timeout 
void semaphore_timed_wait(sem_t* p_sem)
{
  struct timespec ts;
  int status;

  if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
    on_error(MSG_ERR_TIME);
  ts.tv_sec += SEMAPHORE_WAIT_LIMIT;
  while ((status = sem_timedwait(p_sem, &ts)) == -1 && errno == EINTR);
  if (status == -1)
  {
    if (errno == ETIMEDOUT)
    {
      syslog(LOG_INFO, "%s", MSG_INFO_TIMEOUT);
      clean();
      exit(0);
    }
    else
      on_error(MSG_ERR_TIMED_WAIT);
  }
}

//On game initialize
void game_init(struct game_data_t* p_game_data)
{
  p_game_data->goat_alive = 1;
  p_game_data->goat_death_turns = -1;
}

//Send goat alive state through the connector
void game_send_alive_state(struct game_data_t* p_game_data)
{
  if (connector_write(&(p_game_data->goat_alive), sizeof(int)) == CONNECTOR_FAILED)
    on_error(MSG_ERR_CONN_WRITE);
}

//Read goat number through the connector
void game_get_goat_number(struct game_data_t* p_game_data)
{
  if (connector_read(&(p_game_data->goat_number), sizeof(int)) == CONNECTOR_FAILED)
    on_error(MSG_ERR_CONN_READ);
}

//Generate wolf number
void game_generate_wolf_number(struct game_data_t* p_game_data)
{
  p_game_data->wolf_number = 1 + rand() % 101;
}

//Perform action depending on current wolf and goat numbers
void game_analyze_numbers(struct game_data_t* p_game_data)
{
  int num_delta = abs(p_game_data->goat_number - p_game_data->wolf_number);
  if (p_game_data->goat_alive && (num_delta > 70 / N))
  {
    printf("%s\n", GAME_GOAT_DIED); //Goat dies
    p_game_data->goat_alive = 0;
  }
  if (!p_game_data->goat_alive)
  {
    if (num_delta <= ceil(20 / N))
    {
      printf("%s\n", GAME_GOAT_RESURRECTED); //Goat resurrects
      p_game_data->goat_alive = 1;
      p_game_data->goat_death_turns = -1;
    }
    else
      ++p_game_data->goat_death_turns; //Goat is still dead
  }
}

//A game ending condtion
int game_end_condition(struct game_data_t* p_game_data)
{
  return p_game_data->goat_death_turns >= 2;
}

//Send game ending signal to the goat
void game_send_end_signal(struct game_data_t* p_game_data)
{
  p_game_data->goat_alive = -1;
  if (connector_write(&(p_game_data->goat_alive), sizeof(int)) == CONNECTOR_FAILED)
    on_error(MSG_ERR_CONN_WRITE);
}

void host_routine()
{
  int i, num_delta;
  struct game_data_t game_data;

  game_init(&game_data);
  printf("%s\n", GAME_START);
  for (i = 1; i; ++i)
  {
    printf(GAME_FORMAT_TURN, i);
    game_send_alive_state(&game_data);
    sem_post(p_sem_host);
    semaphore_timed_wait(p_sem_client);

    game_generate_wolf_number(&game_data);
    printf(GAME_FORMAT_WOLF_NUMBER, game_data.wolf_number);
    game_get_goat_number(&game_data);
    printf(GAME_FORMAT_GOAT_NUMBER, game_data.goat_number);

    num_delta = abs(game_data.goat_number - game_data.wolf_number);
    printf(GAME_FORMAT_NUM_DIFF, num_delta);
    game_analyze_numbers(&game_data);
    printf("\n");

    if (game_end_condition(&game_data))
      break;
  }
  game_send_end_signal(&game_data);
  sem_post(p_sem_host);
  printf("%s\n", GAME_END);
}

//Read goat alive state from the connector
int game_get_alive_state()
{
  int goat_alive;

  if (connector_read(&goat_alive, sizeof(int)) == CONNECTOR_FAILED)
    on_error(MSG_ERR_CONN_READ);
  return goat_alive;
}

//Generate current goat number
int game_generate_goat_number(int goat_alive)
{
  if (goat_alive)
    return 1 + rand() % 100;
  else
    return 1 + rand() % 50;
}

//Send current goat number through the connector
void game_send_goat_number(int goat_number)
{
  if (connector_write(&goat_number, sizeof(int)) == CONNECTOR_FAILED)
    on_error(MSG_ERR_CONN_WRITE);
}

void client_routine()
{
  int goat_number;
  int goat_alive;

  while (1)
  {
    semaphore_timed_wait(p_sem_host);
    goat_alive = game_get_alive_state();
    if (goat_alive == -1)
      break;
    goat_number = game_generate_goat_number(goat_alive);
    game_send_goat_number(goat_number);
    sem_post(p_sem_client);
  }
}

void host()
{
  host_routine();
  clean();
}

void client()
{
  if ((p_sem_host = sem_open(SEMAPHORE_NAME_H, 0)) == SEM_FAILED || (p_sem_client = sem_open(SEMAPHORE_NAME_C, 0)) == SEM_FAILED)
  {
    sem_unlink(SEMAPHORE_NAME_H);
    sem_unlink(SEMAPHORE_NAME_C);
    on_error(MSG_ERR_SEMAPHORE_OPEN);
  }
  client_routine();
  close_semaphore(p_sem_host, 0, SEMAPHORE_NAME_H);
  close_semaphore(p_sem_client, 0, SEMAPHORE_NAME_C);
}

int main(void)
{
  int pid;

  srand(239); //A specific seed to recreate game flow. Change to srand(time(NULL)) if it is not needed
  connector_create();

  if ((p_sem_host = sem_open(SEMAPHORE_NAME_H, O_CREAT | O_EXCL, S_IRWXU, 0)) == SEM_FAILED)
    on_error(MSG_ERR_SEMAPHORE_CREATE);
  if ((p_sem_client = sem_open(SEMAPHORE_NAME_C, O_CREAT | O_EXCL, S_IRWXU, 0)) == SEM_FAILED)
    on_error(MSG_ERR_SEMAPHORE_CREATE);

  if ((pid = fork()) == -1)
    on_error(MSG_ERR_FORK_FAILED);

  if (pid != 0)
    host();
  else
    client();

  return 0;
}

