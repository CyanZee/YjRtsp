#include "YjRTSPMedia.h"
#include "YjAudioFileSink.hh"
#include "BasicUsageEnvironment.hh"
#include "GroupsockHelper.hh"
#include <pthread.h>
#include "RTSPCommon.hh"
#include "YjMutex.h"
#include "YjOtherFileSink.hh"
#include "YjMpeg4FileSink.hh"
#if defined(__WIN32__) || defined(_WIN32)
#define snprintf _snprintf
#else
#include <signal.h>
#define USE_SIGNALS 1
#endif
#include <list>
//#include <pthread>

#if defined(__WIN32__) || defined(_WIN32)
#pragma comment(lib,"ws2_32.lib")
#endif
using namespace std; 

pthread_mutex_t contiueAfterClientMutex;
pthread_mutex_t contiueAfterOptionMutex;
pthread_mutex_t contiueAfterDescribeMutex;
pthread_mutex_t contiueAfterSetupMutex;
pthread_mutex_t contiueAfterPlayMutex;
pthread_mutex_t contiueAfterTeardownMutex;

pthread_mutex_t subsessionAfterPlayingMutex;
pthread_mutex_t subsessionByHandlerMutex;
pthread_mutex_t sessionAfterPlayingMutex;
pthread_mutex_t sessionTimerHandlerMutex;
pthread_mutex_t periodicQOSMeasurementMutex;
pthread_mutex_t checkForPacketArrivalMutex;
pthread_mutex_t checkInterPacketGapsMutex;
pthread_mutex_t PlayMutex;


pthread_mutex_t mediaListMutex;

list<YjRtspMedia *> mediaList;

YJMutex sendParamMutex;
YJMutex continueAfterGetParmMutex;

void sendLivenessCommand(void* clientData);

#if 0
pthread_mutex_t mediaListMutex;
pthread_mutex_t afterClientCreatMutex;
#endif

#define SINGLE_MEDIUM_VIDEO   0x01
#define SINGLE_MEDIUM_AUDIO   0x01<<1

YjRtspMedia::YjRtspMedia()
{

	progName = NULL;
	scheduler = NULL;
	env = NULL;
	ourClient = NULL;
    ourAuthenticator = NULL;
	streamURL = NULL;
	session = NULL;
	sessionTimerTask = NULL;
	arrivalCheckTimerTask = NULL;
	interPacketGapCheckTimerTask = NULL;
	qosMeasurementTimerTask = NULL;
	createReceivers = True;
	outputQuickTimeFile = False;
	generateMP4Format = False;
	qtOut = NULL;
	outputAVIFile = False;
	aviOut = NULL;
	audioOnly = False;
	videoOnly = False;
	singleMedium = 0;
	verbosityLevel = 1; // by default, print verbose output
	duration = 0;
	durationSlop = -1.0; // extra seconds to play at the end
	initialSeekTime = 0.0f;
	initialAbsoluteSeekTime = NULL;
	scale = 1.0f;
	endTime = .0f;
	interPacketGapMaxTime = 0;
	totNumPacketsReceived = ~0; // used if checking inter-packet gaps
	playContinuously = False;
	simpleRTPoffsetArg = -1;
	sendOptionsRequest = True;
	sendOptionsRequestOnly = False;
	oneFilePerFrame = False;
	notifyOnPacketArrival = False;
	streamUsingTCP = True;	//是否用tcp发送
	forceMulticastOnUnspecified = False;
	desiredPortNum = 0;
	tunnelOverHTTPPortNum = 0;
#if 0
	username = new char[40];
	memset(username, 0, 40);
	password = new char[40];
	memset(password, 0, 40);
#endif
	proxyServerName = NULL;
	proxyServerPortNum = 0;
	desiredAudioRTPPayloadFormat = 0;
	mimeSubtype = NULL;
	movieWidth = 240; // default
	movieWidthOptionSet = False;
	movieHeight = 180; // default
	movieHeightOptionSet = False;
	movieFPS = 15; // default
	movieFPSOptionSet = False;
	fileNamePrefix = NULL;
	fileSinkBufferSize = 100000;
	socketInputBufferSize = 0;
	packetLossCompensate = False;
	syncStreams = False;
	generateHintTracks = False;
	waitForResponseToTEARDOWN = True;
	qosMeasurementIntervalMS = 0; // 0 means: Don't output QOS data
	userAgent = NULL;
	createHandlerServerForREGISTERCommand = False;
	handlerServerForREGISTERCommandPortNum = 0;
	handlerServerForREGISTERCommand = NULL;
	usernameForREGISTER = NULL;
	passwordForREGISTER = NULL;
	authDBForREGISTER = NULL;
	subsession = NULL;
	madeProgress = False;
	nextQOSMeasurementUSecs = 0;
	qosRecordHead = NULL;
	areAlreadyShuttingDown = False;
	shutdownExitCode = -1;
	clientProtocolName = "RTSP";
	ourRTSPClient = NULL;
	func = NULL;
	setupIter = NULL;
	flag = 0;

}
YjRtspMedia::~YjRtspMedia()
{
  if(session){
	  closeMediaSinks();
	  Medium::close(session);
    session = NULL;
	}

  if(ourAuthenticator){
    delete ourAuthenticator;
    ourAuthenticator = NULL;
  }

  if(authDBForREGISTER){
    delete authDBForREGISTER;
    authDBForREGISTER = NULL;
  }

  if(ourClient){
    Medium::close(ourClient);
    ourClient = NULL;
  }

	if(scheduler)
	{
		delete scheduler;
		scheduler = NULL;
	}

	if(env)
	{
		delete env;
		env = NULL;
	}

}

void YjRtspMedia::continueAfterClientCreation1() {
  setUserAgentString(userAgent);

  if (sendOptionsRequest) {
    getOptions(continueAfterOPTIONS, ourAuthenticator);
  } else {
    continueAfterOPTIONS(NULL, 0, NULL);
  }
}


void YjRtspMedia::closeMediaSinks() {
  Medium::close(qtOut);
  Medium::close(aviOut);

  if (session == NULL) return;
  MediaSubsessionIterator iter(*session);
  MediaSubsession* subsession;
  while ((subsession = iter.next()) != NULL) {
    Medium::close(subsession->sink);
    subsession->sink = NULL;
  }
}

void YjRtspMedia::scheduleNextQOSMeasurement() {
  nextQOSMeasurementUSecs += qosMeasurementIntervalMS*1000;
  struct timeval timeNow;
  gettimeofday(&timeNow, NULL);
  unsigned timeNowUSecs = timeNow.tv_sec*1000000 + timeNow.tv_usec;
  int usecsToDelay = nextQOSMeasurementUSecs - timeNowUSecs;

  qosMeasurementTimerTask = env->taskScheduler().scheduleDelayedTask(
     usecsToDelay, (TaskFunc*)periodicQOSMeasurement, (void*)this);
}

