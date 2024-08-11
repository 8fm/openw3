/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "scriptingSystem.h"
#include "../redNetwork/manager.h"
#include "tokenizer.h"
#include "profiler.h"
#include "scriptThread.h"
#include "scriptCompiler.h"
#include "depot.h"
#include "filePath.h"
#include "scriptsOpcodeExaminer.h"
#include "contentManager.h"

CScriptingSystem* GScriptingSystem = NULL;

#ifdef RED_MOD_SUPPORT
namespace Config
{
	TConfigVar< Bool > cvDebugScripts( "Scripts", "DebugScripts", false, eConsoleVarFlag_Developer );
}
#endif // RED_MOD_SUPPORT

CScriptingSystem::CScriptingSystem( const Char* rootPath )
	: m_rootPath( rootPath )
	, m_isValid( false )
#if defined( RED_FINAL_BUILD )
	, m_isFinalRelease( true )
#else
	, m_isFinalRelease( false )
#endif
	, m_nextThreadId( 1 )
	, m_debugFlags( 0 )
{
	m_globals.Resize( GP_SIZE );

#ifndef RED_FINAL_BUILD
	// Enable final release if commandline option is set
	CTokenizer commandline( SGetCommandLine(), TXT(" ") );
	for ( Uint32 i=0; i<commandline.GetNumTokens(); i++ )
	{
		if ( commandline.GetToken(i) == TXT("-finalrelease") )
		{
			LOG_CORE( TXT("RED_FINAL_BUILD RELEASE SCRIPTS ENABLED. NO DEBUG.") );
			m_isFinalRelease = true;
		}
	}
#else
# ifdef RED_MOD_SUPPORT
		const Bool debugScripts = Config::cvDebugScripts.Get() || (Red::System::StringSearch( SGetCommandLine(), TXT( "-debugscripts" ) ) != nullptr);
		m_isFinalRelease = !debugScripts;
# endif
#endif

	// Initialize opcodes
	extern void ExportCoreOpcodes( CScriptNativeFunctionMap& functionMap );
	ExportCoreOpcodes( m_nativeFunctions );
}

Bool CScriptingSystem::LoadScripts( IScriptLogInterface* output, const TDynArray< String >& scriptFiles, Bool fullContext /*= false*/ )
{
	CTimeCounter timer;

	// Mark as invalid
	m_isValid = false;

	// Kill all threads
	for ( Uint32 i=0; i<m_threads.Size(); ++i )
	{
		CScriptThread* thread = m_threads[i];
		thread->ForceKill();
		thread->DeletePendingFrames();
	}

	// Create compiler and compile scripts
	CScriptCompiler compiler( output );
	if ( compiler.CompileFiles( scriptFiles, fullContext ) )
	{
		LOG_CORE( TXT("Scripts loaded and compiled in %1.2fs"), timer.GetTimePeriod() );
		m_isValid = true;
	}

	// Return status
	return m_isValid;
}

void CScriptingSystem::DisableAllBreakpoints()
{
	// Get all functions
	TDynArray< CFunction* > functions;
	SRTTI::GetInstance().EnumFunctions( functions );

	// Disable breakpoints
	for ( Uint32 i=0; i<functions.Size(); ++i )
	{
		CFunction* function = functions[i];
		function->GetCode().BreakpointRemoveAll();
	}
}

Bool CScriptingSystem::ToggleBreakpoint( const ScriptBreakpoint& breakpoint, Bool state )
{
	// Get all functions
	TDynArray< CFunction* > functions;
	SRTTI::GetInstance().EnumFunctions( functions );

	// Toggle breakpoint
	for ( Uint32 i = 0; i < functions.Size(); ++i )
	{
		CFunction* function = functions[ i ];
		if ( function->GetCode().BreakpointToggle( breakpoint, state ) )
		{
			return true;
		}
	}

	// Not toggled
	return false;
}

static Bool GSkipKeyword( const Char*& str, const Char* match )
{
	const Char* ptrStart = str;
	if ( GParseWhitespaces( str ) )
	{
		const Uint32 matchLen = static_cast< Uint32 >(Red::System::StringLength( match ));
		String tokenSoFar;
		while ( *str > 32 && tokenSoFar.GetLength() < matchLen )
		{
			Char text[] = { *str, 0 };
			tokenSoFar += text;
			str++;

			if ( tokenSoFar.EqualsNC( match ) )
			{
				return true;
			}
		}
	}

	str = ptrStart;
	return false;
}

