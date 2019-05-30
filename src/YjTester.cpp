#include <iostream>
#include "YjMutex.h"
#include "YjRTSPMedia.h"

using namespace std;

void realDataCallback(unsigned int type, unsigned char* data, unsigned int datasize, unsigned long long timeStamp, void *user)
{
	printf("+++ the nal_type is: %d, datasize: %d \n", type, datasize);

}

int main() 
{
	char usr[32] = "admin";
	char pwd[32] = "bq123456";
	char url[128] = "rtsp://admin:bq123456@172.17.20.29:554/Streaming/Channels/102?transportmode=unicast&profile=Profile_1";
	YjRtspMedia *media = new YjRtspMedia;
	addMediaToList(media);
	media->setCallbackFunction(realDataCallback, NULL);
	if(media != NULL)
	{
		media->RtspPlay(usr, pwd, url);
	}
	else
	{
		cout << "--- rtsp play error." << endl;
	}
	return 0;
}
