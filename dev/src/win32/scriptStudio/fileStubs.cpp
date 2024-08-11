/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "../../common/redSystem/log.h"

#include "fileStubs.h"
#include "fileParserType.h"
#include "fileParser.h"
#include "documentView.h"
#include "lexData.h"
#include "../../common/redSystem/utilityTimers.h"
#include "frame.h"
#include "solution/file.h"

SSStubSystem GStubSystem;

SSFlagList::SSFlagList()
{
}

SSFlagList::SSFlagList( const wstring& flag )
{
	if ( !flag.empty() )
	{
		m_flags.push_back( flag );
	}
}

bool SSFlagList::Empty() const
{
	return m_flags.empty();
}

void SSFlagList::Clear()
{
	m_flags.clear();
}

bool SSFlagList::Has( const wstring& str ) const
{
	for ( size_t i=0; i<m_flags.size(); ++i )
	{
		if ( m_flags[i] == str )
		{
			return true;
		}
	}

	return false;
}

void SSFlagList::Add( const wstring& flag )
{
	if ( !flag.empty() && !Has( flag ) )
	{
		m_flags.push_back( flag );
	}
}

SSFlagList& SSFlagList::operator=( const wchar_t* flag )
{
	m_flags.clear();

	if( Red::System::StringLength( flag ) > 0 )
	{
		Add( flag );
	}

	return *this;
}

SSFlagList& SSFlagList::operator+( const wstring& flag )
{
	if( !flag.empty() )
	{
		Add( flag );
	}

	return *this;
}

SSFlagList& SSFlagList::operator+( const SSFlagList& flags )
{
	for ( size_t i=0; i < flags.m_flags.size(); ++i )
	{
		Add( flags.m_flags[ i ] );
	}

	return *this;
}

SSBasicStub::SSBasicStub( EStubType type, const SSFileContext& context, const wstring& name, const SSFlagList& flags )
	: m_type( type )
	, m_context( context )
	, m_flags( flags )
	, m_name( name )
{
}

SSBasicStub::~SSBasicStub()
{
}

SSPropertyStub::SSPropertyStub( const SSFileContext& context, const wstring& name, const SSFlagList& flags, const wstring& typeName )
	: SSBasicStub( STUB_Property, context, name, flags )
	, m_typeName( typeName )
{
}

SSPropertyStub::~SSPropertyStub()
{
}

SSFunctionStub::SSFunctionStub( const SSFileContext& context, const wstring& className, const wstring& name, const SSFlagList& flags )
	: SSBasicStub( STUB_Function, context, name, flags )
	, m_lastLine( -1 )
	, m_className( className )
{
}

SSFunctionStub::~SSFunctionStub()
{
	ClearVector( m_params );
	ClearVector( m_locals );
}

void SSFunctionStub::AddParam( SSPropertyStub* param )
{
	RED_ASSERT( param );
	m_params.push_back( param );
}

void SSFunctionStub::AddLocal( SSPropertyStub* local )
{
	RED_ASSERT( local );
	m_locals.push_back( local );
}

bool SSFunctionStub::IsLineInRange( int line ) const
{
	if ( m_lastLine != -1 && line >= m_context.m_line && line <= m_lastLine )
	{
		return true;
	}

	return false;
}

SSPropertyStub* SSFunctionStub::FindField( const wstring& name, bool searchInParent )
{
	// Linear search for local variable
	for ( vector< SSPropertyStub* >::const_iterator it=m_locals.begin(); it!=m_locals.end(); ++it )
	{
		if ( (*it)->m_name == name )
		{
			return *it;
		}
	}

	// Linear search for parameter
	for ( vector< SSPropertyStub* >::const_iterator it=m_params.begin(); it!=m_params.end(); ++it )
	{
		if ( (*it)->m_name == name )
		{
			return *it;
		}
	}

	// Not found
	return NULL;
}

SSClassStub::SSClassStub( const SSFileContext& context, const wstring& name, const SSFlagList& flags )
	: SSBasicStub( STUB_Struct, context, name, flags )
	, m_lastLine( -1 )
{
}

SSClassStub::SSClassStub( const SSFileContext& context, const wstring& name, const SSFlagList& flags, const wstring& extends )
	: SSBasicStub( STUB_Class, context, name, flags )
	, m_extends( extends )
	, m_lastLine( -1 )
{
}

SSClassStub::SSClassStub( const SSFileContext& context, const wstring& name, const SSFlagList& flags, const wstring& extends, const wstring& stateMachine )
	: SSBasicStub( STUB_State, context, name, flags )
	, m_extends( extends )
	, m_stateMachine( stateMachine )
	, m_lastLine( -1 )
{
}

SSClassStub::~SSClassStub()
{
	ClearVector( m_fields );
	ClearVector( m_functions );
}

void SSClassStub::AddField( SSPropertyStub* field )
{
	RED_ASSERT( field );
	m_fields.push_back( field );
}

void SSClassStub::AddFunction( SSFunctionStub* func )
{
	RED_ASSERT( func );
	m_functions.push_back( func );
}

bool SSClassStub::IsLineInRange( int line ) const
{
	if ( m_lastLine != -1 && line >= m_context.m_line && line <= m_lastLine )
	{
		return true;
	}

	return false;
}

