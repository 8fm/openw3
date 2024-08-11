/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#include "perforce.h"
#include "feedback/createChangelist.h"
#include "feedback/getProperty.h"
#include "feedback/getFileStatus.h"
#include "feedback/getDeletedFiles.h"

// P4V Library path
#if _MSC_VER == 1700
#	ifdef _WIN64
#		ifdef _DEBUG
#		define P4V_LIB_PATH "x64\\lib\\VS2012\\debug\\"
#		else
#		define P4V_LIB_PATH "x64\\lib\\VS2012\\release\\"
#		endif
#	else
#		ifdef _DEBUG
#		define P4V_LIB_PATH "lib\\VS2012\\debug\\"
#		else
#		define P4V_LIB_PATH "lib\\VS2012\\release\\"
#		endif
#	endif
#else
#	error Unsupported compiler
#endif

// Perforce libraries
#pragma comment ( lib, P4V_LIB_PATH "libclient.lib" )
#pragma comment ( lib, P4V_LIB_PATH "libp4sslstub.lib" )
#pragma comment ( lib, P4V_LIB_PATH "librpc.lib" )
#pragma comment ( lib, P4V_LIB_PATH "libsupp.lib" )

// Required System Libraries
#pragma comment ( lib, "ws2_32.lib" )

#define RETURN_FALSE_ON_FAILURE( a ) if( !a ) return false

//////////////////////////////////////////////////////////////////////////
// Perforce
//////////////////////////////////////////////////////////////////////////

namespace VersionControl
{
	const char* Perforce::CREDENTIALS[ Credential_Max ] =
	{
		"Username",
		"Password",
		"Workspace",
		"Server"
	};

	Perforce::Perforce()
	{

	}

	Perforce::~Perforce()
	{

	}

	bool Perforce::Initialize( const char* program )
	{
		m_perforce.SetProtocol( "tag", "1" );

		Error feedback;

		m_perforce.Init( &feedback );

		if( feedback.IsFatal() )
		{
			feedback.Fmt( &m_errors );

			return false;
		}

		m_perforce.SetProg( program );

		//m_perforce.SetUi( &m_errorListener );

		return true;
	}

	void Perforce::Shutdown()
	{
		Error feedback;

		m_perforce.Final( &feedback );
	}

	unsigned int Perforce::GetCredentialsCount() const 
	{
		return Credential_Max;
	}

	const char** Perforce::GetCredentials() const 
	{
		return CREDENTIALS;
	}

	bool Perforce::SetCredential( const char* key, const char* value )
	{
		if( StrPtr::CCompare( key, CREDENTIALS[ Credential_Username ] ) == 0 )
		{
			m_perforce.SetUser( value );
			return true;
		}
		else if( StrPtr::CCompare( key, CREDENTIALS[ Credential_Password ] ) == 0 )
		{
			m_perforce.SetPassword( value );
			return true;
		}
		else if( StrPtr::CCompare( key, CREDENTIALS[ Credential_Workspace ] ) == 0 )
		{
			m_perforce.SetClient( value );
			return true;
		}
		else if( StrPtr::CCompare( key, CREDENTIALS[ Credential_Server ] ) == 0 )
		{
			m_perforce.SetPort( value );
			return true;
		}

		return false;
	}

	const char* Perforce::GetCredential( const char* key ) const
	{
		if( StrPtr::CCompare( key, CREDENTIALS[ Credential_Username ] ) == 0 )
		{
			return const_cast< ClientApi& >( m_perforce ).GetUser().Text();
		}
		else if( StrPtr::CCompare( key, CREDENTIALS[ Credential_Password ] ) == 0 )
		{
			return const_cast< ClientApi& >( m_perforce ).GetPassword().Text();
		}
		else if( StrPtr::CCompare( key, CREDENTIALS[ Credential_Workspace ] ) == 0 )
		{
			return const_cast< ClientApi& >( m_perforce ).GetClient().Text();
		}
		else if( StrPtr::CCompare( key, CREDENTIALS[ Credential_Server ] ) == 0 )
		{
			return const_cast< ClientApi& >( m_perforce ).GetPort().Text();
		}

		return nullptr;
	}

	bool Perforce::Add( const char** files, unsigned int numFiles )
	{
		return Run( "add", files, numFiles );
	}

	bool Perforce::Checkout( const char** files, unsigned int numFiles )
	{
		return Run( "edit", files, numFiles );
	}

	bool Perforce::Revert( const char** files, unsigned int numFiles )
	{
		return Run( "revert", files, numFiles );
	}

