/***********************************************************************
  The content of this file includes source code for the sound engine
  portion of the AUDIOKINETIC Wwise Technology and constitutes "Level
  Two Source Code" as defined in the Source Code Addendum attached
  with this file.  Any use of the Level Two Source Code shall be
  subject to the terms and conditions outlined in the Source Code
  Addendum and the End User License Agreement for Wwise(R).

  Version: v2013.2.9  Build: 4872
  Copyright (c) 2006-2014 Audiokinetic Inc.
 ***********************************************************************/

#include "stdafx.h"
#include "SFLibObjects.h"

#include "SFBytesMem.h"

#include <crtdbg.h>
#include <guiddef.h>
#include <objidl.h>

// serializer.

using namespace SoundFrame;

// -----------------------------------------------------------------------------

SFSoundObject::SFSoundObject()
	: m_bHasAttenuation( false )
	, m_dblAttenuationMaxDistance( 0.0 )
{
}

SFSoundObject::~SFSoundObject()
{
}

bool SFSoundObject::HasAttenuation() const
{
	return m_bHasAttenuation;
}

double SFSoundObject::AttenuationMaxDistance() const
{
	return m_dblAttenuationMaxDistance;
}

void SFSoundObject::HasAttenuation( bool in_bHasAttenuation )
{
	m_bHasAttenuation = in_bHasAttenuation;
}

void SFSoundObject::AttenuationMaxDistance( const double& in_rAttenuationMax )
{
	m_dblAttenuationMaxDistance = in_rAttenuationMax;
}

SFSoundObject * SFSoundObject::From( IReadBytes * in_pBytes )
{
	// Read data

	SFSoundObject * pSound = new SFSoundObject();

	pSound->ReadIDs( in_pBytes );

	in_pBytes->ReadString( pSound->m_wszName, kStrSize );

	pSound->m_bHasAttenuation = in_pBytes->Read<bool>();
	if ( pSound->m_bHasAttenuation ) 
	{
		pSound->m_dblAttenuationMaxDistance = in_pBytes->Read<double>();
	}

	return pSound;
}

ISoundObject * ISoundObject::From( IReadBytes * in_pBytes )
{
	return SFSoundObject::From( in_pBytes );
}

bool SFSoundObject::To( IWriteBytes * io_pBytes )
{
	bool bSuccess = WriteIDs( io_pBytes )
				 && io_pBytes->WriteString( m_wszName )
				 && io_pBytes->Write<bool>( m_bHasAttenuation );

	if ( bSuccess && m_bHasAttenuation ) 
	{
		bSuccess = io_pBytes->Write<double>( m_dblAttenuationMaxDistance );
	}

	return bSuccess;
}

// -----------------------------------------------------------------------------

SFEvent::SFEvent()
{
}

SFEvent::~SFEvent()
{
}

IActionList * SFEvent::GetActionList()
{
	return GetChildrenList();
}

// Persistence

SFEvent * SFEvent::From( IReadBytes * in_pBytes )
{
	// Read data

	SFEvent * pEvent = new SFEvent();

	pEvent->ReadIDs( in_pBytes );
	in_pBytes->ReadString( pEvent->m_wszName, kStrSize );

	ListFrom( pEvent, in_pBytes );

	return pEvent;
}

IEvent * IEvent::From( IReadBytes * in_pBytes )
{
	return SFEvent::From( in_pBytes );
}

bool SFEvent::To( IWriteBytes * io_pBytes )
{
	bool bSuccess = WriteIDs( io_pBytes )
				 && io_pBytes->WriteString( m_wszName );

	if ( bSuccess )
	{
		bSuccess = ListTo( io_pBytes );
	}

	return bSuccess;
}


// -----------------------------------------------------------------------------

SFAction::SFAction()
	: m_soundObjectID( AK_INVALID_UNIQUE_ID )
{
}

SFAction::~SFAction()
{
}

AkUniqueID SFAction::GetSoundObjectID() const
{
	return m_soundObjectID;
}

void SFAction::SetSoundObjectID( AkUniqueID in_soundObjectID )
{
	m_soundObjectID = in_soundObjectID;
}

// Persistence

SFAction * SFAction::From( IReadBytes * in_pBytes )
{
	SFAction * pAction = new SFAction();

	in_pBytes->ReadString( pAction->m_wszName, kStrSize );

	pAction->m_soundObjectID = in_pBytes->Read<AkUniqueID>();

	return pAction;
}

