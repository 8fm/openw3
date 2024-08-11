#include "build.h"

#include "ReFileHelpers.h"

Bool CReFileHelpers::ShouldImportFile( IImporter::ImportOptions& options )
{
	const char* path = UNICODE_TO_ANSI( options.m_sourceFilePath.AsChar() );
	ReFile headers = ReFileLoader::OpenFile( path );
	if( !headers )
	{
		options.m_errorCode = IImporter::ImportOptions::EEC_FileNotExist;
		return false;
	}

	ReFileHeader2 hdr;
	if( ReFileLoader::Read( headers, hdr ) )
	{
		const String& fileVersion = ANSI_TO_UNICODE( hdr.getReFileVersion().getData() );
		const String& reSystemVersion = ANSI_TO_UNICODE( ReFileArchive::getReFileSystemVersion().getData() );

		// #TODO W3 backwards compatibility import of RE file
		// we have old, really old files with empty field or with "5" in extra5 field in header
		// we will allow for import of these files.
		if( fileVersion.Size() < reSystemVersion.Size() )
		{
			options.m_errorCode = IImporter::ImportOptions::EEC_FileToReimport;
			return false;
		}

		// because of 3 signs in version
		if( fileVersion.Size() != reSystemVersion.Size() )
		{
			options.m_errorCode = IImporter::ImportOptions::EEC_BadVersion;
			return false;
		}
		else
		{
			if ( fileVersion < reSystemVersion )
			{
				// f.ex. for case 001 < 002; 002 < 003
				options.m_errorCode = IImporter::ImportOptions::EEC_FileToReimport;
				return false;
			}
		}
	}
	else
	{
		// cannot read headers. probably file is broken
		options.m_errorCode = IImporter::ImportOptions::EEC_BadVersion;
		return false;
	}

	return true;
}
