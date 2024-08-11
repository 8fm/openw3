/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "editorAnimCacheGen.h"
#include "../../common/engine/animationCacheDepotSaver.h"
#include "../../common/engine/skeletalAnimation.h"

BEGIN_EVENT_TABLE( CEdAnimCacheGenerator, wxSmartLayoutPanel )
	EVT_BUTTON( XRCID("btnGen"), CEdAnimCacheGenerator::OnGenerate )
	EVT_BUTTON( XRCID("btnClearAnim"), CEdAnimCacheGenerator::OnUnloadAnims )
	EVT_BUTTON( XRCID("btnLoadAnim"), CEdAnimCacheGenerator::OnLoadAnims )
	EVT_BUTTON( XRCID("btnStreamAnim"), CEdAnimCacheGenerator::OnStreamAnims )
	EVT_BUTTON( XRCID("btnReport"), CEdAnimCacheGenerator::OnReport )
	EVT_BUTTON( XRCID("btnLoad"), CEdAnimCacheGenerator::OnLoad )
END_EVENT_TABLE()

CEdAnimCacheGenerator::CEdAnimCacheGenerator( wxWindow* parent )
	: wxSmartLayoutPanel( parent, TXT("AnimCacheGenerator"), true )
{
	m_log = XRCCTRL( *this, "textLog", wxTextCtrl );
}

void CEdAnimCacheGenerator::Log( wxString str )
{
	wxString allStr = m_log->GetValue();
	allStr += wxString::Format( wxT("%s\n"), str.wc_str() );
	m_log->SetValue( allStr );
}

void CEdAnimCacheGenerator::OnGenerate( wxCommandEvent& event )
{
	/*GFeedback->BeginTask( TXT("Generating animation cache..."), false );

	Log( wxT("Collect animations from depot...") );

	SAnimationsCache::GetInstance().Destroy();

	AnimationCacheDepotSaver saver;
	saver.CollectAnimationsFromDepot();

	String path = SAnimationsCache::GetInstance().GetPath();

	Log( wxT("Saving cache file...") );

	if ( saver.SaveToFile( path ) )
	{
		Log( wxT("File anim.cache was generated") );
		
		if ( SAnimationsCache::GetInstance().ForceInitialize() )
		{
			Log( wxT("Animation cache was reloaded") );
		}
		else
		{
			Log( wxT("Couldn't reload animation cache") );
		}
	}
	else
	{
		Log( wxT("Couldn't generate anim.cache file") );
	}

	GAnimationManager->Debug_CacheIds();

	GFeedback->EndTask();*/
}

void CEdAnimCacheGenerator::OnLoad( wxCommandEvent& event )
{

}

void CEdAnimCacheGenerator::OnLoadAnims( wxCommandEvent& event )
{
	Log( wxT("Loading animations sync...") );
	
	Uint32 num = 0;

	// TODO: CSkeletalAnimation is not an object any more
	/*
	for ( ObjectIterator< CSkeletalAnimation > it; it; ++it )
	{
		CSkeletalAnimation* anim = (*it);
		if ( !anim->IsLoaded() && !anim->HasStreamingPending() )
		{
			num++;
			anim->Touch();

			if ( num > 2000 )
			{
				break;
			}
		}
	}
	*/
	Log( wxString::Format("Loaded '%d' animations", num ) );
}

void CEdAnimCacheGenerator::OnStreamAnims( wxCommandEvent& event )
{
	Log( wxT("Streaming animations async...") );

	Uint32 num = 0;

	// TODO: CSkeletalAnimation is not an object any more
	/*
	for ( ObjectIterator< CSkeletalAnimation > it; it; ++it )
	{
		CSkeletalAnimation* anim = (*it);
		if ( !anim->IsLoaded() && !anim->HasStreamingPending() )
		{
			num++;
			anim->Touch();

			if ( num > 2000 )
			{
				break;
			}
		}
	}
	*/
}

void CEdAnimCacheGenerator::OnUnloadAnims( wxCommandEvent& event )
{
	// TODO: animation manager code path seems broken
	/*TDynArray< CSkeletalAnimation* > anims;
	SObjectsMap::GetInstance().GetAllObject( anims );

	for ( Uint32 i=0; i<anims.Size(); ++i )
	{
		CSkeletalAnimation* anim = anims[ i ];
		anim->Unload();
	}

	Log( wxString::Format("Unloaded '%d' animations", anims.Size() ) );*/
}

void CEdAnimCacheGenerator::OnReport( wxCommandEvent& event )
{
	Uint32 loaded = 0;
	Uint32 notLoaded = 0;
	Uint32 streaming = 0;

	// TODO: CSkeletalAnimation is not an object any more
	/*
	for ( ObjectIterator< CSkeletalAnimation > it; it; ++it )
	{
		CSkeletalAnimation* anim = (*it);

		if ( anim->IsLoaded() )
		{
			loaded++;
		}
		else if ( anim->HasStreamingPending() )
		{
			streaming++;
		}
		else
		{
			notLoaded++;
		}
	}
	*/

	Log( wxString::Format( wxT("Loaded animation(s): %d"), loaded ) );
	Log( wxString::Format( wxT("Unloaded animation(s): %d"), notLoaded ) );
	Log( wxString::Format( wxT("Streamed animation(s): %d"), streaming ) );
}