bool SFAction::To( IWriteBytes * io_pBytes )
{
	bool bSuccess = io_pBytes->WriteString( m_wszName )
				 && io_pBytes->Write<AkUniqueID>( m_soundObjectID );

	return bSuccess;
}

// -----------------------------------------------------------------------------

SFDialogueEvent::SFDialogueEvent()
{
}

SFDialogueEvent::~SFDialogueEvent()
{
}

IArgumentList * SFDialogueEvent::GetArgumentList()
{
	return GetChildrenList();
}

// Persistence
SFDialogueEvent * SFDialogueEvent::From( IReadBytes * in_pBytes )
{
	// Read data
	SFDialogueEvent * pDialogueEvent = new SFDialogueEvent();

	pDialogueEvent->ReadIDs( in_pBytes );
	in_pBytes->ReadString( pDialogueEvent->m_wszName, kStrSize );

	ListFrom( pDialogueEvent, in_pBytes );

	return pDialogueEvent;
}

bool SFDialogueEvent::To( IWriteBytes * io_pBytes )
{
	bool bSuccess = WriteIDs( io_pBytes )
				 && io_pBytes->WriteString( m_wszName );

	if ( bSuccess )
	{
		bSuccess = ListTo( io_pBytes );
	}

	return bSuccess;
}

// -----------------------------------------------------------------------------

SFArgument::SFArgument()
{
}

SFArgument::~SFArgument()
{
}

IArgumentValueList * SFArgument::GetArgumentValueList()
{
	return GetChildrenList();
}

// Persistence
// Note: Arguments are persisted either as standalone, or within a (dialogue event) list,
// hence the two separate overloads of From().

// Standalone version.
SFArgument * SFArgument::From( IReadBytes * in_pBytes )
{
	// Read data
	SFArgument * pArgument = new SFArgument();

	pArgument->ReadIDs( in_pBytes );
	in_pBytes->ReadString( pArgument->m_wszName, kStrSize );

	ListFrom( pArgument, in_pBytes );

	return pArgument;
}

bool SFArgument::To( IWriteBytes * io_pBytes )
{
	bool bSuccess = WriteIDs( io_pBytes )
				 && io_pBytes->WriteString( m_wszName );

	if ( bSuccess )
	{
		bSuccess = ListTo( io_pBytes );
	}

	return bSuccess;
}

// -----------------------------------------------------------------------------

SFArgumentValue::SFArgumentValue()
{
}

SFArgumentValue::~SFArgumentValue()
{
}

// Persistence
SFArgumentValue * SFArgumentValue::From( IReadBytes * in_pBytes )
{
	SFArgumentValue * pArgumentValue = new SFArgumentValue();

	in_pBytes->ReadString( pArgumentValue->m_wszName, kStrSize );
	pArgumentValue->ReadIDs( in_pBytes );

	return pArgumentValue;
}

bool SFArgumentValue::To( IWriteBytes * io_pBytes )
{
	bool bSuccess = io_pBytes->WriteString( m_wszName )
				 && WriteIDs( io_pBytes );

	return bSuccess;
}

// -----------------------------------------------------------------------------

SFState::SFState()
{
}

SFState::~SFState()
{
}

// Persistence

SFState * SFState::From( IReadBytes * in_pBytes )
{
	SFState * pState = new SFState();

	in_pBytes->ReadString( pState->m_wszName, kStrSize );
	pState->ReadIDs( in_pBytes );

	return pState;
}
	
bool SFState::To( IWriteBytes * io_pBytes )
{
	bool bSuccess = io_pBytes->WriteString( m_wszName )
				 && WriteIDs( io_pBytes );

	return bSuccess;
}

// -----------------------------------------------------------------------------

SFStateGroup::SFStateGroup()
{
}

SFStateGroup::~SFStateGroup()
{
}

IStateList * SFStateGroup::GetStateList()
{
	return GetChildrenList();
}

// Persistence
SFStateGroup * SFStateGroup::From( IReadBytes * in_pBytes )
{
	// Read data
	SFStateGroup * pStateGroup = new SFStateGroup();

	pStateGroup->ReadIDs( in_pBytes );
	in_pBytes->ReadString( pStateGroup->m_wszName, kStrSize );

	ListFrom( pStateGroup, in_pBytes );

	return pStateGroup;
}

