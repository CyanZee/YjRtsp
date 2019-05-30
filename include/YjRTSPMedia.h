#ifndef _YJRTSP_MEDIA_HH
#define _YJRTSP_MEDIA_HH
#include "liveMedia.hh"
#include "YjH264VideoFileSink.hh"
#include <iostream>
using namespace std;

class qosMeasurementRecord {
public:
  qosMeasurementRecord(struct timeval const& startTime, RTPSource* src)
    : fSource(src), fNext(NULL),
      kbits_per_second_min(1e20), kbits_per_second_max(0),
      kBytesTotal(0.0),
      packet_loss_fraction_min(1.0), packet_loss_fraction_max(0.0),
      totNumPacketsReceived(0), totNumPacketsExpected(0) {
    measurementEndTime = measurementStartTime = startTime;

    RTPReceptionStatsDB::Iterator statsIter(src->receptionStatsDB());
    // Assume that there's only one SSRC source (usually the case):
    RTPReceptionStats* stats = statsIter.next(True);
    if (stats != NULL) {
      kBytesTotal = stats->totNumKBytesReceived();
      totNumPacketsReceived = stats->totNumPacketsReceived();
      totNumPacketsExpected = stats->totNumPacketsExpected();
    }
  }
  virtual ~qosMeasurementRecord() { delete fNext; }

  void periodicQOSMeasurement(struct timeval const& timeNow);

public:
  RTPSource* fSource;
  qosMeasurementRecord* fNext;

public:
  struct timeval measurementStartTime, measurementEndTime;
  double kbits_per_second_min, kbits_per_second_max;
  double kBytesTotal;
  double packet_loss_fraction_min, packet_loss_fraction_max;
  unsigned totNumPacketsReceived, totNumPacketsExpected;
};

class YjRtspMedia
{
public:
		YjRtspMedia();
		~YjRtspMedia();
		void continueAfterClientCreation1();
		
		void setupStreams();
		void closeMediaSinks();
		void shutdown(int exitCode = 1);
		void signalHandlerShutdown(int sig);
		void beginQOSMeasurement();
		void RtspPlay(const char *user, const char *password, const char *url); // forward
		void scheduleNextQOSMeasurement();
		void printQOSData(int exitCode);

