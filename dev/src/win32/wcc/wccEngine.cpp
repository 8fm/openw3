#include "build.h"
#include "../../common/core/commandlet.h"
#include "../../common/core/deferredDataBuffer.h"

CWccEngine::CWccEngine()
	: CBaseEngine()
	, m_numErrors( 0 )
	, m_numWarnings( 0 )
{
	// Get output handles
	m_stdOut = GetStdHandle( STD_OUTPUT_HANDLE );
	m_silentScripts = true;
}

CWccEngine::~CWccEngine()
{
}

Char GGlobalStatus[ 1024 ];
Float GGlobalProgress = 0.0f;

void UpdateConsoleTitle()
{
	Char txt[ 1024 + 10 ];
	Red::System::SNPrintF( txt, ARRAYSIZE(txt), TXT("%1.1f%% - %s"), GGlobalProgress, GGlobalStatus );
	SetConsoleTitleW( txt );
}

void SetGlobalProgress( Int32 current, Int32 max )
{
	GGlobalProgress = max > 0 ? ( current * 100.0f / (Float)max ) : 100.0f;
	UpdateConsoleTitle();
}

void SetGlobalStatus( const TCHAR* format, ... )
{
	va_list arglist;
	va_start( arglist, format );
	vswprintf( GGlobalStatus,  ARRAY_COUNT(GGlobalStatus), format, arglist ); 
	UpdateConsoleTitle();
}

Bool CWccEngine::Main( TDynArray< String > args )
{
	// Touch importer classes
	extern void RegisterImportClasses();
	RegisterImportClasses();

	extern void RegisterWccClasses();
	RegisterWccClasses();

	// in cooking we don't want to use any fancy async loading of DDBs
	DeferredDataBuffer::ForceImmediateAsyncLoading();

	Bool commandletSuccedded = true;
	m_rememberErrors = true;

	if ( args.Size() > 1 ) // wcc with argument
	{
		if ( args[1] == TXT( "help" ) )
		{
			//Only "wcc help"
			if ( args.Size() == 2 )
			{
				PrintCommandletsWithOneliners();
			}
			else //"wcc help sth"
			{
				ICommandlet *cmd = GetCommandlet( CName( args[2] ) );
				if ( cmd ) //commandlet exists
				{
					cmd->PrintHelp( );
				}
				else //no such command
				{
					ERR_WCC( TXT( "Command not found. Type wcc with no arguments to list all possible commands." ) );
					return false;
				}				
			}
		}
		else if ( args[1] == TXT( "nop" ) )
		{
			return true;
		}
		else // run commandlet
		{
			const Char* commandletName = args[1].AsChar();
			ICommandlet *cmd = GetCommandlet( CName( commandletName ) );
			if ( cmd ) //command exists
			{
				ICommandlet::CommandletOptions cmdOptions;
				if( !cmdOptions.ParseCommandline( 2, args ) )
				{
					ERR_WCC( TXT( "Failed to parse the commandline arguments" ) );
					return false;
				}

				LOG_WCC( TXT( "Starting commandlet '%ls'" ), commandletName );
				LOG_WCC( TXT( "---------------------------------------------------------------" ) );

				commandletSuccedded = cmd->Execute( cmdOptions );
			}
			else //no such command
			{
				ERR_WCC( TXT( "Command not found. Type wcc with no arguments to list all possible commands." ) );
				return false;
			}
		}
	}
	else //"rcc" with no arguments
	{
		PrintCommandletsWithOneliners();
	}

	// Report
	{
		// Suppress any more errors from being remembered
		m_rememberErrors = false;

		LOG_WCC( TXT( "---------------------------------------------------------------" ) );

		if( m_numErrors > 0 || m_numWarnings > 0 )
		{
			// Summary header
			LOG_WCC( TXT( "Total errors: %d" ), m_numErrors );
			LOG_WCC( TXT( "Total warnings: %d"), m_numWarnings );

			// Print errors
			if ( m_errors.Size() )
			{
				LOG_WCC( TXT( "---------------------------------------------------------------" ) );
				LOG_WCC( TXT( "Errors:" ) );

				for( Uint32 i = 0; i < m_errors.Size(); i++ )
				{
					LOG_WCC( TXT( "%s" ), m_errors[i].AsChar() );
				}
				LOG_WCC( TXT( "---------------------------------------------------------------" ) );
			}
		}
	}

	// Return status value
	return commandletSuccedded;
}

void CWccEngine::EnumCommandletNames(  TDynArray< CName > &commandletNames  ) const
{
	TDynArray< CClass* > commandletClasses;
	
	SRTTI::GetInstance().EnumClasses( ClassID<ICommandlet>(), commandletClasses );

	for ( Uint32 i=0; i < commandletClasses.Size(); i++ )
	{
		ICommandlet* commandlet = commandletClasses[i]->GetDefaultObject< ICommandlet >();
		commandletNames.PushBack( commandlet->GetName() );
	}
}


void CWccEngine::EnumCommandlets( TDynArray< ICommandlet* > &commandlets ) const
{
	TDynArray< CClass* > commandletClasses;

	SRTTI::GetInstance().EnumClasses( ClassID<ICommandlet>(), commandletClasses );

	for ( Uint32 i=0; i < commandletClasses.Size(); i++ )
	{
		commandlets.PushBack( commandletClasses[i]->GetDefaultObject< ICommandlet >() );
	}
}

void CWccEngine::PrintCommandletsWithOneliners() const
{
	TDynArray< ICommandlet* > commandlets;

	LOG_WCC( TXT( "" ) );

	EnumCommandlets( commandlets );

	for ( Uint32 i=0; i < commandlets.Size(); i++ )
	{
		String message = commandlets[i]->GetName().AsString() + TXT( " - " ) + commandlets[i]->GetOneLiner();
		LOG_WCC( message.AsChar() );
	}
}

ICommandlet* CWccEngine::GetCommandlet( const CName &name ) const
{
	TDynArray< CClass* > commandletClasses;

	SRTTI::GetInstance().EnumClasses( ClassID<ICommandlet>(), commandletClasses );

	for ( Uint32 i=0; i < commandletClasses.Size(); i++ )
	{
		ICommandlet* commandlet = commandletClasses[i]->GetDefaultObject< ICommandlet >();
		if( commandlet->GetName() == name)
			return commandlet;
	}

	return nullptr;
}

// TODO Remove!
void CalcRenderCameraFromEditorPreview( CRenderCamera& camera ) {}
Vector CalcCameraPosFromEditorPreview() { return Vector::ZERO_3D_POINT; }