void YjRtspMedia::beginQOSMeasurement() {
  // Set up a measurement record for each active subsession:
  struct timeval startTime;
  gettimeofday(&startTime, NULL);
  nextQOSMeasurementUSecs = startTime.tv_sec*1000000 + startTime.tv_usec;
  qosMeasurementRecord* qosRecordTail = NULL;
  MediaSubsessionIterator iter(*session);
  MediaSubsession* subsession;
  while ((subsession = iter.next()) != NULL) {
    RTPSource* src = subsession->rtpSource();
    if (src == NULL) continue;

    qosMeasurementRecord* qosRecord
      = new qosMeasurementRecord(startTime, src);
    if (qosRecordHead == NULL) qosRecordHead = qosRecord;
    if (qosRecordTail != NULL) qosRecordTail->fNext = qosRecord;
    qosRecordTail  = qosRecord;
  }

  // Then schedule the first of the periodic measurements:
  scheduleNextQOSMeasurement();
}

void YjRtspMedia::printQOSData(int exitCode) {
  *env << "begin_QOS_statistics\n";
  
  // Print out stats for each active subsession:
  qosMeasurementRecord* curQOSRecord = qosRecordHead;
  if (session != NULL) {
    MediaSubsessionIterator iter(*session);
    MediaSubsession* subsession;
    while ((subsession = iter.next()) != NULL) {
      RTPSource* src = subsession->rtpSource();
      if (src == NULL) continue;
      
      *env << "subsession\t" << subsession->mediumName()
	   << "/" << subsession->codecName() << "\n";
      
      unsigned numPacketsReceived = 0, numPacketsExpected = 0;
      
      if (curQOSRecord != NULL) {
	numPacketsReceived = curQOSRecord->totNumPacketsReceived;
	numPacketsExpected = curQOSRecord->totNumPacketsExpected;
      }
      *env << "num_packets_received\t" << numPacketsReceived << "\n";
      *env << "num_packets_lost\t" << int(numPacketsExpected - numPacketsReceived) << "\n";
      
      if (curQOSRecord != NULL) {
	unsigned secsDiff = curQOSRecord->measurementEndTime.tv_sec
	  - curQOSRecord->measurementStartTime.tv_sec;
	int usecsDiff = curQOSRecord->measurementEndTime.tv_usec
	  - curQOSRecord->measurementStartTime.tv_usec;
	double measurementTime = secsDiff + usecsDiff/1000000.0;
	*env << "elapsed_measurement_time\t" << measurementTime << "\n";
	
	*env << "kBytes_received_total\t" << curQOSRecord->kBytesTotal << "\n";
	
	*env << "measurement_sampling_interval_ms\t" << qosMeasurementIntervalMS << "\n";
	
	if (curQOSRecord->kbits_per_second_max == 0) {
	  // special case: we didn't receive any data:
	  *env <<
	    "kbits_per_second_min\tunavailable\n"
	    "kbits_per_second_ave\tunavailable\n"
	    "kbits_per_second_max\tunavailable\n";
	} else {
	  *env << "kbits_per_second_min\t" << curQOSRecord->kbits_per_second_min << "\n";
	  *env << "kbits_per_second_ave\t"
	       << (measurementTime == 0.0 ? 0.0 : 8*curQOSRecord->kBytesTotal/measurementTime) << "\n";
	  *env << "kbits_per_second_max\t" << curQOSRecord->kbits_per_second_max << "\n";
	}
	
	*env << "packet_loss_percentage_min\t" << 100*curQOSRecord->packet_loss_fraction_min << "\n";
	double packetLossFraction = numPacketsExpected == 0 ? 1.0
	  : 1.0 - numPacketsReceived/(double)numPacketsExpected;
	if (packetLossFraction < 0.0) packetLossFraction = 0.0;
	*env << "packet_loss_percentage_ave\t" << 100*packetLossFraction << "\n";
	*env << "packet_loss_percentage_max\t"
	     << (packetLossFraction == 1.0 ? 100.0 : 100*curQOSRecord->packet_loss_fraction_max) << "\n";
	
	RTPReceptionStatsDB::Iterator statsIter(src->receptionStatsDB());
	// Assume that there's only one SSRC source (usually the case):
	RTPReceptionStats* stats = statsIter.next(True);
	if (stats != NULL) {
	  *env << "inter_packet_gap_ms_min\t" << stats->minInterPacketGapUS()/1000.0 << "\n";
	  struct timeval totalGaps = stats->totalInterPacketGaps();
	  double totalGapsMS = totalGaps.tv_sec*1000.0 + totalGaps.tv_usec/1000.0;
	  unsigned totNumPacketsReceived = stats->totNumPacketsReceived();
	  *env << "inter_packet_gap_ms_ave\t"
	       << (totNumPacketsReceived == 0 ? 0.0 : totalGapsMS/totNumPacketsReceived) << "\n";
	  *env << "inter_packet_gap_ms_max\t" << stats->maxInterPacketGapUS()/1000.0 << "\n";
	}
	
	curQOSRecord = curQOSRecord->fNext;
      }
    }
  }

  *env << "end_QOS_statistics\n";
  delete qosRecordHead;
}


void YjRtspMedia::shutdown(int exitCode) {
	if (areAlreadyShuttingDown) return; // in case we're called after receiving a RTCP "BYE" while in the middle of a "TEARDOWN".
	areAlreadyShuttingDown = True;
  
	shutdownExitCode = exitCode;
	if (env != NULL) {
	  env->taskScheduler().unscheduleDelayedTask(sessionTimerTask);
	  sessionTimerTask = NULL;
	  env->taskScheduler().unscheduleDelayedTask(arrivalCheckTimerTask);
	  arrivalCheckTimerTask = NULL;
	  env->taskScheduler().unscheduleDelayedTask(interPacketGapCheckTimerTask);
	  interPacketGapCheckTimerTask = NULL;
	  env->taskScheduler().unscheduleDelayedTask(qosMeasurementTimerTask);
	  qosMeasurementTimerTask = NULL;
	  env->taskScheduler().unscheduleDelayedTask(fLivenessCommandTask);
	  fLivenessCommandTask = NULL;
	  ////setflag();
	}
  if(session)
  {
      MediaSubsessionIterator iter(*session);
      MediaSubsession* subsession;
      while ((subsession = iter.next()) != NULL) {
      	if (subsession->rtcpInstance() != NULL) {
      	  subsession->rtcpInstance()->setByeHandler(NULL, NULL); // in case the server sends a RTCP "BYE" while handling "TEARDOWN"
      	}
      }
  }
	if (qosMeasurementIntervalMS > 0) {
	  printQOSData(exitCode);
	}
  
	// Teardown, then shutdown, any outstanding RTP/RTCP subsessions
	Boolean shutdownImmediately = True; // by default
	if (session != NULL) {
	  RTSPClient::responseHandler* responseHandlerForTEARDOWN = NULL; // unless:
	  if (waitForResponseToTEARDOWN) {
		shutdownImmediately = False;
		responseHandlerForTEARDOWN = continueAfterTEARDOWN;
	  }
	  tearDownSession(session, responseHandlerForTEARDOWN, NULL);
	}
  
	if (shutdownImmediately) continueAfterTEARDOWN(ourRTSPClient, 0, NULL);

}
  

Medium* YjRtspMedia::createClient(UsageEnvironment& env, char const* url, int verbosityLevel, char const* applicationName, portNumBits tunnelOverHTTPPortNum) {
  return ourRTSPClient = RTSPClient::createNew(env, url, verbosityLevel, applicationName, tunnelOverHTTPPortNum);
}

