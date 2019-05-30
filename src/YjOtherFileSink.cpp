#include "YjOtherFileSink.hh"
YjOtherFileSink::YjOtherFileSink(UsageEnvironment& env, callBack* func, void* custom,
			      unsigned int bufferSize)
	: FileSink(env, NULL, bufferSize, NULL)

{
	callBackFunc = func;
	user = custom;
	m_dwTimeStamp = 0;
	m_preTimeFlag = 0;
}

YjOtherFileSink* YjOtherFileSink::createNew(UsageEnvironment& env, callBack* func, void* custom,
			      unsigned int bufferSize) 
{
	return new YjOtherFileSink(env, func, custom, bufferSize);
}

void YjOtherFileSink::addData(unsigned char const* data, unsigned dataSize, struct timeval presentationTime) 
{
#if 0
	m_dwTimeStamp += 3600;

	callBackFunc(1, (unsigned char*)data, dataSize, m_dwTimeStamp,user);
#endif
}

void YjOtherFileSink::afterGettingFrame(unsigned frameSize,
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