SSClassStub* SSClassStub::FindParentClass()
{
	// Simple class
	if ( m_type == STUB_Class )
	{
		if ( !m_extends.empty() )
		{
			return GStubSystem.FindClass( m_extends );
		}
		else if ( m_name != wxT("CObject") )
		{
			return GStubSystem.FindClass( wxT("CObject") );
		}
	}

	// State
	if ( m_type == STUB_State )
	{
		// Get the state machine class
		SSClassStub* stateMachine = FindStateMachineClass();
		if ( stateMachine )
		{
			if ( m_extends.empty() || m_extends == m_name )
			{
				// Get the state with the same name in the parent class
				SSClassStub* parentStateMachine = stateMachine->FindParentClass();
				if ( parentStateMachine )
				{
					SSClassStub* parentState = parentStateMachine->FindState( m_name, true );
					if ( parentState )
					{
						return parentState;
					}
				}
			}
			else
			{
				// Find named state
				SSClassStub* parentState = stateMachine->FindState( m_extends, true  );
				if ( parentState )
				{
					return parentState;
				}
			}
		}

		// Use the state class
		return GStubSystem.FindClass( wxT("CScriptedState") );
	}

	// Structure, no parent class
	return NULL;
}

SSClassStub* SSClassStub::FindStateMachineClass()
{
	if ( m_type == STUB_State )
	{
		return GStubSystem.FindClass( m_stateMachine );
	}

	return NULL;
}

SSFunctionStub* SSClassStub::FindFunction( const wstring& name, bool searchInParent )
{
	// Linear search
	for ( vector< SSFunctionStub* >::const_iterator it=m_functions.begin(); it!=m_functions.end(); ++it )
	{
		if ( (*it)->m_name == name )
		{
			return *it;
		}
	}

	// Recurse to parent
	if ( searchInParent )
	{
		SSClassStub* parentClass = FindParentClass();
		if ( parentClass )
		{
			return parentClass->FindFunction( name, searchInParent );
		}
	}

	// Not found
	return NULL;
}

SSPropertyStub* SSClassStub::FindField( const wstring& name, bool searchInParent )
{
	// Linear search
	for ( vector< SSPropertyStub* >::const_iterator it=m_fields.begin(); it!=m_fields.end(); ++it )
	{
		if ( (*it)->m_name == name )
		{
			return *it;
		}
	}

	// Recurse to parent
	if ( searchInParent )
	{
		SSClassStub* parentClass = FindParentClass();
		if ( parentClass )
		{
			return parentClass->FindField( name, searchInParent );
		}
	}

	// Not found
	return NULL;
}

SSClassStub* SSClassStub::FindState( const wstring& name, bool searchInParent )
{
	// Check current state
	SSClassStub* state = GStubSystem.FindState( this, name );
	if ( state )
	{
		return state;
	}

	// Recurse to parent
	if ( searchInParent )
	{
		SSClassStub* parentClass = FindParentClass();
		if ( parentClass )
		{
			return parentClass->FindState( name, searchInParent );
		}
	}

	// Not found
	return NULL;
}

SSEnumOptionStub::SSEnumOptionStub( const SSFileContext& context, const wstring& name, int value )
	: SSBasicStub( STUB_EnumOption, context, name, SSFlagList() )
	, m_value( value )
{};

SSEnumOptionStub::~SSEnumOptionStub()
{
}

SSEnumStub::SSEnumStub( const SSFileContext& context, const wstring& name )
	: SSBasicStub( STUB_Enum, context, name, SSFlagList() )
{
}

SSEnumStub::~SSEnumStub()
{
	ClearVector( m_options );
}

void SSEnumStub::AddOption( SSEnumOptionStub* option )
{
	RED_ASSERT( option );
	m_options.push_back( option );
}

SSEnumOptionStub* SSEnumStub::FindOption( const wstring& name )
{
	for ( vector<SSEnumOptionStub* >::iterator i=m_options.begin(); i!=m_options.end(); ++i )
	{
		SSEnumOptionStub* option = *i;
		if ( option->m_name == name )
		{
			return option;
		}
	}
	
	return NULL;
}

SSFileStub::SSFileStub( const wstring& solutionPath )
:	SSBasicStub( STUB_File, SSFileContext( solutionPath, 1 ), solutionPath, SSFlagList() )
,	m_lexData( nullptr )
{
	m_lexData = new SSLexicalData;
}

SSFileStub::~SSFileStub()
{
	ClearVector( m_classes );
	ClearVector( m_functions );
	ClearVector( m_enums );

	if( m_lexData )
	{
		delete m_lexData;
	}
}

void SSFileStub::AddClass( SSClassStub* classStub )
{
	m_classes.push_back( classStub );
}

void SSFileStub::AddEnum( SSEnumStub* enumStub )
{
	m_enums.push_back( enumStub );
}

void SSFileStub::AddFunction( SSFunctionStub* functionStub )
{
	m_functions.push_back( functionStub );
}