void YjRtspMedia::getOptions(RTSPClient::responseHandler* afterFunc, Authenticator* ourAuthenticator){ 
  ourRTSPClient->sendOptionsCommand(afterFunc, ourAuthenticator);
}

void YjRtspMedia::getSDPDescription(RTSPClient::responseHandler* afterFunc, Authenticator* ourAuthenticator) {
  ourRTSPClient->sendDescribeCommand(afterFunc, ourAuthenticator);
}

void YjRtspMedia::setupSubsession(MediaSubsession* subsession, Boolean streamUsingTCP, Boolean forceMulticastOnUnspecified, RTSPClient::responseHandler* afterFunc, Authenticator* ourAuthenticator) {
  
  ourRTSPClient->sendSetupCommand(*subsession, afterFunc, False, streamUsingTCP, forceMulticastOnUnspecified, ourAuthenticator);
}

void YjRtspMedia::startPlayingSession(MediaSession* session, double start, double end, float scale, RTSPClient::responseHandler* afterFunc, Authenticator* ourAuthenticator) {
  ourRTSPClient->sendPlayCommand(*session, afterFunc, start, end, scale, ourAuthenticator);
}

void YjRtspMedia::startPlayingSession(MediaSession* session, char const* absStartTime, char const* absEndTime, float scale, RTSPClient::responseHandler* afterFunc, Authenticator* ourAuthenticator) {
  ourRTSPClient->sendPlayCommand(*session, afterFunc, absStartTime, absEndTime, scale, ourAuthenticator);
}

void YjRtspMedia::tearDownSession(MediaSession* session, RTSPClient::responseHandler* afterFunc, Authenticator* ourAuthenticator) {
  ourRTSPClient->sendTeardownCommand(*session, afterFunc, ourAuthenticator);
}

void YjRtspMedia::setUserAgentString(char const* userAgentString) {
  ourRTSPClient->setUserAgentString(userAgentString);
}

void YjRtspMedia::setupStreams() {
  //static MediaSubsessionIterator* setupIter = NULL;
  if (setupIter == NULL) setupIter = new MediaSubsessionIterator(*session);
  while ((subsession = setupIter->next()) != NULL) {
    // We have another subsession left to set up:
    if (subsession->clientPortNum() == 0) continue; // port # was not set

    setupSubsession(subsession, streamUsingTCP, forceMulticastOnUnspecified, continueAfterSETUP, NULL);
    return;
  }

  // We're done setting up subsessions.
  delete setupIter;
  setupIter = NULL;
  if (!madeProgress){
  	shutdown(0);
	return;
  }

  // Create output files:
  if (createReceivers) {
    if (outputQuickTimeFile) {
      // Create a "QuickTimeFileSink", to write to 'stdout':
      qtOut = QuickTimeFileSink::createNew(*env, *session, "stdout",
					   fileSinkBufferSize,
					   movieWidth, movieHeight,
					   movieFPS,
					   packetLossCompensate,
					   syncStreams,
					   generateHintTracks,
					   generateMP4Format);
      if (qtOut == NULL) {
	*env << "Failed to create QuickTime file sink for stdout: " << env->getResultMsg();
	shutdown(0);
	return;
      }

      qtOut->startPlaying(sessionAfterPlaying, this);
    } else if (outputAVIFile) {
      // Create an "AVIFileSink", to write to 'stdout':
      aviOut = AVIFileSink::createNew(*env, *session, "stdout",
				      fileSinkBufferSize,
				      movieWidth, movieHeight,
				      movieFPS,
				      packetLossCompensate);
      if (aviOut == NULL) {
	*env << "Failed to create AVI file sink for stdout: " << env->getResultMsg();
	shutdown(0);
	return;
      }

      aviOut->startPlaying(sessionAfterPlaying, NULL);
    } else {
      // Create and start "FileSink"s for each subsession:
      madeProgress = False;
      MediaSubsessionIterator iter(*session);
      while ((subsession = iter.next()) != NULL) {
	if (subsession->readSource() == NULL) continue; // was not initiated

	// Create an output file for each desired stream:
	char outFileName[1000];
	if (1/*singleMedium == NULL*/) {
	  // Output file name is
	  //     "<filename-prefix><medium_name>-<codec_name>-<counter>"
	  static unsigned streamCounter = 0;
	  snprintf(outFileName, sizeof outFileName, "%s%s-%s-%d.h264",
		   fileNamePrefix, subsession->mediumName(),
		   subsession->codecName(), ++streamCounter);
	} else {
	  sprintf(outFileName, "stdout");
	}
	FileSink* fileSink;
#if 0
	if (strcmp(subsession->mediumName(), "audio") == 0 &&
	    (strcmp(subsession->codecName(), "AMR") == 0 ||
	     strcmp(subsession->codecName(), "AMR-WB") == 0)) {
	  // For AMR audio streams, we use a special sink that inserts AMR frame hdrs:
	  fileSink = AMRAudioFileSink::createNew(*env, outFileName,
						 fileSinkBufferSize, oneFilePerFrame);
	} else 
#endif
	if (strcmp(subsession->mediumName(), "video") == 0 &&
	    (strcmp(subsession->codecName(), "H264") == 0)) {
	  // For H.264 video stream, we use a special sink that adds 'start codes', and (at the start) the SPS and PPS NAL units:
#if 1
	  fileSink = YjH264VideoFileSink::createNew(*env,
						  subsession->fmtp_spropparametersets(),
						  fileSinkBufferSize, oneFilePerFrame,
						  func, custom);
#else
	  fileSink = H264VideoFileSink::createNew(*env, outFileName,
						subsession->fmtp_spropparametersets(),
						fileSinkBufferSize, oneFilePerFrame);
#endif
	} 
	else if (strcmp(subsession->mediumName(), "video") == 0 &&
	    (strcmp(subsession->codecName(), "MP4V-ES") == 0)) {
	    
		fileSink = YjMpeg4FileSink::createNew(*env, func, custom, fileSinkBufferSize);
	}
	else if(strcmp(subsession->mediumName(), "audio") == 0&&
	    (strcmp(subsession->codecName(), "MPEG4-GENERIC") == 0)){
		fileSink = YjAudioFileSink::createNew(*env, func, custom, fileSinkBufferSize);
	}
	else if(strcmp(subsession->mediumName(), "audio") == 0&&
	    (strcmp(subsession->codecName(), "PCMU") == 0)){
		fileSink = YjAudioFileSink::createNew(*env, func, custom, fileSinkBufferSize);
	}
	else if(strcmp(subsession->mediumName(), "audio") == 0&&
	    (strcmp(subsession->codecName(), "PCMA") == 0)){
		fileSink = YjAudioFileSink::createNew(*env, func, custom, fileSinkBufferSize);
	}
	else {
	  // Normal case:
	  fileSink = YjOtherFileSink::createNew(*env, func, custom, fileSinkBufferSize);
#if 0
	  fileSink = FileSink::createNew(*env, outFileName,
					 fileSinkBufferSize, oneFilePerFrame);
#endif
	}
	subsession->sink = fileSink;
	if (subsession->sink == NULL) {
	  *env << "Failed to create FileSink for \"" << outFileName
		  << "\": " << env->getResultMsg() << "\n";
	} else {
#if 0
	  if (singleMedium == NULL) {
	    *env << "Created output file: \"" << outFileName << "\"\n";
	  } else {
	    *env << "Outputting data from the \"" << subsession->mediumName()
			<< "/" << subsession->codecName()
			<< "\" subsession to 'stdout'\n";
	  }
#endif

	  if (strcmp(subsession->mediumName(), "video") == 0 &&
	      strcmp(subsession->codecName(), "MP4V-ES") == 0 &&
	      subsession->fmtp_config() != NULL) {
	    // For MPEG-4 video RTP streams, the 'config' information
	    // from the SDP description contains useful VOL etc. headers.
	    // Insert this data at the front of the output file:
	    unsigned configLen;
	    unsigned char* configData
	      = parseGeneralConfigStr(subsession->fmtp_config(), configLen);
	    struct timeval timeNow;
	    gettimeofday(&timeNow, NULL);
	    fileSink->addData(configData, configLen, timeNow);
	    delete[] configData;
	  }

	  subsession->sink->startPlaying(*(subsession->readSource()),
					 subsessionAfterPlaying,
					 this);

	  // Also set a handler to be called if a RTCP "BYE" arrives
	  // for this subsession:
	  if (subsession->rtcpInstance() != NULL) {
	    subsession->rtcpInstance()->setByeHandler(subsessionByeHandler, this);
	  }

	  madeProgress = True;
	}
      }
      if (!madeProgress){
		shutdown(0);
		return;
      }
    }
  }

  // Finally, start playing each subsession, to start the data flow:
  if (duration == 0) {
    if (scale > 0) duration = session->playEndTime() - initialSeekTime; // use SDP end time
    else if (scale < 0) duration = initialSeekTime;
  }
  if (duration < 0) duration = 0.0;

  endTime = initialSeekTime;
  if (scale > 0) {
    if (duration <= 0) endTime = -1.0f;
    else endTime = initialSeekTime + duration;
  } else {
    endTime = initialSeekTime - duration;
    if (endTime < 0) endTime = 0.0f;
  }

  char const* absStartTime = initialAbsoluteSeekTime != NULL ? initialAbsoluteSeekTime : session->absStartTime();
  if (absStartTime != NULL) {
    // Either we or the server have specified that seeking should be done by 'absolute' time:
    startPlayingSession(session, absStartTime, session->absEndTime(), scale, continueAfterPLAY, NULL);
  } else {
    // Normal case: Seek by relative time (NPT):
    startPlayingSession(session, initialSeekTime, endTime, scale, continueAfterPLAY, NULL);
  }
}

