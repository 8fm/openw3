// loopbackCapture.h

// call CreateThread on this function
// feed it the address of a LoopbackCaptureThreadFunctionArguments
// it will capture via loopback from the IMMDevice
// and dump output to the HMMIO
// until the stop event is set
// any failures will be propagated back via hr

namespace LoopbackAPI {

struct LoopbackCaptureThreadFunctionArguments {
    IMMDevice *pMMDevice;
    bool bInt16;
    HMMIO hFile;
    HANDLE hStartedEvent;
    HANDLE hStopEvent;
    UINT32 nFrames;
    HRESULT hr;
};

DWORD WINAPI LoopbackCaptureThreadFunction(LPVOID pContext);

}