//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CarParamSerializer.h"
#include "CarSimEngine.h"

namespace
{
	static const ULONG s_uiVersionNumber = 1;
	static const CString s_csFileExtension( L".carset" );
}

CarParamSerializer::CarParamSerializer()
:	m_pCarSimEngine( NULL )
{}
	
CarParamSerializer::~CarParamSerializer()
{}

void CarParamSerializer::Init( CarSimEngine* in_pEngine )
{
	m_pCarSimEngine = in_pEngine;
}

void CarParamSerializer::ClearFileName()
{
	m_csLastFile = L"";
}

bool CarParamSerializer::Save( const CString& in_rFileName )
{
	if( !m_pCarSimEngine )
		return false;

	CString csFileName( in_rFileName );
	EnsureFileExtension( csFileName );

	WriteBytesInFile fileWriteBytes;
	if( ! fileWriteBytes.Open( csFileName ) )
		return false;

	const CarParams& rCarParams = m_pCarSimEngine->GetCar().GetCarParam();

	if( ! fileWriteBytes.Write<ULONG>( s_uiVersionNumber )
		|| ! fileWriteBytes.Write<double>( rCarParams.m_dblWeight ) 
		|| ! fileWriteBytes.Write<double>( rCarParams.m_dblTorque )
		|| ! fileWriteBytes.Write<double>( rCarParams.m_dblDragCoefficient )
		|| ! fileWriteBytes.Write<double>( rCarParams.m_dblFrontSurface )
		|| ! fileWriteBytes.Write<double>( rCarParams.m_dblRollingResistence )
		|| ! fileWriteBytes.Write<size_t>( rCarParams.m_nNumOfGear ) )
	{
		return false;
	}

	// Write vector size
	if( ! fileWriteBytes.Write<size_t>( rCarParams.m_gearRatios.size() ) )
		return false;

	std::vector<double>::const_iterator itEnd = rCarParams.m_gearRatios.end();
	for(std::vector<double>::const_iterator it = rCarParams.m_gearRatios.begin(); it != itEnd; ++it )
	{
		if( ! fileWriteBytes.Write<double>( *it ) )
			return false;
	}

	if( ! fileWriteBytes.Write<double>( rCarParams.m_dblDifferentialRatio ) 
		|| ! fileWriteBytes.Write<double>( rCarParams.m_dblWheelRadius )
		|| ! fileWriteBytes.Write<double>( rCarParams.m_dblMinRPM )
		|| ! fileWriteBytes.Write<double>( rCarParams.m_dblRPMCutOff ) )
	{
		return false;
	}

	m_csLastFile = csFileName;

	return true;
}
	
bool CarParamSerializer::Load( const CString& in_rFileName )
{
	if( !m_pCarSimEngine )
		return false;

	CString csFileName( in_rFileName );
	EnsureFileExtension( csFileName );

	ReadBytesInFile fileReadBytes;
	if( ! fileReadBytes.Open( csFileName ) )
		return false;

	UINT version = fileReadBytes.Read<ULONG>();
	if( version != s_uiVersionNumber )
		return false;

	CarParams carParams;

	carParams.m_dblWeight = fileReadBytes.Read<double>();
	carParams.m_dblTorque = fileReadBytes.Read<double>();
	carParams.m_dblDragCoefficient = fileReadBytes.Read<double>();
	carParams.m_dblFrontSurface = fileReadBytes.Read<double>();
	carParams.m_dblRollingResistence = fileReadBytes.Read<double>();
	carParams.m_nNumOfGear = fileReadBytes.Read<size_t>();

	size_t vectorSize = fileReadBytes.Read<size_t>();
	_ASSERT( vectorSize == CarParam::s_nMaxNumOfGear );	// if not we have a problem ...

	carParams.m_gearRatios.clear();
	for(size_t i = 0; i < vectorSize; ++i)
	{
		carParams.m_gearRatios.push_back( fileReadBytes.Read<double>() );
	}

	carParams.m_dblDifferentialRatio = fileReadBytes.Read<double>();
	carParams.m_dblWheelRadius = fileReadBytes.Read<double>();
	carParams.m_dblMinRPM = fileReadBytes.Read<double>();
	carParams.m_dblRPMCutOff = fileReadBytes.Read<double>();

	m_pCarSimEngine->GetCar().SetCarParams( carParams );

	m_csLastFile = csFileName;

	return true;
}

const CString& CarParamSerializer::GetCurrentFilePath() const
{
	return m_csLastFile;
}

CString CarParamSerializer::GetCurrentFileNameOnly() const
{
	return CarSimHelper::GetFileNameOnly( m_csLastFile );
}
	
const CString& CarParamSerializer::GetFileExtension() const
{
	return s_csFileExtension;
}

void CarParamSerializer::EnsureFileExtension( CString& io_rFileName )
{
	CarSimHelper::EnsureFileExtension( io_rFileName, s_csFileExtension );
}