void YjRtspMedia::signalHandlerShutdown(int /*sig*/) {
	//*env << "Got shutdown signal\n";
	waitForResponseToTEARDOWN = False; // to ensure that we end, even if the server does not respond to our TEARDOWN
	shutdown(0);
}


void YjRtspMedia::setCallbackFunction(callBack *function, void *usr)
{
	func = function;
	custom = usr;
}

void YjRtspMedia::RtspPlay(const char *user, const char *password, const char *url) // forward
{
	scheduler = BasicTaskScheduler::createNew();
	env = BasicUsageEnvironment::createNew(*scheduler);
#if 0
	 int argc = 13;
	for(int i=0; i<argc; i++)
	{
		array[i] = (char *)malloc(40);
	}
	  strcpy(array[0],"yunji");
	  strcpy(array[1],"-u");
	  strcpy(array[2],"admin");
	  strcpy(array[3],"12345");
	  strcpy(array[4],"-f");
	  strcpy(array[5],"20");
	  strcpy(array[6],"-w");
	  strcpy(array[7],"640");
	  strcpy(array[8],"-h");
	  strcpy(array[9],"480");
	  strcpy(array[10],"-b");
	  strcpy(array[11],"400000");
	  strcpy(array[12],"rtsp://192.168.0.12:554");
	  char**argv = (char**)&array[0];
	
	  progName = argv[0];
#endif
	  g_Transmission = True;
	  if(g_Transmission)
	  {
 	  	streamUsingTCP = True;
	  }
	  else
	  {
	  	streamUsingTCP = False;
	  }
	  gettimeofday(&startTime, NULL);
#if 0//def USE_SIGNALS
	  // Allow ourselves to be shut down gracefully by a SIGHUP or a SIGUSR1:
	  signal(SIGHUP, signalHandlerShutdown);
	  signal(SIGUSR1, signalHandlerShutdown);
#endif

	ourAuthenticator = new Authenticator(user, password);
	fileSinkBufferSize = 400000;
	durationSlop = qosMeasurementIntervalMS > 0 ? 0.0 : 5.0;
	streamURL = url;
	//singleMedium = "video";
	singleMedium = SINGLE_MEDIUM_VIDEO;
#if 0
#if 0
  // Create (or arrange to create) our client object:
  if (createHandlerServerForREGISTERCommand) {
    handlerServerForREGISTERCommand
      = HandlerServerForREGISTERCommand::createNew(*env, continueAfterClientCreation0,
						   handlerServerForREGISTERCommandPortNum, authDBForREGISTER,
						   verbosityLevel, progName);
    if (handlerServerForREGISTERCommand == NULL) {
      *env << "Failed to create a server for handling incoming \"REGISTER\" commands: " << env->getResultMsg() << "\n";
    } else {
      *env << "Awaiting an incoming \"REGISTER\" command on port " << handlerServerForREGISTERCommand->serverPortNum() << "\n";
    }
  } else 
  #endif
  {
    ourClient = createClient(*env, streamURL, verbosityLevel, progName, tunnelOverHTTPPortNum);
    if (ourClient == NULL) {
      *env << "Failed to create " << clientProtocolName << " client: " << env->getResultMsg() << "\n";
      shutdown(0);
    }
    continueAfterClientCreation1();
  }
  //pthread_mutex_unlock(&PlayMutex);
  #endif
   	if (createHandlerServerForREGISTERCommand)
   	{
    	handlerServerForREGISTERCommand
      	= HandlerServerForREGISTERCommand::createNew(*env, continueAfterClientCreation0,
						   handlerServerForREGISTERCommandPortNum, authDBForREGISTER,
						   verbosityLevel, progName);
    	if (handlerServerForREGISTERCommand == NULL) 
		{
      		*env << "Failed to create a server for handling incoming \"REGISTER\" commands: " << env->getResultMsg() << "\n";
    	} 
		else 
		{
      		*env << "Awaiting an incoming \"REGISTER\" command on port " << handlerServerForREGISTERCommand->serverPortNum() << "\n";
    	}
  	} 
	else
	{
    	ourClient = createClient(*env, streamURL, verbosityLevel, progName, tunnelOverHTTPPortNum);
    	if (ourClient == NULL) 
		{
      		*env << "Failed to create " << clientProtocolName << " client: " << env->getResultMsg() << "\n";
      		shutdown(0);
    	}
    	continueAfterClientCreation1();
  }

  // All subsequent activity takes place within the event loop:
  env->taskScheduler().doEventLoop(&flag); // does not return

}

void YjRtspMedia::RtspStop() // forward
{
	//pthread_mutex_lock(&PlayMutex);
	shutdown(0);
	//pthread_mutex_unlock(&PlayMutex);
}

