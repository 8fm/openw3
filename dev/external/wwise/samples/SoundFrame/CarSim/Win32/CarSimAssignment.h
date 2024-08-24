//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include <AK/SoundFrame/SF.h>

using namespace AK;
using namespace SoundFrame;

class CarSimAssignment
{
public:
	// Default constructor ...
	CarSimAssignment();
	~CarSimAssignment();

	void Clear();
	void Assign( const CarSimAssignment& in_rAssignement );

	void RemoveObject( AkUniqueID in_objectID );

	AkUniqueID m_RPMParam;
	AkUniqueID m_LoadParam;
	AkUniqueID m_SpeedParam;

	AkUniqueID m_EnginePlayEvent;
	AkUniqueID m_EngineStopEvent;
	AkUniqueID m_UpShiftEvent;
	AkUniqueID m_DownShiftEvent;

	AkUniqueID m_AuxBus;

private:
	CarSimAssignment(const CarSimAssignment&);
	CarSimAssignment& operator=( const CarSimAssignment& );
};