SSStubParsingThread::SSStubParsingThread()
:	CThread( "Stub Parsing Thread" )
,	m_requestExit( false )
,	m_paused( false )
,	m_shutdown( false )
,	m_allocProxy( &realloc, &free )
{
	m_definition.Initialize( &m_allocProxy );

	m_definition.AddLiteral( Red::Scripts::Literal_String, TOKEN_STRING );
	m_definition.AddLiteral( Red::Scripts::Literal_Name, TOKEN_NAME );
	m_definition.AddLiteral( Red::Scripts::Literal_Integer, TOKEN_INTEGER );
	m_definition.AddLiteral( Red::Scripts::Literal_Float, TOKEN_FLOAT );
	m_definition.AddLiteral( Red::Scripts::Literal_Identifier, TOKEN_IDENT );

	// Keywords
	m_definition.AddKeyword( TXT("import"), TOKEN_IMPORT	); 
	m_definition.AddKeyword( TXT("abstract"), TOKEN_ABSTRACT ); 
	m_definition.AddKeyword( TXT("const"), TOKEN_CONST ); 
	m_definition.AddKeyword( TXT("extends"), TOKEN_EXTENDS ); 
	m_definition.AddKeyword( TXT("in"), TOKEN_IN	); 
	m_definition.AddKeyword( TXT("class"), TOKEN_CLASS ); 
	m_definition.AddKeyword( TXT("statemachine"), TOKEN_STATEMACHINE ); 
	m_definition.AddKeyword( TXT("state"), TOKEN_STATE ); 
	m_definition.AddKeyword( TXT("enum"), TOKEN_ENUM ); 
	m_definition.AddKeyword( TXT("struct"), TOKEN_STRUCT ); 
	m_definition.AddKeyword( TXT("function"), TOKEN_FUNCTION ); 
	m_definition.AddKeyword( TXT("def"), TOKEN_DEF ); 
	m_definition.AddKeyword( TXT("editable"), TOKEN_EDITABLE ); 
	m_definition.AddKeyword( TXT("final"), TOKEN_FINAL ); 
	m_definition.AddKeyword( TXT("out"), TOKEN_OUT ); 
	m_definition.AddKeyword( TXT("optional"), TOKEN_OPTIONAL ); 
	m_definition.AddKeyword( TXT("autobind"), TOKEN_AUTOBIND ); 
	m_definition.AddKeyword( TXT("any"), TOKEN_ANY ); 
	m_definition.AddKeyword( TXT("single"), TOKEN_SINGLE ); 
	m_definition.AddKeyword( TXT("local"), TOKEN_LOCAL ); 
	m_definition.AddKeyword( TXT("inlined"), TOKEN_INLINED );
	m_definition.AddKeyword( TXT("private"), TOKEN_PRIVATE );
	m_definition.AddKeyword( TXT("protected"), TOKEN_PROTECTED );
	m_definition.AddKeyword( TXT("public"), TOKEN_PUBLIC );
	m_definition.AddKeyword( TXT("event"), TOKEN_EVENT );
	m_definition.AddKeyword( TXT("timer"), TOKEN_TIMER );
	m_definition.AddKeyword( TXT("cleanup"), TOKEN_CLEANUP );
	m_definition.AddKeyword( TXT("array"), TOKEN_ARRAY );
	m_definition.AddKeyword( TXT("string"), TOKEN_TYPE_STRING );
	m_definition.AddKeyword( TXT("byte"), TOKEN_TYPE_BYTE );
	m_definition.AddKeyword( TXT("bool"), TOKEN_TYPE_BOOL );
	m_definition.AddKeyword( TXT("int"), TOKEN_TYPE_INT );
	m_definition.AddKeyword( TXT("float"), TOKEN_TYPE_FLOAT );
	m_definition.AddKeyword( TXT("name"), TOKEN_TYPE_NAME );
	m_definition.AddKeyword( TXT("void"), TOKEN_TYPE_VOID );
	m_definition.AddKeyword( TXT("defaults"), TOKEN_DEFAULTS );
	m_definition.AddKeyword( TXT("default"), TOKEN_DEFAULT );
	m_definition.AddKeyword( TXT("hint"), TOKEN_HINT );
	m_definition.AddKeyword( TXT("true"), TOKEN_BOOL_TRUE );
	m_definition.AddKeyword( TXT("false"), TOKEN_BOOL_FALSE );
	m_definition.AddKeyword( TXT("NULL"), TOKEN_NULL );
	m_definition.AddKeyword( TXT("entry"), TOKEN_ENTRY );
	m_definition.AddKeyword( TXT("enter"), TOKEN_ENTER );
	m_definition.AddKeyword( TXT("latent"), TOKEN_LATENT );
	m_definition.AddKeyword( TXT("var"), TOKEN_VAR );
	m_definition.AddKeyword( TXT("exec"), TOKEN_EXEC );
	m_definition.AddKeyword( TXT("storyscene"), TOKEN_SCENE );
	m_definition.AddKeyword( TXT("saved"), TOKEN_SAVED );
	m_definition.AddKeyword( TXT("quest"), TOKEN_QUEST );
	m_definition.AddKeyword( TXT("savepoint"), TOKEN_SAVEPOINT );
	m_definition.AddKeyword( TXT("reward"), TOKEN_REWARD );

	m_definition.AddKeyword( TXT("+="), TOKEN_OP_IADD );
	m_definition.AddKeyword( TXT("-="), TOKEN_OP_ISUB );
	m_definition.AddKeyword( TXT("*="), TOKEN_OP_IMUL );
	m_definition.AddKeyword( TXT("/="), TOKEN_OP_IDIV );
	m_definition.AddKeyword( TXT("&="), TOKEN_OP_IAND );
	m_definition.AddKeyword( TXT("|="), TOKEN_OP_IOR );
	m_definition.AddKeyword( TXT("||"), TOKEN_OP_LOGIC_OR );
	m_definition.AddKeyword( TXT("&&"), TOKEN_OP_LOGIC_AND );
	m_definition.AddKeyword( TXT("=="), TOKEN_OP_EQUAL );
	m_definition.AddKeyword( TXT("!="), TOKEN_OP_NOTEQUAL );
	m_definition.AddKeyword( TXT("=="), TOKEN_OP_EQUAL );
	m_definition.AddKeyword( TXT(">="), TOKEN_OP_GREQ );
	m_definition.AddKeyword( TXT("<="), TOKEN_OP_LEEQ );
	m_definition.AddKeyword( TXT("new"), TOKEN_NEW );
	m_definition.AddKeyword( TXT("in"), TOKEN_IN );
	m_definition.AddKeyword( TXT("delete"), TOKEN_DELETE );
	m_definition.AddKeyword( TXT("if"), TOKEN_IF );
	m_definition.AddKeyword( TXT("else"), TOKEN_ELSE );
	m_definition.AddKeyword( TXT("switch"), TOKEN_SWITCH );
	m_definition.AddKeyword( TXT("case"), TOKEN_CASE );
	m_definition.AddKeyword( TXT("for"), TOKEN_FOR );
	m_definition.AddKeyword( TXT("while"), TOKEN_WHILE );
	m_definition.AddKeyword( TXT("do"), TOKEN_DO );
	m_definition.AddKeyword( TXT("return"), TOKEN_RETURN );
	m_definition.AddKeyword( TXT("break"), TOKEN_BREAK );
	m_definition.AddKeyword( TXT("continue"), TOKEN_CONTINUE );
	m_definition.AddKeyword( TXT("this"), TOKEN_THIS );
	m_definition.AddKeyword( TXT("super"), TOKEN_SUPER );
	m_definition.AddKeyword( TXT("parent"), TOKEN_PARENT );
	m_definition.AddKeyword( TXT("virtual_parent"), TOKEN_VIRTUAL_PARENT );

	m_definition.AddKeyword( TXT("theGame"), TOKEN_GET_GAME );
	m_definition.AddKeyword( TXT("thePlayer"), TOKEN_GET_PLAYER );
	m_definition.AddKeyword( TXT("theCamera"), TOKEN_GET_CAMERA );
	m_definition.AddKeyword( TXT("theSound"), TOKEN_GET_SOUND );
	m_definition.AddKeyword( TXT("theDebug"), TOKEN_GET_DEBUG );
	m_definition.AddKeyword( TXT("theTimer"), TOKEN_GET_TIMER );
	m_definition.AddKeyword( TXT("theInput"), TOKEN_GET_INPUT );
	m_definition.AddKeyword( TXT("theTelemetry"), TOKEN_GET_TELEMETRY );

	// Chars
	m_definition.AddChar( '(' );
	m_definition.AddChar( ')' );
	m_definition.AddChar( '{' );
	m_definition.AddChar( '}' );
	m_definition.AddChar( '[' );
	m_definition.AddChar( ']' );
	m_definition.AddChar( '<' );
	m_definition.AddChar( '>' );
	m_definition.AddChar( ',' );
	m_definition.AddChar( '=' );
	m_definition.AddChar( ';' );
	m_definition.AddChar( '&' );
	m_definition.AddChar( '|' );
	m_definition.AddChar( '^' );
	m_definition.AddChar( '+' );
	m_definition.AddChar( '-' );
	m_definition.AddChar( '*' );
	m_definition.AddChar( '/' );
	m_definition.AddChar( '%' );
	m_definition.AddChar( '.' );
	m_definition.AddChar( '!' );
	m_definition.AddChar( '?' );
	m_definition.AddChar( ':' );
	m_definition.AddChar( '~' );
}

