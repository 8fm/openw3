//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"
#include "CarSimSimulationSerializer.h"
#include "CarSimEngine.h"

namespace
{
	static const ULONG s_uiVersionNumber = 2;
	static const CString s_csFileExtension( L".carsim" );
}

CarSimSimulationSerializer::CarSimSimulationSerializer()
:	m_pCarSimEngine( NULL )
{}
	
CarSimSimulationSerializer::~CarSimSimulationSerializer()
{}

void CarSimSimulationSerializer::Init( CarSimEngine* in_pEngine )
{
	m_pCarSimEngine = in_pEngine;
}

void CarSimSimulationSerializer::ClearFileName()
{
	m_csLastFile = L"";
}

bool CarSimSimulationSerializer::Save( const CString& in_rFileName )
{
	if( !m_pCarSimEngine )
		return false;

	CString csFileName( in_rFileName );
	EnsureFileExtension( csFileName );

	WriteBytesInFile fileWriteBytes;
	if( ! fileWriteBytes.Open( csFileName ) )
		return false;

	if( ! fileWriteBytes.Write<ULONG>( s_uiVersionNumber )
		|| ! fileWriteBytes.WriteString( m_pCarSimEngine->GetGameObjectName() )
		|| ! fileWriteBytes.WriteString( m_pCarSimEngine->GetCarParamSerializer().GetCurrentFilePath() ) )
	{
		return false;
	}

	CarSimAssignment carAssignment;
	carAssignment.Assign( m_pCarSimEngine->GetAssignement() );

	if( ! SaveAssignment( fileWriteBytes, carAssignment.m_RPMParam ) 
		|| ! SaveAssignment( fileWriteBytes, carAssignment.m_LoadParam )
		|| ! SaveAssignment( fileWriteBytes, carAssignment.m_SpeedParam )
		|| ! SaveAssignment( fileWriteBytes, carAssignment.m_EnginePlayEvent )
		|| ! SaveAssignment( fileWriteBytes, carAssignment.m_EngineStopEvent )
		|| ! SaveAssignment( fileWriteBytes, carAssignment.m_UpShiftEvent )
		|| ! SaveAssignment( fileWriteBytes, carAssignment.m_DownShiftEvent )
		|| ! SaveAssignment( fileWriteBytes, carAssignment.m_AuxBus ) )
	{
		return false;
	}

	double dblDryVolume = 0.0;
	double dblWetVolume = 0.0;
	bool bUseAuxBus = m_pCarSimEngine->GetAuxBusSettings( dblDryVolume, dblWetVolume );
	
	if( ! fileWriteBytes.Write<bool>( bUseAuxBus ) )
	{
		return false;
	}

	if( ! fileWriteBytes.Write<ULONG>( m_pCarSimEngine->GetFrameInterval() ) )
		return false;

	m_csLastFile = csFileName;

	return true;
}
	
bool CarSimSimulationSerializer::Load( const CString& in_rFileName )
{
	if( !m_pCarSimEngine )
		return false;

	CString csGameObjectName, csCarSettingsFile;

	CString csFileName( in_rFileName );
	EnsureFileExtension( csFileName );

	ReadBytesInFile fileReadBytes;
	if( ! fileReadBytes.Open( csFileName ) )
		return false;

	UINT version = fileReadBytes.Read<UINT>();
	if( version != s_uiVersionNumber )
		return false;

	WCHAR szBuffer[_MAX_PATH] = {0};
	if( ! fileReadBytes.ReadString( szBuffer, _MAX_PATH ) )
		return false;

	csGameObjectName = szBuffer;
	
	if( ! fileReadBytes.ReadString( szBuffer, _MAX_PATH ) )
		return false;

	csCarSettingsFile = szBuffer;

	CarSimAssignment carAssignment;
	if( ! LoadAssignment( fileReadBytes, carAssignment.m_RPMParam )
		|| ! LoadAssignment( fileReadBytes, carAssignment.m_LoadParam ) 
		|| ! LoadAssignment( fileReadBytes, carAssignment.m_SpeedParam ) 
		|| ! LoadAssignment( fileReadBytes, carAssignment.m_EnginePlayEvent ) 
		|| ! LoadAssignment( fileReadBytes, carAssignment.m_EngineStopEvent ) 
		|| ! LoadAssignment( fileReadBytes, carAssignment.m_UpShiftEvent ) 
		|| ! LoadAssignment( fileReadBytes, carAssignment.m_DownShiftEvent ) 
		|| ! LoadAssignment( fileReadBytes, carAssignment.m_AuxBus ) )
	{
		return false;
	}

	bool bUseAuxBus = fileReadBytes.Read<bool>();
	double dblDryVolume = 1.0;
	double dblWetVolume = 0.0;

	ULONG ulTimeInterval =  fileReadBytes.Read<ULONG>();

	m_pCarSimEngine->ChangeGameObject( csGameObjectName );
	m_pCarSimEngine->SetAssignement( carAssignment );
	m_pCarSimEngine->SetAuxBusSettings( bUseAuxBus, dblDryVolume, dblWetVolume );
	m_pCarSimEngine->SetFrameInterval( ulTimeInterval );

	bool bSetDefault = true;
	if( ! csCarSettingsFile.IsEmpty() )
	{
		bSetDefault = ! m_pCarSimEngine->GetCarParamSerializer().Load( csCarSettingsFile );
		if( bSetDefault )
		{
			CString csError;
			csError.Format( IDS_OPEN_ERROR, csCarSettingsFile );
			MessageBox( NULL, csError, NULL, MB_ICONERROR | MB_OK );
		}
	}
	
	if( bSetDefault )
	{
		m_pCarSimEngine->GetCarParamSerializer().ClearFileName();
		m_pCarSimEngine->GetCar().SetDefaultCarParam();
	}

	m_csLastFile = csFileName;

	return true;
}

const CString& CarSimSimulationSerializer::GetCurrentFilePath() const
{
	return m_csLastFile;
}

CString CarSimSimulationSerializer::GetCurrentFileNameOnly() const
{
	return CarSimHelper::GetFileNameOnly( m_csLastFile );
}
	
const CString& CarSimSimulationSerializer::GetFileExtension() const
{
	return s_csFileExtension;
}

void CarSimSimulationSerializer::EnsureFileExtension( CString& io_rFileName )
{
	CarSimHelper::EnsureFileExtension( io_rFileName, s_csFileExtension );
}

bool CarSimSimulationSerializer::LoadAssignment( ReadBytesInFile& in_rReader, AkUniqueID& in_rObjectID )
{
	return in_rReader.Read<AkUniqueID>( in_rObjectID );
}

bool CarSimSimulationSerializer::SaveAssignment( WriteBytesInFile& in_rWriter, AkUniqueID& in_rObjectID )
{
	return in_rWriter.Write<AkUniqueID>( in_rObjectID );
}
