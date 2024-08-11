#include "build.h"

#include "journalLocalizationExporter.h"
#include "journalTree.h"
#include "../../games/r4/journal.h"
#include "../../common/core/feedback.h"
#include "../../common/engine/localizationManager.h"

struct SJournalTreeItemBEG : public SJournalItemHandle
{
public:
	SJournalTreeItemBEG( BatchExportGroup& group )
	:	m_group( group )
	{
	}

	SJournalTreeItemBEG( SJournalTreeItemBEG& other )
	:	m_group( other.m_group )
	{

	}

	virtual Bool IsValid()
	{
		return true;// m_group != NULL;
	}

	BatchExportGroup& m_group;
};

struct SStringConversionHelper
{
	String localizedStringIndexStr;
	LocalizedString localizedString;

	CGUID entryGuid;
	CName propertyName;
	Uint32 order;

	String ToString() const
	{
		String retVal;

		retVal += TXT( "::" );
		retVal += ::ToString( entryGuid );
		retVal += TXT( "::" );
		retVal += propertyName.AsString().AsChar();
		retVal += TXT( "::" );
		retVal += ::ToString( localizedString.GetIndex() );
		retVal += TXT( "::" );
		retVal += ::ToString( order );

		return retVal;
	}

	void FromString( const String& source )
	{
		TDynArray< String > sections = source.Split( TXT( "::" ) );

		String orderStr			= sections.PopBack();
		localizedStringIndexStr	= sections.PopBack();
		String propertyNameStr	= sections.PopBack();
		String guidStr			= sections.PopBack();

		Uint32 stringIndex;
		::FromString( localizedStringIndexStr, stringIndex );
		localizedString.SetIndex( stringIndex );

		propertyName = CName( propertyNameStr );

		entryGuid.FromString( guidStr.AsChar() );

		::FromString( orderStr, order );
	}
};

CJournalLocalizationExporter::CJournalLocalizationExporter()
{
	FillRootExportGroup( m_rootExportGroup );
}

CJournalLocalizationExporter::~CJournalLocalizationExporter()
{

}

void CJournalLocalizationExporter::BeginBatchExport()
{
	m_exportData.Clear();

	TDynArray< String > header;

	// Source column: J
	// Target: P, S, V etc

	header.PushBack( TXT( "Entry ID" ) );																	// A
	header.PushBack( TXT( "String ID" ) );																	// B
	header.PushBack( TXT( "Unused" ) );																		// C
	header.PushBack( TXT( "Journal Entry" ) );																// D
	header.PushBack( TXT( "Parent Entry" ) );																// E
	header.PushBack( TXT( "Journal Entry Type" ) );															// F
	header.PushBack( TXT( "Unused" ) );																		// G	- Source language part
	header.PushBack( TXT( "Journal Field Description" ) );													// H
	header.PushBack( TXT( "Unused" ) );																		// I
	header.PushBack( String::Printf( TXT( "Source Language %s" ), m_sourceLanguage.AsChar() ) );			// J
	header.PushBack( TXT( "Unused" ) );																		// K
	header.PushBack( TXT( "Unused" ) );																		// L

	for( Uint32 i = 0; i < m_targetLanguages.Size(); ++i )
	{
		header.PushBack( TXT( "Unused" ) );																	// M...
		header.PushBack( TXT( "Unused" ) );																	// N...
		header.PushBack( TXT( "Unused" ) );																	// O...
		header.PushBack( String::Printf( TXT( "Target Language %s" ), m_targetLanguages[ i ].AsChar() ) );	// P...
		header.PushBack( TXT( "Unused" ) );																	// Q...
		header.PushBack( TXT( "Unused" ) );																	// R...
	}

	header.PushBack( TXT( "Unused" ) );																		// S...

	m_exportData.PushBack( header );

	PrepareErrorFileHeaders();
}

void CJournalLocalizationExporter::EndBatchExport()
{
	String exportString = ConvertToCSV();
	if ( exportString.Empty() == false )
	{
		GFileManager->SaveStringToFileWithUTF8( m_savePath, exportString );
	}

	DumpErrorsFile();
}