SSStubParsingThread::~SSStubParsingThread()
{
}

bool operator==( const SolutionFileWeakPtr& left, const SolutionFileWeakPtr& right )
{
	return left.lock() == right.lock();
}

void SSStubParsingThread::Schedule( const SolutionFileWeakPtr& file )
{
	SSStubSystemWriteLock lock;

	// Add file to list if not already on it
	deque< SolutionFileWeakPtr >::iterator it = find( m_filesToProcess.begin(), m_filesToProcess.end(), file );
	if ( it == m_filesToProcess.end() )
	{
		m_filesToProcess.push_back( file );
	}

	// Wake up the thread
	m_idleCondition.WakeAll();
}

void SSStubParsingThread::Unschedule( const SolutionFileWeakPtr& file )
{
	SSStubSystemWriteLock lock;
	
	// Remove the file if in the list
	deque< SolutionFileWeakPtr >::iterator it = find( m_filesToProcess.begin(), m_filesToProcess.end(), file );
	if ( it != m_filesToProcess.end() )
	{
		m_filesToProcess.erase( it );
	}
}

void SSStubParsingThread::RequestExit()
{
	m_requestExit = true;

	while( !HasShutdown() )
	{
		m_idleCondition.WakeAll();
		Red::Threads::YieldCurrentThread();
	}
}

void SSStubParsingThread::ProcessFile( const SolutionFileWeakPtr& weakFile )
{
	SolutionFilePtr solutionFile = weakFile.lock();

	if( solutionFile )
	{
		// Stats
		if ( wxTheFrame )
		{
			wxTheFrame->SetStatusBarText( wxT( "Parsing file: %s" ), solutionFile->m_name.c_str() );
		}
		RED_LOG( RED_LOG_CHANNEL( StubParsingThread ), TXT( "Parsing file '%ls'" ), solutionFile->m_solutionPath.c_str() );

		// Get code
		const wstring& solutionPath = solutionFile->m_solutionPath;
		wstring code = solutionFile->GetText().wc_str();
	
		// Parser file
		SSFileParser parser;
		SSFileStub* newFileStub = new SSFileStub( solutionPath );
		if ( parser.ParseFile( newFileStub, solutionPath, code, m_definition ) )
		{
			Red::System::Double registrationTime = 0.0;
			{
				Red::System::ScopedStopClock registrationTimer( registrationTime );

				SSStubSystemWriteLock lock;

				SSFileStub* oldFileStub = solutionFile->m_stubs;
				GStubSystem.Unregister( oldFileStub );
				delete oldFileStub;

				solutionFile->m_stubs = newFileStub;
				GStubSystem.Register( newFileStub );
			}

			RED_LOG( RED_LOG_CHANNEL( StubParsingThread ), TXT( "Registration took %lf" ), registrationTime );

			// Reparse context combo box
			if ( solutionFile->m_documentEx )
			{
				Red::System::Double reparseTime = 0.0;
				{
					Red::System::ScopedStopClock timer( reparseTime );
					solutionFile->m_documentEx->Reparse();
				}

				RED_LOG( RED_LOG_CHANNEL( StubParsingThread ), TXT( "Reparse took %lf" ), reparseTime );
			}
		}
		else
		{
			// Error
			RED_LOG_ERROR( RED_LOG_CHANNEL( StubParsingThread ), TXT( "Error parsing '%ls'" ), solutionFile->m_solutionPath.c_str() );

			delete newFileStub;
		}
	}
}

