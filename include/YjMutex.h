#ifndef YJTHREAD_JMUTEX_H
#define YJTHREAD_JMUTEX_H

#include <pthread.h>

#define ERR_YJMUTEX_ALREADYINIT						-1
#define ERR_YJMUTEX_NOTINIT						-2
#define ERR_YJMUTEX_CANTCREATEMUTEX					-3

class YJMutex
{
public:
	YJMutex();
	~YJMutex();
	int Init();
	int Lock();
	int Unlock();
	bool IsInitialized() 						{ return initialized; }
private:
	pthread_mutex_t mutex;
	bool initialized;
};

#endif // JTHREAD_JMUTEX_H

