#include "SensorStream.h"
#include <fcntl.h>

#define debug

#ifdef debug
#    define dprintf(...) printf(__VA_ARGS__)
#else
#    define dprintf(...)
#endif

#define CHECK_RC(rc,what) if(rc!=XN_STATUS_OK){dprintf("%s Failed: %s\n" , what , xnGetStatusString(rc));}

using namespace xn;



int connectSensor()
{

	context.Init();


	//Only want stream if we can get in in 320x240 at 30Fps
	XnMapOutputMode mapMode = {XN_QVGA_X_RES , XN_QVGA_Y_RES,30};

	xn::Query query;
	query.AddSupportedMapOutputMode(mapMode);

	XnStatus nRetVal;

	//Get the Depth Node
	nRetVal = depth.Create(context, &query);
	CHECK_RC(nRetVal, "Depth Creation");
	if (nRetVal == XN_STATUS_OK)
		depth.SetMapOutputMode(mapMode);
	else
		return nRetVal;

	//Attempt To Create IR node
	nRetVal = ir.Create(context, &query);
	CHECK_RC(nRetVal, "IR Creation");
	if (nRetVal == XN_STATUS_OK)
		ir.SetMapOutputMode(mapMode);

	//Attempt to Create ImageNode
	nRetVal = image.Create(context, &query);
	CHECK_RC(nRetVal, "Image Creation");
	if (nRetVal == XN_STATUS_OK)
		image.SetMapOutputMode(mapMode);

	nRetVal = context.StartGeneratingAll();
	return nRetVal;
}

void releaseSensor()
{
	dprintf("Releasing Sensor\n");
	depth.Release();
	ir.Release();
	image.Release();
	context.Release();
}




int createSocket()
{
	dprintf("Creating Socket\n");
	sock_descriptor = socket(AF_INET, SOCK_STREAM, 0); //Create Socket Domain: IP  , Type: Stream /TCP, Protocall: Default
	if (sock_descriptor < 0)
	{
		dprintf("Failed creating socket\n");
		dprintf("Errno: %s\n", strerror(errno));
		return -1;
	}

	bzero((char *) &serv_addr, sizeof(serv_addr)); // Initialize the server address struct to zero
	serv_addr.sin_family = AF_INET;                // Fill server's address family
	serv_addr.sin_addr.s_addr = INADDR_ANY; 	// Server should allow connections from any ip address
	serv_addr.sin_port = htons(PORT);           //account for endian differences

	int yes = 1;
	if (setsockopt(sock_descriptor, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))
			< 0)
	{
		dprintf("Failed setting socket options\n");
		dprintf("Errno: %s\n", strerror(errno));
		return -2;
	}
	// Attach the server socket to a port.
	if (bind(sock_descriptor, (struct sockaddr *) &serv_addr, sizeof(serv_addr))
			< 0)
	{
		dprintf("Failed to attach to socket port\n");
		dprintf("Errno: %s\n", strerror(errno));
		return -3;
	}

	return 1;
}

int connectSocket()
{
	dprintf("Listening to Socket\n");
	// Server should start listening
	if (listen(sock_descriptor, 5) < 0)
	{
		dprintf("Error Listening to Socket\n");
		dprintf("Errno: %s\n", strerror(errno));
		return -1;
	}

	dprintf("Waiting for connection...\n");
	unsigned int size = sizeof(client_addr);

	//Server blocks on this call until a client tries to establish connection.
	conn_desc = accept(sock_descriptor, (struct sockaddr *) &client_addr,
			&size);

	if (conn_desc == -1)
	{
		dprintf("Failed accepting connection\n");
		dprintf("Errno: %s\n", strerror(errno));
		return -2;
	}

	else
		dprintf("Connected\n");

	return 1;
}

void writeBuffer(const char* buffer, XnUInt32 size)
{
	writeBuffer((const XnUInt8*) buffer, size);
}

void writeBuffer(const XnUInt8* buffer, XnUInt32 size)
{
	dprintf("Writing Buffer: %d\n", size);
	while (size > 0)
	{
		ssize_t written = write(conn_desc, buffer, size);
		size -= written;
		buffer += written;
	}

}

