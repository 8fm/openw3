/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "scriptingSystem.h"
#include "resourceDefManager.h"
#include "scriptStackFrame.h"
#include "tokenizer.h"
#include "depot.h"
#include "scriptThread.h"
#include "enum.h"
#include "filePath.h"
#include "configVarSystem.h"

#ifdef RED_MOD_SUPPORT
static void DefaultLogFunc( CName channel, const String& )
{}

namespace ModHelpers
{
	typedef void (*LOG_FUNC)(CName, const String&);
	LOG_FUNC LogFn = &DefaultLogFunc;
}
#endif //RED_MOD_SUPPORT

void funcLog( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, text, String::EMPTY );
	FINISH_PARAMETERS;

	// Print
	
#ifdef RED_LOGGING_ENABLED
	RED_LOG( Script, TXT("%ls"), text.AsChar() );
#else
# ifdef RED_MOD_SUPPORT
	ModHelpers::LogFn( CNAME(Script), text );
# endif
#endif
	
	RETURN_VOID();
}

namespace // anonymous
{
	// Ugly solution for an ugly problem. I think we need to try solving the problem
	// in a less brutal fashion
	Bool GAllowLogChannel( const CName& channel )
	{
		static TDynArray< CName > logChannels;
		static Bool logSpam = true;
		static Bool initialized = false;

		if ( ! initialized )
		{
			initialized = true;

			CTokenizer tokenizer( SGetCommandLine(), TXT(" ") );
			for ( Uint32 i = 0; i < tokenizer.GetNumTokens(); ++i )
			{
				String token = tokenizer.GetToken( i );
				if ( token == TXT("-show_script_log_channels") )
				{
					logSpam = false;
					String channels = tokenizer.GetToken( i + 1 );
					if ( false == channels.Empty() )
					{
						CTokenizer channelTokenizer( channels, TXT(",") );
						for ( Uint32 j = 0; j < channelTokenizer.GetNumTokens(); ++j )
						{
							CName name( channelTokenizer.GetToken( j ) );
							if ( name != CName::NONE )
							{
								logChannels.PushBackUnique( name );
							}
						}
						break;
					}
				}
			}
		}

		return logSpam || logChannels.Exist( channel );
	}
}

void funcLogChannel( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, channel, CName::NONE );
	GET_PARAMETER( String, text, String::EMPTY );
	FINISH_PARAMETERS;

#ifdef RED_LOGGING_ENABLED

#ifndef NO_CUSTOM_LOG_SPAM_FILTER
	if ( ! GAllowLogChannel( channel ) )
	{
		return;
	}
#endif

	Char channelName[ 64 ];
	Red::System::SNPrintF( channelName, ARRAY_COUNT( channelName ), TXT( "Script_%ls"), channel.AsChar() );
	HandleLogRequest( Red::CNameHash::Hash( channelName ), channelName, Red::System::Log::P_Information, text.AsChar() );

#else

#ifdef RED_MOD_SUPPORT
	ModHelpers::LogFn( channel, text );
#endif

#endif // RED_LOGGING_ENABLED
	
	RETURN_VOID();
}

void funcTrace( CObject*, CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_VOID();
}

void funcDebugBreak( CObject*, CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_VOID();
}

void funcLoadResource( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, unsafePath, String::EMPTY );
	GET_PARAMETER_OPT( Bool, isDepotPath, false );
	FINISH_PARAMETERS;

	String temp;
	const String& resource = CFilePath::ConformPath( unsafePath, temp );
	if ( resource != unsafePath )
	{
		SCRIPT_ERROR( stack, TXT("Path to resource '%ls' is not compatible with engine requirements (should be lower case only, windows path separators)"), unsafePath.AsChar() );
	}

	if( isDepotPath )
	{
		RETURN_OBJECT( Cast< IScriptable >( GDepot->LoadResource( resource ) ) );
	}
	else
	{
		String aliasFilename(CResourceDefManager::RESDEF_PROTOCOL);
		aliasFilename += resource;
		RETURN_OBJECT( Cast< IScriptable >( GDepot->LoadResource( aliasFilename ) ) );
	}
}

extern Bool GLatentFunctionStart;

void funcLoadResourceAsync( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, unsafePath, String::EMPTY );
	GET_PARAMETER_OPT( Bool, isDepotPath, false );
	FINISH_PARAMETERS;

	String temp;
	String resource = CFilePath::ConformPath( unsafePath, temp );
	if ( resource != unsafePath )
	{
		SCRIPT_ERROR( stack, TXT("Path to resource '%ls' is not compatible with engine requirements (should be lower case only, windows path separators)"), unsafePath.AsChar() );
	}

	if( !isDepotPath )
	{
		resource = CResourceDefManager::RESDEF_PROTOCOL + resource;
	}

    THandle< CResource > res;
    if ( GLatentFunctionStart )
    {
	    res = GDepot->LoadResourceAsync( resource );
        if ( res.Get() )
        {
	        RETURN_OBJECT( Cast< IScriptable >(res) );
        }
        else
        {
            stack.m_thread->ForceYield();
        }
    }
    else
    {
        CDepot::AsyncResourceState state = GDepot->GetAsyncResource( resource, res );
        if ( state == CDepot::ARS_LOADING )
        {
            stack.m_thread->ForceYield();
        }
        else
        {
            RETURN_OBJECT( Cast< IScriptable >(res) );
        }
    }
}

