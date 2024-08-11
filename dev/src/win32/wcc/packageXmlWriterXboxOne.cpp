/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"

#include "../../common/core/algorithms.h"
#include "../../common/redSystem/crt.h"
#include "packageXmlWriterXboxOne.h"
#include "packageConstants.h"

CPackageXmlWriterXboxOne::CPackageXmlWriterXboxOne( IFile& fileWriter )
	: m_fileWriter( fileWriter )
	, m_launchMarkerChunk( INVALID_CHUNK_ID )
{}

Bool CPackageXmlWriterXboxOne::ChunkAdd( Uint32 chunkID )
{
	if ( chunkID > ALIGNMENT_CHUNK_ID )
	{
		ERR_WCC(TXT("chunkID %u cannot exceed alignment chunkID %u"), chunkID, ALIGNMENT_CHUNK_ID );
	}

	if ( m_chunkIDFilesMap.Insert( chunkID, TDynArray<SFileMapping>() ) )
	{
		return true;
	}

	ERR_WCC(TXT("CPackageXmlWriterXboxOne: chunkID %u already exists"), chunkID );
	return false;
}

Bool CPackageXmlWriterXboxOne::AddFile( const String& origPath, const String& targPath, Uint32 chunkID )
{
	if ( ! m_chunkIDFilesMap.KeyExist( chunkID ) )
	{
		ERR_WCC(TXT("CPackageXmlWriterXboxOne::AddFile: chunkID %u does not exist in mapping"), chunkID );
		return false;
	}

	m_chunkIDFilesMap[ chunkID ].PushBack( SFileMapping( origPath, targPath ) );
	return true;
}

Bool CPackageXmlWriterXboxOne::AddFile( const String& origPath, Uint32 chunkID )
{
	String targPath = origPath;
	targPath.ReplaceAll(TXT('/'), TXT('\\'));
	return AddFile( origPath, targPath, chunkID );
}

Bool CPackageXmlWriterXboxOne::SetLaunchMarkerChunk( Uint32 chunkID )
{
	if ( ! m_chunkIDFilesMap.KeyExist( chunkID ) )
	{
		ERR_WCC(TXT("CPackageXmlWriterXboxOne::SetLaunchMarkerChunk: chunkID %u does not exist in mapping"), chunkID );
		return false;
	}

	if ( chunkID == ALIGNMENT_CHUNK_ID )
	{
		ERR_WCC(TXT("CPackageXmlWriterXboxOne::SetLaunchMarkerChunk: chunkID %u cannot be the alignment chunk %u"), chunkID, ALIGNMENT_CHUNK_ID );
		return false;
	}

	m_launchMarkerChunk = chunkID;
	return true;
}

static String GetDestinationPath( const String& targetPath )
{
	// I don't trust CFile path, so silly empty checks
	CFilePath filePath( targetPath );
	if ( filePath.GetDirectories().Empty() )
	{
		return String::EMPTY;
	}

	const String destDir = filePath.GetPathString( true );
	return destDir;
}