		Medium* createClient(UsageEnvironment& env, char const* url, int verbosityLevel, char const* applicationName, portNumBits tunnelOverHTTPPortNum);
		void getOptions(RTSPClient::responseHandler* afterFunc, Authenticator* ourAuthenticator);
		void getSDPDescription(RTSPClient::responseHandler* afterFunc, Authenticator* ourAuthenticator);
		void setupSubsession(MediaSubsession* subsession, Boolean streamUsingTCP, Boolean forceMulticastOnUnspecified, RTSPClient::responseHandler* afterFunc, Authenticator* ourAuthenticator);
		void startPlayingSession(MediaSession* session, double start, double end, float scale, RTSPClient::responseHandler* afterFunc, Authenticator* ourAuthenticator);
		void startPlayingSession(MediaSession* session, char const* absStartTime, char const* absEndTime, float scale, RTSPClient::responseHandler* afterFunc, Authenticator* ourAuthenticator);
		void tearDownSession(MediaSession* session, RTSPClient::responseHandler* afterFunc, Authenticator* ourAuthenticator);
		void setUserAgentString(char const* userAgentString);
		void RtspStop();
		void scheduleLivenessCommand();

		
		char* progName;
		TaskScheduler* scheduler;
		UsageEnvironment* env;
		Medium* ourClient;
		Authenticator* ourAuthenticator;
		char const* streamURL;
		MediaSession* session;
		TaskToken sessionTimerTask;
		TaskToken arrivalCheckTimerTask;
		TaskToken interPacketGapCheckTimerTask;
		TaskToken qosMeasurementTimerTask;
		TaskToken fLivenessCommandTask;
		Boolean createReceivers;
		Boolean outputQuickTimeFile;
		Boolean generateMP4Format;
		QuickTimeFileSink* qtOut;
		Boolean outputAVIFile;
		AVIFileSink* aviOut;
		Boolean audioOnly;
		Boolean videoOnly;
		unsigned int singleMedium;
		int verbosityLevel; // by default, print verbose output
		double duration;
		double durationSlop; // extra seconds to play at the end
		double initialSeekTime;
		char* initialAbsoluteSeekTime;
		float scale;
		double endTime;
		unsigned interPacketGapMaxTime;
		unsigned totNumPacketsReceived; // used if checking inter-packet gaps
		Boolean playContinuously;
		int simpleRTPoffsetArg;
		Boolean sendOptionsRequest;
		Boolean sendOptionsRequestOnly;
		Boolean oneFilePerFrame;
		Boolean notifyOnPacketArrival;
		Boolean streamUsingTCP;
		Boolean forceMulticastOnUnspecified;
		unsigned short desiredPortNum;
		portNumBits tunnelOverHTTPPortNum;
#if 0
		char* username;
		char* password;
#endif
		char* proxyServerName;
		unsigned short proxyServerPortNum;
		unsigned char desiredAudioRTPPayloadFormat;
		char* mimeSubtype;
		unsigned short movieWidth; // default
		Boolean movieWidthOptionSet;
		unsigned short movieHeight; // default
		Boolean movieHeightOptionSet;
		unsigned movieFPS; // default
		Boolean movieFPSOptionSet;
		char const* fileNamePrefix;
		unsigned fileSinkBufferSize;
		unsigned socketInputBufferSize;
		Boolean packetLossCompensate;
		Boolean syncStreams;
		Boolean generateHintTracks;
		Boolean waitForResponseToTEARDOWN;
		unsigned qosMeasurementIntervalMS; // 0 means: Don't output QOS data
		char* userAgent;
		Boolean createHandlerServerForREGISTERCommand;
		portNumBits handlerServerForREGISTERCommandPortNum;
		HandlerServerForREGISTERCommand* handlerServerForREGISTERCommand;
		char* usernameForREGISTER;
		char* passwordForREGISTER;
		UserAuthenticationDatabase* authDBForREGISTER;
		MediaSubsession *subsession;
		Boolean madeProgress;

		unsigned nextQOSMeasurementUSecs;
		qosMeasurementRecord* qosRecordHead;

		Boolean areAlreadyShuttingDown;
		int shutdownExitCode;
		struct timeval startTime;
		char const* clientProtocolName;
		RTSPClient* ourRTSPClient;
		MediaSubsessionIterator* setupIter;	
		char flag;
		Boolean fServerSupportsGetParameter;
		int Audio;
		Boolean g_Transmission;
#if 0//test
		char *array[20];//must delete end;
#endif
	typedef void (callBack)(unsigned int type,
				   unsigned char* data, unsigned int size, unsigned long long timeStamp, void* custom);
	void setCallbackFunction(callBack *function, void *usr);
	void setflag() { flag = ~0; }
	callBack *func;
	void *custom;


};

void continueAfterOPTIONS(RTSPClient* client, int resultCode, char* resultString);
void continueAfterDESCRIBE(RTSPClient* client, int resultCode, char* resultString);
void continueAfterSETUP(RTSPClient* client, int resultCode, char* resultString);
void continueAfterPLAY(RTSPClient* client, int resultCode, char* resultString);
void continueAfterTEARDOWN(RTSPClient* client, int resultCode, char* resultString);
void sessionAfterPlaying(void* custom = NULL);
void subsessionAfterPlaying(void* custom);
void subsessionByeHandler(void* custom);
void sessionTimerHandler(void* clientData);
void periodicQOSMeasurement(void* custom); // forward
void checkForPacketArrival(void* custom);
void checkInterPacketGaps(void* custom);
void continueAfterClientCreation0(RTSPClient* client, Boolean requestStreamingOverTCP);

YjRtspMedia *findMediaFromList(RTSPClient* client);
void addMediaToList(YjRtspMedia *media);
void deleteMediaFromList(YjRtspMedia *media);

void mediaRTSPInit(void);


#endif
