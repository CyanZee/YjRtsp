#include "YjMpeg4FileSink.hh"
YjMpeg4FileSink::YjMpeg4FileSink(UsageEnvironment& env, callBack* func, void* custom,
			      unsigned int bufferSize)
	: FileSink(env, NULL, bufferSize, NULL)

{
	callBackFunc = func;
	user = custom;
	m_dwTimeStamp = 0;
	m_preTimeFlag = 0;
}

YjMpeg4FileSink* YjMpeg4FileSink::createNew(UsageEnvironment& env, callBack* func, void* custom,
			      unsigned int bufferSize) 
{
	return new YjMpeg4FileSink(env, func, custom, bufferSize);
}

void YjMpeg4FileSink::addData(unsigned char const* data, unsigned dataSize, struct timeval presentationTime) 
{
	m_dwTimeStamp += 3600;

	unsigned long long time = 1000000 * presentationTime.tv_sec + presentationTime.tv_usec;
	callBackFunc(1, (unsigned char*)data, dataSize, time, user);
}

void YjMpeg4FileSink::afterGettingFrame(unsigned frameSize,
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

