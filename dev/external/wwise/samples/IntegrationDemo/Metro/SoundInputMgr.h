//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#pragma once

#define MAX_NUMBER_MICROPHONES XUSER_MAX_COUNT

#include "SoundInputMgrBase.h"

// SoundInputMgr definition.
class SoundInputMgr : public SoundInputMgrBase
{
public:
	// Get the static instance.
	static SoundInputMgrBase& Instance();

	virtual bool Initialize();
	virtual void Term();	

	// Hidden constructor set to prevent multiple instances of SoundInputMgr.
private:
	SoundInputMgr() {}								// Private empty constructor
	virtual ~SoundInputMgr() {}
	SoundInputMgr(const SoundInputMgr&);			// Prevent copy-construction
	SoundInputMgr& operator=(const SoundInputMgr&);	// Prevent assignment
	
	void DetectInputDevices();	
};