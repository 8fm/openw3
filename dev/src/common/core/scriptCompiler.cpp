/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#include "scriptCompiler.h"

#include "../redSystem/stringWriter.h"

#include "scriptFileParser.h"
#include "scriptFunctionCompiler.h"
#include "scriptableStateMachine.h"
#include "scriptableState.h"
#include "scriptSyntaxNode.h"
#include "scriptingSystem.h"
#include "scriptSnapshot.h"
#include "scriptCodeNodeCompilationPool.h"
#include "scriptable.h"

#include "feedback.h"
#include "engineTime.h"
#include "namesRegistry.h"
#include "../redNetwork/manager.h"
#include "../core/objectDiscardList.h"
#include "events.h"
#include "enum.h"
#include "math.h"
#include "scriptDefaultValue.h"
#include "defaultValue.h"
#include "fileSys.h"
#include "objectGC.h"

RED_DEFINE_NAMED_NAME( AutoState, "autoState" );

#if !defined( RED_FINAL_BUILD ) && defined( RED_PLATFORM_ORBIS )

void DebugSetCompilationProgress( Uint32 progress, Uint32 total )
{
	static Uint32 numBits = 0xffffffff;

	Float percentageComplete = progress / static_cast< Float >( total );

	Uint32 numBitsNow = ( percentageComplete * 8.0f ) + 0.5f;

	if( numBitsNow != numBits )
	{
		Uint32 bits = 0;
		for( Uint32 i = 0; i < numBitsNow; ++i )
		{
			bits |= ( 1 << i );
		}

		numBits = numBitsNow;

		::sceKernelSetGPO( bits );
	}
}

#	define DEBUG_SET_PROGRESS( progress, total ) DebugSetCompilationProgress( progress, total )
#else
#	define DEBUG_SET_PROGRESS( progress, total )
#endif

CScriptCompiler::CScriptCompiler( IScriptLogInterface* output )
	:	m_output( output )
	,	m_hasError( false )
{
	m_strictMode = ( NULL == Red::System::StringSearch( SGetCommandLine(), TXT("-compile_nonstrict") ) );
}

CScriptCompiler::~CScriptCompiler()
{
}

void CScriptCompiler::ScriptLog( const Char* text, ... )
{
	// Format text
	va_list arglist;
	va_start( arglist, text );
	Char formatedText[ 4096 ];
	Red::System::VSNPrintF( formatedText, ARRAY_COUNT( formatedText ), text, arglist ); 

	// Emit to output
	if ( m_output )
	{
		m_output->Log( formatedText );
	}

	LOG_CORE( TXT("[Script]: %ls"), formatedText );

#ifdef RED_NETWORK_ENABLED

	// Emit to debugger
	Red::Network::Manager* network = Red::Network::Manager::GetInstance();
	if( network )
	{
		Red::Network::ChannelPacket packet( RED_NET_CHANNEL_SCRIPT_COMPILER );
		packet.WriteString( "log" );
		packet.WriteString( formatedText );
		network->Send( RED_NET_CHANNEL_SCRIPT_COMPILER, packet );
	}

#endif // RED_NETWORK_ENABLED
}

void CScriptCompiler::ScriptWarn( const CScriptFileContext& context, const Char* text, ... )
{
	// Format text
	va_list arglist;
	va_start( arglist, text );
	Char formatedText[ 4096 ];
	Red::System::VSNPrintF( formatedText, ARRAY_COUNT( formatedText ), text, arglist ); 

	// Emit to output
	if ( m_output )
	{
		m_output->Warn( context, formatedText );		
	}
	
	// We want warnings to be visible after editor start
	WARN_CORE( TXT("[Script]: Warning %ls: %ls"), context.ToString().AsChar(), formatedText );

#ifdef RED_NETWORK_ENABLED
	// Emit to debugger
	Red::Network::Manager* network = Red::Network::Manager::GetInstance();
	if( network )
	{
		Red::Network::ChannelPacket packet( RED_NET_CHANNEL_SCRIPT_COMPILER );
		packet.WriteString( "warn" );
		packet.Write( context.m_line );
		packet.WriteString( context.m_file.AsChar() );
		packet.WriteString( formatedText );
		network->Send( RED_NET_CHANNEL_SCRIPT_COMPILER, packet );
	}
#endif // RED_NETWORK_ENABLED
}

void CScriptCompiler::ScriptError( const CScriptFileContext& context, const Char* text, ... )
{
	// We had an error...
	m_hasError = true;

	// Format text
	va_list arglist;
	va_start( arglist, text );
    Char formatedText[ 4096 ] = {0};
	Red::System::VSNPrintF( formatedText, ARRAY_COUNT( formatedText ), text, arglist ); 

	// Emit to output
	if ( m_output )
	{
		m_output->Error( context, formatedText );
	}
	
	// We want errors to be visible after editor start
	ERR_CORE( TXT("[Script]: Error %ls: %ls"), context.ToString().AsChar(), formatedText );

#ifdef RED_NETWORK_ENABLED
	// Emit to debugger
	Red::Network::Manager* network = Red::Network::Manager::GetInstance();
	if( network )
	{
		Red::Network::ChannelPacket packet( RED_NET_CHANNEL_SCRIPT_COMPILER );
		packet.WriteString( "error" );
		packet.Write( context.m_line );
		packet.WriteString( context.m_file.AsChar() );
		packet.WriteString( formatedText );
		network->Send( RED_NET_CHANNEL_SCRIPT_COMPILER, packet );
	}
#endif // RED_NETWORK_ENABLED
}

void CScriptCompiler::CompilationStarted()
{
#ifdef RED_NETWORK_ENABLED
	// Emit to debugger
	Red::Network::Manager* network = Red::Network::Manager::GetInstance();
	if( network )
	{
		Red::Network::ChannelPacket packet( RED_NET_CHANNEL_SCRIPT_COMPILER );
		packet.WriteString( "started" );
		packet.Write( GScriptingSystem->IsFinalRelease() );
		packet.Write( m_strictMode );
		network->Send( RED_NET_CHANNEL_SCRIPT_COMPILER, packet );
	}
#endif // RED_NETWORK_ENABLED
}

void CScriptCompiler::CompilationFinished()
{
#ifdef RED_NETWORK_ENABLED
	// Emit to debugger
	Red::Network::Manager* network = Red::Network::Manager::GetInstance();
	if( network )
	{
		Red::Network::ChannelPacket packet( RED_NET_CHANNEL_SCRIPT_COMPILER );
		packet.WriteString( "finished" );
		packet.Write( m_hasError );
		network->Send( RED_NET_CHANNEL_SCRIPT_COMPILER, packet );
	}
#endif // RED_NETWORK_ENABLED
}

#define FAIL_IF_ERROR			\
	if ( m_hasError )			\
	{							\
		CompilationFinished();	\
		return false;			\
	}


namespace ScriptCompilationHelpers
{
	Bool CreateFullContextName( const String& scriptFile, Red::System::StringWriter< Char >& stringWriter )
	{
		stringWriter.Reset();

		const Char* pch = scriptFile.AsChar();
		const String& rootPath = GFileManager->GetRootDirectory();

		if ( Red::System::StringCompareNoCase( pch, rootPath.AsChar(), rootPath.GetLength() ) != 0 )
			return false;
		pch += rootPath.GetLength();

		const size_t contentLen = Red::System::StringLengthCompileTime(TXT("content\\"));
		const size_t modsLen = Red::System::StringLengthCompileTime(TXT("mods\\"));
		if ( Red::System::StringCompareNoCase( pch, TXT("content\\"), contentLen ) == 0 )
		{
			pch += contentLen;
		}
		else if ( Red::System::StringCompareNoCase( pch, TXT("mods\\"), modsLen ) == 0 )
		{
			pch += modsLen;
		}
		else
		{
			return false;
		}

		if ( !*pch )
			return false;

		const Char* nextSlash = Red::System::StringSearch( pch, TXT('\\') );
		if ( !nextSlash )
			return false;

		const Uint32 nameLen = (Uint32)(nextSlash - pch);
		if ( nameLen < 1 )
			return false;
		
		stringWriter.Append(TXT("["));
		stringWriter.Append( pch, nameLen );
		stringWriter.Append(TXT("]"));

		pch += nameLen;
		if (!*pch )
			return false;

		const size_t contentLen2 = Red::System::StringLengthCompileTime(TXT("\\content"));
		if ( Red::System::StringCompareNoCase( pch, TXT("\\content"), contentLen2 ) == 0 )
		{
			pch += contentLen2;
			if (!*pch )
				return false;
		}

		const size_t scriptsLen = Red::System::StringLengthCompileTime(TXT("\\scripts\\"));
		if ( Red::System::StringCompareNoCase( pch, TXT("\\scripts\\"), scriptsLen ) != 0 )
			return false;

		pch += scriptsLen;
		if ( !*pch )
			return false;

		stringWriter.Append( pch );

		return true;
	}
}