void YjRtspMedia::scheduleLivenessCommand() {
  // Delay a random time before sending another 'liveness' command.
  unsigned delayMax = ourRTSPClient->sessionTimeoutParameter(); // if the server specified a maximum time between 'liveness' probes, then use that
  if (delayMax == 0) {
    delayMax = 60;
  }

  // Choose a random time from [delayMax/2,delayMax-1) seconds:
  unsigned const us_1stPart = delayMax*500000;
  unsigned uSecondsToDelay;
  if (us_1stPart <= 1000000) {
    uSecondsToDelay = us_1stPart;
  } else {
    unsigned const us_2ndPart = us_1stPart-1000000;
    uSecondsToDelay = us_1stPart + (us_2ndPart*our_random())%us_2ndPart;
  }
  fLivenessCommandTask = env->taskScheduler().scheduleDelayedTask(uSecondsToDelay, sendLivenessCommand, this);
}

void qosMeasurementRecord
::periodicQOSMeasurement(struct timeval const& timeNow) {
  unsigned secsDiff = timeNow.tv_sec - measurementEndTime.tv_sec;
  int usecsDiff = timeNow.tv_usec - measurementEndTime.tv_usec;
  double timeDiff = secsDiff + usecsDiff/1000000.0;
  measurementEndTime = timeNow;

  RTPReceptionStatsDB::Iterator statsIter(fSource->receptionStatsDB());
  // Assume that there's only one SSRC source (usually the case):
  RTPReceptionStats* stats = statsIter.next(True);
  if (stats != NULL) {
    double kBytesTotalNow = stats->totNumKBytesReceived();
    double kBytesDeltaNow = kBytesTotalNow - kBytesTotal;
    kBytesTotal = kBytesTotalNow;

    double kbpsNow = timeDiff == 0.0 ? 0.0 : 8*kBytesDeltaNow/timeDiff;
    if (kbpsNow < 0.0) kbpsNow = 0.0; // in case of roundoff error
    if (kbpsNow < kbits_per_second_min) kbits_per_second_min = kbpsNow;
    if (kbpsNow > kbits_per_second_max) kbits_per_second_max = kbpsNow;

    unsigned totReceivedNow = stats->totNumPacketsReceived();
    unsigned totExpectedNow = stats->totNumPacketsExpected();
    unsigned deltaReceivedNow = totReceivedNow - totNumPacketsReceived;
    unsigned deltaExpectedNow = totExpectedNow - totNumPacketsExpected;
    totNumPacketsReceived = totReceivedNow;
    totNumPacketsExpected = totExpectedNow;

    double lossFractionNow = deltaExpectedNow == 0 ? 0.0
      : 1.0 - deltaReceivedNow/(double)deltaExpectedNow;
    //if (lossFractionNow < 0.0) lossFractionNow = 0.0; //reordering can cause
    if (lossFractionNow < packet_loss_fraction_min) {
      packet_loss_fraction_min = lossFractionNow;
    }
    if (lossFractionNow > packet_loss_fraction_max) {
      packet_loss_fraction_max = lossFractionNow;
    }
  }
}

void continueAfterClientCreation0(RTSPClient* newRTSPClient, Boolean requestStreamingOverTCP){
	pthread_mutex_lock(&contiueAfterClientMutex);
	if (newRTSPClient == NULL)
	{
		pthread_mutex_unlock(&contiueAfterClientMutex);
		return;
	}
	YjRtspMedia *media = findMediaFromList(newRTSPClient);
	if(media == NULL)
	{
		pthread_mutex_unlock(&contiueAfterClientMutex);
		return;
	}
	
	media->streamUsingTCP = requestStreamingOverTCP;

	//assignClient(media->ourClient = newRTSPClient);
	media->streamURL = newRTSPClient->url();

	// Having handled one "REGISTER" command (giving us a "rtsp://" URL to stream from), we don't handle any more:
	Medium::close(media->handlerServerForREGISTERCommand); media->handlerServerForREGISTERCommand = NULL;

	media->continueAfterClientCreation1();
	pthread_mutex_unlock(&contiueAfterClientMutex);

}

void continueAfterOPTIONS(RTSPClient* client, int resultCode, char* resultString){
	pthread_mutex_lock(&contiueAfterOptionMutex);
	YjRtspMedia *media = findMediaFromList(client);
	if(media == NULL)
	{
		pthread_mutex_unlock(&contiueAfterOptionMutex);
		return;
	}

	if (media->sendOptionsRequestOnly) {
		if (resultCode != 0) {
			*(media->env) << media->clientProtocolName << " \"OPTIONS\" request failed: " << resultString << "\n";
			char error[256];
			sprintf(error, "OPTIONS request failed:%s\n", resultString);
			//YJSIP_ErrorMsgInfo(media->m_key.c_str(), error, VIGP_VFS_RTSP_OPTION_ERROR, ENUM_ERR_INVITE);
		} else {
			*(media->env) << media->clientProtocolName << " \"OPTIONS\" request returned: " << resultString << "\n";
		}
		media->shutdown(0);
		pthread_mutex_unlock(&contiueAfterOptionMutex);
		return;
	}
	if (resultCode == 0) {
    // Note whether the server told us that it supports the "GET_PARAMETER" command:
	    media->fServerSupportsGetParameter = RTSPOptionIsSupported("GET_PARAMETER", resultString);
	}

	delete[] resultString;

	// Next, get a SDP description for the stream:
	media->getSDPDescription(continueAfterDESCRIBE, NULL);
	pthread_mutex_unlock(&contiueAfterOptionMutex);
}