void SSStubParsingThread::ThreadFunc()
{
	// Process the data
	while ( !m_requestExit )
	{	
		if( !m_paused )
		{
			if ( !m_filesToProcess.empty() )
			{
				// Pop the file to process
				const SolutionFileWeakPtr& file = m_filesToProcess.front();

				// Process the file
				ProcessFile( file );

				{
					SSStubSystemWriteLock lock;
					m_filesToProcess.pop_front();
				}
			}
			else
			{
				Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_idleLock );
				m_idleCondition.Wait( m_idleLock );
			}
		}
		else
		{
			// Yield for a little while
			Red::Threads::YieldCurrentThread();
		}
	}

	m_shutdown = true;
}

void SSStubParsingThread::Pause( bool flag )
{
	m_paused = flag;
}

SSStubSystemReadLock::SSStubSystemReadLock()
{
	RED_LOG( LOCK, TXT( "read acquire" ) );
	AcquireSRWLockShared( &( GStubSystem.m_rwLock ) );
}

SSStubSystemReadLock::~SSStubSystemReadLock()
{
	ReleaseSRWLockShared( &( GStubSystem.m_rwLock ) );
	RED_LOG( LOCK, TXT( "read release" ) );
}

SSStubSystemReadLockNonBlocking::SSStubSystemReadLockNonBlocking()
:	m_locked( false )
{

}

SSStubSystemReadLockNonBlocking::~SSStubSystemReadLockNonBlocking()
{
	if( m_locked )
	{
		ReleaseSRWLockShared( &( GStubSystem.m_rwLock ) );
		m_locked = false;
	}
}

bool SSStubSystemReadLockNonBlocking::TryLock()
{
	if( TryAcquireSRWLockShared( &( GStubSystem.m_rwLock ) ) )
	{
		m_locked = true;
	}

	return m_locked;
}

SSStubSystemWriteLock::SSStubSystemWriteLock()
{
	RED_LOG( LOCK, TXT( "write acquire" ) );
	AcquireSRWLockExclusive( &( GStubSystem.m_rwLock ) );
}

SSStubSystemWriteLock::~SSStubSystemWriteLock()
{
	ReleaseSRWLockExclusive( &( GStubSystem.m_rwLock ) );
	RED_LOG( LOCK, TXT( "write release" ) );
}

unsigned int SSStubParsingThread::GetFilesCountToProcess() const
{
	SSStubSystemReadLock lock;

	return m_filesToProcess.size();
}

SSStubSystem::SSStubSystem()
	: m_parsingThread( NULL )
{
	InitializeSRWLock( &m_rwLock );
}

SSStubSystem::~SSStubSystem()
{
	if( m_parsingThread )
	{
		StopThread();
	}
}

void SSStubSystem::StartThread()
{
	RED_ASSERT( !m_parsingThread  );

	// Create parsing thread
	m_parsingThread = new SSStubParsingThread();
}

void SSStubSystem::StopThread()
{
	m_parsingThread->RequestExit();
	m_parsingThread->JoinThread();

	delete m_parsingThread;
	m_parsingThread = nullptr;
}

void SSStubSystem::StartProcessing()
{
	// Start processing
	m_parsingThread->InitThread();
}

void SSStubSystem::Pause()
{
	m_parsingThread->Pause( true );
}

void SSStubSystem::Unpause()
{
	m_parsingThread->Pause( false );
}

void SSStubSystem::Register( SSFileStub* fileStub )
{
	// Classes
	for( size_t i = 0; i < fileStub->m_classes.size(); ++i )
	{
		SSClassStub* stub = fileStub->m_classes[ i ];
		m_classes[ stub->m_name ] = stub;
	}

	// Functions
	for( size_t i = 0; i < fileStub->m_functions.size(); ++i )
	{
		SSFunctionStub* stub = fileStub->m_functions[ i ];
		m_functions[ stub->m_name ] = stub;
	}

	// Enums
	for( size_t i = 0; i < fileStub->m_enums.size(); ++i )
	{
		SSEnumStub* stub = fileStub->m_enums[ i ];
		m_enums[ stub->m_name ] = stub;

		for( size_t j = 0; j < stub->m_options.size(); ++j )
		{
			m_enumMembers[ stub->m_options[ j ]->m_name ] = stub;
		}
	}
}

void SSStubSystem::Unregister( SSFileStub* fileStub )
{
	// Classes
	for( size_t i = 0; i < fileStub->m_classes.size(); ++i )
	{
		SSClassStub* stub = fileStub->m_classes[ i ];
		m_classes.erase( stub->m_name );
	}

	// Functions
	for( size_t i = 0; i < fileStub->m_functions.size(); ++i )
	{
		SSFunctionStub* stub = fileStub->m_functions[ i ];
		m_functions.erase( stub->m_name );
	}

	// Enums
	for( size_t i = 0; i < fileStub->m_enums.size(); ++i )
	{
		SSEnumStub* stub = fileStub->m_enums[ i ];
		m_enums.erase( stub->m_name );

		for( size_t j = 0; j < stub->m_options.size(); ++j )
		{
			m_enumMembers.erase( stub->m_options[ j ]->m_name );
		}
	}

	if ( wxTheFrame )
	{
		wxTheFrame->InvalidateParsedFiles();
	}
}