IStateGroup * IStateGroup::From( IReadBytes * in_pBytes )
{
	return SFStateGroup::From( in_pBytes );
}

bool SFStateGroup::To( IWriteBytes * io_pBytes )
{
	bool bSuccess = WriteIDs( io_pBytes )
				 && io_pBytes->WriteString( m_wszName );

	if ( bSuccess )
	{
		bSuccess = ListTo( io_pBytes );
	}

	return bSuccess;
}

// -----------------------------------------------------------------------------

SFSwitch::SFSwitch()
{
}

SFSwitch::~SFSwitch()
{
}

// Persistence

SFSwitch * SFSwitch::From( IReadBytes * in_pBytes )
{
	SFSwitch * pSwitch = new SFSwitch();

	in_pBytes->ReadString( pSwitch->m_wszName, kStrSize );
	pSwitch->ReadIDs( in_pBytes );

	return pSwitch;
}
	
bool SFSwitch::To( IWriteBytes * io_pBytes )
{
	bool bSuccess = io_pBytes->WriteString( m_wszName )
				 && WriteIDs( io_pBytes );

	return bSuccess;
}

// -----------------------------------------------------------------------------

SFSwitchGroup::SFSwitchGroup()
{
}

SFSwitchGroup::~SFSwitchGroup()
{
}

ISwitchList * SFSwitchGroup::GetSwitchList()
{
	return GetChildrenList();
}

// Persistence
SFSwitchGroup * SFSwitchGroup::From( IReadBytes * in_pBytes )
{
	// Read data
	SFSwitchGroup * pSwitchGroup = new SFSwitchGroup();

	pSwitchGroup->ReadIDs( in_pBytes );
	in_pBytes->ReadString( pSwitchGroup->m_wszName, kStrSize );

	ListFrom( pSwitchGroup, in_pBytes );

	return pSwitchGroup;
}

ISwitchGroup * ISwitchGroup::From( IReadBytes * in_pBytes )
{
	return SFSwitchGroup::From( in_pBytes );
}

bool SFSwitchGroup::To( IWriteBytes * io_pBytes )
{
	bool bSuccess = WriteIDs( io_pBytes )
				 && io_pBytes->WriteString( m_wszName );

	if ( bSuccess )
	{
		bSuccess = ListTo( io_pBytes );
	}

	return bSuccess;
}

// -----------------------------------------------------------------------------

SFGameObject::SFGameObject()
{
	m_id = s_InvalidGameObject;
}
	
SFGameObject::~SFGameObject()
{}

// Persistence
SFGameObject * SFGameObject::From( IReadBytes * in_pBytes )
{
	// Read data
	SFGameObject * pGameObject = new SFGameObject();

	pGameObject->SetID(in_pBytes->Read<AkWwiseGameObjectID>());

	in_pBytes->ReadString( pGameObject->m_wszName, kStrSize );
	return pGameObject;
}

IGameObject * IGameObject::From( IReadBytes * in_pBytes )
{
	return SFGameObject::From( in_pBytes );
}

bool SFGameObject::To( IWriteBytes * io_pBytes )
{
	bool bSuccess = io_pBytes->Write<AkWwiseGameObjectID>( m_id )
				 && io_pBytes->WriteString( m_wszName );

	return bSuccess;
}

// -----------------------------------------------------------------------------

SFOriginalFile::SFOriginalFile()
{
}
	
SFOriginalFile::~SFOriginalFile()
{}

// Persistence
SFOriginalFile * SFOriginalFile::From( IReadBytes * in_pBytes )
{
	// Read data
	SFOriginalFile * pOriginalFile = new SFOriginalFile();

	in_pBytes->ReadString( pOriginalFile->m_wszName, kStrSize );

	return pOriginalFile;
}

IOriginalFile * IOriginalFile::From( IReadBytes * in_pBytes )
{
	return SFOriginalFile::From( in_pBytes );
}

bool SFOriginalFile::To( IWriteBytes * io_pBytes )
{
	bool bSuccess = io_pBytes->WriteString( m_wszName );

	return bSuccess;
}

// -----------------------------------------------------------------------------

SFGameParameter::SFGameParameter()
	: m_dblRangeMin(0.0)
	, m_dblRangeMax(0.0)
	, m_dblDefault(0.0)
{
}
	
