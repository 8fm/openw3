#include "build.h"
#include "configDirectory.h"
#include "../../common/core/gameConfiguration.h"

ISavableToConfig::ISavableToConfig()
{
	CConfigurationManager &config = SUserConfigurationManager::GetInstance();
	config.AddSavable( this );
}

ISavableToConfig::~ISavableToConfig()
{
	CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();
	config.RemoveSavable( this );
}

ISavableToCommonConfig::ISavableToCommonConfig()
{
	CCommonConfigurationManager &config = SCommonConfigurationManager::GetInstance();
	config.AddSavable( this );
}

ISavableToCommonConfig::~ISavableToCommonConfig()
{
	CCommonConfigurationManager &config = SCommonConfigurationManager::GetInstance();
	config.RemoveSavable( this );
}

void ISavableToConfig::SaveSession()
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_sessionMutex );
	CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();
	CConfigurationScopedPathSetter pathSetter( config, config.GetSessionPath() );
	SaveSession( config );
}

void ISavableToConfig::RestoreSession()
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_sessionMutex );
	CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();
	CConfigurationScopedPathSetter pathSetter( config, config.GetSessionPath() );
	RestoreSession( config );
}

CUserConfigurationManager::CUserConfigurationManager()
{
	m_root = new CConfigDirectory( String::EMPTY );
	m_current = m_root;
	String baseDir = GFileManager->GetBaseDirectory(); 
	m_filePathMain = baseDir + GGameConfig::GetInstance().GetName() + MACRO_TXT(CONFIG_FILE_MAIN);
	m_filePathSessions = baseDir + GGameConfig::GetInstance().GetName() + MACRO_TXT(CONFIG_FILE_SESSIONS);
	Load();
}

CUserConfigurationManager::~CUserConfigurationManager()
{
	m_savables.Clear();
	if ( m_root )
	{
		delete m_root;
	}
}

CCommonConfigurationManager::CCommonConfigurationManager()
{
	m_root = new CConfigDirectory(String::EMPTY);
	m_current = m_root;
	String baseDir = GFileManager->GetBaseDirectory(); 
	m_filePathMain = baseDir + GGameConfig::GetInstance().GetName() + MACRO_TXT(CONFIG_FILE_COMMON);
	Load();
}

CCommonConfigurationManager::~CCommonConfigurationManager()
{
	m_savables.Clear();
	if ( m_root )
	{
		delete m_root;
	}
}

void CConfigurationManager::RemeberCurrentPath()
{
	m_currentToRestore.PushBack( m_current );
}

void CConfigurationManager::RestoreCurrentPath()
{
	if( m_currentToRestore.Size() )
		m_current = m_currentToRestore.PopBack();
	else
		m_current = m_root;
}

void CUserConfigurationManager::SaveSession()
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_saveSessionMutex );
	CConfigDirectory *previous = m_current;
	m_current = m_root;

	{
		CConfigurationScopedPathSetter pathSetter( *this, GetSessionPath() );

		for( TDynArray< ISavableToConfig* >::iterator it=m_savables.Begin(); it!=m_savables.End(); it++ )
			( *it )->SaveSession( *this );
	}

	m_current = previous;
}

String CUserConfigurationManager::GetSessionPath() const
{
	if ( m_sessionPath.Empty() )
		return TXT("/Session/Default/");
	else
		return TXT("/Session/") + m_sessionPath;
}

void CConfigurationManager::SaveAll( bool withEvent )
{
	if ( withEvent )
	{
		SEvents::GetInstance().QueueEvent( CNAME( SavingConfiguration ), nullptr );
	}

	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_saveAllMutex );
	CConfigDirectory *previous = m_current;
	m_current = m_root;

	for( TDynArray< ISavableToConfig* >::iterator it=m_savables.Begin(); it!=m_savables.End(); it++ )
	{
		( *it )->SaveOptionsToConfig();
	}

	m_current = previous;

	Save();
}

void CUserConfigurationManager::SaveAll( bool withEvent )
{
	if ( withEvent )
	{
		SEvents::GetInstance().QueueEvent( CNAME( SavingConfiguration ), nullptr );
	}

	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_saveAllMutex );
	CConfigDirectory *previous = m_current;
	m_current = m_root;

	for( TDynArray< ISavableToConfig* >::iterator it=m_savables.Begin(); it!=m_savables.End(); it++ )
		( *it )->SaveOptionsToConfig();

	SaveSession();

	m_current = previous;

	Save();
}

void CConfigurationManager::AddSavable( ISavableToConfig* s )
{
	m_savables.PushBack( s );
}

void CConfigurationManager::RemoveSavable( ISavableToConfig* s )
{
	m_savables.Remove( s );
}

