// main.cpp

#include "common.h"

#include "loopbackRecorder.h"

int _cdecl wmain() {
	
    HRESULT hr = S_OK;

    hr = CoInitialize(NULL);
    if (FAILED(hr)) {
        ERR(L"CoInitialize failed: hr = 0x%08x", hr);
        return -__LINE__;
    }

	{
		LoopbackAPI::CoUninitializeOnExit cuoe;

		LoopbackAPI::LoopbackRecorder* loopbackInst = new LoopbackAPI::LoopbackRecorder( (LPCWSTR)"audio.wav", 9 );
		loopbackInst->StartRecording();

		system("pause");

		loopbackInst->StopRecording();

		delete loopbackInst;

		loopbackInst = new LoopbackAPI::LoopbackRecorder( (LPCWSTR)"audio1.wav", 10 );
		loopbackInst->StartRecording();

		system("pause");

		loopbackInst->StopRecording();
	}

    return 0;
}