void continueAfterDESCRIBE(RTSPClient* client, int resultCode, char* resultString){
	//pthread_mutex_lock(&contiueAfterDescribeMutex);
	YjRtspMedia *media = findMediaFromList(client);
	if(media == NULL)
	{
		//pthread_mutex_unlock(&contiueAfterOptionMutex);
		return;
	}

	if (resultCode != 0) {
		if(401 == resultCode)
		{
			//YJSIP_ErrorMsgInfo(media->m_key.c_str(), "用户名密码错误", VIGP_VFS_PASSWORD_ERROR, ENUM_ERR_INVITE);
		}
		else if(400 == resultCode)
		{
			//YJSIP_ErrorMsgInfo(media->m_key.c_str(), "连接设备失败", VIGP_VFS_CONNECT_ERROR, ENUM_ERR_INVITE);
		}
		else
		{
			//YJSIP_ErrorMsgInfo(media->m_key.c_str(), "连接设备失败", VIGP_VFS_CONNECT_ERROR, ENUM_ERR_INVITE);
			
		}
		*(media->env) << "Failed to get a SDP description for the URL \"" << media->streamURL << "\": " << resultString << "\n";
		delete[] resultString;
		media->shutdown(0);
		//pthread_mutex_unlock(&contiueAfterDescribeMutex);
		return;
	}

	char* sdpDescription = resultString;
	//*(media->env) << "Opened URL \"" << media->streamURL << "\", returning a SDP description:\n" << sdpDescription << "\n";

	// Create a media session object from this SDP description:
	media->session = MediaSession::createNew(*(media->env), sdpDescription);
	delete[] sdpDescription;
	if (media->session == NULL) {
		*(media->env) << "Failed to create a MediaSession object from the SDP description: " << media->env->getResultMsg() << "\n";
		media->shutdown(0);
		//pthread_mutex_unlock(&contiueAfterDescribeMutex);
		return;
	} else if (!media->session->hasSubsessions()) {
		*(media->env) << "This session has no media subsessions (i.e., no \"m=\" lines)\n";
		media->shutdown(0);
		//pthread_mutex_unlock(&contiueAfterDescribeMutex);
		return;
	}

	// Then, setup the "RTPSource"s for the session:
	MediaSubsessionIterator iter(*media->session);
	MediaSubsession *subsession;
	Boolean madeProgress = False;
//	char const* singleMediumToTest = media->singleMedium;
	while ((subsession = iter.next()) != NULL)
	{
		// If we've asked to receive only a single medium, then check this now:
		//if (singleMediumToTest != NULL) {
		if ((strcmp(subsession->mediumName(), "video") == 0 && (media->singleMedium & SINGLE_MEDIUM_VIDEO))
			||(strcmp(subsession->mediumName(), "audio") == 0 && (media->singleMedium & (SINGLE_MEDIUM_AUDIO)))) {
			
		} else {
		// Receive this subsession only
		//	singleMediumToTest = "xxxxx";
		// this hack ensures that we get only 1 subsession of this type
		//*(media->env) << "Ignoring \"" << subsession->mediumName()
		//<< "/" << subsession->codecName()
		//<< "\" subsession, because we've asked to receive a single " << media->singleMedium
		//<< " session only\n";
		continue;
		}
//		}

		if (media->desiredPortNum != 0) {
		subsession->setClientPortNum(media->desiredPortNum);
		media->desiredPortNum += 2;
		}

		if (media->createReceivers) 
		{
			if (!subsession->initiate(media->simpleRTPoffsetArg)) {
				*(media->env) << "Unable to create receiver for \"" << subsession->mediumName()
				<< "/" << subsession->codecName()
				<< "\" subsession: " << media->env->getResultMsg() << "\n";
			} else {
				//*(media->env) << "Created receiver for \"" << subsession->mediumName()
				//<< "/" << subsession->codecName()
				//<< "\" subsession (client ports " << subsession->clientPortNum()
				//<< "-" << subsession->clientPortNum()+1 << ")\n";
				madeProgress = True;

				if (subsession->rtpSource() != NULL) {
					// Because we're saving the incoming data, rather than playing
					// it in real time, allow an especially large time threshold
					// (1 second) for reordering misordered incoming packets:
					unsigned const thresh = 1000000; // 1 second
					subsession->rtpSource()->setPacketReorderingThresholdTime(thresh);

					// Set the RTP source's OS socket buffer size as appropriate - either if we were explicitly asked (using -B),
					// or if the desired FileSink buffer size happens to be larger than the current OS socket buffer size.
					// (The latter case is a heuristic, on the assumption that if the user asked for a large FileSink buffer size,
					// then the input data rate may be large enough to justify increasing the OS socket buffer size also.)
					int socketNum = subsession->rtpSource()->RTPgs()->socketNum();
					unsigned curBufferSize = getReceiveBufferSize(*(media->env), socketNum);
					if (media->socketInputBufferSize > 0 || media->fileSinkBufferSize > curBufferSize) {
						unsigned newBufferSize = media->socketInputBufferSize > 0 ? media->socketInputBufferSize : media->fileSinkBufferSize;
						newBufferSize = setReceiveBufferTo(*(media->env), socketNum, newBufferSize);
						if (media->socketInputBufferSize > 0) { // The user explicitly asked for the new socket buffer size; announce it:
							//*(media->env) << "Changed socket receive buffer size for the \""
							//	<< subsession->mediumName()
							//	<< "/" << subsession->codecName()
							//	<< "\" subsession from "
							//	<< curBufferSize << " to "
							//	<< newBufferSize << " bytes\n";
						}
					}
				}
			}
		} 
		else 
		{
			if (subsession->clientPortNum() == 0) {
				*(media->env) << "No client port was specified for the \""
					<< subsession->mediumName()
					<< "/" << subsession->codecName()
					<< "\" subsession.  (Try adding the \"-p <portNum>\" option.)\n";
			} else {
				madeProgress = True;
			}
		}
	}
	if (!madeProgress){
		media->shutdown(0);
		//pthread_mutex_unlock(&contiueAfterDescribeMutex);
		return;
	}

	*(media->env) << "continueAfterDESCRIBE444444444\n";
	// Perform additional 'setup' on each subsession, before playing them:
	media->setupStreams();
	//pthread_mutex_unlock(&contiueAfterDescribeMutex);
}

void continueAfterSETUP(RTSPClient* client, int resultCode, char* resultString) {
	pthread_mutex_lock(&contiueAfterSetupMutex);
	YjRtspMedia *media = findMediaFromList(client);
	if(media == NULL)
	{
		pthread_mutex_unlock(&contiueAfterSetupMutex);
		return;
	}
	
	if (resultCode == 0) {
#if 0
		*(media->env) << "Setup \"" << media->subsession->mediumName()
		<< "/" << media->subsession->codecName()
		<< "\" subsession (client ports " << media->subsession->clientPortNum()
		<< "-" << media->subsession->clientPortNum()+1 << ")\n";
#endif
		media->madeProgress = True;
	} else {
		*(media->env) << "Failed to setup \"" << media->subsession->mediumName()
		<< "/" << media->subsession->codecName()
		<< "\" subsession: " << resultString << "\n";
		char error[256];
		sprintf(error, "Failed to setup:%s\n", resultString);
		//YJSIP_ErrorMsgInfo(media->m_key.c_str(), "rtsp setup失败", VIGP_VFS_RTSP_SETUP_ERROR, ENUM_ERR_INVITE);
	}
	delete[] resultString;

	// Set up the next subsession, if any:
	media->setupStreams();
	pthread_mutex_unlock(&contiueAfterSetupMutex);
}




void continueAfterPLAY(RTSPClient* client, int resultCode, char* resultString) {
	pthread_mutex_lock(&contiueAfterPlayMutex);
	YjRtspMedia *media = findMediaFromList(client);
	if(media == NULL)
	{
		pthread_mutex_unlock(&contiueAfterPlayMutex);
		return;
	}
	
	if (resultCode != 0) {
		*(media->env) << "Failed to start playing session: " << resultString << "\n";
		delete[] resultString;
		media->shutdown(0);
		pthread_mutex_unlock(&contiueAfterPlayMutex);
		return;
	}
	delete[] resultString;

	if (media->qosMeasurementIntervalMS > 0) {
		// Begin periodic QOS measurements:
		media->beginQOSMeasurement();
	}

	// Figure out how long to delay (if at all) before shutting down, or
	// repeating the playing
	Boolean timerIsBeingUsed = False;
	double secondsToDelay = media->duration;
	if (media->duration > 0) {
		// First, adjust "duration" based on any change to the play range (that was specified in the "PLAY" response):
		double rangeAdjustment = (media->session->playEndTime() - media->session->playStartTime()) - (media->endTime - media->initialSeekTime);
		if (media->duration + rangeAdjustment > 0.0) media->duration += rangeAdjustment;

		timerIsBeingUsed = True;
		double absScale = media->scale > 0 ? media->scale : -media->scale; // ASSERT: scale != 0
		secondsToDelay = media->duration/absScale + media->durationSlop;

		int64_t uSecsToDelay = (int64_t)(secondsToDelay*1000000.0);
		media->sessionTimerTask = (media->env)->taskScheduler().scheduleDelayedTask(uSecsToDelay, (TaskFunc*)sessionTimerHandler, (void*)media);
	}

	char const* actionString
		= media->createReceivers? "Receiving streamed data":"Data is being streamed";
	if (timerIsBeingUsed) {
		*(media->env) << actionString
			<< " (for up to " << secondsToDelay
			<< " seconds)...\n";
	} else {
#ifdef USE_SIGNALS
		pid_t ourPid = getpid();
		*(media->env) << actionString
			<< " (signal with \"kill -HUP " << (int)ourPid
			<< "\" or \"kill -USR1 " << (int)ourPid
			<< "\" to terminate)...\n";
#else
	*(media->env) << actionString << "...\n";
#endif
  }

	// Watch for incoming packets (if desired):
	media->scheduleLivenessCommand();
	checkForPacketArrival(media);
	checkInterPacketGaps(media);
	pthread_mutex_unlock(&contiueAfterPlayMutex);
}