Bool CScriptCompiler::CompileFiles( const TDynArray< String >& scriptFiles, Bool fullContext )
{
	static CScriptSnapshot* GLastValidSnapshot = NULL;

	// Reset error flag
	m_hasError = false;

	CompilationStarted();

	// Splash screen info
	GSplash->UpdateProgress( TXT( "Compiling scripts (finding script files)..." ) );

	// Gathered script definitions
	CScriptSystemStub definitions;

	EngineTime lastUpdate( 0.f );

	// Parse definitions
	CScriptFileParser parser( definitions, this );
	LOG_CORE( TXT( "Parsing %i script files..." ), scriptFiles.Size() );

	Red::System::StackStringWriter< Char, 1024 > stackWriter;
	for ( Uint32 i = 0; i < scriptFiles.Size(); ++i )
	{
		// Log file progress
		// Don't always get before scripts dir for context because of how the script studio debugger works
		const String& scriptFilePath = scriptFiles[i];
		String scriptShortFilePath;
		if ( fullContext && ScriptCompilationHelpers::CreateFullContextName( scriptFilePath, stackWriter ) )
		{
			scriptShortFilePath = stackWriter.AsChar();
		}
		else
		{
			scriptShortFilePath = scriptFilePath.StringAfter( TXT("scripts\\") );
		}

		ScriptLog( TXT( "%ls" ), scriptShortFilePath.AsChar() );

		// Splash screen info, twice per second
		if ( EngineTime::GetNow() - lastUpdate > 0.5f )
		{
			lastUpdate = EngineTime::GetNow();
			GSplash->UpdateProgress( TXT("Compiling scripts (parsing file %i/%i)..."), i, scriptFiles.Size() );
		}

		DEBUG_SET_PROGRESS( i, scriptFiles.Size() );

		// Parse file
		parser.ParseFile( scriptFilePath, scriptShortFilePath );

		// Don't have all day
		// On the Durango the script compilation is very slow so let's break on the first error and not wait for all the others
#ifdef RED_PLATFORM_DURANGO
		FAIL_IF_ERROR;
#endif
	}

	// Parsing error, error before taking any more serious actions
	FAIL_IF_ERROR;

	// Collapse property browsers
	EDITOR_DISPATCH_EVENT( CNAME( ScriptCompilationStart ), NULL );

	// Get all scriptable objects in the system
	// Since scripted objects can no longer be a CObjects we cannot assume that the BaseObjectIterator will give us all of the objects
	// Unless we are willing to create some internal list of scripted objects (which would unfortunately recreate the CObject functionality)
	// we need to find another way to gather all relevant IScriptables. Basically, the idea is the good old pointer walking.
	TDynArray< THandle< IScriptable > > allScriptables;
	IScriptable::CollectAllScriptableObjects( allScriptables );

	// Compile data layout
	// TODO: do not compile data layout if only function code changes
	Bool compileDataLayout = true;
	if ( compileDataLayout )
	{
		GObjectGC->CollectNow();
		// Create data snapshot of script state
		if ( !GLastValidSnapshot )
		{
			GLastValidSnapshot = new CScriptSnapshot;
			GLastValidSnapshot->CaptureScriptData( allScriptables );
		}

		// We need to remove all default objects from the game as the snapshot capture may create new objects after we've already
		// collected our list of scriptable objects. So to make sure the system doesn't crash we delete them before we update the types
		TDynArray< THandle< IScriptable > > allDefaultScriptables;
		IScriptable::CollectAllDefaultScriptableObjects( allDefaultScriptables );
		
		for( Uint32 i = 0; i < allDefaultScriptables.Size(); ++i )
		{
			IScriptable* scriptable = allDefaultScriptables[ i ].Get();

			// This will mark the default object for destruction (even if there isn't one)
			scriptable->GetClass()->DestroyDefaultObject();
		}

		// This will clean up all objects scheduled for destruction.
		// Doing this after the types change will cause very strange crashes
		GObjectGC->CollectNow();

		// Remove all script related data from RTTI system
		SRTTI::GetInstance().ClearScriptData( &allScriptables );

		// Script log
		const Uint32 numTypes = definitions.CountTypes();
		ScriptLog( TXT("Creating types (%i types)..."), numTypes );

		// Splash screen info
		GSplash->UpdateProgress( TXT("Compiling scripts (creating RTTI definitions - %i types)..."), numTypes );

		// Create type definitions
		ScriptLog( TXT("Creating types (%i classes)..."), definitions.m_classes.Size() );
		CreateTypes( definitions );
		FAIL_IF_ERROR;

		// Reindex RTTI classes (IsA will work again)
		// Then sort class definitions by their class index so we create base classes first
		// and redundant property definitions will be found independent of class creation order
		// Classes are already indexed based on inheritence hierarchy
		SRTTI::GetInstance().ReindexClasses();
		Sort( definitions.m_classes.Begin(), definitions.m_classes.End(), 
			[]( const CScriptClassStub* a, const CScriptClassStub* b )
			{
				const Int32 indexA = a->m_createdClass ? a->m_createdClass->GetClassIndex() : -1;
				const Int32 indexB = b->m_createdClass ? b->m_createdClass->GetClassIndex() : -1;	
				return indexA < indexB;
			}
		);

		// Examine the class hierarchy for incomplete classes
		ScriptLog( TXT("Scanning for incomplete classes...") );
		CheckClassesForCompleteness( definitions );
		FAIL_IF_ERROR;

		// Create functions
		ScriptLog( TXT("Creating functions...") );
		CreateFunctions( definitions );
		FAIL_IF_ERROR;

		// Create properties 
		ScriptLog( TXT("Creating properties...") );
		CreateProperties( definitions );
		FAIL_IF_ERROR;

		// Bind function
		ScriptLog( TXT("Binding functions...") );
		BindFunctions( definitions );
		FAIL_IF_ERROR;

		// Update data size
		ScriptLog( TXT("Building data layout...") );
		BuildDataLayout( definitions );
		FAIL_IF_ERROR;

		// Create default values
		ScriptLog( TXT("Creating default values...") );
		CreateDefaultValues( definitions );
		FAIL_IF_ERROR;

		// Since we got this far restore RTTI data
		SRTTI::GetInstance().RestoreScriptData( &allScriptables );

		// Restore script snapshot
		if ( GLastValidSnapshot )
		{
			GLastValidSnapshot->RestoreScriptData( allScriptables );
			delete GLastValidSnapshot;
			GLastValidSnapshot = NULL;
		}

		// Cache scripted data
		SRTTI::GetInstance().RecacheClassProperties();

		// Collect post reload garbage
		GObjectGC->CollectNow();
	}

	// Compile functions
	Bool compileFunctions = true;
	if ( compileFunctions )
	{
		// Script log
		const Uint32 numFunctions = definitions.CountFunctions();
		ScriptLog( TXT("Generating code (%i functions)..."), numFunctions );

		// Create default values
		ScriptLog( TXT("Compiling functions...") );
		CompileFunctions( definitions );
		FAIL_IF_ERROR;
	}

	// Restore object browsers
	EDITOR_DISPATCH_EVENT( CNAME( ScriptCompilationEnd ), NULL );

	// Inform all objects about scripts being reloaded
	RED_ASSERT( !m_hasError, TXT( "This function should have exited before now if there had been an error" ) );

	for( Uint32 i = 0; i < allScriptables.Size(); ++i )
	{
		IScriptable* scriptable = allScriptables[ i ].Get();

		if( scriptable )
		{
			scriptable->OnScriptReloaded();
		}
	}

	// Splash screen info
	GSplash->UpdateProgress( TXT( "Script compilation: %ls..." ), m_hasError ? TXT( "error" ) : TXT( "success, saving results" ) );

	CompilationFinished();

	// Compiled
	return !m_hasError;
}

CScriptFunctionStub* CScriptCompiler::FindFunctionStub( CScriptClassStub* parent, const String& name )
{
	for( Uint32 i = 0; i < parent->m_functions.Size(); ++i )
	{
		CScriptFunctionStub* stub = parent->m_functions[ i ];

		if( stub->m_name == name )
		{
			return stub;
		}
	}

	return NULL;
}

CScriptFunctionStub* CScriptCompiler::FindImplementedFunctionInHierarchy( CScriptSystemStub& definitions, CScriptClassStub* classStub, const String& function, CScriptClassStub* stopAtClassStub )
{
	ASSERT( classStub );

	CScriptFunctionStub* functionStub = definitions.FindFunctionStub( classStub, function );

	if( functionStub && !( functionStub->m_flags & FF_UndefinedBody ) )
	{
		return functionStub;
	}

	if ( !classStub->m_extends.Empty() )
	{
		CScriptClassStub* baseClassStub = definitions.FindClassStub( classStub->m_extends );

		if( baseClassStub && baseClassStub != stopAtClassStub )
		{
			return FindImplementedFunctionInHierarchy( definitions, baseClassStub, function, stopAtClassStub );
		}
	}

	return NULL;
}

Bool CScriptCompiler::CreateTypes( CScriptSystemStub& definitions )
{
	// Create structs
	for ( Uint32 i = 0; i < definitions.m_structs.Size(); ++i )
	{
		CScriptStructStub* stub = definitions.m_structs[ i ];
		if ( !CreateStruct( stub ) )
		{
			return false;
		}
	}

	// Create enums
	for ( Uint32 i = 0; i < definitions.m_enums.Size(); ++i )
	{
		CScriptEnumStub* stub = definitions.m_enums[ i ];
		if ( !CreateEnum( stub ) )
		{
			return false;
		}
	}

	// Create classes
	for ( Uint32 i = 0; i < definitions.m_classes.Size(); ++i )
	{
		CScriptClassStub* stub = definitions.m_classes[ i ];
		if ( !CreateClass( stub ) )
		{
			return false;
		}
	}

	// Bind parent classes to created classes
	for ( Uint32 i = 0; i < definitions.m_classes.Size(); ++i )
	{
		CScriptClassStub* stub = definitions.m_classes[ i ];
		if ( !BindParentClass( definitions, stub ) )
		{
			return false;
		}
	}

	// Bind state classes to state machines
	for ( Uint32 i = 0; i < definitions.m_classes.Size(); ++i )
	{
		CScriptClassStub* stub = definitions.m_classes[ i ];
		if ( !BindStateClass( stub ) )
		{
			return false;
		}
	}

	// Bind parent states to parent state classes
	for ( Uint32 i = 0; i < definitions.m_classes.Size(); ++i )
	{
		CScriptClassStub* stub = definitions.m_classes[ i ];
		if ( !BindStateParentClass( stub ) )
		{
			return false;
		}
	}

	// Done
	return true;
}

Bool CScriptCompiler::CreateStruct( CScriptStructStub* stub )
{
	// Trying to import structure
	CName structName( stub->m_name );
	if ( stub->m_flags & CF_Native )
	{
		CClass* existing = SRTTI::GetInstance().FindClass( structName );
		if ( !existing )
		{
			ScriptError( stub->m_context, TXT("Cannot find structure '%ls' to import"), structName.AsString().AsChar() );
			return true;
		}

		// We can import only native structures
		if ( !existing->IsNative() )
		{
			ScriptError( stub->m_context, TXT("Structure '%ls' is defined but is not native. Cannot import."), structName.AsString().AsChar() );
			return true;
		}

		// Structure was already exported
		if ( existing->IsExported() )
		{
			ScriptError( stub->m_context, TXT("Structure '%ls' was already exported."), structName.AsString().AsChar() );
			return true;
		}

		//  This is a normal class
		if ( existing->IsObject() )
		{
			ScriptError( stub->m_context, TXT("Structure '%ls' was defined as class in C++. Cannot import as structure."), structName.AsString().AsChar() );
			return true;
		}

		// Mark structure as exported
		existing->MarkAsExported();
		stub->m_createdStruct = existing;
		return true;
	}

	// We cannot redefine native structures
	CClass* existing = SRTTI::GetInstance().FindClass( structName );
	if ( existing )
	{
		ScriptError( stub->m_context, TXT("Structure '%ls' already defined."), structName.AsString().AsChar() );
		return true;
	}

	// We cannot create empty structures
	if ( !stub->m_fields.Size() )
	{
		ScriptError( stub->m_context, TXT("Cannot define empty structure '%ls'."), structName.AsString().AsChar() );
		return true;
	}

	// Create scripted class without a base class ( a struct )
	stub->m_createdStruct = SRTTI::GetInstance().CreateScriptedClass( structName, CF_Scripted );
	return true;
}

Bool CScriptCompiler::CreateEnum( CScriptEnumStub* stub )
{
	// We cannot redefine native enums
	CName enumName( stub->m_name );
	CEnum* existing = SRTTI::GetInstance().FindEnum( enumName );
	if ( existing )
	{
		// A native struct
		if ( !existing->IsScripted() )
		{
			ScriptError( stub->m_context, TXT("Cannot redefine native enum '%ls'"), enumName.AsString().AsChar() );
			return true;
		}

		// We cannot redefine scripted structures as well
		ScriptError( stub->m_context, TXT("Enum '%ls' already defined."), enumName.AsString().AsChar() );
		return true;
	}

	// We cannot create empty enum
	if ( !stub->m_options.Size() )
	{
		ScriptError( stub->m_context, TXT("Cannot define empty enum '%ls'."), enumName.AsString().AsChar() );
		return true;
	}

	// Check values
	Uint32 maxIndex = 0;
	Int32 lastIndex = 0;
	Bool hasError = false;
	THashMap< CName, Int32 > enumValues;
	THashMap< Int32, CName > enumNames;
	for ( Uint32 i = 0; i < stub->m_options.Size(); ++i )
	{
		CScriptEnumOptionStub* option = stub->m_options[i];

		// Make sure we don't get duplicated names
		CName enumOptionName( option->m_name );
		if ( enumValues.FindPtr( enumOptionName ) != NULL )
		{
			hasError = true;
			ScriptError( option->m_context, TXT("Value '%ls' already defined."), enumOptionName.AsString().AsChar() );
			continue;
		}

		//  No value, assign value
		if ( option->m_value == INT_MAX )
		{
			option->m_value = lastIndex;
			lastIndex++;
		}
		else if ( option->m_value >= lastIndex )
		{
			lastIndex = option->m_value + 1;
		}

		// Accumulate max value
		maxIndex = ::Max<Int32>( maxIndex, ::Abs<Int32>( option->m_value ) );

		// Make sure value is not already taken
		if ( enumNames.FindPtr( option->m_value ) != NULL )
		{
			hasError = true;
			ScriptError( option->m_context, TXT("Value %i already used."), option->m_value );
			continue;
		}

		// Add to map
		enumValues.Insert( enumOptionName, option->m_value );
		enumNames.Insert( option->m_value, enumOptionName );
	}

	// Create enum if no error occured
	if ( !hasError )
	{
		// Determine enum size
		Uint32 enumSize = 1;
		if ( maxIndex > 65535 )
		{
			enumSize = 4;
		}
		else if ( maxIndex > 255 )
		{
			enumSize = 2;
		}

		// Create scripted struct
		stub->m_createdEnum = SRTTI::GetInstance().CreateScriptedEnum( enumName, enumSize );

		// Add options
		for ( Uint32 i = 0; i < stub->m_options.Size(); ++i )
		{
			CScriptEnumOptionStub* option = stub->m_options[i];
			CName enumOptionName( option->m_name );
			stub->m_createdEnum->Add( enumOptionName, option->m_value );
		}
	}

	// No critical error, continue
	return true;
}