void closeSocketConnections()
{
	dprintf("Closing Connection\n");
	close(conn_desc);
	close(sock_descriptor);
}


int setupLED()
{
	ledTriggerFD = open("/sys/class/leds/led0/trigger", O_WRONLY);
	write(ledTriggerFD,"none",4);
	
	ledBrightnessFD = open("/sys/class/leds/led0/brightness", O_WRONLY);
	write(ledBrightnessFD,"0",1);
	
	return 1;
}

void releaseLED()
{
	write(ledTriggerFD,"mmc0",4);
	close(ledTriggerFD);
	close(ledBrightnessFD);
}

void writeLED(bool val)
{
	write(ledBrightnessFD,val ? "1":"0",1);
}

int loop()
{
	size_t readSize;
	dprintf("Starting Loop\n");
	XnStatus nRetVal;

	while (true)
	{
		bzero(buff, sizeof(buff));
		readSize = read(conn_desc, buff, sizeof(buff) - 1);

		if (readSize > 0)
		{
			//Print String
			dprintf("Received: %d %s\n", readSize, buff);

			if (strcmp(buff, "kill") == 0)
			{
				dprintf("Kill Requested\n");
				return 1;
			}

			else if (strcmp(buff, "stop") == 0)
			{
				dprintf("Stop Requested\n");
				return 2;
			}

			else if (strcmp(buff, "getDepth") == 0)
			{
				dprintf("Get Depth Requested\n");
				nRetVal = context.WaitOneUpdateAll(depth);
				if (nRetVal != XN_STATUS_OK)
				{
					dprintf("Update Data failed: %s\n",
							xnGetStatusString(nRetVal));
					writeBuffer("FAILED", 6);
					continue;
				}
				depth.GetMetaData(depthMD);
				writeBuffer((const XnUInt8*) depthMD.Data(),
						depthMD.DataSize());
			}

			else if (strcmp(buff, "getIR") == 0)
			{
				dprintf("Get IR Requested\n");
				if (!ir.IsValid())
				{
					dprintf("IR Node is Invalid");
					writeBuffer("INVALID", 7);
					continue;
				}
				else
				{
					nRetVal = context.WaitOneUpdateAll(ir);
					if (nRetVal != XN_STATUS_OK)
					{
						dprintf("Update Data failed: %s\n",
								xnGetStatusString(nRetVal));
						writeBuffer("FAILED", 6);
						continue;
					}
					ir.GetMetaData(irMD);
					writeBuffer((const XnUInt8*) irMD.Data(), irMD.DataSize());
				}
			}

			else if (strcmp(buff, "getImage") == 0)
			{
				dprintf("Get Image Requested\n");
				if (!image.IsValid())
				{
					dprintf("Image Node is Invalid");
					writeBuffer("INVALID", 7);
					continue;
				}
				else
				{
					nRetVal = context.WaitOneUpdateAll(image);
					if (nRetVal != XN_STATUS_OK)
					{
						dprintf("Update Data failed: %s\n",
								xnGetStatusString(nRetVal));
						writeBuffer("FAILED", 6);
						continue;
					}
					image.GetMetaData(imageMD);
					writeBuffer((const XnUInt8*) imageMD.Data(),
							imageMD.DataSize());
				}
			}
		}

		else
		{   //Client Closed the Connection
			dprintf("Failed Recieving\n");
			return 3;
		}
	}
}

int main(void)
{
	while (true)
	{
		if (connectSensor() != XN_STATUS_OK)
		{
			dprintf("Exiting\n");
			releaseSensor();
			exit(-1);
		}

		if (createSocket() < 0)
		{
			dprintf("Exiting\n");
			closeSocketConnections();
			releaseSensor();
			exit(-2);
		}

		if (connectSocket() < 0)
		{
			dprintf("Exiting\n");
			closeSocketConnections();
			releaseSensor();
			exit(-3);
		}

		if(setupLED()<0)
		{
			dprintf("Exiting\n");
			releaseLED();
			exit(-4);
		}
		writeLED(1);
		bool needReturn = loop() == 1;

		releaseLED();
		releaseSensor();
		closeSocketConnections();

		if (needReturn)
		{
			dprintf("Need Return");
			break;
		}
	}

	dprintf("Exiting Application\n");
}
