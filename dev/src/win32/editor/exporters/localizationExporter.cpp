/**
* Copyright c 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "localizationExporter.h"
#include "../../../common/core/depot.h"

void AbstractLocalizationExporter::SetLanguages( const String& sourceLanguage, const String& targetLanguage /*= String::EMPTY */ )
{
	m_sourceLanguage = sourceLanguage;
}

void AbstractLocalizationExporter::SetNumberOfTargetLanguages( Uint32 numberOfTargetLanguages )
{
	m_numberOfTargetLanguages = numberOfTargetLanguages;
}

void AbstractLocalizationExporter::SetTargetLanguages( const TDynArray< String >& targetLanguages )
{
	m_targetLanguages = targetLanguages;
}

String AbstractLocalizationExporter::GetSourceString( const LocalizedString& content )
{
	String sourceString = content.GetString( m_sourceLanguage );
	sourceString.Trim();
	sourceString.ReplaceAll( TXT( "\n" ), TXT( "" ) );
	return sourceString;
}

String AbstractLocalizationExporter::GetTargetString( const LocalizedString& content, Uint32 targetLanguage /*= 0 */ )
{
	if ( targetLanguage >= m_targetLanguages.Size() )
	{
		return String::EMPTY;
	}
	String targetString = content.GetString( m_targetLanguages[ targetLanguage ] );
	targetString.Trim();
	targetString.ReplaceAll( TXT( "\n" ), TXT( "" ) );
	return targetString;
}

String AbstractLocalizationExporter::ConvertToCSV()
{
	String saveData = TXT( "" );

	for ( Uint32 i = 0; i < m_exportData.Size(); ++i )
	{
		const TDynArray< String >& lineData = m_exportData[ i ];

		for ( Uint32 j = 0; j < lineData.Size(); ++j )
		{
			String cellData = lineData[ j ];
			if ( cellData.Empty() == false )
			{
				ApplyBrackets(cellData);

			}
			saveData += cellData;
			saveData += TXT( ";" );
		}
		saveData += TXT( "\n" );
	}

	return saveData;
}

String AbstractLocalizationExporter::ConvertErrorsToCSV()
{
	String errorData = TXT( "" );

	for ( Uint32 i = 0; i < m_errorData.Size(); ++i )
	{
		const TDynArray< String >& lineData = m_errorData[ i ];

		for ( Uint32 j = 0; j < lineData.Size(); ++j )
		{
			String cellData = lineData[ j ];
			if ( cellData.Empty() == false )
			{
				ApplyBrackets(cellData);
			}
			errorData += cellData;
			errorData += TXT( ";" );
		}
		errorData += TXT( "\n" );
	}

	return errorData;
}

Bool AbstractLocalizationExporter::DumpErrorsFile()
{
	if ( m_errorData.Size() == 1 )
	{
		//
		// Don't create errors file if there is only header information stored in m_errorData.
		//
		return false;
	}
	String exportString = ConvertErrorsToCSV();
	if ( exportString.Empty() == false )
	{
		GFileManager->SaveStringToFileWithUTF8( m_errorsPath, exportString );
	}
	return true;
}

void AbstractLocalizationExporter::PrepareErrorFileHeaders()
{
	m_errorData.Clear();
	TDynArray< String > errorHeaders;
	errorHeaders.PushBack( TXT( "Source File" ) );
	errorHeaders.PushBack( TXT( "Error" ) );

	m_errorData.PushBack( errorHeaders );
}

String AbstractLocalizationExporter::GetResourceName( CResource* resource )
{
	String path = resource->GetFile()->GetDirectory()->GetDepotPath();
	String filename = resource->GetFile()->GetFileName();
	String compressedPath = TXT( "" );

	Uint32 firstPathDepth = m_minResourcePathDepth;
	Uint32 maxPathDepth	= m_maxResourcePathDepth;

	for ( Uint32 i = 0; i < maxPathDepth; ++i )
	{
		if ( i >= firstPathDepth && i < maxPathDepth )
		{
			String pathPart = path.StringBefore( TXT( "\\" ) );

			if ( pathPart.ContainsCharacter( TXT( '_' ) ) )
			{
				pathPart = pathPart.StringBefore( TXT( "_" ) );
			}

			if ( compressedPath.Empty() == false )
			{
				compressedPath += TXT( "_" );
			}
			compressedPath += pathPart;

		}

		path = path.StringAfter( TXT( "\\" ) );
	}

	if ( compressedPath.Empty() == false )
	{
		return String::Printf( TXT( "%s@%s" ), compressedPath.AsChar(), filename );
	}

	return filename;
}

void AbstractLocalizationExporter::ApplyBrackets( String& cellData )
{
	String replacedText = String::EMPTY, left, right = cellData;
	while ( right.Split( TXT( "\"" ), &left, &right ) == true )
	{
		replacedText += left + TXT( "\"\"" );
	}
	replacedText += right;
	
	cellData = replacedText;

	cellData = TXT( "\"" ) + cellData + TXT( "\"" );
}

void AbstractLocalizationExporter::FillRootExportGroup( BatchExportGroup& exportGroup )
{
	AddDirectoryToBatchGroup( GDepot, exportGroup );
}

void AbstractLocalizationExporter::AddDirectoryToBatchGroup( CDirectory* directory, BatchExportGroup& batchGroup )
{
	if ( directory == NULL )
	{
		return;
	}

	batchGroup.m_groupName = directory->GetName();

	String filename;
	String depotPath;

	const TFiles & directoryFiles = directory->GetFiles();
	for ( TFiles::const_iterator fileIter = directoryFiles.Begin();
		fileIter != directoryFiles.End(); ++fileIter )
	{
		CDiskFile* file = (*fileIter);
		

		//if ( filename.EndsWith( GetResourceExtension() ) == false )
		if ( CanExportFile( file ) == false )
		{
			continue;
		}

		filename = file->GetFileName();
		depotPath = file->GetDepotPath();
		batchGroup.m_groupEntries.PushBack( depotPath );
	}

	for( CDirectory* dir : directory->GetDirectories() )
	{
		BatchExportGroup subGroup;
		batchGroup.m_subGroups.PushBack( subGroup );

		//batchGroup.m_subGroups.PushBack( new BatchExportGroup() );

		AddDirectoryToBatchGroup( dir, batchGroup.m_subGroups.Back() );		
	}

}