Bool CScriptCompiler::CreateClass( CScriptClassStub* stub )
{
	// Translate state name to something more useful
	CName className( stub->m_name );
	if ( stub->m_isState )
	{
		// Assemble new class name
		String newClassName = String::Printf( TXT( "%lsState%ls" ), stub->m_stateParentClass.AsChar(), stub->m_name.AsChar() );

		// Use new class name for states
		className = CName( newClassName );
	}

	// Trying to import structure
	if ( stub->m_flags & CF_Native )
	{
		CClass* existing = SRTTI::GetInstance().FindClass( className );
		if ( !existing )
		{
			ScriptError( stub->m_context, TXT("Cannot find class '%ls' to import"), className.AsString().AsChar() );
			return true;
		}

		// We can import only native structures
		if ( !existing->IsNative() )
		{
			ScriptError( stub->m_context, TXT("Class '%ls' is defined but is not native. Cannot import."), className.AsString().AsChar() );
			return true;
		}

		// Structure was already exported
		if ( existing->IsExported() )
		{
			ScriptError( stub->m_context, TXT("Class '%ls' was already exported."), className.AsString().AsChar() );
			return true;
		}

		//  This is a structure, not a class
		if ( !existing->HasBaseClass() )
		{
			ScriptError( stub->m_context, TXT("Class '%ls' was defined as a structure in C++. Cannot import as a class."), className.AsString().AsChar() );
			return true;
		}

		// Should we mark the existing class as state machine ?
		if ( stub->m_flags & CF_StateMachine )
		{
			existing->MarkAsStateMachine();
		}

		// Mark structure as exported
		existing->MarkAsExported();
		existing->MarkAsScripted();
		stub->m_createdClass = existing;
		return true;
	}

	// We can redefine native class
	CClass* existing = SRTTI::GetInstance().FindClass( className );
	if ( existing )
	{
		ScriptError( stub->m_context, TXT("Class '%ls' already defined."), className.AsString().AsChar() );
		return true;
	}

	// Create scripted class
	stub->m_createdClass = SRTTI::GetInstance().CreateScriptedClass( className, stub->m_flags | CF_Scripted );
	return true;
}

Bool CScriptCompiler::BindParentClass( CScriptSystemStub& definitions, CScriptClassStub* stub )
{
	// No class... can happen due to errors
	if ( !stub->m_createdClass )
	{
		return true;
	}

	// State classes are not checked here
	if ( stub->m_isState )
	{
		return true;
	}

	// No base class, use object class
	if ( stub->m_extends.Empty() )
	{
		// Non native classes are easy
		if ( !stub->m_createdClass->IsNative() )
		{
			//// Just use IScriptable as a base class
			//CClass* objectClass = ClassID< IScriptable >();
			CClass* objectClass = ClassID< CObject >(); // <- changed to CObject since we are not ready for full switch to IScriptable yet
			stub->m_createdClass->AddParentClass( objectClass );
			return true;
		}

		// Make sure we are not redefining a structure to be a class
		CClass* baseClass = stub->m_createdClass->GetBaseClass();
		if ( !baseClass && stub->m_createdClass != ClassID< IScriptable >() )
		{
			ScriptError( stub->m_context, TXT("Cannot redefine native structure '%ls' to be a class."), stub->m_name.AsChar() );
			return true;
		}

		// Redefined class should be based on IScriptable
		// IScriptable is kind of hacked because it is derived from ISerializable but we don't want the script system to see it
		if ( stub->m_createdClass != IScriptable::GetStaticClass() )
		{
			if ( baseClass && (baseClass != ClassID< CObject >() && baseClass != ClassID< IScriptable >()) )
			{
				ScriptError( stub->m_context, TXT("Native class '%ls' is not based on '%ls'. Change the script declaration to match the C++ one."), stub->m_name.AsChar(), stub->m_extends.AsChar() );
				return true;
			}
		}

		// Seems ok
		return true;
	}

	// Find base class by name
	CName baseClassName( stub->m_extends.AsChar() );
	CClass* baseClass = SRTTI::GetInstance().FindClass( baseClassName );
	if ( !baseClass )
	{
		ScriptError( stub->m_context, TXT("Unknown base class '%ls'"), stub->m_extends.AsChar() );
		return true;
	}

	// We are binding base to a native class it should match
	if ( stub->m_createdClass->IsNative() )
	{
		// Check class
		CClass* nativeBaseClass = stub->m_createdClass->GetBaseClass();
		if ( nativeBaseClass != baseClass )
		{
			if ( nativeBaseClass )
			{
				ScriptError( stub->m_context, TXT("Base class mismatch. Class '%ls' specifies '%ls' as a base but native code uses '%ls'"), stub->m_name.AsChar(), stub->m_extends.AsChar(), nativeBaseClass->GetName().AsString().AsChar() );
				return true;
			}
			else
			{
				ScriptError( stub->m_context, TXT("Trying to extent native type '%ls' that has no base"), stub->m_extends.AsChar() );
				return true;
			}
		}

		// Native class matches, no more work to do
		return true;
	}

	// Check for cyclic dependencies
	if ( baseClass->DynamicIsA( stub->m_createdClass ) ) // we use dynamic IsA because the class hierarchy is being built
	{
		ScriptError( stub->m_context, TXT("Base class '%ls' creates cyclic dependency for '%ls'."), stub->m_extends.AsChar(), stub->m_name.AsChar() );
		return true;
	}

	// We cannot base on abstract class
	if ( baseClass->IsAbstract() && baseClass->IsNative() )
	{
		ScriptError( stub->m_context, TXT("Cannot base class '%ls' on first-order native abstract class '%ls'."), stub->m_name.AsChar(), stub->m_extends.AsChar() );
		return true;
	}

	// If the base class has declared some functions, but not implemented them, see if this class does anything about it
	if ( baseClass->HasUndefinedScriptFunctions() )
	{
		CScriptClassStub* baseClassStub = definitions.FindClassStub( stub->m_extends );

		for( Uint32 i = 0; i < baseClassStub->m_functions.Size(); ++i )
		{
			CScriptFunctionStub* baseFunction = baseClassStub->m_functions[ i ];

			if( baseFunction->m_flags & FF_UndefinedBody )
			{
				CScriptFunctionStub* function = definitions.FindFunctionStub( stub, baseFunction->m_name );

				if( !function || function->m_flags & FF_UndefinedBody )
				{
					stub->m_createdClass->MarkAsIncomplete();

					break;
				}
			}
		}
	}

	// Set base class
	stub->m_createdClass->AddParentClass( baseClass );
	return true;
}

Bool CScriptCompiler::BindStateClass( CScriptClassStub* stub )
{
	// Skip invalid classes
	if ( !stub->m_createdClass )
	{
		return true;
	}

	// Class should be state class
	if ( !stub->m_isState )
	{
		return true;
	}

	// Find the state machine class
	CName stateMachineClassName( stub->m_stateParentClass );
	CClass* stateMachineClass = SRTTI::GetInstance().FindClass( stateMachineClassName );
	if ( !stateMachineClass )
	{
		ScriptError( stub->m_context, TXT("Cannot define state '%ls' in context of unknown class '%ls'"), stub->m_name.AsChar(), stateMachineClassName.AsString().AsChar() );
		return true;
	}

	// Make sure that we are adding state to a state machine
	if ( !stateMachineClass->IsStateMachine() )
	{
		ScriptWarn( stub->m_context, TXT("Adding state '%ls' to class '%ls' which is not a state machine. Did you forget the 'statemachine' keyword in class?"), stub->m_name.AsChar(), stateMachineClassName.AsString().AsChar() );
	}

	// Make sure we do not redefine state. This should not happen...
	CName stateName( stub->m_name );
	CClass* existingState = stateMachineClass->FindStateClass( stateName );
	if ( existingState && existingState->GetStateMachineClass() == stateMachineClass )
	{
		ScriptError( stub->m_context, TXT("State '%ls' is already defined in '%ls'"), stub->m_name.AsChar(), stateMachineClassName.AsString().AsChar() );
		return true;
	}

	// Register as state in state machine class
	stateMachineClass->AddState( stateName, stub->m_createdClass );
	return true;
}

Bool CScriptCompiler::BindStateParentClass( CScriptClassStub* stub )
{
	// No class... can happen due to errors
	if ( !stub->m_createdClass )
	{
		return true;
	}

	// Only state classes are checked here
	if ( !stub->m_isState )
	{
		return true;
	}

	// States without state machines are not checked
	CClass* stateClass = stub->m_createdClass;
	CClass* stateMachineClass = stateClass->GetStateMachineClass();
	if ( !stateMachineClass )
	{
		return true;
	}

	// Custom extend state was given, find it
	CClass* baseStateClass = NULL;
	if ( !stub->m_extends.Empty() )
	{
		// Find base state
		CName stateExtendName( stub->m_extends );
		baseStateClass = stateMachineClass->FindStateClass( stateExtendName );
		if ( !baseStateClass )
		{
			ScriptError( stub->m_context, TXT("Unknown base state '%ls' in context of class '%ls'"), stateExtendName.AsString().AsChar(), stateMachineClass->GetName().AsString().AsChar() );
			return true;
		}

		// Do not allow recursive binding
		if ( baseStateClass->DynamicIsA( stateClass ) || (baseStateClass == stateClass) )
		{
			ScriptError( stub->m_context, TXT("Recursive state definition. '%ls' in already based on '%ls'"), stateExtendName.AsString().AsChar(), stub->m_name.AsChar() );
			return true;
		}
	}
	else 
	{
		// If the state machine class is based on another state machine find the state to extend
		CClass* baseStateMachineClass = stateMachineClass->GetBaseClass();
		if ( baseStateMachineClass )
		{
			// Use state from base class as a base class for state in this class :)
			CName stateName( stub->m_name );
			baseStateClass = baseStateMachineClass->FindStateClass( stateName );
		}
	}

	// By default every state extends the CScriptedState
	if ( !baseStateClass )
	{
		baseStateClass = ClassID< CScriptableState >();
	}

	// If the state class is native make sure class definitions are consistent
	if ( stub->m_createdClass->IsNative() )
	{
		// Check class
		CClass* nativeBaseClass = stub->m_createdClass->GetBaseClass();
		if ( nativeBaseClass != baseStateClass )
		{
			ASSERT( nativeBaseClass );
			ScriptError( stub->m_context, TXT("State '%ls' is based on different class '%ls' in C++."), stub->m_name.AsChar(), nativeBaseClass->GetName().AsString().AsChar() );
			return true;
		}

		// Valid set
		return true;
	}

	// Set base class
	stub->m_createdClass->AddParentClass( baseStateClass );
	return true;
}

Bool CScriptCompiler::CreateFunctions( CScriptSystemStub& definitions )
{
	// Create class functions
	for ( Uint32 i = 0; i < definitions.m_classes.Size(); ++i )
	{
		CScriptClassStub* classStub = definitions.m_classes[ i ];
		for ( Uint32 j = 0; j < classStub->m_functions.Size(); ++j )
		{
			CScriptFunctionStub* funcStub = classStub->m_functions[ j ];
			if ( !CreateClassFunction( classStub, funcStub ) )
			{
				return false;
			}
		}
	}

	// Create global functions
	for ( Uint32 i = 0; i < definitions.m_functions.Size(); ++i )
	{
		CScriptFunctionStub* funcStub = definitions.m_functions[ i ];
		if ( !CreateGlobalFunction( funcStub ) )
		{
			return false;
		}
	}

	// Done
	return true;
}

const Uint32 funcMask = FF_EventFunction | FF_ExecFunction | FF_LatentFunction | FF_FinalFunction | FF_AccessModifiers;

