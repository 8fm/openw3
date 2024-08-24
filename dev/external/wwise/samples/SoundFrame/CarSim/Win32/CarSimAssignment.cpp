//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CarSimAssignment.h"
#include "CarSimHelpers.h"

CarSimAssignment::CarSimAssignment()
{
	Clear();
}

CarSimAssignment::~CarSimAssignment()
{
	Clear();
}

void CarSimAssignment::Clear()
{
	m_RPMParam = AK_INVALID_UNIQUE_ID;
	m_LoadParam = AK_INVALID_UNIQUE_ID;
	m_SpeedParam = AK_INVALID_UNIQUE_ID;
	m_EnginePlayEvent = AK_INVALID_UNIQUE_ID;
	m_EngineStopEvent = AK_INVALID_UNIQUE_ID;
	m_UpShiftEvent = AK_INVALID_UNIQUE_ID;
	m_DownShiftEvent = AK_INVALID_UNIQUE_ID;
	m_AuxBus = AK_INVALID_UNIQUE_ID;
}
	
void CarSimAssignment::Assign( const CarSimAssignment& in_rAssignement )
{
	Clear();

	m_RPMParam = in_rAssignement.m_RPMParam;
	m_LoadParam = in_rAssignement.m_LoadParam;
	m_SpeedParam = in_rAssignement.m_SpeedParam;
	m_EnginePlayEvent = in_rAssignement.m_EnginePlayEvent;
	m_EngineStopEvent = in_rAssignement.m_EngineStopEvent;
	m_UpShiftEvent = in_rAssignement.m_UpShiftEvent;
	m_DownShiftEvent = in_rAssignement.m_DownShiftEvent;
	m_AuxBus = in_rAssignement.m_AuxBus;
}

void CarSimAssignment::RemoveObject( AkUniqueID in_objectID )
{
	if( m_RPMParam == in_objectID )
		m_RPMParam = AK_INVALID_UNIQUE_ID;
	else if( m_LoadParam == in_objectID )
		m_LoadParam = AK_INVALID_UNIQUE_ID;
	else if( m_SpeedParam == in_objectID )
		m_SpeedParam = AK_INVALID_UNIQUE_ID;
	else if( m_EnginePlayEvent == in_objectID )
		m_EnginePlayEvent = AK_INVALID_UNIQUE_ID;
	else if( m_EngineStopEvent == in_objectID )
		m_EngineStopEvent = AK_INVALID_UNIQUE_ID;
	else if( m_UpShiftEvent == in_objectID )
		m_UpShiftEvent = AK_INVALID_UNIQUE_ID;
	else if( m_DownShiftEvent == in_objectID )
		m_DownShiftEvent = AK_INVALID_UNIQUE_ID;
	else if( m_AuxBus == in_objectID )
		m_AuxBus = AK_INVALID_UNIQUE_ID;
}