Bool CConfigurationManager::AllSettings( TDynArray< TPair< String, String > > &settings )
{
	THashMap< String, String > :: iterator setting;
	for ( setting = m_current->GetSettings().Begin(); setting != m_current->GetSettings().End(); ++setting )
	{
		if( !setting->m_first.Empty() )
		{
			settings.PushBack( TPair< String, String >( setting->m_first, setting->m_second ) );
		}
	}

	return true;
}

Bool CConfigurationManager::GetDirectories( TDynArray< String > &directories ) const
{
	THashMap< String, CConfigDirectory* >::const_iterator directory;
	for ( directory = m_current->GetDirectories().Begin(); directory != m_current->GetDirectories().End(); ++directory )
	{
		if( !directory->m_first.Empty() )
		{
			directories.PushBack( directory->m_first );
		}
	}

	return true;
}

void CConfigurationManager::DeleteAllEntriesFromCurrentDir()
{
	m_current->GetSettings().Clear();
}

void CConfigurationManager::Write( const String &path, const String &value )
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_mutex );
	// treat as an absolute path
	CConfigDirectory *previous = m_current;
	if ( path.BeginsWith( TXT("/") ) )	
		m_current = m_root;
	// split path into parts
	TDynArray< String > parts;
	CTokenizer tokenizer(path, TXT("/"));
	String setting = tokenizer.GetToken(tokenizer.GetNumTokens() - 1);
	for (Uint32 i = 0; i < tokenizer.GetNumTokens() - 1; i++)
		parts.PushBack( tokenizer.GetToken( i ) );

	// add setting to the directory
	CConfigDirectory *directory = AddDirectory( parts );
	m_current = previous;
	if ( !directory )
		return;
	directory->SetValue( setting, value );
}

void CConfigurationManager::Write( const String &path, const Int32 value )
{
	String sValue;
	sValue = ToString(value);
	Write(path, sValue);
}

void CConfigurationManager::Write( const String &path, const Float value )
{
	String sValue;
	sValue = ToString(value);
	Write(path, sValue);
}

Bool CConfigurationManager::Read( const String &path, String *result )
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_mutex );
	// treat as an absolute path
	CConfigDirectory *previous = m_current;
	if ( path.BeginsWith( TXT("/") ) )
		m_current = m_root;
	// split path into parts
	TDynArray< String > parts;
	CTokenizer tokenizer(path, TXT("/"));
	String setting = tokenizer.GetToken(tokenizer.GetNumTokens() - 1);

	for (Uint32 i = 0; i < tokenizer.GetNumTokens() - 1; i++)
		parts.PushBack( tokenizer.GetToken( i ) );
	// add setting to the directory
	CConfigDirectory *directory = NULL;
	if ( parts.Empty() )
		directory = m_current;
	else
		directory = GetDirectory( parts );
	m_current = previous;
	if ( !directory )
	{
		return false;
	}
	return directory->GetValue( setting, *result );
}

Int32 CConfigurationManager::Read( const String &path, Int32 default_value )
{
	String sResult;
	if ( !Read( path, &sResult ) )
		return default_value;
	Int32 iResult;
	if ( !FromString(sResult, iResult) )
		return default_value;
	return iResult;
}

Float CConfigurationManager::Read( const String &path, Float default_value )
{
	String sResult;
	if ( !Read( path, &sResult ) )
		return default_value;
	Float iResult;
	if ( !FromString(sResult, iResult) )
		return default_value;
	return iResult;
}

String CConfigurationManager::Read( const String &path, const String &default_value)
{
	String sResult;
	if ( !Read( path, &sResult ) )
		return default_value;
	return sResult;
}

Bool CConfigurationManager::DeleteEntry( const String &name, const Bool withDirectory )
{
	return m_current->DeleteValue( name, withDirectory );
}

Bool CConfigurationManager::DeleteDirectory( const String &path )
{
	CTokenizer tokenizer(path, TXT("/"));
	TDynArray< String > parts;
	CConfigDirectory *current;
	if ( path.BeginsWith(TXT("/")) )
		current = m_root;
	else
		current = m_current;
	Uint32 i = 0;
	for ( ; i < tokenizer.GetNumTokens()-1; i++)
	{
		String name = tokenizer.GetToken(i);
		current = current->GetChild( name );
		if( !current )
			return false;
	}

	return current->DeleteChild( tokenizer.GetToken(i) );
}

void CConfigurationManager::Load()
{
	Load( m_filePathMain );
}

void CUserConfigurationManager::Load()
{
	CConfigurationManager::Load( m_filePathMain );
	CConfigurationManager::Load( m_filePathSessions );
}

