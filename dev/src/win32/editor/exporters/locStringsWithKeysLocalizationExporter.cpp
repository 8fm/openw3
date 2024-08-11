/**
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "locStringsWithKeysLocalizationExporter.h"
#include "../../../common/engine/localizationManager.h"

void CLocStringsWithKeysLocalizationExporter::BeginBatchExport()
{
	m_locStrMan.Initialize();

	m_exportData.Clear();

	TDynArray< String > headers;
	headers.PushBack( TXT( "Source File" ) ); // <category>@CUSTOM
	headers.PushBack( TXT( "StringID" ) );
	headers.PushBack( TXT( "" ) );
	headers.PushBack( TXT( "Category" ) ); // <category>
	headers.PushBack( TXT( "" ) );
	headers.PushBack( TXT( "" ) );
	headers.PushBack( TXT( "" ) );
	headers.PushBack( TXT( "" ) );
	headers.PushBack( TXT( "Key" ) );
	headers.PushBack( TXT( "Text SL" ) ); // Text in Source Language
	headers.PushBack( TXT( "" ) );
	headers.PushBack( TXT( "" ) );

	for ( Uint32 i = 0; i < m_numberOfTargetLanguages; ++i )
	{
		headers.PushBack( TXT( "" ) );
		headers.PushBack( TXT( "" ) );
		headers.PushBack( TXT( "Key" ) );
		headers.PushBack( TXT( "Text TL" ) ); // Text in Target Language
		headers.PushBack( TXT( "" ) );
		headers.PushBack( TXT( "" ) );
	}

	headers.PushBack( TXT( "" ) );

	m_exportData.PushBack( headers );

	PrepareErrorFileHeaders();
}

void CLocStringsWithKeysLocalizationExporter::EndBatchExport()
{
	String exportString = ConvertToCSV();
	if ( exportString.Empty() == false )
	{
		GFileManager->SaveStringToFileWithUTF8( m_savePath, exportString );
	}

	DumpErrorsFile();
}

void CLocStringsWithKeysLocalizationExporter::ExportCustom( const CVariant& val )
{
	String category = val.ToString();

	for ( Uint32 i = 0; i < m_locStrMan.GetSize(); ++i )
	{
		// Omit strings that doesn't match category
		if ( !m_locStrMan.DoesMatchStringCategory( i, category ) ) continue;

		LocalizedString locString = m_locStrMan.GetLocString( i );

		String sourceFile = category + TXT("@CUSTOM");
		String stringId = ToString( locString.GetIndex() );
		String testSL = locString.GetString();

		TDynArray< String > elementData;
		elementData.PushBack( sourceFile );
		elementData.PushBack( stringId );
		elementData.PushBack( TXT( "" ) );
		elementData.PushBack( category );
		elementData.PushBack( TXT( "" ) );
		elementData.PushBack( TXT( "" ) );
		elementData.PushBack( TXT( "" ) );
		elementData.PushBack( TXT( "" ) );
		elementData.PushBack( TXT( "" ) );
		elementData.PushBack( testSL );
		elementData.PushBack( TXT( "" ) );
		elementData.PushBack( TXT( "" ) );

		for ( Uint32 k = 0; k < m_numberOfTargetLanguages; ++k )
		{
			// TODO: MBuda - what is this number languages?
			String testDL = String::EMPTY;
			if ( k < m_targetLanguages.Size() )
			{
				testDL = locString.GetString( m_targetLanguages[ k ] );
			}

			elementData.PushBack( TXT( "" ) );
			elementData.PushBack( TXT( "" ) );
			elementData.PushBack( TXT( "" ) );
			elementData.PushBack( testDL );
			elementData.PushBack( TXT( "" ) );
			elementData.PushBack( TXT( "" ) );
		}

		elementData.PushBack( TXT( "" ) );

		m_exportData.PushBack( elementData );
	}
}

void CLocStringsWithKeysLocalizationExporter::FillRootExportGroup( BatchExportGroup& exportGroup )
{
	exportGroup.m_groupName = TXT( "root" );

	BatchExportGroup subGroup;
	subGroup.m_groupName = TXT( "Custom localized Strings" );

	SLocalizationManager::GetInstance().ReadAllStringsCategories( subGroup.m_groupEntries );

	exportGroup.m_subGroups.PushBack( subGroup );
}

void CLocStringsWithKeysLocalizationExporter::ExportBatchEntry( const String& entry )
{
	String category = entry;

	for ( Uint32 i = 0; i < m_locStrMan.GetSize(); ++i )
	{
		// Omit strings that doesn't match category
		if ( !m_locStrMan.DoesMatchStringCategory( i, category ) ) continue;

		LocalizedString locString = m_locStrMan.GetLocString( i );

		String sourceFile = category + TXT("@CUSTOM");
		String stringId = ToString( locString.GetIndex() );
		String testSL = locString.GetString();
		String stringKey = m_locStrMan.GetStringKey( i );

		TDynArray< String > elementData;
		elementData.PushBack( sourceFile );
		elementData.PushBack( stringId );
		elementData.PushBack( TXT( "" ) );
		elementData.PushBack( category );
		elementData.PushBack( TXT( "" ) );
		elementData.PushBack( TXT( "" ) );
		elementData.PushBack( TXT( "" ) );
		elementData.PushBack( TXT( "" ) );
		elementData.PushBack( stringKey );
		elementData.PushBack( testSL );
		elementData.PushBack( TXT( "" ) );
		elementData.PushBack( TXT( "" ) );

		for ( Uint32 k = 0; k < m_numberOfTargetLanguages; ++k )
		{
			// TODO: MBuda - what is this number languages?
			String testDL = String::EMPTY;
			if ( k < m_targetLanguages.Size() )
			{
				testDL = locString.GetString( m_targetLanguages[ k ] );
			}

			elementData.PushBack( TXT( "" ) );
			elementData.PushBack( TXT( "" ) );
			elementData.PushBack( stringKey );
			elementData.PushBack( testDL );
			elementData.PushBack( TXT( "" ) );
			elementData.PushBack( TXT( "" ) );
		}

		elementData.PushBack( TXT( "" ) );

		m_exportData.PushBack( elementData );
	}

}

CLocStringsWithKeysLocalizationExporter::CLocStringsWithKeysLocalizationExporter()
	: AbstractLocalizationExporter()
{
	FillRootExportGroup( m_rootExportGroup );
}

Bool CLocStringsWithKeysLocalizationExporter::IsBatchEntryValid( const String& entry ) const
{
	// nothing checked - entry is valid
	return true;
}

Bool CLocStringsWithKeysLocalizationExporter::ValidateBatchEntry( const String& entry )
{
	// nothing checked, nothing to validate - entry is valid
	return true;
}

void CLocStringsWithKeysLocalizationExporter::PushBatchEntryError( const String& entry, const String& errorMsg )
{
	TDynArray< String > csvRow;
	csvRow.PushBack( entry );
	csvRow.PushBack( errorMsg );
	m_errorData.PushBack( csvRow );
}