static Bool GParseStringFunctionParam( const Char*& stream, String& outVal )
{
	if ( *stream == '\"' || *stream == '\'' )
	{
		// Try parse as quoted string
		++stream;
		while ( *stream )
		{
			if ( *stream == '\"' || *stream == '\'' )
			{
				++stream;
				break;
			}

			Char chars[2] = { *stream, 0 };
			outVal += chars;
			++stream;
		}
		return true;
	}

	return false;
}

static Bool GParseFunctionParam( const Char*& stream, String& token )
{
	String outputVal;
	if ( GParseWhitespaces( stream ) )
	{	
		outputVal.Reserve( 32 );

		if ( !GParseStringFunctionParam( stream, outputVal ) )
		{
			// Grab normal text
			while ( *stream && *stream > ' ' )
			{
				if ( *stream == '(' || *stream == ')' || *stream == ',' )
				{
					break;
				}

				Char chars[2] = { *stream, 0 };
				outputVal += chars;
				stream++;				
			}
		}

		if ( outputVal.GetLength() )
		{
			token = std::move( outputVal );
			return true;
		}
	}

	// Not grabbed
	return false;
}

Bool CScriptingSystem::CallGlobalExecFunction( const String& functionAndParams, Bool silentErrors )
{
	const Char* str = functionAndParams.AsChar();

	// Tokenize, get function name
	String functionName;
	if ( !GParseFunctionParam( str, functionName ) )
	{
		if ( !silentErrors )
		{
			RED_LOG( Script, TXT("Error: Unable to parse function name") );
		}
		return false;
	}

	// Find global function with that name
	CName tempshitName( functionName );
	CFunction* global = SRTTI::GetInstance().FindGlobalFunction( tempshitName );
	if ( !global )
	{
		if ( !silentErrors )
		{
			RED_LOG( Script, TXT("Error: Unknown global function '%ls'"), functionName.AsChar() );
		}
		return false;
	}

	// It's not an exec function, we cannot call it unless in debug build
	if ( !global->IsNative() && !global->IsExec() )
	{
		RED_LOG( Script, TXT("Error: No privledges to call script function '%ls'. Add 'exec' keyword."), functionName.AsChar() );
		return false;		
	}

	// Expect a '(' and parameter list
	TDynArray< String > parameters;
	if ( GSkipKeyword( str, TXT("(") ) )
	{
		// Parse parameters
		while ( *str )
		{
			// End of parameters
			if ( GSkipKeyword( str, TXT(")") ) )
			{
				break;
			}

			// Detect to many parameters
			if ( parameters.Size() == global->GetNumParameters() )
			{
				RED_LOG( Script, TXT("Error: To many parameters for function '%ls'."), functionName.AsChar() );
				return false;
			}

			// We should have an ',' separating parameters
			if ( parameters.Size() )
			{
				if ( !GSkipKeyword( str, TXT(",") ) )
				{
					RED_LOG( Script, TXT("Error: Parse error: expecting ','") );
					return false;
				}
			}

			// Get the matching function parameter
			CProperty* param = global->GetParameter( parameters.Size() );
			ASSERT( param );

			// Parse token
			String paramValue;
			if ( GParseFunctionParam( str, paramValue ) )
			{
				// Add to value list
				parameters.PushBack( paramValue );
			}
			else
			{
				// If not optional it's an error
				if ( !(param->GetFlags() & PF_FuncOptionaParam) )
				{
					RED_LOG( Script, TXT("Error: unable to parse value for parameter '%ls'"), param->GetName().AsString().AsChar() );
					return false;
				}

				// No parameter given
				parameters.PushBack( String::EMPTY );
			}
		}
	}

	// Count non optional parameters
	for ( Uint32 i=parameters.Size(); i<global->GetNumParameters(); i++ )
	{
		// Get the matching function parameter
		CProperty* param = global->GetParameter( i );
		ASSERT( param );

		// If not optional it's an error
		if ( !(param->GetFlags() & PF_FuncOptionaParam) )
		{
			RED_LOG( Script, TXT("Error: value for parameter '%ls' required"), param->GetName().AsString().AsChar() );
			return false;
		}
	}

	// Call the function
	String resultValue;
	global->Call( NULL, parameters, &resultValue );

	// Print result
	if ( global->GetReturnValue() )
	{
		RED_LOG( Script, TXT("Result: '%ls'"), resultValue.AsChar() );
	}

	// Function was executed
	return true;
}

