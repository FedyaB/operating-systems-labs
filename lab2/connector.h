#ifndef CONNECTOR_H_FJJDDD__
#define CONNECTOR_H_FJJDDD__

//Return codes
#define CONNECTOR_FAILED 0
#define CONNECTOR_SUCCEEDED 1

//Create a connector instance
void connector_create();

//Check whether the connector is created
int connector_is_created();

//Read data from the connector to a buffer
int connector_read(void* buffer, size_t size);

//Write data from a buffer to the connector
int connector_write(void* buffer, size_t size);

//Destroy connector instance
void connector_destruct();

#endif