Bool CScriptCompiler::CreateGlobalFunction( CScriptFunctionStub* stub )
{
	// Final keyword has no sense on global functions
	CName functionName( stub->m_name );

	if ( IsStrictMode() && ( stub->m_flags & FF_PrivateFunction ) )
	{
		ScriptError( stub->m_context, TXT("'private' has no sense for global function '%ls'."), functionName.AsString().AsChar() );
		return true;
	}

	if ( IsStrictMode() && ( stub->m_flags & FF_ProtectedFunction ) )
	{
		ScriptError( stub->m_context, TXT("'protected' has no sense for global function '%ls'."), functionName.AsString().AsChar() );
		return true;
	}

	if ( IsStrictMode() && ( stub->m_flags & FF_PublicFunction ) )
	{
		ScriptError( stub->m_context, TXT("'public' has no sense for global function '%ls'."), functionName.AsString().AsChar() );
		return true;
	}

	if ( stub->m_flags & FF_FinalFunction )
	{
		ScriptError( stub->m_context, TXT("'final' has no sense for global function '%ls'."), functionName.AsString().AsChar() );
		return true;
	}

	// Event keyword has no sense on global functions
	if ( stub->m_flags & FF_EventFunction )
	{
		ScriptError( stub->m_context, TXT("'event' has no sense for global function '%ls'."), functionName.AsString().AsChar() );
		return true;
	}

	// Latent keyword has no sense on some functions
	if ( stub->m_flags & FF_LatentFunction && ( stub->m_flags & FF_EntryFunction || stub->m_flags & FF_TimerFunction || stub->m_flags & FF_CleanupFunction || stub->m_flags & FF_EventFunction || stub->m_flags & FF_ExecFunction ) )
	{
		ScriptError( stub->m_context, TXT("'latent' has no sense for entry, timer, event or exec function '%ls'."), functionName.AsString().AsChar() );
		return true;
	}

	// Entry keyword has no sense on global functions
	if ( stub->m_flags & FF_EntryFunction )
	{
		ScriptError( stub->m_context, TXT("'entry' has no sense for global function '%ls'."), functionName.AsString().AsChar() );
		return true;
	}

	// Timer keyword has no sense on global functions
	if ( stub->m_flags & FF_TimerFunction )
	{
		ScriptError( stub->m_context, TXT("'timer' has no sense for global function '%ls'."), functionName.AsString().AsChar() );
		return true;
	}

	// Cleanup keyword has no sense on global functions
	if ( stub->m_flags & FF_CleanupFunction )
	{
		ScriptError( stub->m_context, TXT("'cleanup' has no sense for global function '%ls'."), functionName.AsString().AsChar() );
		return true;
	}

	// Scene functions require first parameter to be special :)
	if ( stub->m_flags & FF_SceneFunction )
	{
		if ( stub->m_params.Size() < 1 )
		{
			ScriptError( stub->m_context, TXT("Scene function '%ls' should take at least one parameter."), functionName.AsString().AsChar() );
			return true;
		}

		if ( stub->m_params[0]->m_typeName != TXT("CStoryScenePlayer") )
		{
			ScriptError( stub->m_context, TXT("Scene function '%ls' should take CStoryScenePlayer as first parameter."), functionName.AsString().AsChar() );
			return true;
		}
	}

	// Make sure we don't override function in the same class
	CFunction* testFunction = SRTTI::GetInstance().FindGlobalFunction( functionName );
	if ( testFunction )
	{
		// Non native functions cannot be redefined
		if ( !testFunction->IsNative() )
		{
			ScriptError( stub->m_context, TXT("Global function '%ls' is already defined."), functionName.AsString().AsChar() );
			return true;
		}

		// This is a native function, we cannot export native functions twice
		if ( testFunction->IsExported() )
		{
			ScriptError( stub->m_context, TXT("Global native function '%ls' was already exported."), functionName.AsString().AsChar() );
			return true;
		}

		// Native function cannot have code
		if ( !stub->m_code.IsEmpty() )
		{
			ScriptError( stub->m_context, TXT("Global native function '%ls' cannot have script code."), functionName.AsString().AsChar() );
			return true;
		}

		// Mark as exported
		testFunction->m_flags |= FF_ExportedFunction;	
		testFunction->m_flags = ( testFunction->m_flags & ~funcMask ) | ( stub->m_flags	& funcMask );
		stub->m_createdFunction = testFunction;
		return true;
	}

	// Make sure function is static
	ASSERT( stub->m_flags & FF_StaticFunction );

	// A native function was not found, well, it's an error
	if ( stub->m_flags & FF_NativeFunction )
	{
		ScriptWarn( stub->m_context, TXT("Global native function '%ls' was not exported from C++ code."), functionName.AsString().AsChar() );
	}

	// Create function stub
	CFunction* func = new CFunction( NULL, functionName, stub->m_flags );
	stub->m_createdFunction = func;
	SRTTI::GetInstance().RegisterGlobalFunction( func );
	return true;
}

Bool CScriptCompiler::CreateClassFunction( CScriptClassStub* classStub, CScriptFunctionStub* stub )
{
	// Special check for event functions
	CName functionName( stub->m_name );

	if ( IsStrictMode() && ! IsPow2( stub->m_flags & FF_AccessModifiers ) )
	{
		ScriptError( stub->m_context, TXT("Function '%ls' has multiple access modifiers."), functionName.AsString().AsChar() );
		return true;
	}

	// Default permission for class functions
	if ( ( stub->m_flags & FF_AccessModifiers ) == 0 )
	{
		stub->m_flags |= FF_PublicFunction;
	}

	if ( stub->m_flags & FF_EventFunction )
	{
		// Event functions cannot return value
		if ( stub->m_retValue != NULL )
		{
			ScriptError( stub->m_context, TXT("Event function '%ls' cannot return implicit a value. By default they return bool."), functionName.AsString().AsChar() );
			return true;
		}

		// Event function names should start with "On"
		if ( !stub->m_name.BeginsWith( TXT("On") ) )
		{
			ScriptError( stub->m_context, TXT("Event function '%ls' name should start with \"On\"."), functionName.AsString().AsChar() );
			return true;
		}

		// Event functions cannot be latent
		if ( stub->m_flags & FF_LatentFunction )
		{
			ScriptError( stub->m_context, TXT("Event function '%ls' cannot be latent."), functionName.AsString().AsChar() );
			return true;
		}

		// State entry functions cannot be events
		if ( stub->m_flags & FF_EntryFunction )
		{
			ScriptError( stub->m_context, TXT("Event function '%ls' cannot be state entry function."), functionName.AsString().AsChar() );
			return true;
		}

		// Timer functions cannot be events
		if ( stub->m_flags & FF_TimerFunction )
		{
			ScriptError( stub->m_context, TXT("Event function '%ls' cannot be timer function."), functionName.AsString().AsChar() );
			return true;
		}

		// Cleanup functions cannot be events
		if ( stub->m_flags & FF_CleanupFunction )
		{
			ScriptError( stub->m_context, TXT("Event function '%ls' cannot be cleanup function."), functionName.AsString().AsChar() );
			return true;
		}

		// Event functions returns bool by default
		const String retType = ::GetTypeName< Bool >().AsString();
		stub->m_retValue = new CScriptPropertyStub( stub->m_context, TXT("__return"), PF_FuncRetValue, retType );
	}

	// We cannot define entry function outside of state
	CClass* theClass = classStub->m_createdClass;
	if ( !theClass->IsState() && ( stub->m_flags & FF_EntryFunction ) )
	{
		ScriptError( stub->m_context, TXT("State entry function '%ls' cannot be declared outside of state."), functionName.AsString().AsChar() );
		return true;
	}

	if ( !theClass->IsState() && ( stub->m_flags & FF_CleanupFunction ) )
	{
		ScriptError( stub->m_context, TXT("Cleanup function '%ls' cannot be declared outside of state."), functionName.AsString().AsChar() );
		return true;
	}

	// Entry function cannot return a value
	if ( stub->m_flags & FF_EntryFunction && stub->m_retValue )
	{
		ScriptError( stub->m_context, TXT("State entry function '%ls' cannot return anything."), functionName.AsString().AsChar() );
		return true;
	}

	// We cannot define exec functions in classes
	if ( stub->m_flags & FF_ExecFunction )
	{
		ScriptError( stub->m_context, TXT("Exec function '%ls' cannot be declared inside a class."), functionName.AsString().AsChar() );
		return true;
	}

	// Make sure we don't override function in the same class
	const CFunction* testFunction = theClass->FindFunctionNonCached( functionName );
	if ( testFunction && testFunction->GetClass() == theClass )
	{
		// Function found, but different nativity
		if ( !testFunction->IsNative() && stub->m_flags & FF_NativeFunction )
		{
			ScriptWarn( stub->m_context, TXT("Native function '%ls' was not exported from class '%ls' in C++ code."), functionName.AsString().AsChar(), theClass->GetName().AsString().AsChar() );
		}

		// Non native functions cannot be redefined
		if ( !testFunction->IsNative() )
		{
			ScriptError( stub->m_context, TXT("Function '%ls' is already defined in class '%ls'."), functionName.AsString().AsChar(), theClass->GetName().AsString().AsChar() );
			return true;
		}

		// This is a native function, we cannot export native functions twice
		if ( testFunction->IsExported() )
		{
			ScriptError( stub->m_context, TXT("Native function '%ls' was already exported."), functionName.AsString().AsChar() );
			return true;
		}

		// Native function cannot have code
		if ( !stub->m_code.IsEmpty() )
		{
			ScriptError( stub->m_context, TXT("Native function '%ls' cannot have script code."), functionName.AsString().AsChar() );
			return true;
		}

		// Native functions cannot be state entry function
		if ( stub->m_flags & FF_EntryFunction )
		{
			ScriptError( stub->m_context, TXT("Native function '%ls' cannot be state entry function."), functionName.AsString().AsChar() );
			return true;
		}

		// Native functions cannot be timer functions
		if ( stub->m_flags & FF_TimerFunction )
		{
			ScriptError( stub->m_context, TXT("Time function '%ls' cannot be declared inside a native class."), functionName.AsString().AsChar() );
			return true;
		}

		// Native functions cannot be cleanup functions
		if ( stub->m_flags & FF_CleanupFunction )
		{
			ScriptError( stub->m_context, TXT("Cleanup function '%ls' cannot be declared inside a native class."), functionName.AsString().AsChar() );
			return true;
		}

		// Mark as exported
		stub->m_createdFunction = const_cast< CFunction* >( testFunction );
		stub->m_createdFunction->m_flags |= FF_ExportedFunction;
		stub->m_createdFunction->m_flags = ( testFunction->m_flags & ~funcMask ) | ( stub->m_flags	& funcMask );
		return true;
	}
	else if ( IsStrictMode() && testFunction )
	{
		if ( testFunction->m_flags & FF_FinalFunction )
		{
			ScriptError( stub->m_context, TXT("Cannot override function '%ls' declared 'final' in class '%ls'."), functionName.AsString().AsChar(), testFunction->GetClass()->GetName().AsString().AsChar() );
			return true;
		}

		if ( ( stub->m_flags & FF_AccessModifiers ) != FF_PrivateFunction && ( testFunction->m_flags & FF_AccessModifiers ) == FF_PrivateFunction )
		{
			ScriptError( stub->m_context, TXT("Function '%ls' cannot have a weaker access modifier than in ancestor class '%ls'."), functionName.AsString().AsChar(), testFunction->GetClass()->GetName().AsString().AsChar() );
			return true;
		}
		
		if ( ( stub->m_flags & ( FF_ProtectedFunction | FF_PrivateFunction ) ) == 0 && ( testFunction->m_flags & FF_AccessModifiers ) == FF_ProtectedFunction )
		{
			ScriptError( stub->m_context, TXT("Function '%ls' cannot have a weaker access modifier than in ancestor class '%ls'."), functionName.AsString().AsChar(), testFunction->GetClass()->GetName().AsString().AsChar() );
			return true;
		}
	}

	// Make sure function is not static
	ASSERT( 0 == ( stub->m_flags & FF_StaticFunction ) );

	// A native function was not found, well, it's an error
	if ( stub->m_flags & FF_NativeFunction )
	{
		ScriptWarn( stub->m_context, TXT("Native function '%ls' was not exported from class '%ls' in C++ code."), functionName.AsString().AsChar(), theClass->GetName().AsString().AsChar() );
		stub->m_flags &= ~FF_NativeFunction;
	}

	// Only native functions can be latent
	if ( stub->m_flags & FF_LatentFunction )
	{
		//ScriptError( stub->m_context, TXT("Non native function '%ls' cannot be latent."), functionName.AsChar() );
		//return true;
	}

	// Special checks for timer functions
	if ( stub->m_flags & FF_TimerFunction )
	{
		// We cannot return anything
		if ( stub->m_retValue )
		{
			ScriptError( stub->m_context, TXT("Timer function '%ls' cannot return annything."), functionName.AsString().AsChar() );
			return true;
		}

		// There should be only two parameters
		if ( stub->m_params.Size() != 2 )
		{
			ScriptError( stub->m_context, TXT("Timer function '%ls' should take exactly two parameters (id and deltaTime)."), functionName.AsString().AsChar() );
			return true;
		}

		// Parameter type should be float
		if ( ( stub->m_params[0]->m_typeName != TXT("Float") && stub->m_params[0]->m_typeName != TXT("GameTime") ) ||	// time delta
			 stub->m_params[1]->m_typeName != TXT("Int32") )															// timer id
		{
			ScriptError( stub->m_context, TXT("Timer function '%ls' should take float/gametime parameter (delta time) and int parameter (id)."), functionName.AsString().AsChar() );
			return true;
		}
	}

	// Special checks for cleanup functions
	if ( stub->m_flags & FF_CleanupFunction )
	{
		// We cannot return anything
		if ( stub->m_retValue )
		{
			ScriptError( stub->m_context, TXT("Cleanup function '%ls' cannot return annything."), functionName.AsString().AsChar() );
			return true;
		}

		// There should be no parameters
		if ( stub->m_params.Size() != 0 )
		{
			ScriptError( stub->m_context, TXT("Cleanup function '%ls' should take no parameters."), functionName.AsString().AsChar() );
			return true;
		}
	}

	// Create function stub
	CFunction* func = new CFunction( theClass, functionName, stub->m_flags );
	stub->m_createdFunction = func;
	theClass->AddFunction( func );
	return true;
}