Bool CPackageXmlWriterXboxOne::WriteXml( EPackageType packageType )
{
	// #hack DLC submission validator specifically requests the Update.AlignmentChunk have the launch marker

	if ( packageType == ePackageType_Dlc )
	{
		// Just set it to something so we won't make other chunks the LaunchMarker
		m_launchMarkerChunk = ALIGNMENT_CHUNK_ID;
	}

	if ( m_launchMarkerChunk == INVALID_CHUNK_ID )
	{
		ERR_WCC(TXT("CPackageXmlWriterXboxOne::WriteXml: no launch marker chunkID set"));
		return false;
	}

	StringAnsi xmlLine("<?xml version=\"1.0\" encoding=\"utf-8\"?>\r\n");
	m_fileWriter.Serialize( (void*)xmlLine.AsChar(), xmlLine.GetLength() );

	StringAnsi packageStartTag("<Package>\r\n");
	m_fileWriter.Serialize( (void*)packageStartTag.AsChar(), packageStartTag.GetLength() );

	// Registration files
	{
		StringAnsi chunkStartTag = "\t<Chunk Id=\"Registration\">\r\n";
		m_fileWriter.Serialize( (void*)chunkStartTag.AsChar(), chunkStartTag.GetLength() );

		// FIXME: revisit for patching
		const Char* const *registrationFiles;
		size_t filesCount;
		if( packageType == ePackageType_App || packageType == ePackageType_Patch )
		{
			registrationFiles = REGISTRATION_FILES_XBOX_APP;
			filesCount = ARRAY_COUNT_U32( REGISTRATION_FILES_XBOX_APP );
		}else if( packageType == ePackageType_Dlc )
		{
			registrationFiles = REGISTRATION_FILES_XBOX_DLC;
			filesCount = ARRAY_COUNT_U32( REGISTRATION_FILES_XBOX_DLC );
		}else
		{
			// Unsupported
			return false;
		}

		for ( Uint32 i = 0 ; i < filesCount; ++i )
		{
			const Char* regFile = registrationFiles[i];
			
			String dirPath;
			auto dirEnd = Red::System::StringSearchLast( regFile, '\\' );
			if( dirEnd != NULL )
			{
				dirPath = String( regFile, Red::System::StringSearchLast( regFile, '\\' ) - regFile + 1 );
			}

			StringAnsi fileGroupTag = StringAnsi::Printf("\t\t<FileGroup DestinationPath=\"\\%ls\" SourcePath=\".\" Include=\"%ls\"/>\r\n", dirPath.AsChar(), regFile);
			m_fileWriter.Serialize( (void*)fileGroupTag.AsChar(), fileGroupTag.GetLength() );
		}
		StringAnsi chunkEndTag = StringAnsi::Printf("\t</Chunk>\r\n");
		m_fileWriter.Serialize( (void*)chunkEndTag.AsChar(), chunkEndTag.GetLength() );
	}

	// Normal files
	TDynArray< Uint32 > chunkIDs;
	m_chunkIDFilesMap.GetKeys( chunkIDs );
	::Sort( chunkIDs.Begin(), chunkIDs.End() );

	for ( Uint32 chunkID : chunkIDs )
	{
		StringAnsi chunkStartTag = StringAnsi::Printf("\t<Chunk Id=\"%u\"%ls>\r\n", chunkID, (chunkID == m_launchMarkerChunk) ? TXT(" Marker=\"Launch\"") : TXT(""));
		m_fileWriter.Serialize( (void*)chunkStartTag.AsChar(), chunkStartTag.GetLength() );

		for ( const SFileMapping& fileMapping : m_chunkIDFilesMap[ chunkID ] )
		{
			StringAnsi fileGroupTag = StringAnsi::Printf("\t\t<FileGroup DestinationPath=\"\\%ls\" SourcePath=\".\\\" Include=\"%ls\"/>\r\n", 
				GetDestinationPath( fileMapping.m_targPath ).AsChar(), fileMapping.m_origPath.AsChar() );
			m_fileWriter.Serialize( (void*)fileGroupTag.AsChar(), fileGroupTag.GetLength() );
		}

		StringAnsi chunkEndTag = StringAnsi::Printf("\t</Chunk>\r\n");
		m_fileWriter.Serialize( (void*)chunkEndTag.AsChar(), chunkEndTag.GetLength() );
	}

	// Alignment chunk
	{
		StringAnsi chunkStartTag = StringAnsi::Printf("\t<Chunk Id=\"%u\"%ls>\r\n", ALIGNMENT_CHUNK_ID, (packageType == ePackageType_Dlc) ? TXT(" Marker=\"Launch\"") : TXT(""));
		m_fileWriter.Serialize( (void*)chunkStartTag.AsChar(), chunkStartTag.GetLength() );
		
		StringAnsi fileGroupTag = StringAnsi::Printf("\t\t<FileGroup DestinationPath=\"\\\" SourcePath=\".\\\" Include=\"Update.AlignmentChunk\"/>\r\n");
		m_fileWriter.Serialize( (void*)fileGroupTag.AsChar(), fileGroupTag.GetLength() );
		
		StringAnsi chunkEndTag = StringAnsi::Printf("\t</Chunk>\r\n");
		m_fileWriter.Serialize( (void*)chunkEndTag.AsChar(), chunkEndTag.GetLength() );
	}

	StringAnsi packageEndTag("</Package>\r\n");
	m_fileWriter.Serialize( (void*)packageEndTag.AsChar(), packageEndTag.GetLength() );

	return true;
}
