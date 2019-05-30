#include "YjAudioFileSink.hh"
YjAudioFileSink::YjAudioFileSink(UsageEnvironment& env, callBack* func, void* custom,
			      unsigned int bufferSize)
	: FileSink(env, NULL, bufferSize, NULL)

{
	callBackFunc = func;
	user = custom;
	m_dwTimeStamp = 0;
	m_preTimeFlag = 0;
}

YjAudioFileSink* YjAudioFileSink::createNew(UsageEnvironment& env, callBack* func, void* custom,
			      unsigned int bufferSize) 
{
	return new YjAudioFileSink(env, func, custom, bufferSize);
}

void YjAudioFileSink::addData(unsigned char const* data, unsigned dataSize, struct timeval presentationTime) 
{
#if 0
	if(!m_preTimeFlag)
	{
		memcpy(&m_preTime, &presentationTime, sizeof(timeval));
		m_dwTimeStamp = 0;
		m_preTimeFlag =1;
	}
	else
	{
		m_dwTimeStamp = ((presentationTime.tv_sec - m_preTime.tv_sec)*1000000 + (presentationTime.tv_usec - m_preTime.tv_usec))*9/100;
	}
#endif
	m_dwTimeStamp += 3600;
	unsigned long long time = 1000000 * presentationTime.tv_sec + presentationTime.tv_usec;
	callBackFunc(2, (unsigned char*)data, dataSize, time,user);
}

void YjAudioFileSink::afterGettingFrame(unsigned frameSize,
				 unsigned numTruncatedBytes,
				 struct timeval presentationTime) {
  if (numTruncatedBytes > 0) {
    envir() << "FileSink::afterGettingFrame(): The input frame data was too large for our buffer size ("
	    << fBufferSize << ").  "
            << numTruncatedBytes << " bytes of trailing data was dropped!  Correct this by increasing the \"bufferSize\" parameter in the \"createNew()\" call to at least "
            << fBufferSize + numTruncatedBytes << "\n";
  }
  addData(fBuffer, frameSize, presentationTime);
  // Then try getting the next frame:
  continuePlaying();
}

