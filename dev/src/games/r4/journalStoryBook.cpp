#include "build.h"
#include "journalStoryBook.h"
#include "../../common/core/diskFile.h"
#include "../../common/engine/localizationManager.h"

//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------

IMPLEMENT_ENGINE_CLASS( CJournalStoryBookPageDescription );

CJournalStoryBookPageDescription::CJournalStoryBookPageDescription()
	: m_isFinal( false )
{

}

CJournalStoryBookPageDescription::~CJournalStoryBookPageDescription()
{

}

Bool CJournalStoryBookPageDescription::IsParentClass( CJournalBase* other ) const
{
	return other->IsA< CJournalStoryBookPage >();
}

#ifndef NO_EDITOR_RESOURCE_SAVE

void CJournalStoryBookPageDescription::OnPreSave()
{
	// Generate Voiceover filename
	CJournalStoryBookPage* page = Cast< CJournalStoryBookPage >( GetParent() );
	CResource* parentResource = page->GetParentAs< CResource >();

	CFilePath resourceFilename( parentResource->GetFile()->GetFileName().AsChar() );

	String voiceoverFilename( CLocalizationManager::VOICEFILE_UNDEFINED );
	voiceoverFilename.Replace( CLocalizationManager::VOICEFILE_UNDEFINED_VOICE, TXT( "JSKR" ) );
	voiceoverFilename.Replace( CLocalizationManager::VOICEFILE_UNDEFINED_GROUP, TXT( "SBOOK" ) );
	voiceoverFilename.Replace( CLocalizationManager::VOICEFILE_UNDEFINED_ID, String::Printf( TXT( "%u" ), m_description.GetIndex() ) );

	// Only update the database if the filename has changed
	if( SLocalizationManager::GetInstance().GetVoiceoverFilename( m_description.GetIndex() ) != voiceoverFilename )
	{
RED_WARNING_PUSH()
RED_DISABLE_WARNING_MSC( 4512 )
RED_MESSAGE( "CStringDBSubmitter needs to address warning 4512 ( No assignment operator can be generated )" )
		class CStringDBSubmitter : public ILocalizableObject
		{
		public:
			CStringDBSubmitter( LocalizedStringEntry& stringEntry )
				:	m_stringEntry( stringEntry )
			{
			}

			virtual void GetLocalizedStrings( TDynArray< LocalizedStringEntry >& localizedStrings )
			{
				localizedStrings.PushBack( m_stringEntry );
			}

		private:
			LocalizedStringEntry& m_stringEntry;
		};
RED_WARNING_POP()
		LocalizedStringEntry stringEntry( &m_description, JOURNAL_STRINGDB_PROPERTY_NAME, parentResource, voiceoverFilename );
		CStringDBSubmitter submitter( stringEntry );

		JOURNAL_LOG( TXT( "Generated story book string (ID:%u) voiceover filename: %s" ), m_description.GetIndex(), voiceoverFilename.AsChar() );

		SLocalizationManager::GetInstance().UpdateStringDatabase( &submitter, true );
	}
}

#endif // NO_EDITOR_RESOURCE_SAVE

void CJournalStoryBookPageDescription::funcGetVideoFilename( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS
		RETURN_STRING( m_videoFilename )
}

void CJournalStoryBookPageDescription::funcGetDescriptionStringId( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS
	RETURN_INT( m_description.GetIndex() )
}

//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------

IMPLEMENT_ENGINE_CLASS( CJournalStoryBookPage );

CJournalStoryBookPage::CJournalStoryBookPage()
{

}

CJournalStoryBookPage::~CJournalStoryBookPage()
{

}

void CJournalStoryBookPage::DefaultValues()
{
	m_world = 0;
}

Bool CJournalStoryBookPage::IsParentClass( CJournalBase* other ) const
{
	return other->IsA< CJournalStoryBookChapter >();
}

void CJournalStoryBookPage::funcGetTitleStringId( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS
	RETURN_INT( m_title.GetIndex() )
}

//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------

IMPLEMENT_ENGINE_CLASS( CJournalStoryBookChapter );

CJournalStoryBookChapter::CJournalStoryBookChapter()
{

}

CJournalStoryBookChapter::~CJournalStoryBookChapter()
{

}

Bool CJournalStoryBookChapter::IsParentClass( CJournalBase* other ) const
{
	return other->IsA< CJournalStoryBookRoot >();
}

void CJournalStoryBookChapter::funcGetTitleStringId( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS
	RETURN_INT( m_title.GetIndex() )
}

void CJournalStoryBookChapter::funcGetImage( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_STRING( m_image );
}

//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------

IMPLEMENT_ENGINE_CLASS( CJournalStoryBookRoot );

CJournalStoryBookRoot::CJournalStoryBookRoot()
{

}

CJournalStoryBookRoot::~CJournalStoryBookRoot()
{

}

Bool CJournalStoryBookRoot::IsParentClass( CJournalBase* other ) const
{
	return false;
}