Bool CScriptingSystem::CallLocalFunction( IScriptable* context, const String& functionAndParams, Bool silentErrors )
{
	const Char* str = functionAndParams.AsChar();

	// Tokenize, get function name
	String functionName;
	if ( !GParseFunctionParam( str, functionName ) )
	{
		if ( !silentErrors )
		{
			RED_LOG( Script, TXT("Unable to parse function name") );
		}
		return false;
	}

	// Find global function with that name
	CName tempshitName( functionName );
	const CFunction* function;
	
	if ( !FindFunction( context, tempshitName, function ) )
	{	
		if ( !silentErrors )
		{
			RED_LOG( Script, TXT("Unknown local function '%ls'"), functionName.AsChar() );
		}
		return false;
	}

	// It's not an exec function, we cannot call it unless in debug build
	if ( !function->IsEvent() && !function->IsExec() )
	{
		RED_LOG( Script, TXT("No privledges to call script function '%ls'."), functionName.AsChar() );
		return false;		
	}

	// Expect a '(' and parameter list
	TDynArray< String > parameters;
	if ( GSkipKeyword( str, TXT("(") ) )
	{
		// Parse parameters
		while ( *str )
		{
			// End of parameters
			if ( GSkipKeyword( str, TXT(")") ) )
			{
				break;
			}

			// Detect to many parameters
			if ( parameters.Size() == function->GetNumParameters() )
			{
				RED_LOG( Script, TXT("To many parameters for function '%ls'."), functionName.AsChar() );
				return false;
			}

			// We should have an ',' separating parameters
			if ( parameters.Size() )
			{
				if ( !GSkipKeyword( str, TXT(",") ) )
				{
					RED_LOG( Script, TXT("Parse error: expecting ','") );
					return false;
				}
			}

			// Get the matching function parameter
			CProperty* param = function->GetParameter( parameters.Size() );
			ASSERT( param );

			// Parse token
			String paramValue;
			if ( GParseFunctionParam( str, paramValue ) )
			{
				// Add to value list
				parameters.PushBack( paramValue );
			}
			else
			{
				// If not optional it's an error
				if ( !(param->GetFlags() & PF_FuncOptionaParam) )
				{
					RED_LOG( Script, TXT("Parse error: unable to parse value for parameter '%ls'"), param->GetName().AsString().AsChar() );
					return false;
				}

				// No parameter given
				parameters.PushBack( String::EMPTY );
			}
		}
	}

	// Count non optional parameters
	for ( Uint32 i=parameters.Size(); i<function->GetNumParameters(); i++ )
	{
		// Get the matching function parameter
		CProperty* param = function->GetParameter( i );
		ASSERT( param );

		// If not optional it's an error
		if ( !(param->GetFlags() & PF_FuncOptionaParam) )
		{
			RED_LOG( Script, TXT("Parse error: value for parameter '%ls' required"), param->GetName().AsString().AsChar() );
			return false;
		}
	}

	// Call the function
	String resultValue;
	function->Call( NULL, parameters, &resultValue );

	// Print result
	if ( function->GetReturnValue() )
	{
		RED_LOG( Script, TXT("Result: '%ls'"), resultValue.AsChar() );
	}

	// Function was executed
	return true;
}