Bool CScriptCompiler::ConvertRawType( IRTTIType* rawType, String& scriptTypeName )
{
	// Simple type, use directly
	ERTTITypeType type = rawType->GetType();
	if ( type == RT_Simple || type == RT_Enum || type == RT_Fundamental )
	{
		scriptTypeName = rawType->GetName().AsString();
		return true;
	}

	// Array type, recurse
	if ( type == RT_Array )
	{
		CRTTIArrayType* arrayType = static_cast< CRTTIArrayType* >( rawType );
		if ( !ConvertRawType( arrayType->GetInnerType(), scriptTypeName ) )
		{
			return false;
		}

		// Append array prefix
		scriptTypeName = String::Printf
		(
			TXT("array:%d,%d,%ls"), 
			(Int32)arrayType->GetMemoryClass(), 
			(Int32)arrayType->GetMemoryPool(), 
			scriptTypeName.AsChar()
		);

		return true;
	}

	// Static array type
	if ( type == RT_NativeArray )
	{
		CRTTINativeArrayType* arrayType = static_cast< CRTTINativeArrayType* >( rawType );
		if ( !ConvertRawType( arrayType->GetInnerType(), scriptTypeName ) )
		{
			return false;
		}

		// Append array prefix
		scriptTypeName = String::Printf
		(
			TXT("[%d]%ls"), 
			arrayType->GetElementCount(), 
			scriptTypeName.AsChar()
		);
		return true;
	}

	// Class
	if ( type == RT_Class )
	{
		CClass* theClass = ( CClass* ) rawType;

		// Struct
		if ( !theClass->HasBaseClass() && theClass != ClassID< IScriptable >() )
		{
			scriptTypeName = theClass->GetName().AsString();
			return true;
		}

		// Pointer, use handles
		scriptTypeName = String::Printf( TXT( "handle:%ls" ), theClass->GetName().AsString().AsChar() );
		return true;
	}

	// Invalid type
	return false;
}

CClass* FindFirstNativeBaseClass( CClass* anyClass )
{
	if ( anyClass && anyClass->IsNative() )
		return anyClass;

	if ( anyClass && anyClass->HasBaseClass() )
		return FindFirstNativeBaseClass( anyClass->GetBaseClass() );

	return nullptr;
}

CProperty* CScriptCompiler::CreateProperty( CScriptPropertyStub* stub, CClass* parentClass, Uint32 extraFlags )
{
	// Find raw type
	CName typeName( stub->m_typeName.AsChar() );
	IRTTIType* rawType = SRTTI::GetInstance().FindType( typeName );
	if ( !rawType )
	{
		ScriptError( stub->m_context, TXT("Unknown type '%ls' for property '%ls'."), typeName.AsString().AsChar(), stub->m_name.AsChar() );
		return NULL;
	}

	// Convert type name to script type
	String scriptTypeName;
	if ( !ConvertRawType( rawType, scriptTypeName ) )
	{
		ScriptError( stub->m_context, TXT("Unable to convert type '%ls' for property '%ls' to script type."), typeName.AsString().AsChar(), stub->m_name.AsChar() );
		return NULL;
	}

	// Find script type
	IRTTIType* scriptType = SRTTI::GetInstance().FindType( CName( scriptTypeName ) );
	if ( !scriptType )
	{
		ScriptError( stub->m_context, TXT("Unknown script type '%ls' for property '%ls'."), scriptTypeName.AsChar(), stub->m_name.AsChar() );
		return NULL;
	}

	// Validate type for bindable property
	if ( stub->m_flags & PF_AutoBind )
	{
		// only handles for now
		if ( scriptType->GetType() != RT_Handle )
		{
			ScriptError( stub->m_context, TXT("Auto bindable property '%ls' uses unsuported type '%ls'."), 
				stub->m_name.AsChar(), scriptTypeName.AsChar() );
			return NULL;
		}

		// auto bindable properties are not allowed outside classes
		CClass* nativeClass = FindFirstNativeBaseClass( parentClass );
		if ( !nativeClass )
		{
			ScriptError( stub->m_context, TXT("Auto bindable properties ('%ls') can be used only inside a class."), 
				stub->m_name.AsChar(), scriptTypeName.AsChar() );
			return NULL;
		}

		// auto bindable properties are only allowed in scriptable classes
		if ( !nativeClass->IsA< IScriptable >() )
		{
			ScriptError( stub->m_context, TXT("Auto bindable properties ('%ls') can be used only inside scriptable classes."), 
				stub->m_name.AsChar(), scriptTypeName.AsChar() );
			return NULL;
		}

		// limitation: no abstract classes :(
		if ( nativeClass->IsAbstract() )
		{
			ScriptError( stub->m_context, TXT("Auto bindable properties ('%ls') can be used only inside a non-abstract classes."), 
				stub->m_name.AsChar(), scriptTypeName.AsChar() );
			return NULL;
		}


		// validate the type with the parent class		
		IScriptable* realObject = nativeClass->GetDefaultObject< IScriptable >();
		if ( !realObject || !realObject->IsAutoBindingPropertyTypeSupported( scriptType ) )
		{
			ScriptError( stub->m_context, TXT("Auto bindable property '%ls' uses type '%ls' that is not allowed by class '%ls'."), 
				stub->m_name.AsChar(), scriptTypeName.AsChar(), nativeClass->GetName().AsChar() );
			return NULL;
		}
	}

	// Validate type for saved property
	if ( stub->m_flags & PF_Saved )
	{
		static CClass* nodeClass = SRTTI::GetInstance().FindClass( CName( TXT("CNode") ) );
		RED_ASSERT( nodeClass );

		if ( scriptType->IsPointerType() )
		{
			CClass* pointedClass = static_cast< const IRTTIPointerTypeBase* >( scriptType )->GetPointedType();
			RED_ASSERT( pointedClass );

			if ( pointedClass->IsBasedOn( nodeClass ) )
			{
				ScriptWarn( stub->m_context, TXT("Saved property '%ls' of type '%ls' won't be saved because it references CNode. Use EntityHandle instead if that's an entity or remove saved property attribute."), 
					stub->m_name.AsChar(), scriptTypeName.AsChar() );
			}
		}
		else if ( scriptType->IsArrayType() )
		{
			IRTTIType* innerType = static_cast< const IRTTIBaseArrayType* >( scriptType )->ArrayGetInnerType();
			if ( CClass* innerPointedClass = ( innerType->IsPointerType() ? static_cast< const IRTTIPointerTypeBase* >( innerType )->GetPointedType() : nullptr ) )
			{
				if ( innerPointedClass->IsBasedOn( nodeClass ) )
				{
					ScriptWarn( stub->m_context, TXT("Saved property '%ls' of type '%ls' won't be saved because it references CNode. Use EntityHandle instead if that's an entity or remove saved property attribute."), 
						stub->m_name.AsChar(), scriptTypeName.AsChar() );
				}
			}
		}
	}

	// Create property
	const Uint32 propertyFlags = stub->m_flags | extraFlags;
	CProperty* prop = new CProperty( scriptType, parentClass, 0, CName( stub->m_name ), stub->m_hint, propertyFlags );
	return prop;	
}

Bool CScriptCompiler::CreateStructProperties( CScriptStructStub* stub )
{
	// Create properties
	CClass* createdStruct = stub->m_createdStruct;
	const Uint32 numFields = stub->m_fields.Size();
	for ( Uint32 i = 0; i < numFields; ++i )
	{
		CScriptPropertyStub* propStub = stub->m_fields[ i ];

		if ( IsStrictMode() )
		{
			if ( ! IsPow2( propStub->m_flags & PF_AccessModifiers ) )
			{
				ScriptError( propStub->m_context, TXT("Property '%ls' has multiple access modifiers."), propStub->m_name.AsChar() );
				continue;
			}

			if ( ( propStub->m_flags & PF_Private ) != 0 )
			{
				ScriptError( propStub->m_context, TXT("'private' has no sense for property '%ls' in struct '%ls'."), propStub->m_name.AsChar(), createdStruct->GetName().AsString().AsChar() );
				continue;
			}

			if ( ( propStub->m_flags & PF_Protected ) != 0 )
			{
				ScriptError( propStub->m_context, TXT("'protected' has no sense for property '%ls' in struct '%ls'."), propStub->m_name.AsChar(), createdStruct->GetName().AsString().AsChar() );
				continue;
			}

			if ( ( propStub->m_flags & PF_Public ) != 0 )
			{
				ScriptError( propStub->m_context, TXT("'public' has no sense for property '%ls' in struct '%ls'."), propStub->m_name.AsChar(), createdStruct->GetName().AsString().AsChar() );
				continue;
			}
		}

		// Exported property, make sure it fits the format
		CName propName( propStub->m_name );
		if ( propStub->m_flags & PF_Native )
		{
			// Get existing property
			CProperty* existingProp = createdStruct->FindProperty( propName );
			if ( !existingProp )
			{
				ScriptError( propStub->m_context, TXT("Property '%ls' does not exist in struct '%ls'. Not imported."), propName.AsString().AsChar(), createdStruct->GetName().AsString().AsChar() );
				continue;
			}

			// Already exported
			if ( existingProp->IsExported() )
			{
				ScriptError( propStub->m_context, TXT("Property '%ls' from '%ls' was alraedy imported."), propName.AsString().AsChar(), createdStruct->GetName().AsString().AsChar() );
				continue;
			}

			// Create dummy property
			CProperty* tempProp = CreateProperty( propStub, createdStruct, 0 );
			if ( !tempProp )
			{
				continue;
			}

			// Check type
			if ( !CheckTypeCompatibility( existingProp->GetType(), tempProp->GetType() ) )
			{
				ScriptError( propStub->m_context, TXT("Imported property '%ls' from '%ls' has type '%ls' not '%ls'."), propName.AsString().AsChar(), createdStruct->GetName().AsString().AsChar(), existingProp->GetType()->GetName().AsString().AsChar(), tempProp->GetType()->GetName().AsString().AsChar() );
				delete tempProp;
				continue;
			}

			// Mark property as exported
			delete tempProp;
			CProperty::ChangePropertyFlag( createdStruct, existingProp->GetName(), 0, PF_Exported );
			continue;
		}

		// Structure is an imported structure, we cannot add new properties
		if ( createdStruct->IsExported() )
		{
			// Exported structure should be native
			ASSERT( createdStruct->IsNative() );

			// Bail with error
			ScriptError( propStub->m_context, TXT("Cannot add scripted property '%ls' to imported structure '%ls'."), propName.AsString().AsChar(), createdStruct->GetName().AsString().AsChar() );
			continue;
		}

		// Make sure such property is not already defined
		if ( createdStruct->FindProperty( propName ) )
		{
			ScriptError( propStub->m_context, TXT("Property '%ls' already exists in struct '%ls'"), propName.AsString().AsChar(), createdStruct->GetName().AsString().AsChar() );
			continue;
		}

		// We cannot add properties to native structured 
		ASSERT( !createdStruct->IsNative() );

		// Create property
		CProperty* prop = CreateProperty( propStub, createdStruct, 0 );
		if ( prop )
		{
			createdStruct->AddProperty( prop );
		}
	}

	// Done
	return true;
}