void SSStubSystem::Schedule( const SolutionFilePtr& file )
{
	m_parsingThread->Schedule( file );
}

void SSStubSystem::Unschedule( const SolutionFilePtr& file )
{
	m_parsingThread->Unschedule( file );
}

unsigned int SSStubSystem::GetFilesCountToProcess() const
{
	if ( m_parsingThread )
	{
		return m_parsingThread->GetFilesCountToProcess();
	}
	else
	{
		return 0;
	}
}

void SSStubSystem::RegisterGlobalKeyword( const wstring& scriptKeyword, const wstring& cppClass )
{
	m_globalKeywords[ scriptKeyword ] = cppClass;
}

SSClassStub* SSStubSystem::FindClass( const wstring& name ) const
{
	map< wstring, SSClassStub* >::const_iterator it = m_classes.find( name );
	if ( it != m_classes.end() )
	{
		return it->second;
	}
	
	return nullptr;
}

SSClassStub* SSStubSystem::FindState( SSClassStub* classStub, const wstring& name ) const
{
	if ( classStub )
	{
		for ( map< wstring, SSClassStub* >::const_iterator it = m_classes.begin(); it != m_classes.end(); ++it )
		{
			SSClassStub* state = it->second;
			if ( state->m_type == STUB_State )
			{
				if ( state->m_stateMachine == classStub->m_name )
				{
					if ( state->m_name == name )
					{
						return state;
					}
				}
			}
		}
	}

	return NULL;
}

SSEnumStub* SSStubSystem::FindEnum( const wstring& name ) const
{
	map< wstring, SSEnumStub* >::const_iterator it = m_enums.find( name );
	if ( it != m_enums.end() )
	{
		return it->second;
	}

	return NULL;
}

SSEnumStub* SSStubSystem::FindEnumByMember( const wstring& name ) const
{
	map< wstring, SSEnumStub* >::const_iterator it = m_enumMembers.find( name );
	if ( it != m_enumMembers.end() )
	{
		return it->second;
	}

	return NULL;
}

SSFunctionStub* SSStubSystem::FindFunction( const wstring& name ) const
{
	map< wstring, SSFunctionStub* >::const_iterator it = m_functions.find( name );
	if ( it != m_functions.end() )
	{
		return it->second;
	}

	return NULL;
}

bool SSStubSystem::FindFunctionAtLine( const wstring& file, int line, SSClassStub*& classStub, SSFunctionStub*& functionStub ) const
{
	// Search global functions
	for ( map< wstring, SSFunctionStub* >::const_iterator it = m_functions.begin(); it != m_functions.end(); ++it )
	{
		SSFunctionStub* stub = it->second;
		if( stub->m_context.m_file == file )
		{
			if ( stub->IsLineInRange( line ) )
			{
				classStub = NULL;
				functionStub = stub;
				return true;
			}
		}
	}

	// Search in classes
	for ( map< wstring, SSClassStub* >::const_iterator it = m_classes.begin(); it != m_classes.end(); ++it )
	{
		SSClassStub* stub = it->second;
		if( stub->m_context.m_file == file )
		{
			if ( stub->IsLineInRange( line ) )
			{
				// Search in functions in this class
				for ( vector< SSFunctionStub* >::const_iterator fit = stub->m_functions.begin(); fit != stub->m_functions.end(); ++fit )
				{
					if ( (*fit)->IsLineInRange( line ) )
					{
						classStub = stub;
						functionStub = *fit;
						return true;
					}
				}

				// Only class was found
				classStub = stub;
				functionStub = NULL;
				return true;
			}
		}
	}

	// Not found
	return false;
}

static bool TestMatch( SSBasicStub* stub, const wstring& initialMatch )
{
	if ( !initialMatch.empty() )
	{
		const size_t matchLength = initialMatch.length();
		return 0 == wcsncmp( stub->m_name.c_str(), initialMatch.c_str(), matchLength );
	}

	return true;
}

static bool SkipBrakets( size_t& pos, const vector< wstring >& tokens, const wstring& left, const wstring& right )
{
	int count = 1;
	while ( pos < tokens.size() )
	{
		// Pop token
		wstring name = tokens[pos++];

		// Check for brackets match
		if ( name == left )
		{
			count++;
		}
		if ( name == right )
		{
			count--;
			if ( !count )
			{
				return true;
			}
		}
	}

	// Not matched
	return false;
}