#ifdef RED_NETWORK_ENABLED
void CScriptingSystem::OnPacketReceived( const Red::System::AnsiChar*, Red::Network::IncomingPacket& packet )
{
	AnsiChar requestType[ 32 ];
	RED_VERIFY( packet.ReadString( requestType, ARRAY_COUNT_U32( requestType ) ), TXT( "Buffer too small for packet request type" ) );

	if( Red::System::StringCompare( requestType, "BreakpointToggle" ) == 0 )
	{
		const Uint32 MAX_FILE_SIZE = 256;

		Char file[ MAX_FILE_SIZE ];
		Int32 line;
		Bool enabled;

		RED_VERIFY( packet.ReadString( file, ARRAY_COUNT_U32( file ) ), TXT( "Buffer too small for string or packet corrupt" ) );
		RED_VERIFY( packet.Read( line ), TXT( "Packet corrupt" ) );
		RED_VERIFY( packet.Read( enabled ), TXT( "Packet corrupt" ) );

		// We need to normalise the directory separators, since they may be different if script studio has connected
		// from another platform (i.e. from windows to orbis
		Char normalizedFile[ MAX_FILE_SIZE ];
		Red::System::StringCopy( normalizedFile, file, MAX_FILE_SIZE );

		Char* fileSearch = normalizedFile;
		while( ( fileSearch = Red::System::StringSearch( fileSearch, CFileManager::ALTERNATIVE_DIRECTORY_SEPARATOR ) ) != nullptr )
		{
			*fileSearch = CFileManager::DIRECTORY_SEPARATOR;
		}

		Bool breakpointSuccessfullyToggled = ToggleBreakpoint( ScriptBreakpoint( normalizedFile, line ), enabled );

		Red::Network::ChannelPacket confirmationPacket( RED_NET_CHANNEL_SCRIPT_DEBUGGER );

		RED_VERIFY( confirmationPacket.WriteString( "BreakpointConfirm" ) );

		// We need to send back the original path sent to us unaltered
		RED_VERIFY( confirmationPacket.WriteString( file ) );
		RED_VERIFY( confirmationPacket.Write( line ) );
		RED_VERIFY( confirmationPacket.Write( breakpointSuccessfullyToggled ) );

		Red::Network::Manager::GetInstance()->Send( RED_NET_CHANNEL_SCRIPT_DEBUGGER, confirmationPacket );
	}
	else if( Red::System::StringCompare( requestType, "ClearAllBreakpoints" ) == 0 )
	{
		DisableAllBreakpoints();
	}
	else if( Red::System::StringCompare( requestType, "SortLocals" ) == 0 )
	{
		Bool enabled;

		RED_VERIFY( packet.Read( enabled ), TXT( "Packet corrupt" ) );

		if( enabled )
		{
			m_debugFlags |= FLAG( DF_SortLocalsAlphaAsc );
		}
		else
		{
			m_debugFlags &= ~FLAG( DF_SortLocalsAlphaAsc );
		}
	}
	else if( Red::System::StringCompare( requestType, "UnfilteredLocals" ) == 0 )
	{
		Bool enabled;

		RED_VERIFY( packet.Read( enabled ), TXT( "Packet corrupt" ) );

		if( enabled )
		{
			m_debugFlags |= FLAG( DF_UnfilteredLocals );
		}
		else
		{
			m_debugFlags &= ~FLAG( DF_UnfilteredLocals );
		}
	}
	else if( Red::System::StringCompare( requestType, "RootPath" ) == 0 )
	{
		Red::Network::ChannelPacket rootPathPacket( RED_NET_CHANNEL_SCRIPT_COMPILER );

		RED_VERIFY( rootPathPacket.WriteString( "RootPathConfirm" ) );
		RED_VERIFY( rootPathPacket.WriteString( m_rootPath.AsChar() ) );

		Red::Network::Manager::GetInstance()->Send( RED_NET_CHANNEL_SCRIPT_COMPILER, rootPathPacket );
	}
	else if( Red::System::StringCompare( requestType, "OpcodeBreakdownRequest" ) == 0 )
	{
		Char functionName[ 256 ];
		Bool isClassFunction = false;

		RED_VERIFY( packet.ReadString( functionName, ARRAY_COUNT_U32( functionName ) ) );
		RED_VERIFY( packet.Read( isClassFunction ) );

		String opcodes;

		if( isClassFunction )
		{
			Char className[ 256 ];
			RED_VERIFY( packet.ReadString( className, ARRAY_COUNT_U32( className ) ) );

			SendOpcodes( functionName, className );
		}
		else
		{
			SendOpcodes( functionName, nullptr );
		}
	}
}
#else
void CScriptingSystem::OnPacketReceived( const Red::System::AnsiChar*, Red::Network::IncomingPacket& ) {}
#endif // RED_NETWORK_ENABLED

CScriptThread* CScriptingSystem::CreateThread( IScriptable* context, const CFunction* entryFunction, CScriptStackFrame& callingFrame, void * returnValue /*= NULL*/ )
{
	//ASSERT( context );
	ASSERT( entryFunction );

	// Allocate ID for new thread
	const Uint32 threadId = m_nextThreadId++;

	// Create initial frame
	CScriptStackFrame* topFrame = entryFunction->CreateEntryFrame( context, callingFrame );
	ASSERT( topFrame );

	// Create thread
	CScriptThread* newThread = new CScriptThread( threadId, context, entryFunction, *topFrame, returnValue );
	m_threads.PushBack( newThread );

	// Return created thread
	return newThread;
}

CScriptThread* CScriptingSystem::CreateThreadUseGivenFrame( IScriptable* context, const CFunction* entryFunction, CScriptStackFrame& topFrame, void * returnValue /*= NULL*/ )
{
	//ASSERT( context );
	ASSERT( entryFunction );

	// Allocate ID for new thread
	const Uint32 threadId = m_nextThreadId++;

	// Create thread
	CScriptThread* newThread = new CScriptThread( threadId, context, entryFunction, topFrame, returnValue );
	m_threads.PushBack( newThread );

	// Return created thread
	return newThread;
}

