
#include "../interface/versionControl.h"

#include <windows.h>
#include <stdio.h>

#ifdef _DEBUG
#	define DLL_FILENAME	"redscm_d.dll"
#else
#	define DLL_FILENAME	"redscm.dll"
#endif

int main( int argc, const char* argv[] )
{
	if( argc > 1 )
	{
		printf( "Loading DLL\n" );
		HMODULE perforceDll = LoadLibraryA( DLL_FILENAME );

		if( perforceDll )
		{
			printf( "Grabbing Entry Functions\n" );
			CreateInterfaceFunc create;
			DestroyInterfaceFunc destroy;

			create = (CreateInterfaceFunc) GetProcAddress( perforceDll, "CreateInterface" );
			destroy = (DestroyInterfaceFunc) GetProcAddress( perforceDll, "DestroyInterface" );

			if( create && destroy )
			{
				printf( "Creating Interface\n" );
				VersionControl::Interface* versioningSystem = create();

				if( versioningSystem )
				{
					printf( "Initialising\n" );
					if( versioningSystem->Initialize( "VersionControlTestProgram" ) )
					{
						printf( "Get File Status: %s\n", argv[ 1 ] );

						VersionControl::FileStatus status = versioningSystem->GetStatus( argv[ 1 ] );

						if( status.HasFlag( VersionControl::VCSF_InDepot ) )
						{
							printf( "\tFile is in depot\n" );
						}
						else
						{
							printf( "\tFile is not under source control\n" );
						}

						if( status.HasFlag( VersionControl::VCSF_Added ) )
						{
							printf( "\tMarked for Add\n" );
						}

						if( status.HasFlag( VersionControl::VCSF_Deleted ) )
						{
							printf( "\tMarked for Deletion\n" );
						}

						if( status.HasFlag( VersionControl::VCSF_CheckedOut ) )
						{
							printf( "\tChecked out\n" );
						}

						if( status.HasFlag( VersionControl::VCSF_CheckedOutByAnother ) )
						{
							printf( "\tChecked out by another\n" );
						}

						if( status.HasFlag( VersionControl::VCSF_OutOfDate ) )
						{
							printf( "\tOut of date\n" );
						}
						else
						{
							printf( "\tSync'd to latest revision\n" );
						}

						VersionControl::Filelist* files = versioningSystem->CreateFileList();

						for( int i = 1; i < argc; ++i )
						{
							files->Add( argv[ i ] );
						}

						if( !versioningSystem->Checkout( files ) )
						{
							printf( "error: %s", versioningSystem->GetLastError() );
						}

						if( !versioningSystem->Revert( files ) )
						{
							printf( "error: %s", versioningSystem->GetLastError() );
						}

// 						const char** files = argv + 1;
// 						if( !versioningSystem->Submit( files, argc - 1, "test submission" ) )
// 						{
// 							printf( "error: %s", versioningSystem->GetLastError() );
// 						}

						versioningSystem->DestroyFileList( files );

						printf( "Files marked for deletion:\n" );
						VersionControl::Filelist* deletedFilesList = versioningSystem->CreateFileList();

						versioningSystem->GetDeletedFiles( deletedFilesList, "E:\\Perforce\\w3\\Lava\\dev\\src\\win32\\scriptStudio\\" );

						const char** deletedFiles = deletedFilesList->Get();

						for( unsigned int i = 0; i < deletedFilesList->Size(); ++i )
						{
							printf( "%i: %s\n", i, deletedFiles[ i ] );
						}

						versioningSystem->DestroyFileList( deletedFilesList );

						printf( "Shutdown\n" );
						versioningSystem->Shutdown();
					}

					printf( "Destroying interface\n" );
					destroy( versioningSystem );
				}
			}
			else
			{
				DWORD err = GetLastError();
				printf( "Error finding functions: %i\n", err );
			}

			printf( "Unloading DLL\n" );
			BOOL free_success = FreeLibrary( perforceDll );
		}
		else
		{
			DWORD err = GetLastError();
			printf( "Error loading DLL: %i\n", err );
		}
	}

	return 0;
}
