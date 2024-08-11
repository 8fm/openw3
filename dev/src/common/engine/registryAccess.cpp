/**
* Copyright c 2011 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "registryAccess.h"

#ifdef RED_PLATFORM_WINPC

const Uint32	CRegistryAccess::BUFFER_SIZE = 1024;

const Char*	CRegistryAccess::GAME_KEY			= TXT( "Software\\CD Projekt RED\\The Witcher 3" );
const Char*	CRegistryAccess::TEXT_LANGUAGE		= TXT( "Language" );
const Char*	CRegistryAccess::SPEECH_LANGUAGE	= TXT( "Speech" );
const Char*	CRegistryAccess::USER_SERIAL		= TXT( "SerialNumber" );
const Char*	CRegistryAccess::GOG_VALUE			= TXT( "RuntimeData" );

HKEY CRegistryAccess::GetKey( HKEY mainKey, const String& subKey, Bool readOnly )
{
	HKEY key;
	long queryResult = 0;
	if ( readOnly == false )
	{
		queryResult = RegCreateKeyEx( mainKey, subKey.AsChar(), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &key, NULL );
	}
	else
	{
		queryResult = RegOpenKeyEx( mainKey, subKey.AsChar(), 0, KEY_READ, &key );
	}
	if ( queryResult != ERROR_SUCCESS )
	{
		ERR_ENGINE( TXT("Failed to open registry key: 0x%x %s"), mainKey, subKey.AsChar() );
	}
	return key;
}

HKEY CRegistryAccess::GetMachineKey( const String& subKey, Bool readOnly )
{
	return GetKey( HKEY_LOCAL_MACHINE, subKey, readOnly );
}

HKEY CRegistryAccess::GetUserKey( const String& subKey, Bool readOnly )
{
	return GetKey( HKEY_CURRENT_USER, subKey, readOnly );
}

void CRegistryAccess::CloseKey( HKEY key )
{
	RegCloseKey( key );
}

Bool CRegistryAccess::ReadString( HKEY key, const String& valueName, String& value )
{
	Char buffer[ BUFFER_SIZE ];
	Uint32 registryValueType = REG_SZ;
	Uint32 registryValueSize = BUFFER_SIZE * 2;

	long queryResult = RegQueryValueEx( key, valueName.AsChar(), NULL, (LPDWORD) &registryValueType, (LPBYTE) &buffer, (LPDWORD) &registryValueSize );

	if ( queryResult == ERROR_SUCCESS )
	{
		value.Set( buffer );
		return true;
	}

	return false;
}

Bool CRegistryAccess::ReadMachineGameString( const String& valueName, String& value )
{
	HKEY registryKey = GetMachineKey( GAME_KEY, true );
	Bool result = ReadString( registryKey, valueName, value );
	CloseKey( registryKey );

	return result;
}

Bool CRegistryAccess::ReadUserGameString( const String& valueName, String& value )
{
	HKEY registryKey = GetUserKey( GAME_KEY, true );
	Bool result = ReadString( registryKey, valueName, value );
	CloseKey( registryKey );

	return result;
}


Bool CRegistryAccess::ReadUserGameAnsi( const String& valueName, AnsiChar* buffer, Int32* size )
{
	HKEY registryKey = GetUserKey( GAME_KEY, true );
	long queryResult = RegQueryValueEx( registryKey, valueName.AsChar(), NULL, NULL, (LPBYTE) buffer, (LPDWORD) size );
	CloseKey( registryKey );
	return queryResult == ERROR_SUCCESS;
}

Bool CRegistryAccess::ReadUserGameBytes( const String& valueName, Uint8* bytes, Int32 size )
{
	HKEY registryKey = GetUserKey( GAME_KEY, true );
	long queryResult = RegQueryValueEx( registryKey, valueName.AsChar(), NULL, NULL, (LPBYTE) bytes, (LPDWORD) &size );
	CloseKey( registryKey );
	return queryResult == ERROR_SUCCESS;
}

Bool CRegistryAccess::WriteString( HKEY key, const String& valueName, const String& value )
{
	Uint16 registryValueType = REG_SZ;
	Uint16 registryValueSize = (Uint16) ( sizeof( Char ) * value.Size() );

	Bool result = false;
	if ( value.Empty() == false )
	{
		result = ( RegSetValueEx( key, valueName.AsChar(), NULL, registryValueType, (const BYTE*) value.TypedData(), registryValueSize ) == ERROR_SUCCESS );
	}
	return result;
}

Bool CRegistryAccess::WriteMachineGameString( const String& valueName, const String& value )
{
	HKEY registryKey = GetMachineKey( GAME_KEY, false );
	Bool result = WriteString( registryKey, valueName, value );
	CloseKey( registryKey );

	return result;
}

Bool CRegistryAccess::WriteUserGameString( const String& valueName, const String& value )
{
	HKEY registryKey = GetUserKey( GAME_KEY, false );
	Bool result = WriteString( registryKey, valueName, value );
	CloseKey( registryKey );

	return result;
}

Bool CRegistryAccess::WriteUserGameAnsi( const String& valueName, AnsiChar* buffer, Int32 size )
{
	HKEY registryKey = GetUserKey( GAME_KEY, false );
	long queryResult = RegSetValueExA(registryKey, UNICODE_TO_ANSI( valueName.AsChar() ), NULL, NULL, ( LPBYTE ) buffer, size );
	CloseKey( registryKey );
	return queryResult == ERROR_SUCCESS;
}

Bool CRegistryAccess::WriteUserGameBytes( const String& valueName, Uint8* bytes, Int32 size )
{
	HKEY registryKey = GetUserKey( GAME_KEY, false );
	long queryResult = RegSetValueEx(registryKey, valueName.AsChar(), NULL, NULL, ( LPBYTE ) bytes, size );
	CloseKey( registryKey );
	return queryResult == ERROR_SUCCESS;
}

#endif
