#include <string.h>
#include <stdio.h>
#include <fcntl.h>           
#include <sys/stat.h>
#include <mqueue.h>

#include "connector.h"

#define QUEUE_NAME "/goatgame"

static mqd_t w_queue;
static mqd_t r_queue;
static struct mq_attr attr;

void connector_create()
{
  attr.mq_maxmsg = 1;
  attr.mq_msgsize = sizeof(int);
  attr.mq_flags = 0;
  attr.mq_curmsgs = 0;

  w_queue = mq_open(QUEUE_NAME, O_CREAT | O_WRONLY, 0600, &attr);
  r_queue = mq_open(QUEUE_NAME, O_RDONLY);
}

int connector_read(void* buffer, size_t size)
{
  unsigned int prio;
  return mq_receive(r_queue, buffer, size, &prio) == -1 ? CONNECTOR_FAILED : CONNECTOR_SUCCEEDED;
}

int connector_is_created()
{
  return w_queue != -1 ? CONNECTOR_SUCCEEDED : CONNECTOR_FAILED;
}

int connector_write(void* buffer, size_t size)
{
  return mq_send(w_queue, buffer, size, 0) == -1 ? CONNECTOR_FAILED : CONNECTOR_SUCCEEDED;
}

void connector_destruct()
{
  mq_close(w_queue);
  mq_close(r_queue);
  mq_unlink(QUEUE_NAME);
}