Bool CScriptCompiler::CreateClassProperties( CScriptClassStub* stub )
{
	// Create properties
	CClass* createdClass = stub->m_createdClass;
	for ( Uint32 i=0; i<stub->m_properties.Size(); i++ )
	{
		CScriptPropertyStub* propStub = stub->m_properties[i];
		
		if ( IsStrictMode() && ! IsPow2( propStub->m_flags & PF_AccessModifiers ) )
		{
			ScriptError( propStub->m_context, TXT("Property '%ls' has multiple access modifiers."), propStub->m_name.AsChar() );
			continue;
		}

		// Default class property access modifier
		if ( ( propStub->m_flags & PF_AccessModifiers ) == 0 )
		{
			propStub->m_flags |= PF_Public;
		}

		// Exported property, make sure it fits the format
		CName propName( propStub->m_name );
		if ( propStub->m_flags & PF_Native )
		{
			// Get existing property
			CProperty* existingProp = createdClass->FindProperty( propName );
			if ( !existingProp )
			{
				ScriptError( propStub->m_context, TXT("Property '%ls' does not exist in class '%ls'. Not imported."), propName.AsString().AsChar(), createdClass->GetName().AsString().AsChar() );
				continue;
			}

			// Already exported
			if ( existingProp->IsExported() )
			{
				ScriptError( propStub->m_context, TXT("Property '%ls' from '%ls' was alraedy imported."), propName.AsString().AsChar(), createdClass->GetName().AsString().AsChar() );
				continue;
			}

			// Create dummy property
			CProperty* tempProp = CreateProperty( propStub, createdClass, 0 );
			if ( !tempProp )
			{
				continue;
			}

			// Check type
			if ( !CheckTypeCompatibility( existingProp->GetType(), tempProp->GetType() ) )
			{
				ScriptError( propStub->m_context, TXT("Imported property '%ls' from '%ls' has type '%ls' not '%ls'."), propName.AsString().AsChar(), createdClass->GetName().AsString().AsChar(), existingProp->GetType()->GetName().AsString().AsChar(), tempProp->GetType()->GetName().AsString().AsChar() );
				delete tempProp;
				continue;
			}

			// Mark property as exported
			const Uint32 importFlags = propStub->m_flags & PF_AccessModifiers;
			CProperty::ChangePropertyFlag( createdClass, existingProp->GetName(), PF_AccessModifiers, PF_Exported | importFlags );
			continue;
		}

		// Make sure such property is not already defined
		CProperty* existingProp = createdClass->FindProperty( propName );
		if ( existingProp )
		{
			// If it's a bindable property we ALLOW overloading - but there are some restrictions
			const Bool isAutoBindable = 0 != (propStub->m_flags & PF_AutoBind); 
			if ( isAutoBindable )
			{
				// make sure we have binding information
				if ( propStub->m_binding.Empty() )
				{
					ScriptError( propStub->m_context, TXT("Auto bindable property '%ls' has no binding information"), 
						propName.AsString().AsChar() );

					continue;
				}

				// auto bind property overrides a non bindable one - not legal
				if ( !existingProp->IsAutoBindable() )
				{
					ScriptError( propStub->m_context, TXT("Auto bindable property '%ls' overrides non-bindable property '%ls' from class '%ls'"), 
						propName.AsString().AsChar(), propName.AsString().AsChar(), 
						createdClass->GetName().AsString().AsChar() );

					continue;
				}
			}
			else
			{
				if ( existingProp->IsAutoBindable() )
				{
					// normal property overrides a bindable one - not legal
					ScriptError( propStub->m_context, TXT("Property '%ls' overrides bindable property '%ls' from class '%ls'. If you want to override the binding just redeclare the property."), 
						propName.AsString().AsChar(), propName.AsString().AsChar(), 
						createdClass->GetName().AsString().AsChar() );					

					continue;
				}
				else
				{
					// just generic problem
					const CName propertyOwnerName = existingProp->GetParent() ? existingProp->GetParent()->GetName() : CName::NONE;
					ScriptError( propStub->m_context, TXT("Property '%ls' already exists in class '%ls'"), propName.AsString().AsChar(), propertyOwnerName.AsString().AsChar() );
					continue;
				}
			}
		}

		// Create property
		CProperty* prop = CreateProperty( propStub, createdClass, PF_Scripted );
		if ( prop )
		{
			// add binding information
			if ( prop->IsAutoBindable() )
			{
				createdClass->AddPropertyBinding( prop->GetName(), CName( propStub->m_binding.AsChar() ) );
			}

			// extra checks for property binding
			if ( existingProp && prop->IsAutoBindable() )
			{
				RED_ASSERT( existingProp->IsAutoBindable() );

				// Get the types
				const IRTTIType* currentType = prop->GetType();
				const IRTTIType* baseType = existingProp->GetType();
				RED_ASSERT( currentType->GetType() == RT_Handle );
				RED_ASSERT( baseType->GetType() == RT_Handle );

				// Check class compatibility
				const CClass* currentClass = static_cast< const CRTTIHandleType* >( currentType )->GetPointedType();
				const CClass* baseClass = static_cast< const CRTTIHandleType* >( baseType )->GetPointedType();
				if ( !currentClass->IsA( baseClass ) )
				{
					ScriptError( propStub->m_context, TXT("Auto bind property '%ls' uses class '%ls' which is not compatible in the base declaration in class '%ls' where it's using a class '%ls'"), 
						propName.AsString().AsChar(), currentClass->GetName().AsChar(), 
						existingProp->GetParent()->GetName().AsChar(), 
						baseClass->GetName().AsChar() );

					continue;
				}

				// DO NOT ADD new property - use the existing one
				delete prop;
				continue;
			}

			// add new property
			createdClass->AddProperty( prop );
		}
	}

	// Create properties in functions
	for ( Uint32 i = 0; i < stub->m_functions.Size(); ++i )
	{
		CScriptFunctionStub* funcStub = stub->m_functions[i];
		if ( !CreateFunctionProperties( funcStub ) )
		{
			return false;
		}
	}

	// Done
	return true;
}

Bool CScriptCompiler::CreateFunctionProperties( CScriptFunctionStub* stub )
{
	CFunction* func = stub->m_createdFunction;
	ASSERT( func );

	// Create return type
	if ( stub->m_retValue )
	{
		CProperty* retProperty = CreateProperty( stub->m_retValue, NULL, PF_FuncRetValue );
		if ( retProperty )
		{
			func->m_returnProperty = retProperty;
		}
	}

	// Create parameters
	for ( Uint32 i = 0; i < stub->m_params.Size(); ++i )
	{
		CScriptPropertyStub* propStub = stub->m_params[i];

		// Make sure it's not duplicated
		CName propName( propStub->m_name );
		if ( func->FindProperty( propName ) )
		{
			ScriptError( propStub->m_context, TXT("Parameter '%ls' is already defined"), propName.AsString().AsChar() );
			continue;
		}

		ASSERT( ( propStub->m_flags & PF_AccessModifiers ) == 0 );

		// Create property
		CProperty* prop = CreateProperty( propStub, NULL, PF_FuncParam );
		if ( prop )
		{
			func->m_parameters.PushBack( prop );
		}
	}

	// Native function cannot have local variables
	if ( stub->m_flags & FF_NativeFunction )
	{
		if ( stub->m_locals.Size() )
		{
			ScriptError( stub->m_context, TXT("Native function '%ls' cannot have local variables"), stub->m_name.AsChar() );
			return true;
		}
	}

	// Create local variables
	for ( Uint32 i = 0; i < stub->m_locals.Size(); ++i )
	{
		CScriptPropertyStub* propStub = stub->m_locals[i];

		// Make sure it's not duplicated
		CName propName( propStub->m_name );
		if ( func->FindProperty( propName ) )
		{
			ScriptError( propStub->m_context, TXT("Variable '%ls' is already defined"), propName.AsString().AsChar() );
			continue;
		}

		if ( IsStrictMode() )
		{
			if ( ( propStub->m_flags & PF_AccessModifiers) != 0 )
			{
				ScriptError( propStub->m_context, TXT("Local variable '%ls' in function '%ls' has an access modifier."), propName.AsString().AsChar(), func->GetName().AsString().AsChar() );
				continue;
			}
		}

		// Create property
		CProperty* prop = CreateProperty( propStub, NULL, PF_FuncLocal );
		if ( prop )
		{
			func->m_localVars.PushBack( prop );
		}
	}

	// No fatal errors
	return true;
}

Bool CScriptCompiler::CreateProperties( CScriptSystemStub& definitions )
{
	// Create struct properties
	for ( Uint32 i = 0; i < definitions.m_structs.Size(); ++i )
	{
		CScriptStructStub* stub = definitions.m_structs[i];
		if ( !CreateStructProperties( stub ) )
		{
			return false;
		}
	}

	// Create class properties
	for ( Uint32 i = 0; i < definitions.m_classes.Size(); ++i )
	{
		CScriptClassStub* stub = definitions.m_classes[i];
		if ( !CreateClassProperties( stub ) )
		{
			return false;
		}
	}

	// Create properties in global functions
	for ( Uint32 i = 0; i < definitions.m_functions.Size(); ++i )
	{
		CScriptFunctionStub* funcStub = definitions.m_functions[i];
		if ( !CreateFunctionProperties( funcStub ) )
		{
			return false;
		}
	}

	// Done
	return true;
}

static CName GetPropertyTypeName( CProperty* prop )
{
	if ( !prop )
	{
		return CNAME( void );
	}
	else
	{
		return prop->GetType()->GetName();
	}
}

Bool CScriptCompiler::CheckTypeCompatibility( const IRTTIType* nativeType, const IRTTIType* importType ) const
{
	// types are the same
	if ( nativeType == importType )
	{
		return true;
	}

	// special case for dynamic arrays: on the C++ side we use MemoryClass and MemoryPool to specialize the array type more
	// on the script side we don't want to see this that much, therefore we hide the values
	if ( nativeType->GetType() == RT_Array && importType->GetType() == RT_Array )
	{
		const CRTTIArrayType* nativeArrayType = static_cast< const CRTTIArrayType* > ( nativeType );
		const CRTTIArrayType* importArrayType = static_cast< const CRTTIArrayType* > ( importType );
		return CheckTypeCompatibility( nativeArrayType->GetInnerType(), importArrayType->GetInnerType() );
	}

	// types do not match
	return false;
}

Bool CScriptCompiler::MatchFunctionHeader( CScriptFunctionStub* baseContextStub, const CFunction* base, const CFunction* super )
{
	ASSERT( super );
	ASSERT( base );

	// Different return type
	CName baseRetType = GetPropertyTypeName( base->m_returnProperty );
	CName superRetType = GetPropertyTypeName( super->m_returnProperty );
	if ( baseRetType != superRetType )
	{
		ScriptError( baseContextStub->m_context, TXT("Function '%ls' has different return type '%ls' than base function (%ls)."), baseContextStub->m_name.AsChar(), baseRetType.AsString().AsChar(), superRetType.AsString().AsChar() );
		return false;
	}

	// Check parameter count
	if ( base->m_parameters.Size() != super->m_parameters.Size() )
	{
		ScriptError( baseContextStub->m_context, TXT("Function '%ls' takes %i parameter(s) which is inconsistent with base function (%i)."), baseContextStub->m_name.AsChar(), base->m_parameters.Size(), super->m_parameters.Size() );
		return false;
	}

	// Check parameter types
	Bool isMatching = true;
	const Uint32 numParams = base->m_parameters.Size();
	for ( Uint32 i = 0; i < numParams; ++i )
	{
		CName baseType = GetPropertyTypeName( base->m_parameters[i] );
		CName superType = GetPropertyTypeName( super->m_parameters[i] );
		if ( baseType != superType )
		{
			isMatching = false;
			ScriptError( baseContextStub->m_context, TXT("Function '%ls' parameter '%ls' has different type '%ls' than in base function (%ls)."), baseContextStub->m_name.AsChar(), base->m_parameters[i]->GetName().AsString().AsChar(), baseType.AsString().AsChar(), superType.AsString().AsChar() );
			continue;
		}
	}

	// Same functions
	return isMatching;
}

