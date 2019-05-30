/**********
This library is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the
Free Software Foundation; either version 2.1 of the License, or (at your
option) any later version. (See <http://www.gnu.org/copyleft/lesser.html>.)

This library is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
more details.

You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
**********/
// "liveMedia"
// Copyright (c) 1996-2013 Live Networks, Inc.  All rights reserved.
// H.264 Video File sinks
// Implementation

#include "YjH264VideoFileSink.hh"
#include "OutputFile.hh"
#include "H264VideoRTPSource.hh"

////////// H264VideoFileSink //////////
extern int GetAnnexbNALU (char *output);
extern void OpenBitstreamFile (char *fn);


YjH264VideoFileSink
::YjH264VideoFileSink(UsageEnvironment& env, char const* sPropParameterSetsStr,
		    unsigned bufferSize, char const* perFrameFileNamePrefix,
		    callBack* func, void* custom)
  : FileSink(env, NULL, bufferSize, perFrameFileNamePrefix),
    fSPropParameterSetsStr(sPropParameterSetsStr), fHaveWrittenFirstFrame(False), first_flag(False) {
#if 0
	udp_fd = socket(PF_INET, SOCK_DGRAM, 0);
	int size = 1<<20;
	setsockopt(udp_fd, SOL_SOCKET, SO_RCVBUF, (const char *)&size, sizeof(int));
	sock_to.sin_family = AF_INET;
	sock_to.sin_port = htons(15000);
	sock_to.sin_addr.s_addr = inet_addr("192.168.0.156");
#endif

	m_dwTimeStamp = 0;
	sps_str = NULL;
	pps_str = NULL;
	user = custom;
	callBackFunc = func;
}

YjH264VideoFileSink::~YjH264VideoFileSink() {
#if 0
	closesocket(udp_fd);
#endif
	if(sps_str)
	{
		delete [] sps_str;
	}

	if(pps_str)
	{
		delete [] pps_str;
	}
}

YjH264VideoFileSink*
YjH264VideoFileSink::createNew(UsageEnvironment& env,
			     char const* sPropParameterSetsStr,
			     unsigned bufferSize, Boolean oneFilePerFrame, callBack* func, void* custom) {
  do {
	  char const* perFrameFileNamePrefix;
      perFrameFileNamePrefix = NULL;

    return new YjH264VideoFileSink(env, sPropParameterSetsStr, bufferSize, perFrameFileNamePrefix, func, custom);
  } while (0);

  return NULL;
}

void YjH264VideoFileSink::getSpsppsFrame(char *sps_str, int &sps_len, char *pps_str, int &pps_len)
{
	 unsigned char const start_code[4] = {0x00, 0x00, 0x00, 0x01};
    // If we have PPS/SPS NAL units encoded in a "sprop parameter string", prepend these to the file:
	unsigned numSPropRecords;
	SPropRecord* sPropRecords = parseSPropParameterSets(fSPropParameterSetsStr, numSPropRecords);

	if( NULL != sPropRecords)
	{
		sps_len = sPropRecords[0].sPropLength + 4;
		sps_str = new char[sps_len];
		memcpy(sps_str, start_code,4);
		memcpy(sps_str + 4, sPropRecords[0].sPropBytes, sPropRecords[0].sPropLength);
		pps_len = sPropRecords[1].sPropLength + 4;
		pps_str = new char[pps_len];
		memcpy(pps_str, start_code,4);
		memcpy(pps_str + 4, sPropRecords[1].sPropBytes, sPropRecords[1].sPropLength);
	}

	if(sPropRecords)
	{
		delete[] sPropRecords;
		sPropRecords = NULL;
	}

	  return; 
	
}

