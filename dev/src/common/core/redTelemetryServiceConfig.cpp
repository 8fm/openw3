/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "redTelemetryServiceConfig.h"
#include "../core/configVarSystem.h"
#include "../core/configVarLegacyWrapper.h"

const Char* SRedTelemetryServiceConfig::s_defaultDumpFileName = TXT( "telemetry_dump.dat" );

Bool SRedTelemetryServiceConfig::LoadConfig( const String& fileName )
{
	if( fileName.Empty() )
	{
		return false;
	}


	SConfig::GetInstance().GetLegacy().ReadParam( fileName.AsChar(), TXT("General"),		TXT( "Server" ),		this->serverHost );
	SConfig::GetInstance().GetLegacy().ReadParam( fileName.AsChar(), TXT("General"),		TXT( "Port" ),			this->serverPort );
	SConfig::GetInstance().GetLegacy().ReadParam( fileName.AsChar(), TXT("General"),		TXT( "DumpFileName" ),	this->dumpFileName );
	SConfig::GetInstance().GetLegacy().ReadParam( fileName.AsChar(), TXT("General"),		TXT( "GoogleTrackingID" ), this->googleTrackingID );

	
	SConfig::GetInstance().GetLegacy().ReadParam( fileName.AsChar(), TXT("BatchEvents"),	TXT( "Count" ),			this->batchCount );
	SConfig::GetInstance().GetLegacy().ReadParam( fileName.AsChar(), TXT("BatchEvents"),	TXT( "Timeout" ),		this->batchSeconds );

	SConfig::GetInstance().GetLegacy().ReadParam( fileName.AsChar(), TXT("Http"),			TXT( "Timeout" ),		this->curlTimeout );

	SConfig::GetInstance().GetLegacy().ReadParam( fileName.AsChar(), TXT("Game"),			TXT( "Id" ),			this->gameId );
	SConfig::GetInstance().GetLegacy().ReadParam( fileName.AsChar(), TXT("Game"),			TXT( "Version" ),		this->gameVersion );
	
	String paramName;

	SConfig::GetInstance().GetLegacy().ReadParam( TXT("user"),	   TXT("RedTelemetry"), TXT( "UserCategory" ),	paramName );

	if( paramName.Empty() == true )
		SConfig::GetInstance().GetLegacy().ReadParam( fileName.AsChar(), TXT("User"),			TXT( "Name" ),	paramName );

	if( paramName.Empty() == false )
		SetUserBasedOnString( paramName );

	return true;
}

#if !defined ( NO_TELEMETRY_DEBUG )
Bool SRedTelemetryServiceConfig::SaveConfig( const String& fileName ) const
{
	if( fileName.Empty() )
	{
		return false;
	}

	String paramName;

	SConfig::GetInstance().GetLegacy().WriteParam( fileName.AsChar(), TXT("General"),		TXT( "Server" ),		this->serverHost );
	SConfig::GetInstance().GetLegacy().WriteParam( fileName.AsChar(), TXT("General"),		TXT( "Port" ),			this->serverPort );
	paramName = this->GetDumpFileName();
	SConfig::GetInstance().GetLegacy().WriteParam( fileName.AsChar(), TXT("General"),		TXT( "DumpFileName" ),	paramName );
	SConfig::GetInstance().GetLegacy().WriteParam( fileName.AsChar(), TXT("General"),		TXT( "GoogleTrackingID" ), this->googleTrackingID );
	
	SConfig::GetInstance().GetLegacy().WriteParam( fileName.AsChar(), TXT("BatchEvents"), TXT( "Count" ),			this->batchCount );
	SConfig::GetInstance().GetLegacy().WriteParam( fileName.AsChar(), TXT("BatchEvents"), TXT( "Timeout" ),		this->batchSeconds );

	SConfig::GetInstance().GetLegacy().WriteParam( fileName.AsChar(), TXT("Http"),		TXT( "Timeout" ),		this->curlTimeout );

	SConfig::GetInstance().GetLegacy().WriteParam( fileName.AsChar(), TXT("Game"),		TXT( "Id" ),			this->gameId );
	SConfig::GetInstance().GetLegacy().WriteParam( fileName.AsChar(), TXT("Game"),		TXT( "Version" ),		this->gameVersion );

	return true;
}
#endif


void SRedTelemetryServiceConfig::SetUserBasedOnString( const String& user )
{
	if( user == TXT( "debug" ) )
	{
		userCategory =  Telemetry::UC_DEBUG;
	}
	else if( user == TXT( "deb" ) )
	{
		userCategory =  Telemetry::UC_DEBUG;
	}
	else if( user == TXT( "dev" ) )
	{
		userCategory =  Telemetry::UC_DEV;
	}
	else if( user == TXT( "qa" ) )
	{
		userCategory =  Telemetry::UC_QA;
	}
	else if( user == TXT( "beta" ) )
	{
		userCategory = Telemetry::UC_BETA;
	}
	else if( user == TXT( "bet" ) )
	{
		userCategory = Telemetry::UC_BETA;
	}
	else if( user == TXT( "retail" ) )
	{
		userCategory = Telemetry::UC_RETAIL;
	}
	else if( user == TXT( "ret" ) )
	{
		userCategory = Telemetry::UC_RETAIL;
	}
	else
	{
		RED_ASSERT( false, TXT( "Bad user category in telemetry config" ) );
	}
}

void SRedTelemetryServiceConfig::UserToString( String& user ) const
{
	switch( this->userCategory )
	{
	case Telemetry::UC_DEBUG:
		user = TXT( "deb" );
		break;
	case Telemetry::UC_DEV:
		user = TXT( "dev" );
		break;
	case Telemetry::UC_QA:
		user = TXT( "qa" );
		break;
	case Telemetry::UC_BETA:
		user = TXT( "bet" );
		break;
	case Telemetry::UC_RETAIL:
		user = TXT( "ret" );
		break;
	default:
		user = TXT( "dev" );
		break;
	}
}

void SRedTelemetryServiceConfig::SetPlatformBasedOnString( const String& platform )
{
	if( platform == TXT( "pc" ) )
	{
		this->platform = Telemetry::P_PC;
	}
	else if( platform == TXT( "xbox" ) )
	{
		this->platform = Telemetry::P_XBONE;
	}
	else if( platform == TXT( "ps4" ) )
	{
		this->platform = Telemetry::P_PS4;
	}
}

void SRedTelemetryServiceConfig::PlatformToString( String& platform ) const
{
	switch ( this->platform )
	{
	case Telemetry::P_PC:
		platform = TXT( "pc" );
		break;
	case Telemetry::P_PS4:
		platform = TXT( "ps4" );
		break;
	case Telemetry::P_XBONE:
		platform = TXT( "xbox" );
		break;
	default:
		platform = TXT( "pc" );
		break;
	}
}

const Char* SRedTelemetryServiceConfig::GetDumpFileName() const
{
	if ( dumpFileName.Empty() == false )
	{
		return dumpFileName.AsChar();
	}
	return s_defaultDumpFileName;
}