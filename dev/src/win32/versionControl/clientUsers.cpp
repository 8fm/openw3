#include "build.h"
#include "clientUsers.h"
#include <time.h>
#include "../../common/core/tokenizer.h"

CWatcher::CWatcher()
	: ClientUser()
	, m_result( SC_OK )
{};

CWatcher::CWatcher( Int32 result )
	: ClientUser()
	, m_result( result )
{};

void CWatcher::HandleError(Error *error)
{
	StrBuf  m;
	error->Fmt( &m );
	if ( ( Red::System::StringSearch(m.Text(), "no such file(s)" ) || ( Red::System::StringSearch( m.Text(), "file(s) not on client"))) )
	{
		m_result = SC_NOT_IN_DEPOT;
	}
	else if ( Red::System::StringSearch(m.Text(), "Can't clobber writable file") )
	{
		m_result = SC_WRITABLE;
	}
	else if ( Red::System::StringSearch(m.Text(), "file(s) up-to-date") )
	{
		m_result = SC_ERROR;
	}
	else if ( Red::System::StringSearch(m.Text(), "No files to submit") )
	{
		m_result = SC_FILES_IDENTICAL;
	}
	else
	{
		RED_LOG( RED_LOG_CHANNEL( Perforce ), TXT( "P4 Error: %" ) RED_PRIWas, m.Text() );
		m_result = SC_ERROR;
	}
};

void CFStatWatcher::OutputStat(StrDict *results)
{
	StrRef var, val;
	SetResult(SC_OK);

	// cleaning up structure
	m_stats->haveRev = -1;
	m_stats->headRev = -1;
	m_stats->headChange = -1;
	while ( m_stats->otherOpen.Size() > 0 )
	{
		m_stats->otherOpen.PopBack();
	}
	m_stats->action = String::EMPTY;
	m_stats->headAction = String::EMPTY;

	for ( int i = 0; results->GetVar( i, var, val ); i++ )
	{
		if ( var == "action" )
		{
			m_stats->action = ANSI_TO_UNICODE( val.Text() );
		}
		else if ( var == "headAction" )
		{
			m_stats->headAction = ANSI_TO_UNICODE( val.Text() );
		}
		else if ( Red::System::StringSearch(var.Text(), "otherOpen") && ( var != "otherOpen" ) )
		{
			m_stats->otherOpen.PushBack( ANSI_TO_UNICODE( val.Text() ) );
		}
		else if ( var == "headRev" )
		{
			m_stats->headRev = atoi(val.Text());
		}
		else if ( var == "haveRev" )
		{
			m_stats->haveRev = atoi(val.Text());
		}
		else if ( var == "headChange" )
		{
			m_stats->headChange = atoi(val.Text());
		}
	}
}

void CNewChangelistWatcher::InputData( StrBuf *strbuf, Error *e )
{
	RED_UNUSED( e );
	String result;

	// creating submit form - must be identical with editable submit form
	// provided by p4 submit shell command
	result += TXT("Change: new\nClient:");
	result += m_client;
	result += TXT("\nUser: ");
	result += m_user;
	result += TXT("\nStatus: new\nDescription:");
	result += m_description;
	result += TXT("\n");

	// setting form for the submit
	strbuf->Set(UNICODE_TO_ANSI(result.AsChar()));
}

void CNewChangelistWatcher::OutputInfo( char level, const char *data )
{
	RED_UNUSED( level );
	String returned = ANSI_TO_UNICODE( data );
	CTokenizer tokenizer( returned, TXT(" ") );
	// Expected output from p4: Change <number> created.
	if ( ! ( tokenizer.GetToken( 0 ) == TXT("Change") && tokenizer.GetToken( 2 ) == TXT("created.") && FromString<Uint32>( tokenizer.GetToken( 1 ), m_number ) ) )
	{
		m_number = 0;
	}
}

