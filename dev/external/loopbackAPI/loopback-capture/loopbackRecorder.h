#pragma once

#ifdef LOOPBACKDLL_EXPORTS
#define LOOPBACKDLL_API __declspec(dllexport) 
#else
#define LOOPBACKDLL_API __declspec(dllimport) 
#endif

namespace LoopbackAPI {

const int STATUS_NO_DEVICE_HANDLE = 2;
const int STATUS_NO_FILE_HANDLE = 3;

class CPrefs;

struct LoopbackCaptureThreadFunctionArguments;

class LoopbackRecorder
{
	CPrefs*										m_prefs;

	LoopbackCaptureThreadFunctionArguments*		m_threadArgs;
	HANDLE										m_hThread;

	HANDLE										m_hStopEvent;

	bool										m_isRecording;

public:
	LOOPBACKDLL_API LoopbackRecorder( LPCWSTR outputFileName, size_t fileNameSize );
	LOOPBACKDLL_API ~LoopbackRecorder();

	LOOPBACKDLL_API int StartRecording();
	LOOPBACKDLL_API int StopRecording();

	LOOPBACKDLL_API const bool IsRecording() const;
};

}