//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "SoundInputMgr.h"
#include "SoundInput.h"

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
// SoundInputMgr implementation
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
SoundInputMgrBase& SoundInputMgr::Instance()
{
	static SoundInputMgr Singleton;
	return Singleton;
}

bool SoundInputMgr::Initialize()
{
	if (!SoundInputMgrBase::Initialize())
		return false;

	DetectInputDevices();
	return m_MicCount != 0;
}

void SoundInputMgr::Term()
{
	SoundInputMgrBase::Term();
}

// Detects / Register   Input devices
void SoundInputMgr::DetectInputDevices()
{	
	ClearDevices(); // Reset the count.
}