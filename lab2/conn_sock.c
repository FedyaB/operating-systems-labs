#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h> 
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>

#include "connector.h"

#define SOCKET_NAME "goatgame"

static int sockets[2];
static int created = 0;

void connector_create()
{
  created = (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) != -1);
}

int connector_read(void* buffer, size_t size)
{
  return read(sockets[0], buffer, size) == -1 ? CONNECTOR_FAILED : CONNECTOR_SUCCEEDED;
}

int connector_is_created()
{
  return created != 0 ? CONNECTOR_SUCCEEDED : CONNECTOR_FAILED;
}

int connector_write(void* buffer, size_t size)
{
  return write(sockets[1], buffer, size) == -1 ? CONNECTOR_FAILED : CONNECTOR_SUCCEEDED;
}

void connector_destruct()
{
  close(sockets[0]);
  close(sockets[1]);
}

