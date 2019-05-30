#include "YjMutex.h"

YJMutex::YJMutex()
{
	initialized = false;
}

YJMutex::~YJMutex()
{
	if (initialized)
		pthread_mutex_destroy(&mutex);
}

int YJMutex::Init()
{
	if (initialized)
		return ERR_YJMUTEX_ALREADYINIT;
	
	pthread_mutex_init(&mutex,NULL);
	initialized = true;
	return 0;	
}

int YJMutex::Lock()
{
	if (!initialized)
		return ERR_YJMUTEX_NOTINIT;
		
	pthread_mutex_lock(&mutex);
	return 0;
}

int YJMutex::Unlock()
{
	if (!initialized)
		return ERR_YJMUTEX_NOTINIT;
	
	pthread_mutex_unlock(&mutex);
	return 0;
}