void funcDumpClassHierarchy( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, className, CName::NONE );
	FINISH_PARAMETERS;
	CClass * c = SRTTI::GetInstance().FindClass( className );
	if( c )
	{
		SRTTI::GetInstance().DumpClassHierarchy( c );
		RETURN_BOOL( true );
	}
	else
	{
		RETURN_BOOL( false );
	}
}

void funcArraySortInts( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( TDynArray< Int32 >, items, TDynArray< Int32 >() );
	FINISH_PARAMETERS;

	Sort( items.Begin(), items.End() );

	RETURN_VOID();
}

void funcArraySortFloats( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( TDynArray< Float >, items, TDynArray< Float >() );
	FINISH_PARAMETERS;

	Sort( items.Begin(), items.End() );

	RETURN_VOID();
}

void funcArraySortStrings( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( TDynArray< String >, items, TDynArray< String >() );
	FINISH_PARAMETERS;

	Sort( items.Begin(), items.End() );

	RETURN_VOID();
}

void funcUint64ToString( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Uint64, i, 0 );
	FINISH_PARAMETERS;

	RETURN_STRING( String::Printf( TXT("%llu"), i ) );
}

void funcEnumGetMax( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, name, CName::NONE );
	FINISH_PARAMETERS;

	Int32 max = 0, v = 0;
	CEnum *e = SRTTI::GetInstance().FindEnum( name );
	if ( e )
	{
		const TDynArray< CName >& options = e->GetOptions();
		for ( Uint32 i = 0; i < options.Size(); ++i )
		{
			if ( e->FindValue( options[ i ], v ) && v > max)
			{
				max = v;
			}
		}
	}

	RETURN_INT( max );
}

void funcEnumGetMin( CObject*, CScriptStackFrame& stack, void* result )
{
    GET_PARAMETER( CName, name, CName::NONE );
    FINISH_PARAMETERS;

    Int32 min = 0, v = 0;
    CEnum *e = SRTTI::GetInstance().FindEnum( name );
    if ( e )
    {
        const TDynArray< CName >& options = e->GetOptions();
        for ( Uint32 i = 0; i < options.Size(); ++i )
        {
            if ( e->FindValue( options[ i ], v ) && v < min)
            {
                min = v;
            }
        }
    }

    RETURN_INT( min );
}

void ExportCoreNatives()
{
	// Math
	extern void ExportCoreMathNatives();
	ExportCoreMathNatives();

	// String natives
	extern void ExportCoreStringNatives();
	ExportCoreStringNatives();

	extern void ExportCoreEngineTimeNatives();
	ExportCoreEngineTimeNatives();
	extern void ExportCoreLatentFunctions();
	ExportCoreLatentFunctions();

	extern void ExportEngineEntityHandleNatives();
	ExportEngineEntityHandleNatives();
	extern void ExportEnginePersistentRefNatives();
	ExportEnginePersistentRefNatives();
	extern void ExportAnimationNatives();
	ExportAnimationNatives();

	extern void ExportDebugWindowsNatives();
	ExportDebugWindowsNatives();

	// General
	NATIVE_GLOBAL_FUNCTION( "Log", funcLog );
	NATIVE_GLOBAL_FUNCTION( "LogChannel", funcLogChannel );
	NATIVE_GLOBAL_FUNCTION( "Trace", funcTrace );
	NATIVE_GLOBAL_FUNCTION( "DebugBreak", funcDebugBreak );
	NATIVE_GLOBAL_FUNCTION( "LoadResourceAsync", funcLoadResourceAsync );
	NATIVE_GLOBAL_FUNCTION( "LoadResource", funcLoadResource );
	NATIVE_GLOBAL_FUNCTION( "DumpClassHierarchy", funcDumpClassHierarchy );
	NATIVE_GLOBAL_FUNCTION( "Uint64ToString", funcUint64ToString );

	NATIVE_GLOBAL_FUNCTION( "ArraySortInts", funcArraySortInts );
	NATIVE_GLOBAL_FUNCTION( "ArraySortFloats", funcArraySortFloats );
	NATIVE_GLOBAL_FUNCTION( "ArraySortStrings", funcArraySortStrings );

	NATIVE_GLOBAL_FUNCTION( "EnumGetMax", funcEnumGetMax );
	NATIVE_GLOBAL_FUNCTION( "EnumGetMin", funcEnumGetMin );
 }
