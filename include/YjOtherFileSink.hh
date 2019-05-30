#ifndef _FILE_OTHER_SINK_HH
#define _FILE_OTHER_SINK_HH
#ifndef _FILE_SINK_HH
#include "FileSink.hh"
#endif

class YjOtherFileSink : public FileSink{
public:
	typedef void (callBack)(unsigned int type,
					 unsigned char* data, unsigned int size, unsigned long long timeStamp, void* custom);

	YjOtherFileSink(UsageEnvironment& env, callBack* func, void* custom,
					  unsigned int bufferSize);
	static YjOtherFileSink* createNew(UsageEnvironment& env, callBack* func, void* custom,
				unsigned bufferSize = 20000);

	void addData(unsigned char const* data, unsigned dataSize, struct timeval presentationTime);

	void afterGettingFrame(unsigned frameSize,
					 unsigned numTruncatedBytes,
					 struct timeval presentationTime);

private:
	callBack *callBackFunc;
	void *user;
	int m_dwTimeStamp;
	int m_preTimeFlag;
	struct timeval m_preTime;
};
#endif