Bool CScriptCompiler::BindSuperFunctions( CScriptFunctionStub* stub )
{
	// Get the class function is defined in
	CClass* baseClass = stub->m_createdFunction->GetClass();
	if ( !baseClass )
	{
		return true;
	}

	// Get the base class
	CClass* superClass = baseClass->GetBaseClass();
	if ( !superClass )
	{
		return true;
	}

	// Find the function with the same name
	CName functionName( stub->m_name );
	const CFunction* superFunction = superClass->FindFunctionNonCached( functionName );
	if ( superFunction )
	{
		// We are trying to override a final function
		if ( superFunction->IsFinal() )
		{
			ScriptError( stub->m_context, TXT("Function '%ls' in class '%ls' is marked as final. Cannot override."), functionName.AsString().AsChar(), superFunction->GetClass()->GetName().AsString().AsChar() );
			return true;
		}

		// We are trying to override a timer function
		if ( superFunction->IsTimer() )
		{
			ScriptError( stub->m_context, TXT("Function '%ls' in class '%ls' is a timer function. Cannot override it directly."), functionName.AsString().AsChar(), superFunction->GetClass()->GetName().AsString().AsChar() );
			return true;
		}

		// Latent function flag should match
		if ( superFunction->IsLatent() != stub->m_createdFunction->IsLatent() )
		{
			if ( superFunction->IsLatent() )
			{
				ScriptError( stub->m_context, TXT("Function '%ls' was declared as latent function in the class '%ls'. Cannot override as normal function."), functionName.AsString().AsChar(), superFunction->GetClass()->GetName().AsString().AsChar() );
				return true;
			}
			else
			{
				ScriptError( stub->m_context, TXT("Function '%ls' was declared as normal function in the class '%ls'. Cannot override as latent function."), functionName.AsString().AsChar(), superFunction->GetClass()->GetName().AsString().AsChar() );
				return true;
			}
		}

		// Event flag should match
		if ( superFunction->IsEvent() != stub->m_createdFunction->IsEvent() )
		{
			if ( superFunction->IsEvent() )
			{
				ScriptError( stub->m_context, TXT("Function '%ls' was declared as event function in the class '%ls'. Cannot override as normal function."), functionName.AsString().AsChar(), superFunction->GetClass()->GetName().AsString().AsChar() );
				return true;
			}
			else
			{
				ScriptError( stub->m_context, TXT("Function '%ls' was declared as normal function in the class '%ls'. Cannot override as event function."), functionName.AsString().AsChar(), superFunction->GetClass()->GetName().AsString().AsChar() );
				return true;
			}
		}

		// Entry function flag should match
		if ( superFunction->IsEntry() != stub->m_createdFunction->IsEntry() )
		{
			if ( superFunction->IsEntry() )
			{
				ScriptError( stub->m_context, TXT("Function '%ls' was declared as entry function in the class '%ls'. Cannot override as normal function."), functionName.AsString().AsChar(), superFunction->GetClass()->GetName().AsString().AsChar() );
				return true;
			}
			else
			{
				ScriptError( stub->m_context, TXT("Function '%ls' was declared as normal function in the class '%ls'. Cannot override as entry function."), functionName.AsString().AsChar(), superFunction->GetClass()->GetName().AsString().AsChar() );
				return true;
			}
		}

		// If super function is an entry function we cannot be from the same state machine
		if ( superFunction->IsEntry() )
		{
			CClass* ourStateMachine = stub->m_createdFunction->GetClass()->GetStateMachineClass();
			CClass* superStateMachine = superFunction->GetClass()->GetStateMachineClass();
			if ( ourStateMachine == superStateMachine && ourStateMachine )
			{
				ScriptError( stub->m_context, TXT("You cannot override state entry function '%ls' within the same state machine class '%ls'."), functionName.AsString().AsChar(), ourStateMachine->GetName().AsString().AsChar() );
				return true;
			}
		}

		// Headers must match
		if ( !MatchFunctionHeader( stub, stub->m_createdFunction, superFunction ) )
		{
			return true;
		}

		// Bind as super function
		stub->m_createdFunction->m_superFunction = superFunction;
	}

	/// State parent function
	if ( baseClass->IsState() )
	{
		// Get the parent state machine class
		CClass* stateMachineClass = baseClass->GetStateMachineClass();
		if ( stateMachineClass )
		{
			// Find the function
			const CFunction* stateMachineParentFunction = stateMachineClass->FindFunctionNonCached( functionName );
			if ( stateMachineParentFunction )
			{
				// We cannot override state entry functions
				if ( stub->m_createdFunction->IsEntry() )
				{
					ScriptError( stub->m_context, TXT("State entry function '%ls' cannot be overriden."), functionName.AsString().AsChar() );
					return true;
				}

				// We cannot override timer functions
				if ( stateMachineParentFunction->IsTimer() )
				{
					ScriptError( stub->m_context, TXT("State machine function '%ls' from '%ls' is a timer function and cannot be overriden."), functionName.AsString().AsChar(), stateMachineParentFunction->GetClass()->GetName().AsString().AsChar() );
					return true;
				}

				// We cannot override final functions
				if ( stateMachineParentFunction->IsFinal() )
				{
					ScriptError( stub->m_context, TXT("State machine function '%ls' from '%ls' is a final function and cannot be overriden."), functionName.AsString().AsChar(), stateMachineParentFunction->GetClass()->GetName().AsString().AsChar() );
					return true;
				}

				// Event flag should match
				if ( stateMachineParentFunction->IsEvent() != stub->m_createdFunction->IsEvent() )
				{
					if ( stateMachineParentFunction->IsEvent() )
					{
						ScriptError( stub->m_context, TXT("Function '%ls' was declared as event function in the class '%ls'. Cannot override as normal function."), functionName.AsString().AsChar(), stateMachineParentFunction->GetClass()->GetName().AsString().AsChar() );
						return true;
					}
					else
					{
						ScriptError( stub->m_context, TXT("Function '%ls' was declared as normal function in the class '%ls'. Cannot override as event function."), functionName.AsString().AsChar(), stateMachineParentFunction->GetClass()->GetName().AsString().AsChar() );
						return true;
					}
				}

				// On case of event function check the function header
				if ( stub->m_createdFunction->IsEvent() )
				{
					// Headers must match
					if ( !MatchFunctionHeader( stub, stub->m_createdFunction, stateMachineParentFunction ) )
					{
						return true;
					}
				}
			}

			// This is a state entry function, check that name is not duplicated
			if ( stub->m_createdFunction->IsEntry() )
			{
				// Test if signature matches across all state machines
				CClass* curStateMachine = stateMachineClass;
				while ( curStateMachine )
				{
					const auto& stateClasses = curStateMachine->GetStateClasses();
					for ( Uint32 i = 0; i < stateClasses.Size(); ++i )
					{
						CName functionName = stub->m_createdFunction->GetName();
						const CFunction* entryFunction = stateClasses[i]->FindFunctionNonCached( functionName );
						if ( entryFunction && entryFunction->IsEntry() && entryFunction != stub->m_createdFunction )
						{
							// Headers must match
							if ( !MatchFunctionHeader( stub, stub->m_createdFunction, entryFunction ) )
							{
								ScriptError( stub->m_context, TXT("State Entry Function '%ls' signature is mismatched already defined at the scope of state '%ls' in state machine '%ls'. Cannot redefine."), functionName.AsString().AsChar(), stateClasses[i]->GetStateName().AsString().AsChar(), curStateMachine->GetName().AsString().AsChar() );
								return true;
							}
						}
					}

					if ( curStateMachine->HasBaseClass() )
					{
						curStateMachine = curStateMachine->GetBaseClass();
					}
					else
					{
						curStateMachine = NULL;
					}
				}
			}
		}
	}

	return true;
}

Bool CScriptCompiler::BindFunctions( CScriptSystemStub& definitions )
{
	// Create class properties
	for ( Uint32 i = 0; i < definitions.m_classes.Size(); ++i )
	{
		CScriptClassStub* stub = definitions.m_classes[ i ];
		for ( Uint32 j = 0; j < stub->m_functions.Size(); ++j )
		{
			CScriptFunctionStub* funcStub = stub->m_functions[ j ];
			if ( !BindSuperFunctions( funcStub ) )
			{
				return false;
			}
		}
	}

	// No fatal errors
	return true;
}

Bool CScriptCompiler::BuildDataLayout( CScriptSystemStub& definitions )
{
	// Get existing classes
	TDynArray< CClass* > existingClasses;
	SRTTI::GetInstance().EnumClasses( ClassID< IScriptable >(), existingClasses, NULL, true );

	// Create struct data size
	Uint32 iteration = 0;
	Bool sizeChanging = true;
	while ( sizeChanging )
	{
		// Check iteration count
		LOG_CORE( TXT("Layout iteration #%i..."), iteration );
		if ( ++iteration > 100 )
		{
			ScriptError( CScriptFileContext(), TXT("INTERNAL ERROR: BuildDataLayout endless loop") );
			return false;
		}

		// Let's hope this is the last iteration
		sizeChanging = false;

		// Update struct size
		for ( Uint32 i = 0; i < definitions.m_structs.Size(); ++i )
		{
			CScriptStructStub* stub = definitions.m_structs[i];
			ASSERT( stub->m_createdStruct );

			// Skip native structures
			if ( stub->m_createdStruct->IsNative() )
			{
				continue;
			}

			// Update size
			Uint32 prevSize = stub->m_createdStruct->GetSize();
			stub->m_createdStruct->RecalculateClassDataSize();
			if ( prevSize != stub->m_createdStruct->GetSize() )
			{
				//LOG_CORE( TXT("Layout of '%ls' %i->%i"), stub->m_name.AsChar(), prevSize, stub->m_createdStruct->GetSize() );
				sizeChanging = true;
			}
		}

		// Update class size
		for ( Uint32 i = 0; i < definitions.m_classes.Size(); ++i )
		{
			CScriptClassStub* stub = definitions.m_classes[i];
			ASSERT( stub->m_createdClass );

			// Base class is obligatory
			CClass* theClass = stub->m_createdClass;
			ASSERT( theClass == ClassID< IScriptable >() || theClass->GetBaseClass() );

			// Update size
			Uint32 prevSize = theClass->GetSize();
			Uint32 prevScriptSize = theClass->GetScriptDataSize();
			theClass->RecalculateClassDataSize();
			if ( prevSize != theClass->GetSize() || prevScriptSize != theClass->GetScriptDataSize() )
			{
				//LOG_CORE( TXT("Layout of '%ls' %i->%i (%i->%i)"), stub->m_name.AsChar(), prevSize, theClass->GetSize(), prevScriptSize, theClass->GetScriptDataSize() );
				sizeChanging = true;
			}
		}

		// Update existing classes
		for ( Uint32 i = 0; i < existingClasses.Size(); ++i )
		{
			// Base class is obligatory
			CClass* theClass = existingClasses[i];
			if ( theClass->IsBasedOn( ClassID< IScriptable >() ) )
			{
				// Update size
				Uint32 prevSize = theClass->GetSize();
				Uint32 prevScriptSize = theClass->GetScriptDataSize();
				theClass->RecalculateClassDataSize();
				if ( prevSize != theClass->GetSize() || prevScriptSize != theClass->GetScriptDataSize() )
				{
					//LOG_CORE( TXT("Layout of '%ls' %i->%i (%i->%i)"), theClass->GetName().AsChar(), prevSize, theClass->GetSize(), prevScriptSize, theClass->GetScriptDataSize() );
					sizeChanging = true;
				}
			}
		}
	}

	// Build layout of class functions
	for ( Uint32 i = 0; i < definitions.m_classes.Size(); ++i )
	{
		CScriptClassStub* stub = definitions.m_classes[i];
		for ( Uint32 j = 0; j < stub->m_functions.Size(); ++j )
		{
			CScriptFunctionStub* functionStub = stub->m_functions[j];
			if ( functionStub )
			{
				CFunction* func = functionStub->m_createdFunction;
				if ( func )
				{
					func->CalcDataLayout();
				}
			}
		}
	}

	// Build layout of global functions
	for ( Uint32 i = 0; i < definitions.m_functions.Size(); ++i )
	{
		CScriptFunctionStub* functionStub = definitions.m_functions[i];
		if ( functionStub )
		{
			CFunction* func = functionStub->m_createdFunction;
			if ( func )
			{
				func->CalcDataLayout();
			}
		}
	}

	// No errors
	return true;
}

