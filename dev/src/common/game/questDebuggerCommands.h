/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma once

class CDebugServerPlugin;
//class ObjectPropertiesTest
//{
//public:
//	static void Test();
//};
class CDebugServerCommandGetQuestLayout
{
public: 
	static Uint32 ProcessCommand( CDebugServerPlugin* owner, const TDynArray<String>& data );
	static const Bool gameThread = true;
};

class CDebugServerCommandGetQuestData
{
public: 
	static Uint32 ProcessCommand( CDebugServerPlugin* owner, const TDynArray<String>& data );
	static const Bool gameThread = true;
};

class CDebugServerCommandToggleConnection
{
public:
	static Uint32 ProcessCommand( CDebugServerPlugin* owner, const TDynArray<String>& data );
	static const Bool gameThread = true;
};

class CDebugServerCommandToggleBreakpoint
{
public:
	static Uint32 ProcessCommand( CDebugServerPlugin* owner, const TDynArray<String>& data );
	static const Bool gameThread = true;
};

class CDebugServerCommandStartInteractionDialog
{
public:
	static Uint32 ProcessCommand( CDebugServerPlugin* owner, const TDynArray<String>& data );
	static const Bool gameThread = true;
};

class CDebugServerCommandContinueFromBreakpoint
{
public:
	static Uint32 ProcessCommand( CDebugServerPlugin* owner, const TDynArray<String>& data );
	static const Bool gameThread = true;
};

class CDebugServerCommandContinueFromPin
{
public:
	static Uint32 ProcessCommand( CDebugServerPlugin* owner, const TDynArray<String>& data );
	static const Bool gameThread = true;
};

class CDebugServerCommandKillSignal
{
public:
	static Uint32 ProcessCommand( CDebugServerPlugin* owner, const TDynArray<String>& data );
	static const Bool gameThread = true;
};

class CDebugServerCommandGetQuestBlockProperties
{
public:
	static Uint32 ProcessCommand( CDebugServerPlugin* owner, const TDynArray<String>& data );
	static const Bool gameThread = true;
};

class CDebugServerCommandGetCallstack
{
public:
	static Uint32 ProcessCommand( CDebugServerPlugin* owner, const TDynArray<String>& data );
	static const Bool gameThread = true;
};

class CQuestLayoutDumper
{
public:
	static Bool Dump( const String& questPath, const String& dumpPath );
};

class CQuestAppearancesDumper
{
public:
	static Bool Dump( const String& dumpPath, const String& type, void (*logCallback)( const String&) );

private:
	static void GetQuestEntities( TDynArray< CDiskFile* >& entities );
	static void ProcessQuestEntities( const String& dumpPath, TDynArray< CDiskFile* >& entities, void (*logCallback)( const String& ) );
	static void GetScenesEntities( TDynArray< CDiskFile* >& entities );	
	static void ProcessSceneEntities( const String& dumpPath, TDynArray< CDiskFile* >& entities, void (*logCallback)( const String& ) );
	static void ProcessEntityFile( String& outputCSV, CDiskFile* entityFile );
	static void ProcessEntityTemplateAppearances( String& outputCSV, const String& entityDepotFile, const CEntityTemplate* entityTemplate );
	
	static void ProcessSceneFile( String& outputCSV, CDiskFile* entityFile );
	static void ProcessStorySceneAppearances( String& outputCSV, const String& sceneDepotFile, const CStoryScene* storySceneActor );
};


//////////////////////////////////////////////////////////////////////////
// EOF
