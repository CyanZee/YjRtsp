#ifndef _FILE_SINK_HH
#include "FileSink.hh"
#endif


class YjAudioFileSink : public FileSink{
public:
	typedef void (callBack)(unsigned int type,
					 unsigned char* data, unsigned int size, unsigned long long timeStamp, void* custom);

	YjAudioFileSink(UsageEnvironment& env, callBack* func, void* custom,
					  unsigned int bufferSize);
	static YjAudioFileSink* createNew(UsageEnvironment& env, callBack* func, void* custom,
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