void CSubmitWatcher::InputData( StrBuf *strbuf, Error *e )
{
	RED_UNUSED( e );
	String result;

	// creating submit form - must be identical with editable submit form
	// provided by p4 submit shell command
	result += TXT("Change: new\nClient:");
	result += m_client;
	result += TXT("\nUser: ");
	result += m_user;
	result += TXT("\nStatus: new\nDescription:");
	result += m_description;
	result += TXT("\nFiles:\n");
	for (Uint32 i = 0; i < m_chosen.Size(); i++)
	{
		result += TXT("\t\t");
		result += m_chosen[i];
		result += TXT("\n");
	}

	// setting form for the submit
	strbuf->Set(UNICODE_TO_ANSI(result.AsChar()));
}

void COpenedWatcher::OutputStat(StrDict *results)
{
	StrRef var, val;
	for (int i = 0; results->GetVar( i, var, val ); i++)
	{
		if ( var == "depotFile" )
		{
			m_files.PushBack( ANSI_TO_UNICODE(val.Text()) );
		}
	}
}

void CFilesWatcher::OutputStat(StrDict *results)
{
	StrRef var, val;
	for (int i = 0; results->GetVar( i, var, val ); i++)
	{
		if ( var == "depotFile" )
		{
			m_files.PushBack( ANSI_TO_UNICODE(val.Text()) );
		}
	}
}

void CWhereWatcher::OutputStat(StrDict *results)
{
	StrRef var, val;

	for (int i = 0; results->GetVar( i, var, val ); i++)
	{
		if ( var == "depotFile" )
		{
			m_depot = ANSI_TO_UNICODE(val.Text());
		}
		else if ( var == "path" )
		{
			m_local = ANSI_TO_UNICODE(val.Text());
		}
	}	
}

void CFileLogWatcher::OutputStat( StrDict *varList )
{
	StrRef var, val;
	THashMap< String, String > map;
	for (int i = 0; varList->GetVar( i, var, val ); i++)
	{
		if ( Red::System::StringSearch( var.Text(), "rev" ) )
		{
			m_history.PushBack( map );
			m_history[m_history.Size() - 1].Insert( TXT("Revision"), ANSI_TO_UNICODE(val.Text()) );
		}

		if ( Red::System::StringSearch(var.Text(), "change") )
		{
			m_history[m_history.Size() - 1].Insert( TXT("Changelist"), ANSI_TO_UNICODE(val.Text()) );
		}

		if ( Red::System::StringSearch(var.Text(), "action") )
		{
			m_history[m_history.Size() - 1].Insert( TXT("Action"), ANSI_TO_UNICODE(val.Text()) );
		}

		if ( Red::System::StringSearch(var.Text(), "time") )
		{
			time_t time = atoi( val.Text() );
			struct tm timeInfo;
			char buf[100];
			localtime_s( &timeInfo, &time );
			strftime( buf, 100, "%Y-%m-%d %H:%M", &timeInfo );
			m_history[m_history.Size() - 1].Insert( TXT("Date"), ANSI_TO_UNICODE( buf ) );
		}

		if ( Red::System::StringSearch(var.Text(), "user") )
		{
			m_history[m_history.Size() - 1].Insert( TXT("Submitter"), ANSI_TO_UNICODE(val.Text()) );
		}

		if ( Red::System::StringSearch(var.Text(), "client") )
		{
			m_history[m_history.Size() - 1].Insert( TXT("Workspace"), ANSI_TO_UNICODE(val.Text()) );
		}

		if ( Red::System::StringSearch(var.Text(), "type") )
		{
			m_history[m_history.Size() - 1].Insert( TXT("Type"), ANSI_TO_UNICODE(val.Text()) );
		}

		if ( Red::System::StringSearch(var.Text(), "desc") )
		{
			m_history[m_history.Size() - 1].Insert( TXT("Description"), ANSI_TO_UNICODE(val.Text()) );
		}
	}
}

