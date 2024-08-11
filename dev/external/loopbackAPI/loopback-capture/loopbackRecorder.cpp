#include "common.h"
#include "loopbackRecorder.h"

namespace LoopbackAPI {

LoopbackRecorder::LoopbackRecorder( LPCWSTR outputFileName, size_t fileNameSize )
	: m_hThread( 0 )
	, m_hStopEvent( 0 )
	, m_isRecording( false )
{
	m_prefs = new CPrefs( outputFileName, fileNameSize );

	m_threadArgs = new LoopbackCaptureThreadFunctionArguments();
	// create arguments for loopback capture thread
	m_threadArgs->hr = E_UNEXPECTED; // thread will overwrite this
	m_threadArgs->pMMDevice = m_prefs->m_pMMDevice;
	m_threadArgs->bInt16 = m_prefs->m_bInt16;
	m_threadArgs->hFile = m_prefs->m_hFile;
	m_threadArgs->hStartedEvent = 0;
	m_threadArgs->hStopEvent = 0;
	m_threadArgs->nFrames = 0;
}

LoopbackRecorder::~LoopbackRecorder()
{
	if ( m_hThread && m_hStopEvent )
	{
		StopRecording();
	}
	else if ( m_hThread )
	{
		CloseHandleOnExit closeHandle( m_hThread );
	}
	else if ( m_hStopEvent )
	{
		CloseHandleOnExit closeHandle( m_hStopEvent );
	}

	delete m_threadArgs;
	delete m_prefs;
}

int LoopbackRecorder::StopRecording()
{
	CloseHandleOnExit closeThread(m_hThread);
	CloseHandleOnExit closeStopEvent(m_hStopEvent);

	{
		WaitForSingleObjectOnExit waitForThread(m_hThread);
		SetEventOnExit setStopEvent(m_hStopEvent);
	}

	DWORD exitCode;
	if (!GetExitCodeThread(m_hThread, &exitCode)) {
		ERR(L"GetExitCodeThread failed: last error is %u", GetLastError());
		return -__LINE__;
	}

	if (0 != exitCode) {
		ERR(L"Loopback capture thread exit code is %u; expected 0", exitCode);
		return -__LINE__;
	}

	if (S_OK != m_threadArgs->hr) {
		ERR(L"Thread HRESULT is 0x%08x", m_threadArgs->hr);
		return -__LINE__;
	}

	// everything went well... fixup the fact chunk in the file
	MMRESULT result = mmioClose(m_prefs->m_hFile, 0);
	m_prefs->m_hFile = NULL;
	if (MMSYSERR_NOERROR != result) {
		ERR(L"mmioClose failed: MMSYSERR = %u", result);
		return -__LINE__;
	}

	// reopen the file in read/write mode
	MMIOINFO mi = {0};
	m_prefs->m_hFile = mmioOpen(const_cast<LPWSTR>(m_prefs->m_szFilename), &mi, MMIO_READWRITE);
	if (NULL == m_prefs->m_hFile) {
		ERR(L"mmioOpen(\"%ls\", ...) failed. wErrorRet == %u", m_prefs->m_szFilename, mi.wErrorRet);
		return -__LINE__;
	}

	// descend into the RIFF/WAVE chunk
	MMCKINFO ckRIFF = {0};
	ckRIFF.ckid = MAKEFOURCC('W', 'A', 'V', 'E'); // this is right for mmioDescend
	result = mmioDescend(m_prefs->m_hFile, &ckRIFF, NULL, MMIO_FINDRIFF);
	if (MMSYSERR_NOERROR != result) {
		ERR(L"mmioDescend(\"WAVE\") failed: MMSYSERR = %u", result);
		return -__LINE__;
	}

	// descend into the fact chunk
	MMCKINFO ckFact = {0};
	ckFact.ckid = MAKEFOURCC('f', 'a', 'c', 't');
	result = mmioDescend(m_prefs->m_hFile, &ckFact, &ckRIFF, MMIO_FINDCHUNK);
	if (MMSYSERR_NOERROR != result) {
		ERR(L"mmioDescend(\"fact\") failed: MMSYSERR = %u", result);
		return -__LINE__;
	}

	// write the correct data to the fact chunk
	LONG lBytesWritten = mmioWrite(
		m_prefs->m_hFile,
		reinterpret_cast<PCHAR>(&m_threadArgs->nFrames),
		sizeof(m_threadArgs->nFrames)
		);
	if (lBytesWritten != sizeof(m_threadArgs->nFrames)) {
		ERR(L"Updating the fact chunk wrote %u bytes; expected %u", lBytesWritten, (UINT32)sizeof(m_threadArgs->nFrames));
		return -__LINE__;
	}

	// ascend out of the fact chunk
	result = mmioAscend(m_prefs->m_hFile, &ckFact, 0);
	if (MMSYSERR_NOERROR != result) {
		ERR(L"mmioAscend(\"fact\") failed: MMSYSERR = %u", result);
		return -__LINE__;
	}

	m_hThread = NULL;
	m_hStopEvent = NULL;

	m_isRecording = false;

	return 0;
}

int LoopbackRecorder::StartRecording()
{
	if( m_threadArgs->pMMDevice == NULL )
	{
		return STATUS_NO_DEVICE_HANDLE;
	}
	if ( m_threadArgs->hFile == NULL )
	{
		return STATUS_NO_FILE_HANDLE;
	}

	// create a "loopback capture has started" event
	HANDLE hStartedEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (NULL == hStartedEvent) {
		ERR(L"CreateEvent failed: last error is %u", GetLastError());
		return -__LINE__;
	}
	m_threadArgs->hStartedEvent = hStartedEvent;

	CloseHandleOnExit closeStartedEvent(hStartedEvent);

	// create a "stop capturing now" event
	m_hStopEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (NULL == m_hStopEvent) {
		ERR(L"CreateEvent failed: last error is %u", GetLastError());
		return -__LINE__;
	}
	m_threadArgs->hStopEvent = m_hStopEvent;

	m_hThread = CreateThread(
		NULL, 0,
		LoopbackCaptureThreadFunction, m_threadArgs,
		0, NULL
		);
	if (NULL == m_hThread) {
		ERR(L"CreateThread failed: last error is %u", GetLastError());
		return -__LINE__;
	}

	// wait for either capture to start or the thread to end
	HANDLE waitArray[2] = { hStartedEvent, m_hThread };
	DWORD dwWaitResult;
	dwWaitResult = WaitForMultipleObjects(
		ARRAYSIZE(waitArray), waitArray,
		FALSE, INFINITE
		);

	if (WAIT_OBJECT_0 + 1 == dwWaitResult) {
		ERR(L"Thread aborted before starting to loopback capture: hr = 0x%08x", m_threadArgs->hr);
		return -__LINE__;
	}

	if (WAIT_OBJECT_0 != dwWaitResult) {
		ERR(L"Unexpected WaitForMultipleObjects return value %u", dwWaitResult);
		return -__LINE__;
	}

	m_isRecording = true;

	return S_OK;
}

const bool LoopbackRecorder::IsRecording() const
{
	return m_isRecording;
}

}