void YjH264VideoFileSink::afterGettingFrame(unsigned frameSize, unsigned numTruncatedBytes, struct timeval presentationTime) {
  unsigned char const start_code[4] = {0x00, 0x00, 0x00, 0x01};
  char *data = NULL;
  int offset = 0;

#if 0
  if (!fHaveWrittenFirstFrame) {
    // If we have PPS/SPS NAL units encoded in a "sprop parameter string", prepend these to the file:
    unsigned numSPropRecords;
    SPropRecord* sPropRecords = parseSPropParameterSets(fSPropParameterSetsStr, numSPropRecords);
	int len = 0;
#if 0
    for (unsigned i = 0; i < numSPropRecords; ++i) {
		len = len + 4 + sPropRecords[i].sPropLength;
	}
	len += frameSize + 4;
	data = new char[len];
	memset(data, 0x00, len);
    for (unsigned i = 0; i < numSPropRecords; ++i) {
		memcpy(data + offset,start_code,4);
		memcpy(data + offset + 4, sPropRecords[i].sPropBytes, sPropRecords[i].sPropLength);
		offset = offset + 4 + sPropRecords[i].sPropLength;
//      addData(start_code, 4, presentationTime);
//      addData(sPropRecords[i].sPropBytes, sPropRecords[i].sPropLength, presentationTime);
    }
#endif
		if( NULL != sPropRecords)
		{
			sps_len = sPropRecords[0].sPropLength + 4;
			sps_str = new char[sps_len];
			memcpy(sps_str, start_code,4);
			memcpy(sps_str + 4, sPropRecords[0].sPropBytes, sPropRecords[0].sPropLength);
			pps_len = sPropRecords[1].sPropLength + 4;
			pps_str = new char[pps_len];
			memcpy(pps_str, start_code,4);
			memcpy(pps_str + 4, sPropRecords[1].sPropBytes, sPropRecords[1].sPropLength);
		}
		
		delete[] sPropRecords;
		sPropRecords = NULL;
		fHaveWrittenFirstFrame = True; // for next time
  }
#endif
  // Write the input data to the file, with the start code in front:
  //addData(start_code, 4, presentationTime);

  // Call the parent class to complete the normal file write with the input data:
 // FileSink::afterGettingFrame(frameSize, numTruncatedBytes, presentationTime);
  // addData(fBuffer, frameSize, presentationTime);

  if(1)
  {
	  int nal_type = 0;
	  nal_type = fBuffer[0] & 0x1f;

	printf("+++ nal_type = %d\n", nal_type);
	  if(nal_type != 7 && nal_type != 8 && nal_type != 5 && nal_type != 1 && nal_type != 9)
  	  {
		  continuePlaying();
		  return; 
  	  }

	 if(!first_flag && nal_type != 7 && nal_type != 5)
	 {
		continuePlaying();
		return; 
	 }

	  if(nal_type == 7)//SPS
	  {
	  	first_flag = True;
	  	sps_len = frameSize + 4;
		if(sps_str)
		{
			delete[] sps_str;
			sps_str = NULL;
		}
		if(pps_str)
		{
			delete[] pps_str;
			pps_str = NULL;
		}
		sps_str = new char[sps_len];
		memcpy(sps_str,start_code,4);
		memcpy(sps_str + 4, fBuffer, frameSize);
		if(data)
		{
			delete [] data;
			data = NULL;
		}
		continuePlaying();
		return;	
	  }

	  if(sps_str && nal_type == 8)//SPS
	  {
		  if(pps_str)
		  {
			  continuePlaying();
			  return; 
		  }
		  pps_len = frameSize + 4;
		  pps_str = new char[sps_len];
		  memcpy(pps_str,start_code,4);
		  memcpy(pps_str + 4, fBuffer, frameSize);
		  continuePlaying();
		  return; 
	  }

	  if(nal_type == 5 && NULL == sps_str && NULL == sps_str)
	  {
	  	getSpsppsFrame(sps_str, sps_len, pps_str, pps_len);
	  }

	  if(sps_str && pps_str && nal_type != 5)
  	  {
		  continuePlaying();
		  return; 
  	  }
	  if(sps_str && pps_str && !data)
	  {
	  	int len = pps_len + sps_len + frameSize + 4;
		if(data)
		{
			delete [] data;
			data = NULL;
		}
	    data = new char[len];
		memset(data, 0x00, len);

		memcpy(data, sps_str, sps_len);
		offset += sps_len;

		memcpy(data + offset, pps_str, pps_len);
		offset += pps_len;
		delete [] sps_str;
		sps_str = NULL;
		sps_len = 0;
		delete [] pps_str;
		pps_str = NULL;
		pps_len = 0;
	  }


  
	  if(NULL == data)
	  {
	    data = new char[frameSize + 4];
		memset(data, 0x00, frameSize + 4);
	  }
	  memcpy(data + offset,start_code,4);
	  offset += 4;
	  memcpy(data + offset, fBuffer, frameSize);
	  addData(nal_type, (unsigned char const*)data, frameSize+offset, presentationTime);

	  // Then try getting the next frame:
	  delete [] data;
	  continuePlaying();
  }


}

void YjH264VideoFileSink::addData(unsigned int type, unsigned char const* data, unsigned dataSize,
		       struct timeval presentationTime) {
  //printf("addData dataSize=%d\n",dataSize);

#if 1
/*  int nPSBufSize = dataSize + 1000;
  unsigned char *pPSBuf = new unsigned char[nPSBufSize];
  //nPSBufSize = h264newPSPack((unsigned char*)data, dataSize, pPSBuf, nPSBufSize, m_dwTimeStamp);
  nPSBufSize = h264NALPSPack((unsigned char*)data, dataSize, pPSBuf, nPSBufSize, m_dwTimeStamp);
  m_dwTimeStamp += 3600;
  if (m_dwTimeStamp >= 0x0fffffff)
  {
	m_dwTimeStamp = 0;
  }*/

  unsigned long long time = 1000000 * presentationTime.tv_sec + presentationTime.tv_usec;
  callBackFunc(type, (unsigned char*)data, dataSize, time, user);
//  delete [] pPSBuf;
#if 0
  int send_len = 0;
  while (nPSBufSize > 1300 && nPSBufSize>0)
  {
	  int len = sendto(udp_fd, (const char *)(pPSBuf + send_len), 1300, 0, (struct sockaddr*)&sock_to, sizeof(sock_to));
	  nPSBufSize -= len;
	  send_len += len;
	  Sleep(1);
  }
  
  int len = sendto(udp_fd, (const char *)(pPSBuf + send_len), nPSBufSize, 0, (struct sockaddr*)&sock_to, sizeof(sock_to));
#endif
#endif
  // Write to our file:
return;
}

