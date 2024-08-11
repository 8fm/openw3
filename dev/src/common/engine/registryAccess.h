/**
* Copyright c 2011 CD Projekt Red. All Rights Reserved.
*/
#pragma once


#ifdef W2_PLATFORM_WIN32

class CRegistryAccess
{
public:
	static const	Char*	GAME_KEY;
	static const	Char*	TEXT_LANGUAGE;
	static const	Char*	SPEECH_LANGUAGE;
	static const	Char*	USER_SERIAL;
	static const	Char*	GOG_VALUE;

private:
	static const Uint32	BUFFER_SIZE;

public:
	CRegistryAccess() {}
	~CRegistryAccess() {}

	HKEY	GetKey( HKEY mainKey, const String& subKey, Bool readOnly );
	HKEY	GetMachineKey( const String& subKey, Bool readOnly );
	HKEY	GetUserKey( const String& subKey, Bool readOnly );
	void	CloseKey( HKEY key );
	
	Bool ReadString( HKEY key, const String& valueName, String& value );
	Bool ReadMachineGameString(  const String& valueName, String& value );
	Bool ReadUserGameString( const String& valueName, String& value );
	Bool ReadUserGameAnsi( const String& valueName, AnsiChar* buffer, Int32* size );
	Bool ReadUserGameBytes( const String& valueName, Uint8* bytes, Int32 size );

	Bool WriteString( HKEY key, const String& valueName, const String& value );
	Bool WriteMachineGameString(  const String& valueName, const String& value );
	Bool WriteUserGameString( const String& valueName, const String& value );
	Bool WriteUserGameAnsi( const String& valueName, AnsiChar* buffer, Int32 size );
	Bool WriteUserGameBytes( const String& valueName, Uint8* bytes, Int32 size );
};

typedef TSingleton< CRegistryAccess > SRegistryAccess;

#endif