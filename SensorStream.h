#ifndef SENSOR_STREAM_H_
#define SENSOR_STREAM_H_

#include <XnOpenNI.h>
#include <XnLog.h>
#include <XnCppWrapper.h>
#include <XnFPSCalculator.h>

#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <errno.h>

#include <stdlib.h>
#include <string.h>

#define MAX_SIZE 50
#define PORT 1234

/**
 * Creates, Connects, and Processes Socket Data
 */
int main(void);

/**
 * Creates the Socket
 */
int createSocket();

/**
 * Connects to the Socket
 */
int connectSocket();

/**
 * Sets up the LED for Writing
 */
int setupLED();
void releaseLED();
void writeLED(bool val);

/**
 * Starts Processing Data
 */
int loop();

/**
 * Closes the Connections
 */
void closeSocketConnections();

/**
 * Writes a buffer to the TCP Socket, takeing into account that all bytes
 * are not gauranteed to be written in a single write
 *
 * @param buffer a pointer to the begining of the buffer to write
 * @param size the number of bytes to write
 */
void writeBuffer(const char* buffer, XnUInt32 size);
void writeBuffer(const XnUInt8* buffer, XnUInt32 size);


//***************************************************************************************
//LED Variables
//***************************************************************************************
int ledTriggerFD;
int ledBrightnessFD;

//***************************************************************************************
//Socket Variables
//***************************************************************************************
int sock_descriptor;                       //Socket Descriptor for
int conn_desc;                               //Socket Descriptor for
struct sockaddr_in serv_addr;       //Socket address for server
struct sockaddr_in client_addr;     //Socket address for client
char buff[MAX_SIZE];                      //Incoming Data Buffer


//***************************************************************************************
//Kinect Variables
//***************************************************************************************
xn::Context context;				//The OpenNi Context

xn::DepthGenerator depth;			//The Depth Generator
xn::DepthMetaData depthMD;		//The Depth MetaData

xn::IRGenerator ir;					//The IR Generator
xn::IRMetaData irMD;				//The IR MetaData

xn::ImageGenerator image;			//The image generator
xn::ImageMetaData imageMD;		//The image metadata

#endif /* HELLORPI_H_ */