CDefaultValue* CScriptCompiler::CreateDefaultValue( CProperty* prop, IRTTIType* rawType, const CScriptDefaultValue* valueStub )
{
	ASSERT( rawType );

	// Simple value	
	ERTTITypeType typeType = rawType->GetType();
	if ( typeType == RT_Simple || typeType == RT_Enum || typeType == RT_Fundamental )
	{
		// Parse value from string
		CVariant value( rawType->GetName(), NULL );
		if ( !value.FromString( valueStub->GetValue() ) )
		{
			ScriptError( valueStub->GetContext(), TXT("Unable to parse value") );
			return NULL;
		}

		// Create default value definition
		CDefaultValue* defaultValue = new CDefaultValue( prop );
		defaultValue->SetValue( value );
		return defaultValue;
	}

	// Array
	if ( typeType == RT_Array )
	{
		// Get the inner type of array
		CRTTIArrayType* arrayType = ( CRTTIArrayType* ) rawType;
		IRTTIType* innerType = arrayType->GetInnerType();

		// Create default value definition
		CDefaultValue* defaultValue = new CDefaultValue( prop );

		// Create array elements
		const Uint32 numSubElements = static_cast< Uint32 >(valueStub->GetNumSubValues());
		for ( Uint32 i = 0; i < numSubElements; ++i )
		{
			const CScriptDefaultValue* subElementStub = valueStub->GetSubValue( i );

			// Make sure it's not names
			if ( subElementStub->GetName() )
			{
				ScriptError( subElementStub->GetContext(), TXT("Named array elemnts are not supported") );
				return NULL;
			}

			// Convert to default value
			CDefaultValue* subElementValue = CreateDefaultValue( NULL, innerType, subElementStub );
			if ( subElementValue )
			{
				defaultValue->AddSubValue( subElementValue );
			}
		}
		return defaultValue;
	}

	// Structure
	if ( typeType == RT_Class )
	{
		CClass* pointedClass = ( CClass* ) rawType;

		// Create default value definition
		CDefaultValue* defaultValue = new CDefaultValue( prop );

		// Create struct elements
		const Uint32 numSubElements = static_cast< Uint32 >( valueStub->GetNumSubValues() );
		for ( Uint32 i = 0; i < numSubElements; ++i )
		{
			const CScriptDefaultValue* subElementStub = valueStub->GetSubValue( i );

			// Unnamed elements are not supported
			if ( !subElementStub->GetName() )
			{
				ScriptError( subElementStub->GetContext(), TXT("Unnamed structure elemnts are not supported") );
				return NULL;
			}

			// Find property
			CProperty* subProp = pointedClass->FindProperty( subElementStub->GetName() );
			if ( !subProp )
			{
				ScriptError( subElementStub->GetContext(), TXT("Property '%ls' is not member of '%ls'"), subElementStub->GetName().AsString().AsChar(), pointedClass->GetName().AsString().AsChar() );
				return NULL;
			}

			// Create default value
			CDefaultValue* subElementValue = CreateDefaultValue( subProp, subProp->GetType(), subElementStub );
			if ( subElementValue )
			{
				defaultValue->AddSubValue( subElementValue );
			}
		}

		return defaultValue;
	}

	// Inlined object
	if ( typeType == RT_Handle )
	{
		// Get the pointed type
		CRTTIHandleType* handleType = ( CRTTIHandleType* ) rawType;
		CClass* handleClass = ( CClass* )handleType->GetPointedType();
		ASSERT( handleClass->GetType() == RT_Class );

		// Get the class to use
		CClass* classToCreate = handleClass;
		if ( !valueStub->GetValue().Empty() )
		{
			// Find class by name
			CName className( valueStub->GetValue() );
			classToCreate = SRTTI::GetInstance().FindClass( className );
			if ( !classToCreate )
			{
				ScriptError( valueStub->GetContext(), TXT("Unknown class '%ls'"), className.AsString().AsChar() );
				return NULL;
			}
		}

		// Cannot use abstract classes
		if ( classToCreate->IsAbstract() )
		{
			ScriptError( valueStub->GetContext(), TXT("Cannot instance abstract class '%ls'"), classToCreate->GetName().AsString().AsChar() );
			return NULL;
		}

		// Class type mismatch
		if ( !classToCreate->IsA( handleClass ) )
		{
			ScriptError( valueStub->GetContext(), TXT("Class '%ls' is not compatible with handle '%ls'"), classToCreate->GetName().AsString().AsChar(), handleClass->GetName().AsString().AsChar() );
			return NULL;
		}

		// Create default value definition
		CDefaultValue* defaultValue = new CDefaultValue( prop );
		defaultValue->SetInlineClass( classToCreate	);

		// Create struct elements
		const Uint32 numSubElements = static_cast< Uint32 >( valueStub->GetNumSubValues() );
		for ( Uint32 i = 0; i < numSubElements; ++i )
		{
			const CScriptDefaultValue* subElementStub = valueStub->GetSubValue( i );

			// Unnamed elements are not supported
			if ( !subElementStub->GetName() )
			{
				ScriptError( subElementStub->GetContext(), TXT("Unnamed structure elemnts are not supported") );
				return NULL;
			}

			// Find property
			CProperty* subProp = classToCreate->FindProperty( subElementStub->GetName() );
			if ( !subProp )
			{
				ScriptError( subElementStub->GetContext(), TXT("Property '%ls' is not member of '%ls'"), subElementStub->GetName().AsString().AsChar(), classToCreate->GetName().AsString().AsChar() );
				return NULL;
			}

			// Create default value
			CDefaultValue* subElementValue = CreateDefaultValue( subProp, subProp->GetType(), subElementStub );
			if ( subElementValue )
			{
				defaultValue->AddSubValue( subElementValue );
			}
		}

		return defaultValue;
	}

	// Invalid type 
	ScriptError( valueStub->GetContext(), TXT("Invalid property type '%ls'. No default value can be parsed."), rawType->GetName().AsString().AsChar() );
	return NULL;
}

Bool CScriptCompiler::CreateDefaultValue( CClass* createdClass, CScriptDefaultValueStub* stub )
{
	ASSERT( stub->m_value );

	// Make sure target property exists
	CName propertyName( stub->m_name );
	CProperty* prop = createdClass->FindProperty( propertyName );
	if ( !prop )
	{
		// A little magic for state machines
		if ( propertyName == CNAME(AutoState) )
		{
			// Resolve the value (to name)
			if ( nullptr == stub->m_value )
			{
				ScriptError( stub->m_context, TXT("Expecting a value for 'autoState'") );
				return false;
			}

			// Get the state name
			const CName stateName( stub->m_value->GetValue().AsChar() );
			if ( nullptr == createdClass->FindStateClass( stateName ) )
			{
				// that's not an error
				ScriptWarn( stub->m_context, TXT("State '%ls' is not a recognized state in the '%ls' state machine."), 
					stateName.AsChar(), createdClass->GetName().AsString().AsChar() );
			}

			// Set the class "autoState" variable
			createdClass->SetAutoStateName( stateName );
			return true;
		}

		// Unknown property
		ScriptError( stub->m_context, TXT("Property '%ls' does not exist in '%ls'"), propertyName.AsString().AsChar(), createdClass->GetName().AsString().AsChar() );
		return false;
	}

	// Create default value
	CDefaultValue* value = CreateDefaultValue( prop, prop->GetType(), stub->m_value );
	stub->m_createdValue = value;

	// Add as class default value
	if ( value )
	{
		createdClass->AddDefaultValue( value );
	}

	// Done
	return true;
}

Bool CScriptCompiler::CreateDefaultValues( CScriptSystemStub& definitions )
{
	// Create struct default values
	for ( Uint32 i = 0; i < definitions.m_structs.Size(); ++i )
	{
		CScriptStructStub* stub = definitions.m_structs[i];
		if ( stub->m_createdStruct )
		{
			for ( Uint32 j = 0; j<stub->m_defaultValues.Size(); ++j )
			{
				CScriptDefaultValueStub* valueStub = stub->m_defaultValues[j];
				CreateDefaultValue( stub->m_createdStruct, valueStub );
			}
		}
	}

	// Create class default values
	for ( Uint32 i = 0; i < definitions.m_classes.Size(); ++i )
	{
		CScriptClassStub* stub = definitions.m_classes[i];
		if ( stub->m_createdClass )
		{
			for ( Uint32 j = 0; j < stub->m_defaultValues.Size(); ++j )
			{
				CScriptDefaultValueStub* valueStub = stub->m_defaultValues[j];
				CreateDefaultValue( stub->m_createdClass, valueStub );
			}
		}
	}

	// Done
	return true;
}

Bool CScriptCompiler::CompileFunctions( CScriptSystemStub& definitions )
{
	// Count functions
	Uint32 totalFunctions = definitions.m_functions.Size(), compiledFunctions = 0;
	for ( Uint32 i = 0; i < definitions.m_classes.Size(); ++i )
	{
		CScriptClassStub* classStub = definitions.m_classes[i];
		totalFunctions += classStub->m_functions.Size();
	}

	CScriptSyntaxNode::InitialiseNodeCheckers();

	EngineTime lastSplashUpdate( 0.f );

	CScriptCodeNodeCompilationPool pool;

	// Compile all global functions
	CScriptFunctionCompiler compiler( this, definitions, m_strictMode );
	for ( Uint32 i = 0; i < definitions.m_functions.Size(); ++i )
	{
		CScriptFunctionStub* funcStub = definitions.m_functions[ i ];

		// Splash screen info
		++compiledFunctions;
		if ( EngineTime::GetNow() - lastSplashUpdate > 0.5f )
		{
			lastSplashUpdate = EngineTime::GetNow();
			GSplash->UpdateProgress( TXT("Compiling scripts (compiling function %i/%i)..."), compiledFunctions, totalFunctions );
		}

		DEBUG_SET_PROGRESS( compiledFunctions, totalFunctions );

		compiler.Compile( NULL, funcStub, pool );
	}

	// Compile all functions in classes
	for ( Uint32 i = 0; i < definitions.m_classes.Size(); ++i )
	{
		CScriptClassStub* classStub = definitions.m_classes[ i ];
		for ( Uint32 j = 0; j < classStub->m_functions.Size(); ++j )
		{
			CScriptFunctionStub* funcStub = classStub->m_functions[ j ];

			// Splash screen info
			++compiledFunctions;
			if ( EngineTime::GetNow() - lastSplashUpdate > 0.5f )
			{
				lastSplashUpdate = EngineTime::GetNow();
				GSplash->UpdateProgress( TXT( "Compiling scripts (compiling function %i/%i)..." ), compiledFunctions, totalFunctions );
			}

			DEBUG_SET_PROGRESS( compiledFunctions, totalFunctions );

			compiler.Compile( classStub, funcStub, pool );
		}
	}

	LOG_CORE( TXT( "Generated %u Script code nodes" ), pool.GetNumElementsUsed() );

	// Functions compiled
	return true;
}

void CScriptCompiler::CheckClassesForCompleteness( CScriptSystemStub& definitions )
{
	for ( Uint32 i = 0; i < definitions.m_classes.Size(); ++i )
	{
		CScriptClassStub* topLevelStub = definitions.m_classes[ i ];

		// Is it missing functions, but not marked as abstract?
		if( topLevelStub->m_createdClass->HasUndefinedScriptFunctions() && !( topLevelStub->m_createdClass->IsAbstract() ) )
		{
			// Primary error - this class is incomplete
			ScriptError( topLevelStub->m_context, TXT( "Class %" ) RED_PRIWs TXT( " is not marked as abstract, but has missing functions!" ), topLevelStub->m_name.AsChar() );

			// Secondary errors - list the offending functions throughout the hierarchy
			CScriptClassStub* stub = topLevelStub;

			do
			{
				if( !( stub->m_createdClass->HasUndefinedScriptFunctions() ) )
				{
					break;
				}

				for( Uint32 i = 0; i < stub->m_functions.Size(); ++i )
				{
					CScriptFunctionStub* functionStub = stub->m_functions[ i ];
					if( functionStub->m_flags & FF_UndefinedBody && !( functionStub->m_flags & FF_NativeFunction ) )
					{
						// Output for the missing functions
						if( !FindImplementedFunctionInHierarchy( definitions, topLevelStub, functionStub->m_name, stub ) )
						{
							ScriptError( functionStub->m_context, TXT( " - '%" ) RED_PRIWs TXT( "' in base class '%" ) RED_PRIWs TXT( "'" ), functionStub->m_name.AsChar(), stub->m_name.AsChar() );
						}
					}
				}

				stub = definitions.FindClassStub( stub->m_extends );
			}
			while( stub );
		}
	}
}
