#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h> 
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#include "connector.h"

#define FIFO_NAME "goatgame"

static int created = 0;
static int fdr = 0;
static int fdw = 0;

void connector_create()
{
  created = (mkfifo(FIFO_NAME, S_IRWXU) == 0);
  if (!created)
    return;
  fdr = open(FIFO_NAME, O_RDONLY | O_NONBLOCK);
  fdw = open(FIFO_NAME, O_WRONLY | O_NONBLOCK);
  created = (fdr != -1 && fdw != -1);
  printf("%d\n%d\n", created, errno);
}

int connector_read(void* buffer, size_t size)
{
  return read(fdr, buffer, size) == -1 ? CONNECTOR_FAILED : CONNECTOR_SUCCEEDED;
}

int connector_is_created()
{
  return created != 0 ? CONNECTOR_SUCCEEDED : CONNECTOR_FAILED;
}

int connector_write(void* buffer, size_t size)
{
  return write(fdw, buffer, size) == -1 ? CONNECTOR_FAILED : CONNECTOR_SUCCEEDED;
}

void connector_destruct()
{
  unlink(FIFO_NAME);
}