bool SSStubSystem::ResolveExpressionType( SSClassStub* classStub, SSFunctionStub* functionStub, const vector< wstring >& expressionTokens, wstring& outTypeName, SSFunctionStub*& outFunction ) const
{	
	// dex_fix: this is a very simple implementation, this should be more complicated than this
	// dex_fix: right now it does not support arrays, function calls, etc

	// Assume context of class
	wstring typeName = wxEmptyString;
	SSFunctionStub* lastFunction = NULL;

	// Process tokens
	size_t pos = 0;
	bool functionsOnly = false;
	bool firstImportantToken = true;
	while ( pos < expressionTokens.size() )
	{
		// Get the symbol
		wstring name = expressionTokens[ pos++ ];

		// Since we've parsed next token it could not be the function
		lastFunction = NULL;

		if( name == wxT( "class" ) && pos < expressionTokens.size() )
		{
			typeName = expressionTokens[ pos++ ];
			lastFunction = nullptr;
			break;
		}

		if( name == wxT( "return" ) || name == wxT( "import" ) || name == wxT( "statemachine" ) )
		{
			continue;
		}

		// Brackets, skip
		if ( name == wxT("(") || name == wxT(")") )
		{
			// This is not safe :)
			continue;
		}

		// Array access
		if ( name == wxT("[") )
		{
			// Skip till the closing bracket
			if ( !SkipBrakets( pos, expressionTokens, wxT("["), wxT("]") ) )
			{
				return false;
			}

			// Array type ?
			if ( !typeName.empty() && typeName[0] == '@' )
			{
				// Skip array type
				typeName = typeName.c_str() + 1;
				continue;
			}

			// Not an array
			return false;
		}

		// Check if it's not casting
		if ( (int)pos - 2 >= 0 && expressionTokens[ pos - 2 ] == wxT("(") && pos < expressionTokens.size() && expressionTokens[ pos ] == wxT(")") )
		{
			bool foundCasting = false;
			for ( map< wstring, SSClassStub* >::const_iterator i = m_classes.begin(); i != m_classes.end(); ++i )
			{
				if ( i->first == name )
				{
					foundCasting = true;
					break;
				}
			}

			if ( foundCasting )
			{
				continue;
			}
		}

		// Context operator
		if ( name == wxT(".") || firstImportantToken )
		{
			// Get the real context
			if ( name == wxT(".") )
			{
				// Nothing more, invalid expression
				if ( pos >= expressionTokens.size() )
				{
					return false;
				}

				// Pop the indent
				name = expressionTokens[ pos++ ];
			}

			// This :)
			if ( name == wxT("this") )
			{
				if ( pos == 1 && classStub )
				{
					typeName = classStub->m_name;
					firstImportantToken = false;
					continue;
				}

				// Invalid context for "this"
				return false;
			}

			// Super 
			if ( name == wxT("super") )
			{
				if ( pos == 1 && classStub )
				{
					classStub = classStub->FindParentClass();
					if ( classStub )
					{
						typeName = classStub->m_name;
						firstImportantToken = false;
						functionsOnly = true;
						continue;
					}
				}

				// Invalid context for "super"
				return false;
			}

			// Parent, valid only in classes
			if ( name == wxT("parent") )
			{
				if ( classStub )
				{
					classStub = classStub->FindStateMachineClass();
					if ( classStub )
					{
						typeName = classStub->m_name;
						firstImportantToken = false;
						continue;
					}
				}

				// Invalid context for "parent"
				return false;
			}

			// check globals
			bool foundGlobalStub = false;
			for( map< wstring, wstring >::const_iterator iter = m_globalKeywords.begin(); iter != m_globalKeywords.end(); ++iter )
			{
				if( name == iter->first )
				{
					if( pos == 1 )
					{
						classStub = GStubSystem.FindClass( iter->second );
						if ( classStub )
						{
							typeName = classStub->m_name;
							functionsOnly = false;

							foundGlobalStub = true;
							break;
						}
					}

					// This should never happen
					return false;
				}
			}

			if( foundGlobalStub )
			{
				firstImportantToken = false;
				continue;
			}

			// Get the scope
			SSClassStub* scopeType = typeName.empty() ? classStub : FindClass( typeName );
			if ( scopeType )
			{
				// Find property
				SSPropertyStub* prop = scopeType->FindField( name, true );
				if ( prop && !functionsOnly )
				{
					// Use type property type
					typeName = prop->m_typeName;
					firstImportantToken = false;
					continue;
				}

				// Try as function class
				SSFunctionStub* func = scopeType->FindFunction( name, true );
				if ( !func )
				{
					// State entry function
					for ( map< wstring, SSClassStub* >::const_iterator i=m_classes.begin(); i!=m_classes.end(); ++i )
					{
						SSClassStub* stateClassStub = i->second;
						if ( stateClassStub->m_stateMachine == scopeType->m_name )
						{
							SSFunctionStub* entryFunc = stateClassStub->FindFunction( name, false );
							if ( entryFunc && entryFunc->m_flags.Has( wxT("entry") ) )
							{
								func = entryFunc;
								break;
							}
						}
					}
				}		

				// Process
				if ( func )
				{
					// Skip function params
					if ( pos < expressionTokens.size() && expressionTokens[pos] == wxT("(") )
					{
						// Skip the bracket :)
						pos++;

						// Skip the function call
						if ( !SkipBrakets( pos, expressionTokens, wxT("("), wxT(")") ) )
						{
							return false;
						}
					}

					// Remember last function
					lastFunction = func;

					// Use function return type
					typeName = func->m_retValueType;
					firstImportantToken = false;
					continue;
				}
			}

			// Global function
			SSFunctionStub* func = FindFunction( name );
			if ( func )
			{
				// Skip function params
				if ( pos < expressionTokens.size() && expressionTokens[pos] == wxT("(") )
				{
					// Skip the bracket :)
					pos++;

					// Skip the function call
					if ( !SkipBrakets( pos, expressionTokens, wxT("("), wxT(")") ) )
					{
						return false;
					}
				}

				// Remember last function
				lastFunction = func;

				// Use function return type
				typeName = func->m_retValueType;
				firstImportantToken = false;
				continue;
			}

			// Function variable
			if ( functionStub && firstImportantToken && !functionsOnly )
			{
				SSPropertyStub* prop = functionStub->FindField( name, false );
				if ( prop )
				{
					typeName = prop->m_typeName;
					firstImportantToken = false;
					continue;
				}
			}

			// Not  found
			return false;
		}

		// Invalid match
		return false;
	}

	// Something was matched
	outFunction = lastFunction;
	outTypeName = typeName;
	return true;
}

SSFunctionStub* SSStubSystem::ResolveFunctionCall( SSClassStub* classStub, SSFunctionStub* functionStub, const vector< wstring >& expressionTokens ) const
{
	// Resolve expression as function name
	wstring type;
	SSFunctionStub* func = NULL;
	if ( ResolveExpressionType( classStub, functionStub, expressionTokens, type, func ) )
	{
		return func;
	}

	// Not resolved
	return NULL;
}

