// Copyright © 2014 CD Projekt Red. All Rights Reserved.

#include "build.h"
#include "w2StringImporter.h"
#include "../../common/game/storyScene.h"
#include "../../common/game/storySceneSection.h"
#include "../../common/game/storySceneLine.h"
#include "../../common/engine/localizationManager.h"

// =================================================================================================
namespace {
// =================================================================================================

// functions in this namespace
Bool GetImportW2StringMarkerValue( const String& str, Uint32& outW2StringId );
void GetLinesWithImportW2StringMarker( const CStorySceneSection& section, TDynArray< CStorySceneLine* >& outLines );

/*
Gets value of import W2 string marker from specified string.

Import W2 string marker looks like this: W2_123, where 123 denotes id of a string from W2 String DB.

\param str String in which to search for import W2 string marker.
\param outW2StringId (out) Storage for the value of import W2 string marker.
\return True - import W2 string marker found and its value was stored in outW2StringId, false - import W2 string marker not found or invalid.
*/
Bool GetImportW2StringMarkerValue( const String& str, Uint32& outW2StringId )
{
	static const String markerPattern( TXT( "W2_" ) );

	if( str.LeftString( markerPattern.GetLength() ) == markerPattern )
	{
		Int32 numFieldsRecognized = swscanf_s( str.AsChar() + markerPattern.GetLength(), TXT( "%u" ), &outW2StringId );
		if( numFieldsRecognized == 1 )
		{
			return true;
		}
	}

	return false;
}

/*
Searches story scene section for all dialog lines containing W2 import string marker.

\param section Section to be searched.
\param outLines (out) List of dialog lines containing W2 import string marker. Not cleared before use.

Note that this function will find only those dialog lines whose text contains W2 import string marker.
Dialog line comment is not examined.
*/
void GetLinesWithImportW2StringMarker( const CStorySceneSection& section, TDynArray< CStorySceneLine* >& outLines )
{
	TDynArray< CAbstractStorySceneLine* > lines;
	section.GetLines( lines );

	for( auto it = lines.Begin(), end = lines.End(); it != end; ++it )
	{
		if( CStorySceneLine* line = Cast< CStorySceneLine >( *it ) )
		{
			Uint32 w2StringId = 0;
			Bool foundW2StringId = GetImportW2StringMarkerValue( line->GetLocalizedContent()->GetString(), w2StringId );

			if( foundW2StringId && w2StringId != 0 )
			{
				outLines.PushBack( line );
			}
		}
	}
}

// =================================================================================================
} // unnamed namespace
// =================================================================================================

/*
Ctor.
*/
W2StringImporter::W2StringImporter()
: m_isInitialized( false )
, m_connW2StringDb( SQL_NULL_HDBC )
, m_connW3StringDb( SQL_NULL_HDBC )
{}

/*
Dtor.
*/
W2StringImporter::~W2StringImporter()
{
	// assert that importer was properly uninitialized
	ASSERT( !IsInitialized() );
}

Bool W2StringImporter::IsInitialized() const
{
	return m_isInitialized;
}

/*
Initializes importer.

\return True - importer successfully initialized, false - failure.
*/
Bool W2StringImporter::Initialize()
{
	if( m_odbcMgr.Initialize() )
	{
		// connect to W2 String DB and W3 String DB
		m_connW2StringDb = ConnectW2StringDb( m_odbcMgr );
		m_connW3StringDb = ConnectW3StringDb( m_odbcMgr );

		// prepare all sql statements that we're going to use
		m_querySelectStringsW2.Prepare( m_connW2StringDb );
		m_queryGetNextStringIdW3.Prepare( m_connW3StringDb );
		m_queryInsertStringInfoW3.Prepare( m_connW3StringDb );
		m_queryInsertStringW3.Prepare( m_connW3StringDb );
		m_queryDeleteStringW3.Prepare( m_connW3StringDb );
		m_queryDeleteStringInfoW3.Prepare( m_connW3StringDb );

		m_isInitialized = true;
	}

	return m_isInitialized;
}

/*
Uninitializes importer.
*/
Bool W2StringImporter::Uninitialize()
{
	m_querySelectStringsW2.Reset();
	m_queryGetNextStringIdW3.Reset();
	m_queryInsertStringInfoW3.Reset();
	m_queryInsertStringW3.Reset();
	m_queryDeleteStringW3.Reset();
	m_queryDeleteStringInfoW3.Reset();

	m_odbcMgr.Disconnect( m_connW2StringDb );
	m_odbcMgr.Disconnect( m_connW3StringDb );

	m_odbcMgr.Uninitialize();

	m_isInitialized = false;

	return true;
}

Bool W2StringImporter::ProcessSection( const CStorySceneSection& section )
{
	const String resourceName = section.GetScene()->GetFriendlyName();

	TDynArray< CStorySceneLine* > linesToProcess;
	GetLinesWithImportW2StringMarker( section, linesToProcess );

	for( auto it = linesToProcess.Begin(), end = linesToProcess.End(); it != end; ++it )
	{
		CStorySceneLine* line = *it;
		ProcessLocalizedString( *line->GetLocalizedContent(), resourceName, TXT( "Line text" ) );
		ProcessLocalizedString( *line->GetLocalizedComment(), resourceName, TXT( "Line comment" ) );
	}

	return true;
}

/*
Processes localized string.

\param lstr Localized string to process.
\param resourceName String to use as a value for RESOURCE column in STRING_INFO table.
\param propertyName String to use as a value for PROPERTY_NAME column in STRING_INFO table.
*/
void W2StringImporter::ProcessLocalizedString( LocalizedString& lstr, const String& resourceName, const String& propertyName )
{
	ASSERT( IsInitialized() );

	Uint32 w2StringId = 0;
	Bool foundW2StringId = GetImportW2StringMarkerValue( lstr.GetString(), w2StringId );

	if( foundW2StringId && w2StringId != 0 )
	{
		// Get text entries from W2 String DB.
		TDynArray< StringRow > stringRows;
		m_querySelectStringsW2.Execute( w2StringId, stringRows );

		if( !stringRows.Empty() )
		{
			Uint32 nextStringIdW3 = 0;
			Bool res = m_queryGetNextStringIdW3.Execute( nextStringIdW3 );

			if( res )
			{
				// Construct string info for new W3 string.
				StringInfoRow si;
				si.m_stringId = nextStringIdW3;
				si.m_resourceName = resourceName;
				si.m_propertyName = propertyName;
				si.m_voiceoverName = String::EMPTY;
				si.m_stringKey = String::EMPTY;

				// Insert string info entry into STRING_INFO table in W3 String DB.
				res = m_queryInsertStringInfoW3.Execute( si );

				if( res )
				{
					// Insert rows into STRINGS table in W3 String DB.
					for( auto it = stringRows.Begin(), end = stringRows.End(); it != end; ++it )
					{
						StringRow& row = *it;
						row.m_stringId = nextStringIdW3;
						m_queryInsertStringW3.Execute( row );
					}

					Uint32 oldStringId = lstr.GetIndex();
					lstr.SetIndex( si.m_stringId );

					// New string id has been assigned to lstr so we can get rid of all entries with old string id.
					SLocalizationManager::GetInstance().DiscardStringModification( oldStringId );
					if( !IsUnstableStringId( oldStringId) )
					{
						res = m_queryDeleteStringW3.Execute( oldStringId );
						if( res )
						{
							m_queryDeleteStringInfoW3.Execute( oldStringId );
						}
					}
				}
				else
				{
					// Failed to insert row into STRING_INFO table. Nothing to do here.
				}
			}
		}
	}
}
