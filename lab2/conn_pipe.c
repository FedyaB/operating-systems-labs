#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>           

#include "connector.h"

static int file_desc[2];
static int created = 0;

void connector_create()
{
  created = (pipe(file_desc) != -1);
}

int connector_read(void* buffer, size_t size)
{
  return read(file_desc[0], buffer, size) == -1 ? CONNECTOR_FAILED : CONNECTOR_SUCCEEDED;
}

int connector_is_created()
{
  return created != 0 ? CONNECTOR_SUCCEEDED : CONNECTOR_FAILED;
}

int connector_write(void* buffer, size_t size)
{
  return write(file_desc[1], buffer, size) == -1 ? CONNECTOR_FAILED : CONNECTOR_SUCCEEDED;
}

void connector_destruct()
{
  close(file_desc[0]);
  close(file_desc[1]);
}