void continueAfterTEARDOWN(RTSPClient* client, int /*resultCode*/, char* resultString) {
	pthread_mutex_lock(&contiueAfterTeardownMutex);
//	cout<<"continueAfterTEARDOWN start"<<endl;
	YjRtspMedia *media = findMediaFromList(client);
	delete[] resultString;
  
	// Now that we've stopped any more incoming data from arriving, close our output files:
	if(!media)
	{
		pthread_mutex_unlock(&contiueAfterTeardownMutex);
		return;
	}
	if(media->session){
	  media->closeMediaSinks();
	  Medium::close(media->session);
	  media->session = NULL;
	}
	
	  // Finally, shut down our client:
	  if(media->ourAuthenticator){
	  delete media->ourAuthenticator;
	  media->ourAuthenticator = NULL;
	}
	
	if(media->authDBForREGISTER){
	  delete media->authDBForREGISTER;
	  media->authDBForREGISTER = NULL;
	}
	
	if(media->ourClient){
	  Medium::close(media->ourClient);
	  media->ourClient = NULL;
	}

//	cout<<"continueAfterTEARDOWN end"<<endl;
	pthread_mutex_unlock(&contiueAfterTeardownMutex);
}

void subsessionAfterPlaying(void* custom) {
	pthread_mutex_lock(&subsessionAfterPlayingMutex);
	YjRtspMedia *p = (YjRtspMedia *)custom;
	// Begin by closing this media subsession's stream:
	MediaSubsessionIterator tmpIter(*p->session);
	tmpIter.reset();
	MediaSubsession* subsession = tmpIter.next();
	Medium::close(subsession->sink);
	subsession->sink = NULL;

	// Next, check whether *all* subsessions' streams have now been closed:
	MediaSession& session = subsession->parentSession();
	MediaSubsessionIterator iter(session);
	while ((subsession = iter.next()) != NULL) {
		if (subsession->sink != NULL){
			pthread_mutex_unlock(&subsessionAfterPlayingMutex);			
			return; // this subsession is still active
		}
	}

	// All subsessions' streams have now been closed
	sessionAfterPlaying(p);
	pthread_mutex_unlock(&subsessionAfterPlayingMutex); 		
}

void subsessionByeHandler(void* custom) {
	pthread_mutex_lock(&subsessionByHandlerMutex);
	YjRtspMedia *p = (YjRtspMedia *)custom;
	struct timeval timeNow;
	gettimeofday(&timeNow, NULL);
	////unsigned secsDiff = timeNow.tv_sec - p->startTime.tv_sec;

	MediaSubsessionIterator iter(*p->session);
	iter.reset();
	////MediaSubsession* subsession = iter.next();
#if 0
	*(p->env) << "Received RTCP \"BYE\" on \"" << subsession->mediumName()
		<< "/" << subsession->codecName()
		<< "\" subsession (after " << secsDiff
		<< " seconds)\n";
#endif
	// Act now as if the subsession had closed:
	subsessionAfterPlaying(p);
	pthread_mutex_unlock(&subsessionByHandlerMutex); 		
}

void sessionAfterPlaying(void* custom) {
	pthread_mutex_lock(&sessionAfterPlayingMutex);
	YjRtspMedia *p = (YjRtspMedia *)custom;
	if (!p->playContinuously) {
		p->shutdown(0);
	} else {
	// We've been asked to play the stream(s) over again.
	// First, reset state from the current session:
		if (p->env != NULL) {
			p->env->taskScheduler().unscheduleDelayedTask(p->sessionTimerTask);
			p->sessionTimerTask = NULL;
			p->env->taskScheduler().unscheduleDelayedTask(p->arrivalCheckTimerTask);
			p->arrivalCheckTimerTask = NULL;
			p->env->taskScheduler().unscheduleDelayedTask(p->interPacketGapCheckTimerTask);
			p->interPacketGapCheckTimerTask = NULL;
			p->env->taskScheduler().unscheduleDelayedTask(p->qosMeasurementTimerTask);
			p->qosMeasurementTimerTask = NULL;
		}
		p->totNumPacketsReceived = ~0;

		p->startPlayingSession(p->session, p->initialSeekTime, p->endTime, p->scale, continueAfterPLAY, NULL);
	}
	pthread_mutex_unlock(&sessionAfterPlayingMutex); 		
}

void sessionTimerHandler(void* custom) {
	pthread_mutex_lock(&sessionTimerHandlerMutex);
	YjRtspMedia *p = (YjRtspMedia *)custom;
	p->sessionTimerTask = NULL;

	sessionAfterPlaying(p);
	pthread_mutex_unlock(&sessionTimerHandlerMutex); 		
}

void periodicQOSMeasurement(void* custom) {
	pthread_mutex_lock(&periodicQOSMeasurementMutex);
	YjRtspMedia *p = (YjRtspMedia *)custom;

	struct timeval timeNow;
	gettimeofday(&timeNow, NULL);

	for (qosMeasurementRecord* qosRecord = p->qosRecordHead;
		qosRecord != NULL; qosRecord = qosRecord->fNext) {
			qosRecord->periodicQOSMeasurement(timeNow);
	}

  // Do this again later:
  p->scheduleNextQOSMeasurement();
  pthread_mutex_unlock(&periodicQOSMeasurementMutex);
}  