void CScriptingSystem::AdvanceThreads( Float timeDelta )
{
	// Advance thread list
	TScriptThreadArray threads = m_threads;
	for ( Uint32 i=0; i<threads.Size(); i++ )
	{
		CScriptThread* thread = threads[i];

		// Advance the thread. If finished remove from thread list.
		if ( thread->Advance( timeDelta ) )
		{
			m_threads.Remove( thread );

			delete thread;
		}
	}
	// TODO: It looks like above "safety valve" is wasteful (because there is no possibility to externally delete&erase thread
	// here is my proposition
	//for ( Uint32 i=0; i<m_threads.Size(); )
	//{
	//	CScriptThread* thread = m_threads[i];

	//	// Advance the thread. If finished remove from thread list.
	//	if ( thread->Advance( timeDelta ) )
	//	{
	//		m_threads.Erase( m_threads.Begin() + i );

	//		delete thread;
	//	}
	//	else
	//		++i;
	//}
}

void CScriptingSystem::RegisterGlobal( EGlobalPointers globalType, IScriptable* objPtr )
{
	m_globals[ globalType ] = objPtr;
}

IScriptable* CScriptingSystem::GetGlobal( EGlobalPointers globalType )
{
	return m_globals[ globalType ].Get();
}

CResource* CScriptingSystem::LoadScriptResource( const String& path )
{
	String temp;
	const String& safePath = CFilePath::ConformPath( path, temp );
	if ( safePath != path )
	{
		ERR_CORE( TXT("Script resource path '%ls' used in scripts is not compatible with engine requirements"), path.AsChar() );
	}
	return GDepot->LoadResource( safePath );
}

#ifdef RED_NETWORK_ENABLED
void CScriptingSystem::RegisterNetworkChannelListener()
{
	if( Red::Network::Manager::GetInstance() )
	{
		Red::Network::Manager::GetInstance()->CreateChannel( RED_NET_CHANNEL_SCRIPT_COMPILER );

		Red::Network::Manager::GetInstance()->RegisterListener( RED_NET_CHANNEL_SCRIPT_DEBUGGER, this );
		Red::Network::Manager::GetInstance()->RegisterListener( RED_NET_CHANNEL_SCRIPT_COMPILER, this );
	}
}

void CScriptingSystem::SendOpcodes( const Char* functionName, const Char* className ) const
{
	const CClass* classInstance			= ( className )? SRTTI::GetInstance().FindClass( CName( className ) ) : nullptr;
	const CFunction* functionInstance	= ( classInstance )? classInstance->FindFunction( CName( functionName ) ) : SRTTI::GetInstance().FindGlobalFunction( CName( functionName ) );

	if( !functionInstance )
	{
		return;
	}

	if( !functionInstance->IsNative() )
	{
		CScriptOpCodeExaminer examiner;
		examiner.Examine( functionInstance );

		const TDynArray< CScriptOpCodeExaminer::DisassembledFunction >& data = examiner.GetOutput();
		const Uint32 numFunctions = data.Size();

		Red::Network::ChannelPacket packet( RED_NET_CHANNEL_SCRIPT_DEBUGGER );
	
		RED_VERIFY( packet.WriteString( "OpcodeBreakdownResponse" ) );
		RED_VERIFY( packet.Write( numFunctions ) );

		for( Uint32 i = 0; i < numFunctions; ++i )
		{
			const CScriptOpCodeExaminer::DisassembledFunction& disassembledFunction = data[ i ];
			const Uint32 numLines = disassembledFunction.m_lines.Size();

			RED_VERIFY( packet.WriteString( disassembledFunction.m_file.AsChar() ) );
			RED_VERIFY( packet.Write( numLines ) );

			for( Uint32 j = 0; j < numLines; ++j )
			{
				const CScriptOpCodeExaminer::DisassembledLine& disassembledLine = disassembledFunction.m_lines[ j ];

				RED_VERIFY( packet.Write( disassembledLine.m_line ) );
				RED_VERIFY( packet.WriteString( disassembledLine.m_details.AsChar() ) );
			}
		}

		Red::Network::Manager::GetInstance()->Send( RED_NET_CHANNEL_SCRIPT_DEBUGGER, packet );
	}
}

#endif // RED_NETWORK_ENABLED