void CJournalLocalizationExporter::ExportBatchEntry( const String& entry )
{
	SStringConversionHelper helper;

	helper.FromString( entry );

	// Get Journal Entry
	CJournalBase* journalEntry = NULL;
	VERIFY( m_journalEntries.Find( helper.entryGuid, journalEntry ), TXT( "Journal Entry must exist in map to reach this point" ) );

	// Get Localized String Property
	CProperty* property = journalEntry->GetClass()->FindProperty( helper.propertyName );
	ASSERT( property != NULL, TXT( "Property %s doesn't exist in journal entry %s (%s)" ), helper.propertyName.AsString().AsChar(), journalEntry->GetName().AsChar(), journalEntry->GetClass()->GetName().AsString().AsChar() );

	// Create Row
	TDynArray< String > csvRow;

	String parentJournalEntryName = String::EMPTY;
	if( journalEntry->GetParent()->IsA< CJournalBase >() )						
	{
		parentJournalEntryName = journalEntry->GetParentAs< CJournalBase >()->GetName();
	}

	// Info part

	csvRow.PushBack( helper.ToString() );																	// A - Entry ID
	csvRow.PushBack( helper.localizedStringIndexStr );														// B - String ID
	csvRow.PushBack( String::EMPTY );																		// C
	csvRow.PushBack( journalEntry->GetName() );																// D - Journal Entry
	csvRow.PushBack( parentJournalEntryName );																// E - Parent Entry
	csvRow.PushBack( journalEntry->GetClass()->GetName().AsString() );										// F - Journal Entry Type
	
	// Source language part
	csvRow.PushBack( String::EMPTY );																		// G
	csvRow.PushBack( property->GetHint() );																	// H - Journal Field Description
	csvRow.PushBack( String::EMPTY );																		// I
	csvRow.PushBack( GetSourceString( helper.localizedString ) );											// J - Source Language
	csvRow.PushBack( String::EMPTY );																		// K
	csvRow.PushBack( String::EMPTY );																		// L

	// Target Languages
	for( Uint32 i = 0; i < m_targetLanguages.Size(); ++i )
	{
		csvRow.PushBack( String::EMPTY );																	// M...
		csvRow.PushBack( String::EMPTY );																	// N...
		csvRow.PushBack( String::EMPTY );																	// O...
		csvRow.PushBack( GetTargetString( helper.localizedString, i ) );									// P...
		csvRow.PushBack( String::EMPTY );																	// Q...
		csvRow.PushBack( String::EMPTY );																	// R...
	}

	csvRow.PushBack( SLocalizationManager::GetInstance().GetVoiceoverFilename( helper.localizedString.GetIndex() ) );	// S... - Voiceover name

	m_exportData.PushBack( csvRow );
}

String CJournalLocalizationExporter::GetResourceExtension() const
{
	return CJournalResource::GetFileExtension();
}

void CJournalLocalizationExporter::FillRootExportGroup( BatchExportGroup& exportGroup )
{
	GFeedback->BeginTask( TXT( "Generating Journal Tree" ), false );

	exportGroup.m_groupName = TXT( "Journal" );

	TDynArray< String > categories;
	categories.PushBack( TXT( "Quests" ) );
	categories.PushBack( TXT( "Characters" ) );
	categories.PushBack( TXT( "Glossary" ) );
	categories.PushBack( TXT( "Tutorial" ) );
	categories.PushBack( TXT( "Items" ) );
	categories.PushBack( TXT( "Creatures" ) );
	categories.PushBack( TXT( "Story Book" ) );
	categories.PushBack( TXT( "Places" ) );

	TDynArray< TDynArray< CDirectory* > > directories;
	directories.PushBack( CEdJournalTree::GetQuestsDirectories() );
	directories.PushBack( CEdJournalTree::GetCharactersDirectories() );
	directories.PushBack( CEdJournalTree::GetGlossaryDirectories() );
	directories.PushBack( CEdJournalTree::GetTutorialDirectories() );
	directories.PushBack( CEdJournalTree::GetItemsDirectories() );
	directories.PushBack( CEdJournalTree::GetCreaturesDirectories() );
	directories.PushBack( CEdJournalTree::GetStoryBookDirectories() );
	directories.PushBack( CEdJournalTree::GetPlacesDirectories() );

	ASSERT( categories.Size() == directories.Size(), TXT( "Journal Categories missing in localization export" ) );

	for( Uint32 i = 0; i < categories.Size(); ++i )
	{
		GFeedback->UpdateTaskProgress( i, categories.Size() );

		// Quests
		BatchExportGroup group;
		group.m_groupName = categories[ i ];

		PopulateTreeSection( new SJournalTreeItemBEG( group ), directories[ i ] );
		exportGroup.m_subGroups.PushBack( group );
	}

	Uint32 order = 1;
	m_journalStrings.ClearFast();
	FillJournalStrings( m_journalStrings, exportGroup, order );

	GFeedback->EndTask();
}