void CLogWatcher::OutputStat( StrDict *varList )
{
	StrRef var, val;

	for ( int i = 0; varList->GetVar( i, var, val ); ++i )
	{
		RED_LOG( RED_LOG_CHANNEL( Perforce ), TXT( "%d->%" ) RED_PRIWas TXT( "=%" ) RED_PRIWas, i, var.Text(), val.Text() );
	}
}

void CChangelistWatcher::OutputStat( StrDict *varList )
{
	StrRef var, val;

	for (int i = 0; varList->GetVar( i, var, val ); i++)
	{
		if ( Red::System::StringSearch(var.Text(), "time") )
		{
			m_data.m_time = ANSI_TO_UNICODE(val.Text());
		}
		else if ( Red::System::StringSearch(var.Text(), "user") )
		{
			m_data.m_user = ANSI_TO_UNICODE(val.Text());
		}
		else if ( Red::System::StringSearch(var.Text(), "client") )
		{
			m_data.m_client = ANSI_TO_UNICODE(val.Text());
		}
		else if ( Red::System::StringSearch(var.Text(), "desc") )
		{
			m_data.m_desc = ANSI_TO_UNICODE(val.Text());
		}
		else if ( Red::System::StringSearch(var.Text(), "depotFile") )
		{
			Int32 index = static_cast<Int32>( m_data.m_fields.Grow( 1 ) );
			m_data.m_fields[ index ].m_file = ANSI_TO_UNICODE(val.Text());
		}
		else if ( Red::System::StringSearch(var.Text(), "action") )
		{
			Int32 index = m_data.m_fields.SizeInt() - 1;
			ASSERT( index >= 0 );
			m_data.m_fields[ index ].m_action = ANSI_TO_UNICODE(val.Text());
		}
		else if ( Red::System::StringSearch(var.Text(), "type") )
		{
			Int32 index = m_data.m_fields.SizeInt() - 1;
			ASSERT( index >= 0 );
			m_data.m_fields[ index ].m_type = ANSI_TO_UNICODE(val.Text());
		}
		else if ( Red::System::StringSearch(var.Text(), "rev") )
		{
			Int32 index = m_data.m_fields.SizeInt() - 1;
			ASSERT( index >= 0 );
			m_data.m_fields[ index ].m_revision = ANSI_TO_UNICODE(val.Text());
		}
	}
}

void CChangesWatcher::OutputStat( StrDict *varList )
{
	StrRef var, val;

	for (int i = 0; varList->GetVar( i, var, val ); i++)
	{
		if ( Red::System::StringSearch(var.Text(), "change") )
		{
			Int32 index = static_cast< Int32 >( m_list.Grow( 1 ) );
			m_list[ index ].m_change = ANSI_TO_UNICODE(val.Text());
		}
		else if ( Red::System::StringSearch(var.Text(), "time") )
		{
			Int32 index = m_list.SizeInt() - 1;
			ASSERT( index >= 0 );
			m_list[ index ].m_time = ANSI_TO_UNICODE(val.Text());
		}
		else if ( Red::System::StringSearch(var.Text(), "user") )
		{
			Int32 index = m_list.SizeInt() - 1;
			ASSERT( index >= 0 );
			m_list[ index ].m_user = ANSI_TO_UNICODE(val.Text());
		}
		else if ( Red::System::StringSearch(var.Text(), "client") )
		{
			Int32 index = m_list.SizeInt() - 1;
			ASSERT( index >= 0 );
			m_list[ index ].m_client = ANSI_TO_UNICODE(val.Text());
		}
		else if ( Red::System::StringSearch(var.Text(), "desc") )
		{
			Int32 index = m_list.SizeInt() - 1;
			ASSERT( index >= 0 );
			m_list[ index ].m_desc = ANSI_TO_UNICODE(val.Text());
		}
	}
}

int CKeepAlive::IsAlive()
{
	if (++m_counter > 4)
	{
		m_broken = true;
		m_counter = 0;
		return 0;
	}
	return 1;
}
