//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "SoundInputBase.h"

class SoundInput : public SoundInputBase
{
public:
	SoundInput() {}
	~SoundInput() {}

	static SoundInput& Instance()
	{
		return ms_Instance;
	}

	virtual bool InputOn( unsigned int in_DevNumber = 0 ) { return false;}		// Start input recording
	virtual bool InputOff() { return false; }									// Stop input recording

	virtual void Execute( AkAudioBuffer* io_pBufferOut ) {}
	virtual void GetFormatCallback( AkAudioFormat&  io_AudioFormat ) {}

private:
	static SoundInput ms_Instance;

};