void CJournalLocalizationExporter::FillJournalStrings( THashMap< String, Int32 >& journalStrings, BatchExportGroup& exportGroup, Uint32& order )
{
	for ( Uint32 i = 0; i < exportGroup.m_groupEntries.Size(); ++i )
	{
		journalStrings[ exportGroup.m_groupEntries[ i ] ] = order++;
	}
	for ( Uint32 i = 0; i < exportGroup.m_subGroups.Size(); ++i )
	{
		FillJournalStrings( journalStrings, exportGroup.m_subGroups[ i ], order );
	}
};

SJournalItemHandle* CJournalLocalizationExporter::AddItemAppend( SJournalItemHandle* parentItem, CJournalBase* entry, CJournalResource* journalResource, CDirectory* sectionDirectory )
{
	GFeedback->UpdateTaskInfo( entry->GetName().AsChar() );

	VERIFY( m_journalEntries.Insert( entry->GetGUID(), entry ), TXT( "Duplicate guid in journal: %s" ), entry->GetName().AsChar() );

	BatchExportGroup group;
	group.m_groupName = entry->GetName();
	group.m_order = entry->GetOrder();

	// Localised strings
	TDynArray< CProperty* > properties;
	CClass* journalClass = entry->GetClass();
	journalClass->GetProperties( properties );

	for( Uint32 i = 0; i < properties.Size(); ++i )
	{
		if( properties[ i ]->GetType()->GetName() == CNAME( LocalizedString ) )
		{
			SStringConversionHelper helper;

			properties[ i ]->Get( entry, &( helper.localizedString ) );

			String locStr = helper.localizedString.GetString();

			if( !locStr.Empty() )
			{
				helper.entryGuid = entry->GetGUID();
				helper.propertyName = properties[ i ]->GetName();
				helper.order = entry->GetOrder();

				// Hide the string data by adding a space
				// Ideally we'd use internal "userdata" but there isn't any with the localisation exporter
				locStr += TXT( "                                                                                                        " );
				locStr += helper.ToString();

				group.m_groupEntries.PushBack( locStr );
			}
		}
	}

	SJournalTreeItemBEG* parentHandle = static_cast< SJournalTreeItemBEG* >( parentItem );
	parentHandle->m_group.m_subGroups.PushBack( group );

	return new SJournalTreeItemBEG( ( parentHandle->m_group.m_subGroups.Back() ) );
}

void CJournalLocalizationExporter::MaximumNumberOfChildEntries( SJournalItemHandle* parentItem, Uint32 number )
{
	SJournalTreeItemBEG* parentHandle = static_cast< SJournalTreeItemBEG* >( parentItem );
	parentHandle->m_group.m_subGroups.Reserve( number );
}

void CJournalLocalizationExporter::Sort( SJournalItemHandle* parentHandleBase )
{
 	SJournalTreeItemBEG* parentHandle = static_cast< SJournalTreeItemBEG* >( parentHandleBase );
 
	if ( !parentHandle )
	{
		return;
	}
	TDynArray< BatchExportGroup >& subgroups = parentHandle->m_group.m_subGroups;
	if ( subgroups.Empty() )
	{
		return;
	}

	struct SBEGPredicate
	{
		bool operator()( const BatchExportGroup& a, const BatchExportGroup& b ) const
		{
			return a.m_order < b.m_order;
		}
	};

	::Sort( subgroups.Begin(), subgroups.End(), SBEGPredicate() );
}

Bool CJournalLocalizationExporter::IsBatchEntryValid( const String& entry ) const
{
	// nothing checked - entry is valid
	return true;
}

Bool CJournalLocalizationExporter::ValidateBatchEntry( const String& entry )
{
	// nothing checked, nothing to validate - entry is valid
	return true;
}

void CJournalLocalizationExporter::PushBatchEntryError( const String& entry, const String& errorMsg )
{
	TDynArray< String > csvRow;
	csvRow.PushBack( entry );
	csvRow.PushBack( errorMsg );
	m_errorData.PushBack( csvRow );
}