void checkForPacketArrival(void* custom) {
	pthread_mutex_lock(&checkForPacketArrivalMutex);
	YjRtspMedia *p = (YjRtspMedia *)custom;
	if (!p->notifyOnPacketArrival){
		pthread_mutex_unlock(&checkForPacketArrivalMutex);
		return; // we're not checking
	}
  
	// Check each subsession, to see whether it has received data packets:
	unsigned numSubsessionsChecked = 0;
	unsigned numSubsessionsWithReceivedData = 0;
	unsigned numSubsessionsThatHaveBeenSynced = 0;
  
	MediaSubsessionIterator iter(*p->session);
	MediaSubsession* subsession;
	while ((subsession = iter.next()) != NULL) {
	  RTPSource* src = subsession->rtpSource();
	  if (src == NULL) continue;
	  ++numSubsessionsChecked;
  
	  if (src->receptionStatsDB().numActiveSourcesSinceLastReset() > 0) {
		// At least one data packet has arrived
		++numSubsessionsWithReceivedData;
	  }
	  if (src->hasBeenSynchronizedUsingRTCP()) {
		++numSubsessionsThatHaveBeenSynced;
	  }
	}
  
	unsigned numSubsessionsToCheck = numSubsessionsChecked;
	// Special case for "QuickTimeFileSink"s and "AVIFileSink"s:
	// They might not use all of the input sources:
	if (p->qtOut != NULL) {
	  numSubsessionsToCheck = p->qtOut->numActiveSubsessions();
	} else if (p->aviOut != NULL) {
	  numSubsessionsToCheck = p->aviOut->numActiveSubsessions();
	}
  
	Boolean notifyTheUser;
	if (!p->syncStreams) {
	  notifyTheUser = numSubsessionsWithReceivedData > 0; // easy case
	} else {
	  notifyTheUser = numSubsessionsWithReceivedData >= numSubsessionsToCheck
		&& numSubsessionsThatHaveBeenSynced == numSubsessionsChecked;
	  // Note: A subsession with no active sources is considered to be synced
	}
	if (notifyTheUser) {
	  struct timeval timeNow;
	  gettimeofday(&timeNow, NULL);
	  char timestampStr[100];
	  sprintf(timestampStr, "%ld%03ld", timeNow.tv_sec, (long)(timeNow.tv_usec/1000));
	  *(p->env) << (p->syncStreams ? "Synchronized d" : "D")
		  << "ata packets have begun arriving [" << timestampStr << "]\007\n";
	  pthread_mutex_unlock(&checkForPacketArrivalMutex);
	  return;
	}
  
	// No luck, so reschedule this check again, after a delay:
	int uSecsToDelay = 100000; // 100 ms
	p->arrivalCheckTimerTask
	  = p->env->taskScheduler().scheduleDelayedTask(uSecsToDelay,
					 (TaskFunc*)checkForPacketArrival, p);
	pthread_mutex_unlock(&checkForPacketArrivalMutex);
}
  
void checkInterPacketGaps(void* custom) {
	pthread_mutex_lock(&checkInterPacketGapsMutex);
	YjRtspMedia *p = (YjRtspMedia *)custom;
	if (p->interPacketGapMaxTime == 0){
		pthread_mutex_unlock(&checkInterPacketGapsMutex);
		return; // we're not checking
	}
  
	// Check each subsession, counting up how many packets have been received:
	unsigned newTotNumPacketsReceived = 0;
  
	MediaSubsessionIterator iter(*(p->session));
	MediaSubsession* subsession;
	while ((subsession = iter.next()) != NULL) {
	  RTPSource* src = subsession->rtpSource();
	  if (src == NULL) continue;
	  newTotNumPacketsReceived += src->receptionStatsDB().totNumPacketsReceived();
	}
  
	if (newTotNumPacketsReceived == p->totNumPacketsReceived) {
	  // No additional packets have been received since the last time we
	  // checked, so end this stream:
	  *(p->env) << "Closing session, because we stopped receiving packets.\n";
	  p->interPacketGapCheckTimerTask = NULL;
	  sessionAfterPlaying(custom);
	} else {
	  p->totNumPacketsReceived = newTotNumPacketsReceived;
	  // Check again, after the specified delay:
	  p->interPacketGapCheckTimerTask
		= (p->env)->taskScheduler().scheduleDelayedTask(p->interPacketGapMaxTime*1000000,
				   (TaskFunc*)checkInterPacketGaps, p);
	}
	pthread_mutex_unlock(&checkInterPacketGapsMutex);
}

void mediaRTSPInit(void)
{
#if 1
	pthread_mutex_init(&mediaListMutex, NULL);
	pthread_mutex_init(&contiueAfterClientMutex, NULL);
	pthread_mutex_init(&contiueAfterOptionMutex, NULL);
	pthread_mutex_init(&contiueAfterDescribeMutex, NULL);
	pthread_mutex_init(&contiueAfterSetupMutex, NULL);
	pthread_mutex_init(&contiueAfterPlayMutex, NULL);
	pthread_mutex_init(&contiueAfterTeardownMutex, NULL);

	pthread_mutex_init(&subsessionAfterPlayingMutex, NULL);
	pthread_mutex_init(&subsessionByHandlerMutex, NULL);
	pthread_mutex_init(&sessionAfterPlayingMutex, NULL);
	pthread_mutex_init(&sessionAfterPlayingMutex, NULL);
	pthread_mutex_init(&periodicQOSMeasurementMutex, NULL);
	pthread_mutex_init(&checkForPacketArrivalMutex, NULL);
	pthread_mutex_init(&checkInterPacketGapsMutex, NULL);
	pthread_mutex_init(&PlayMutex, NULL);
	
	sendParamMutex.Init();
	continueAfterGetParmMutex.Init();
//	pthread_mutex_init(&afterClientCreatMutex, NULL);
#endif

}
void addMediaToList(YjRtspMedia *media)
{
	pthread_mutex_lock(&mediaListMutex);
	mediaList.push_back(media);
	pthread_mutex_unlock(&mediaListMutex);
}

void deleteMediaFromList(YjRtspMedia *media)
{
	pthread_mutex_lock(&mediaListMutex);
	list<YjRtspMedia *>::iterator iter;
	for(iter = mediaList.begin(); iter != mediaList.end(); iter++)
	{
		if(*iter == media)
		{
			mediaList.erase(iter);
			break;
		}
	}
	pthread_mutex_unlock(&mediaListMutex);
}

YjRtspMedia *findMediaFromList(RTSPClient* client)
{
	pthread_mutex_lock(&mediaListMutex);
	list<YjRtspMedia *>::iterator iter;
	YjRtspMedia *ret = NULL;
	YjRtspMedia *media = NULL;
	for(iter = mediaList.begin(); iter != mediaList.end(); iter++)
	{
		media = *iter;
		if(media->ourClient == client)
		{
			ret = *iter;
			break;
		}
	}
	pthread_mutex_unlock(&mediaListMutex);
	return ret;

}

void continueAfterGET_PARAMETER(RTSPClient* rtspClient, int resultCode, char* resultString) {
  continueAfterGetParmMutex.Lock();

  YjRtspMedia *media = findMediaFromList(rtspClient);
  if(media == NULL)
  {
  	continueAfterGetParmMutex.Unlock();
	return;
  }
  if (resultCode != 0) {
    media->fServerSupportsGetParameter = False; // until we learn otherwise, in response to a future "OPTIONS" command
	continueAfterGetParmMutex.Unlock();
	return;
  }
  
  media->scheduleLivenessCommand();
  delete[] resultString;

  continueAfterGetParmMutex.Unlock();
}

void sendLivenessCommand(void* clientData) {
  sendParamMutex.Lock();
  YjRtspMedia* p = (YjRtspMedia*)clientData;
  RTSPClient* rtspClient = p->ourRTSPClient;

  // Note.  By default, we do not send "GET_PARAMETER" as our 'liveness notification' command, even if the server previously
  // indicated (in its response to our earlier "OPTIONS" command) that it supported "GET_PARAMETER".  This is because
  // "GET_PARAMETER" crashes some camera servers (even though they claimed to support "GET_PARAMETER").
  MediaSession* sess = p->session;

  if (p->fServerSupportsGetParameter && p->session) {
    rtspClient->sendGetParameterCommand(*sess, continueAfterGET_PARAMETER, "", NULL);
  }
  sendParamMutex.Unlock();
}