void CConfigurationManager::Load( const String &fileName )
{
	Uint32 length = 1024, read = 0;
	AnsiChar buffer[1025];
	CSystemFile file;
	if ( !GFileManager->FileExist( fileName ) )
	{
		return;
	}
	file.CreateReader( fileName.AsChar() );
	if ( !file )
	{
		return;
	}
	String content;
	TDynArray< String> lines;

	// reading all data from the file
	do {
		for (Uint32 i = 0; i <= length; i++)
			buffer[i] = 0;
		read = file.Read(buffer, length);
		if ( read != 0 )
			content += ANSI_TO_UNICODE(buffer);
	} while ( read == length );
	file.Close();

	// Add empty line at the end of the content 
	content += String::Chr((AnsiChar) 13) + String::Chr((AnsiChar) 10);

	// Scan the content for each line and process it
	String line;
	Char* contentData = content.TypedData();
	Uint32 lineLength = 0, start = 0;
	line.Reserve( content.GetLength() + 2 );
	for ( Uint32 head=0; head < content.GetLength(); )
	{
		// EOL reached, process line
		if ( contentData[head] == 13 && contentData[head + 1] == 10 )
		{
			Red::System::MemoryCopy( line.TypedData(), contentData + start, lineLength * sizeof( Char ) );
			line.TypedData()[lineLength] = 0;
			if ( lineLength > 0 )
			{
				ProcessLine( line.TypedData() );
			}
			lineLength = 0;
			head += 2;
			start = head;
		}
		else // something else found, add it to the line buffer
		{
			line.TypedData()[ lineLength++ ] = contentData[ head++ ];
		}
	}

	if ( m_root )
	{
		m_root->Shrink();
	}
}

void CConfigurationManager::Save()
{
	RemeberCurrentPath();
	String result;
	//Uint32 written;

	SetPath( TXT("/Session/") );
	CConfigDirectory *configDir = m_current; 

	m_root->Save( result, String::EMPTY, configDir );
	CSystemFile file;
	file.CreateWriter( m_filePathMain.AsChar() );
	file.Write( UNICODE_TO_ANSI(result.AsChar()), result.Size() );
	file.Close();

	RestoreCurrentPath();
}

void CUserConfigurationManager::Save()
{
	String result;
	//Uint32 written;
	CConfigurationScopedPathSetter pathSetter( *this, TXT("/Session") );

	CConfigDirectory *configDir = m_current; 

	m_root->Save( result, String::EMPTY, configDir );
	CSystemFile file;
	file.CreateWriter( m_filePathMain.AsChar() );
	file.Write( UNICODE_TO_ANSI(result.AsChar()), result.Size() );
	file.Close();

	result.Clear();
	configDir->Save( result, TXT("Session") );
	file.CreateWriter( m_filePathSessions.AsChar() );
	file.Write( UNICODE_TO_ANSI(result.AsChar()), result.Size() );
	file.Close();
}

void CConfigurationManager::ProcessLine( const String &line )
{
	if ( line.BeginsWith(TXT("[")) )
	{
		// process directory line
		String result, temporary, left;
		line.Split(TXT("["), &left, &temporary);
		temporary.Split(TXT("]"), &result, &left);
		m_current = m_root;
		m_current = AddDirectory(result);
	}
	else
	{
		// process setting line
		AddSetting(line);
	}
}

CConfigDirectory* CConfigurationManager::AddDirectory( const String &path )
{
	CTokenizer tokenizer(path, TXT("/"));
	TDynArray< String > parts;
	for (Uint32 i = 0; i < tokenizer.GetNumTokens(); i++)
		parts.PushBack(tokenizer.GetToken(i));
	return AddDirectory( parts );
}

CConfigDirectory* CConfigurationManager::AddDirectory( const TDynArray< String > &path )
{
	CConfigDirectory *current = m_current;
	CConfigDirectory *previous = NULL;
	for ( Uint32 i = 0; i < path.Size(); i++ )
	{
		previous = current;
		current = previous->GetChild( path[i] );
		if ( current == NULL )
		{
			previous->AddChild( path[i] );
			current = previous->GetChild( path[i] );
		}
	}
	return current;
}

CConfigDirectory* CConfigurationManager::GetDirectory( const TDynArray< String > &path )
{
	CConfigDirectory *current = m_current;
	for ( Uint32 i = 0; (i < path.Size()) && ( current != NULL ); i++ )
	{
		current = current->GetChild( path[i] );
	}
	return current;
}

void CConfigurationManager::AddSetting( const String &setting )
{
	if ( setting != String::EMPTY )
	{
		String variable, value;
		setting.Split(TXT("="), &variable, &value);
		m_current->SetValue( variable, value );
	}
}

void CConfigurationManager::SetPath( const String &path )
{
	if ( path.BeginsWith(TXT("/")) )
		m_current = m_root;
	m_current = AddDirectory( path );
}