SFGameParameter::~SFGameParameter()
{
}

double SFGameParameter::RangeMin() const
{
	return m_dblRangeMin;
}

double SFGameParameter::RangeMax() const
{
	return m_dblRangeMax;
}

double SFGameParameter::Default() const
{
	return m_dblDefault;
}

void SFGameParameter::SetRange( double in_dblMin, double in_dblMax )
{
	m_dblRangeMin = in_dblMin;
	m_dblRangeMax = in_dblMax;
}

void SFGameParameter::SetDefault( double in_dblDefault )
{
	m_dblDefault = in_dblDefault;
}

// Persistence

SFGameParameter * SFGameParameter::From( IReadBytes * in_pBytes )
{
	SFGameParameter * pGameParameter = new SFGameParameter();

	pGameParameter->ReadIDs( in_pBytes );
	in_pBytes->ReadString( pGameParameter->m_wszName, kStrSize );

	pGameParameter->m_dblRangeMin = in_pBytes->Read<double>();
	pGameParameter->m_dblRangeMax = in_pBytes->Read<double>();
	pGameParameter->m_dblDefault = in_pBytes->Read<double>();

	return pGameParameter;
}

IGameParameter * IGameParameter::From( IReadBytes * in_pBytes )
{
	return SFGameParameter::From( in_pBytes );
}

bool SFGameParameter::To( IWriteBytes * io_pBytes )
{
	bool bSuccess = WriteIDs( io_pBytes )
				 && io_pBytes->WriteString( m_wszName )
				 && io_pBytes->Write<double>( m_dblRangeMin )
				 && io_pBytes->Write<double>( m_dblRangeMax )
				 && io_pBytes->Write<double>( m_dblDefault );

	return bSuccess;
}

// -----------------------------------------------------------------------------

SFTrigger::SFTrigger()
{
}

SFTrigger::~SFTrigger()
{
}

// Persistence

SFTrigger * SFTrigger::From( IReadBytes * in_pBytes )
{
	SFTrigger * pTrigger = new SFTrigger();

	pTrigger->ReadIDs( in_pBytes );
	in_pBytes->ReadString( pTrigger->m_wszName, kStrSize );

	return pTrigger;
}

ITrigger * ITrigger::From( IReadBytes * in_pBytes )
{
	return SFTrigger::From( in_pBytes );
}
	
bool SFTrigger::To( IWriteBytes * io_pBytes )
{
	bool bSuccess = WriteIDs( io_pBytes )
		&& io_pBytes->WriteString( m_wszName );

	return bSuccess;
}

// -----------------------------------------------------------------------------

SFAuxBus::SFAuxBus()
{
}

SFAuxBus::~SFAuxBus()
{
}

// Persistence

SFAuxBus * SFAuxBus::From( IReadBytes * in_pBytes )
{
	SFAuxBus * pAuxBus = new SFAuxBus();

	pAuxBus->ReadIDs( in_pBytes );
	in_pBytes->ReadString( pAuxBus->m_wszName, kStrSize );

	return pAuxBus;
}

IAuxBus * IAuxBus::From( IReadBytes * in_pBytes )
{
	return SFAuxBus::From( in_pBytes );
}
	
bool SFAuxBus::To( IWriteBytes * io_pBytes )
{
	bool bSuccess = WriteIDs( io_pBytes )
		&& io_pBytes->WriteString( m_wszName );

	return bSuccess;
}

// -----------------------------------------------------------------------------

SFConversionSettings::SFConversionSettings()
{
}

SFConversionSettings::~SFConversionSettings()
{
}

// Persistence

SFConversionSettings * SFConversionSettings::From( IReadBytes * in_pBytes )
{
	SFConversionSettings * pObj = new SFConversionSettings();

	pObj->ReadIDs( in_pBytes );
	in_pBytes->ReadString( pObj->m_wszName, kStrSize );

	return pObj;
}

IConversionSettings * IConversionSettings::From( IReadBytes * in_pBytes )
{
	return SFConversionSettings::From( in_pBytes );
}
	
bool SFConversionSettings::To( IWriteBytes * io_pBytes )
{
	bool bSuccess = WriteIDs( io_pBytes )
		&& io_pBytes->WriteString( m_wszName );

	return bSuccess;
}