	bool Perforce::Submit( const char** files, unsigned int numFiles, const char* description )
	{
		if( numFiles > 0 )
		{
			// Create new changelist
			{
				const char* params[] =
				{
					"-i"
				};

				Feedback::CreateChangelist feedback;

				feedback.SetDescription( description );

				RETURN_FALSE_ON_FAILURE( Run( "change", params, 1, feedback ) );
			}

			// Get number of changelist that we just created
			StrBuf changelistNumber;
			RETURN_FALSE_ON_FAILURE( GetLatestPendingChangelist( changelistNumber ) );

			// Move files into changelist
			{
				const char* params[] =
				{
					"-c",
					changelistNumber.Text(),
					nullptr
				};

				for( unsigned int i = 0; i < numFiles; ++i )
				{
					params[ 2 ] = files[ i ];

					RETURN_FALSE_ON_FAILURE( Run( "reopen", params, 3 ) );
				}
			}

			// Submit changelist
			{
				const char* params[] =
				{
					"-c",
					changelistNumber.Text()
				};

				RETURN_FALSE_ON_FAILURE( Run( "submit", params, 2 ) );
			}

			return true;
		}
	
		return false;
	}

	bool Perforce::Delete( const char** files, unsigned int numFiles )
	{
		return Run( "delete", files, numFiles );
	}

	bool Perforce::Sync( const char** files, unsigned int numFiles )
	{
		return Run( "sync", files, numFiles );
	}

	bool Perforce::GetDeletedFiles( Filelist* list, const char* directory )
	{
		// suffix = "\*" and null terminator
		size_t size = sizeof( char ) * ( strlen( directory ) + 3 );
		char* p4dir = static_cast< char* >( alloca( size ) );

		strcpy_s( p4dir, size, directory );
		strcat_s( p4dir, size, "*" );

		Filelist perforcePathList;

		Feedback::GetDeletedFiles feedback( &perforcePathList );

		const char* intermediary = p4dir;

		if( Run( "opened", &intermediary, 1, feedback ) )
		{
			// Since the "clientFile" parameter isn't the same for "opened" as it is for "fstat" we need to convert them to local paths
			const char** depotPaths = perforcePathList.Get();

			for( unsigned int i = 0; i < perforcePathList.Size(); ++i )
			{
				StrBuf localPath;
				if( !GetLocalPath( depotPaths[ i ], localPath ) )
				{
					return false;
				}
				else
				{
					list->Add( localPath.Text() );
				}
			}

			return true;
		}

		return false;
	}

	FileStatus Perforce::GetStatus( const char* file )
	{
		Feedback::GetFileStatus feedback;

		Run( "fstat", &file, 1, feedback );

		return feedback.Get();
	}

	const char* Perforce::GetLastError()
	{
		return m_errors.Text();
	}

	bool Perforce::GetFileProperty( const char* file, const char* propertyName, StrBuf& property )
	{
		const char* params[ 3 ] =
		{
			"-T",
			propertyName,
			file
		};

		Feedback::GetProperty feedback( propertyName );

		if( Run( "fstat", params, 3, feedback ) )
		{
			property.Set( feedback.GetValue() );
			return true;
		}

		return false;
	}

	bool Perforce::GetChangelistNumber( const char* file, StrBuf& changelist )
	{
		return GetFileProperty( file, "change", changelist );
	}

	bool Perforce::GetDepotPath( const char* file, StrBuf& path )
	{
		return GetFileProperty( file, "depotFile", path );
	}

	bool Perforce::GetLocalPath( const char* file, StrBuf& path )
	{
		return GetFileProperty( file, "clientFile", path );
	}

	bool Perforce::GetLatestPendingChangelist( StrBuf& changelist )
	{
		const unsigned int size = 6;
		const char* params[ size ] =
		{
			"-m",
			"1",
			"-s",
			"pending",
			"-u",
			m_perforce.GetUser().Text()
		};

		Feedback::GetProperty feedback( "change" );

		if( Run( "changelists", params, size, feedback ) )
		{
			changelist.Set( feedback.GetValue() );
			return true;
		}

		return false;
	}

	bool Perforce::Run( const char* command, const char** parameters, unsigned int numParameters, Feedback::Base& feedback )
	{
		m_perforce.SetArgv( numParameters, const_cast< char* const* >( parameters ) );
		m_perforce.SetVar( "enableStreams" );

		m_perforce.Run( command, &feedback );

		if( feedback.GetNumErrors() == 0 )
		{
			return true;
		}
		else
		{
			m_errors.Set( feedback.GetError() );

			return false;
		}
	}
}

//////////////////////////////////////////////////////////////////////////
// Interface creation functions
//////////////////////////////////////////////////////////////////////////

RED_VC_DLL_C VersionControl::Interface* CreateInterface()
{
	return new VersionControl::Perforce;
}

RED_VC_DLL_C void DestroyInterface( VersionControl::Interface* perforce )
{
	delete perforce;
}
