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
// H.264 Video File Sinks
// C++ header

#ifndef _YJH264_VIDEO_FILE_SINK_HH
#define _YJH264_VIDEO_FILE_SINK_HH

#ifndef _FILE_SINK_HH
#include "FileSink.hh"
#endif

class YjH264VideoFileSink: public FileSink {
public:
  typedef void (callBack)(unsigned int type,
	    		   unsigned char* data, unsigned int size, unsigned long long timeStamp, void* custom);
  static YjH264VideoFileSink* createNew(UsageEnvironment& env,
			     char const* sPropParameterSetsStr,
			     unsigned bufferSize, Boolean oneFilePerFrame, 
			     callBack* func, void* custom);
  // See "FileSink.hh" for a description of these parameters.

  void addData(unsigned int type, unsigned char const* data, unsigned dataSize,
	       struct timeval presentationTime);

  void getSpsppsFrame(char *sps_str, int &sps_len, char *pps_str, int &pps_len);

protected:
  YjH264VideoFileSink(UsageEnvironment& env, char const* sPropParameterSetsStr,
		    unsigned bufferSize, char const* perFrameFileNamePrefix,
		    callBack* func, void* custom);
      // called only by createNew()
  virtual ~YjH264VideoFileSink();

protected: // redefined virtual functions:
  virtual void afterGettingFrame(unsigned frameSize, unsigned numTruncatedBytes, struct timeval presentationTime);


private:
  char const* fSPropParameterSetsStr;
  Boolean fHaveWrittenFirstFrame;
  FILE* fOutFid;
  int udp_fd;
  sockaddr_in sock_to;
  unsigned int m_dwTimeStamp;
  char *sps_str;
  int sps_len;
  char *pps_str;
  int pps_len;
  void *user;
  Boolean first_flag;
  callBack *callBackFunc;
  
};

#endif
