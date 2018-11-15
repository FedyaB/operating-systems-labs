#include <string.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "connector.h"

#define COMMUNICATION_BUFFER_SIZE 80
#define SH_FILE_NAME "/goatgame"

static void* p_communication_buffer = NULL;
static int file_desc = 0;

void connector_create()
{
  file_desc = shm_open(SH_FILE_NAME, O_CREAT | O_EXCL | O_RDWR, 0600);
  if (file_desc == -1)
    return;
  if (ftruncate(file_desc, COMMUNICATION_BUFFER_SIZE) == -1)
    return;
  p_communication_buffer = (void*)mmap(NULL, COMMUNICATION_BUFFER_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, file_desc, 0);
  if (p_communication_buffer == MAP_FAILED)
    p_communication_buffer = NULL;
}

static int _memory_copy(void* destination, void* source, size_t size)
{
  if (size > COMMUNICATION_BUFFER_SIZE)
    return CONNECTOR_FAILED;
  if (memcpy(destination, source, size) == NULL)
    return CONNECTOR_FAILED;
  return CONNECTOR_SUCCEEDED;
}

int connector_read(void* buffer, size_t size)
{
  return _memory_copy(buffer, p_communication_buffer, size);
}

int connector_is_created()
{
  return p_communication_buffer ? CONNECTOR_SUCCEEDED : CONNECTOR_FAILED;
}

int connector_write(void* buffer, size_t size)
{
  return _memory_copy(p_communication_buffer, buffer, size);
}

void connector_destruct()
{
  munmap(p_communication_buffer, COMMUNICATION_BUFFER_SIZE);
  shm_unlink(SH_FILE_NAME);
}

