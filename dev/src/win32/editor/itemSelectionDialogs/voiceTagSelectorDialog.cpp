#include "build.h"

#include "voiceTagSelectorDialog.h"
#include "../editorExternalResources.h"

#include "../../../common/core/depot.h"
#include "../../../common/game/storySceneVoiceTagsManager.h"

CEdVoiceTagSelectorDialog::CEdVoiceTagSelectorDialog( wxWindow* parent, const CName& defaultSelected )
	: CEdItemSelectorDialog( parent, TXT( "/Frames/VoiceTagSelectorDialog" ), TXT( "Voice Tags" ) )
	, m_defaultSelected( defaultSelected )
{
}

CEdVoiceTagSelectorDialog::~CEdVoiceTagSelectorDialog()
{

}

void CEdVoiceTagSelectorDialog::Populate()
{
	const C2dArray* voiceTagArray = SStorySceneVoiceTagsManager::GetInstance().ReloadVoiceTags();
	if ( voiceTagArray )
	{
		m_tags.Reserve( voiceTagArray->GetNumberOfRows() + 1 );

		// Add null
		m_tags.PushBack( CName( String::EMPTY ) );
		AddItem( TXT( "Undefined" ), &m_tags.Back(), true, m_tags.Back() == m_defaultSelected );

		for ( Uint32 i = 0; i < voiceTagArray->GetNumberOfRows(); ++i )
		{
			CName tag( voiceTagArray->GetValue( 0, i ) );
			m_tags.PushBack( tag );

			AddItem( tag.AsString(), &m_tags.Back(), true, -1, m_tags.Back() == m_defaultSelected );
		}
	}
	else
	{
		AddItem( TXT( "ERROR: No Voice Tags Table" ), NULL, false );
	}
}