bool SSStubSystem::CollectVisibleSymbols( SSClassStub* classStub, SSFunctionStub* functionStub, vector< wstring >& expressionTokens, const wstring& initialMatch, bool force, vector< SSBasicStub* >& stubs ) const
{
	// Skip the "local"
	bool typesOnly = false;
	if ( expressionTokens.size() && expressionTokens[0] == wxT("local") )
	{
		expressionTokens.erase( expressionTokens.begin() );
		typesOnly = true;
	}

	// No code, types only
	if ( !functionStub )
	{
		typesOnly = true;
	}

	// Resolve expression type
	if ( !expressionTokens.empty() )
	{
		// Resolve type
		wstring type;
		SSFunctionStub* func = NULL;
		if ( !ResolveExpressionType( classStub, functionStub, expressionTokens, type, func ) )
		{
			return false;
		}

		// Change context
		classStub = FindClass( type );
		//functionStub = NULL;
	}

	// Function symbols
	if ( functionStub && !typesOnly )
	{
		// Local variables
		for ( size_t i=0; i<functionStub->m_locals.size(); i++ )
		{
			if ( TestMatch( functionStub->m_locals[i], initialMatch	 ) )
			{
				stubs.push_back( functionStub->m_locals[i] );
			}
		}

		// Parameters
		for ( size_t i=0; i<functionStub->m_params.size(); i++ )
		{
			if ( TestMatch( functionStub->m_params[i], initialMatch	 ) )
			{
				stubs.push_back( functionStub->m_params[i] );
			}
		}
	}

	// Class stuff
	if ( !typesOnly && classStub )
	{
		vector< SSClassStub* > classHierarchy;

		// in case of casting check if any class name is present in tokens
		SSClassStub* tmpClassStub = classStub;
		for ( map< wstring, SSClassStub* >::const_iterator i = m_classes.begin(); i != m_classes.end(); ++i )
		{
			SSClassStub* tmpClassStub = i->second;
			if ( std::find( expressionTokens.begin(), expressionTokens.end(), tmpClassStub->m_name ) != expressionTokens.end() )
			{
				while ( tmpClassStub )
				{
					if ( tmpClassStub->m_extends == classStub->m_name )
					{
						classHierarchy.push_back( i->second );
						break;
					}
					tmpClassStub = GStubSystem.FindClass( tmpClassStub->m_extends );
				}
			}
		}

		size_t pos = 0;
		int n = classHierarchy.size();
		for( ; pos < n; ++pos )
		{
			tmpClassStub = FindClass( classHierarchy[ pos++ ]->m_extends );
			while ( tmpClassStub->m_name != classStub->m_name )
			{
				classHierarchy.push_back( tmpClassStub );
				tmpClassStub = FindClass( tmpClassStub->m_extends );
			}
		}

		tmpClassStub = classStub;
		while ( tmpClassStub )
		{
			classHierarchy.push_back( tmpClassStub );
			tmpClassStub = tmpClassStub->FindParentClass();
		}

		pos = 0;
		while ( pos < classHierarchy.size() )
		{
			classStub = classHierarchy[ pos++ ];

			// Properties
			for ( size_t i=0; i<classStub->m_fields.size(); i++ )
			{
				if ( TestMatch( classStub->m_fields[i], initialMatch ) )
				{
					stubs.push_back( classStub->m_fields[i] );
				}
			}

			// Functions
			for ( size_t i=0; i<classStub->m_functions.size(); i++ )
			{
				if ( TestMatch( classStub->m_functions[i], initialMatch ) )
				{
					stubs.push_back( classStub->m_functions[i] );
				}
			}

			// Collect from states
			for ( map< wstring, SSClassStub* >::const_iterator i=m_classes.begin(); i!=m_classes.end(); ++i )
			{
				SSClassStub* stateClassStub = i->second;
				if ( stateClassStub->m_stateMachine == classStub->m_name )
				{
					for ( size_t j=0; j<stateClassStub->m_functions.size(); j++ )
					{
						SSFunctionStub* stateFunctionStub = stateClassStub->m_functions[j];
						if ( stateFunctionStub->m_flags.Has( wxT("entry") ) )
						{
							if ( TestMatch( stateFunctionStub, initialMatch ) )
							{
								stubs.push_back( stateFunctionStub );
							}
						}
					}
				}
			}
		}
	}

	// Force, add all visible sysmbols :)
	if ( force && ( expressionTokens.empty() || stubs.empty() ) )
	{
		if ( !typesOnly )
		{
			// Add global functions
			for ( map< wstring, SSFunctionStub* >::const_iterator it=m_functions.begin(); it!=m_functions.end(); ++it )
			{
				if ( TestMatch( it->second, initialMatch ) )
				{
					stubs.push_back( it->second );
				}
			}
		}

		// Add classes
		for ( map< wstring, SSClassStub* >::const_iterator it=m_classes.begin(); it!=m_classes.end(); ++it )
		{
			if ( TestMatch( it->second, initialMatch ) )
			{
				stubs.push_back( it->second );
			}
		}

		// Add enums
		for ( map< wstring, SSEnumStub* >::const_iterator it=m_enums.begin(); it!=m_enums.end(); ++it )
		{
			if ( TestMatch( it->second, initialMatch ) )
			{
				stubs.push_back( it->second );
			}
		}
	}
	
	struct LocalComparator
	{
		bool operator() ( SSBasicStub* e1, SSBasicStub* e2 )
		{
			return e1->m_name.length() < e2->m_name.length();
		}
	} localComparator;

	// Processed
	std::sort( stubs.begin(), stubs.end(), localComparator );
	return stubs.size() > 0;
}
