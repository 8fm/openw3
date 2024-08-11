/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "animBrowserCompression.h"
#include "animBrowserPreview.h"
#include "undoManager.h"
#include "undoAnimBrowser.h"
#include "../../common/engine/behaviorGraphStack.h"
#include "../../common/game/actor.h"
#include "../../common/core/dependencySaver.h"
#include "../../common/engine/skeletalAnimationEntry.h"
#include "../../common/engine/animatedSkeleton.h"
#include "../../common/engine/animationCompression.h"
#include "../../common/engine/dynamicLayer.h"
#include "../../common/engine/worldTick.h"

BEGIN_EVENT_TABLE( CAnimBrowserCompressionPage, wxPanel )
	EVT_CHOICE( XRCID( "animChoice"), CAnimBrowserCompressionPage::OnAnimChanged )
	EVT_BUTTON( XRCID( "loadButt" ), CAnimBrowserCompressionPage::OnLoadAnimation )
	EVT_BUTTON( XRCID( "buttAnimUp" ), CAnimBrowserCompressionPage::OnAnimUp)
	EVT_BUTTON( XRCID( "buttAnimDown" ), CAnimBrowserCompressionPage::OnAnimDown )
	EVT_COMMAND_SCROLL( XRCID( "sliderOffsetPos" ), CAnimBrowserCompressionPage::OnPosOffsetChanged )
	EVT_COMMAND_SCROLL( XRCID( "compressionSlider" ), CAnimBrowserCompressionPage::OnCompressSliderScrolled )
	EVT_CHECKBOX( XRCID( "checkShowMeshes" ), CAnimBrowserCompressionPage::OnShowMeshes )
	EVT_BUTTON( XRCID( "buttApply" ), CAnimBrowserCompressionPage::OnApply )
	EVT_RADIOBUTTON( XRCID( "radioFps5" ), CAnimBrowserCompressionPage::OnFpsChanged )
	EVT_RADIOBUTTON( XRCID( "radioFps10" ), CAnimBrowserCompressionPage::OnFpsChanged )
	EVT_RADIOBUTTON( XRCID( "radioFps15" ), CAnimBrowserCompressionPage::OnFpsChanged )
	EVT_RADIOBUTTON( XRCID( "radioFps30" ), CAnimBrowserCompressionPage::OnFpsChanged )
END_EVENT_TABLE()

CAnimBrowserCompressionPage::CAnimBrowserCompressionPage( wxWindow* parent, CEdAnimBrowser* browser, CEdUndoManager* undoManager )
	: wxPanel( parent )
	, m_browser( browser )
	, m_compression( NULL )
	, m_compressedAnimation( NULL )
	, m_uncompressedAnimation( NULL )
	, m_active( false )
	, m_undoManager( undoManager )
{
	// Load designed frame from resource
	wxPanel* innerPanel = wxXmlResource::Get()->LoadPanel( this, wxT("AnimBrowserCompressionPanel") );

	m_compressionNotebook = XRCCTRL( *this, "compressionNotebook", wxNotebook );
	m_comprAnimationChoice = XRCCTRL( *this, "animChoice", wxChoice );
	m_infoWindow = XRCCTRL( *this, "infoWin", wxHtmlWindow );

	m_compressionSlider = XRCCTRL( *this, "compressionSlider", wxSlider );
	m_compressionSlider->SetTickFreq(100);
	m_compressionSlider->GetToolTip()->SetDelay(1);
	m_compressionSlider->Enable( false );

	wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );
	sizer->Add( innerPanel, 1, wxEXPAND|wxALL, 0 );

	SetSizer( sizer );
	Layout();

	m_uncomprElem.Clear();
	m_comprElem.Clear();
}

void CAnimBrowserCompressionPage::DestroyPanel()
{
	DestroyEntities();
	DestroyUncompressedAnimations();
	DestroyCompressedAnimation();
}

void CAnimBrowserCompressionPage::EnablePanel( Bool flag )
{
	if ( flag == m_active )
	{
		return;
	}

	m_browser->ShowEntity( !flag );
	m_browser->Pause( !flag );

	if ( flag && m_browser->m_animatedComponent )
	{
		if ( CloneEntitiesFromComponent( m_browser->m_animatedComponent ) )
		{
			TDynArray< CAnimatedComponent* > comps;
			comps.PushBack( m_uncomprElem.m_component );
			comps.PushBack( m_comprElem.m_component );
			m_browser->m_previewPanel->SetAnimatedComponents( comps );
		}

		FillAnimationList( m_browser->m_selectedAnimSet );

		//ChangeCompressionPage( PAGE_COMPR_INFO );

		CreateCompression();
		CreateCompressedAnimation();

		OnRefreshAnimation();
	}
	else
	{
		DestroyCompression();
		DestroyCompressedAnimation();

		TDynArray< CAnimatedComponent* > comps;
		comps.PushBack( m_browser->m_animatedComponent );
		m_browser->m_previewPanel->SetAnimatedComponents( comps );

		DestroyEntities();

		DestroyUncompressedAnimations();
		DestroyCompressedAnimation();

		FillAnimationList( NULL );
	}

	m_active = flag;
}

void CAnimBrowserCompressionPage::CreateCompression()
{
	ASSERT( m_compression == NULL, TXT( "Compression already created" ) );

	m_compression = CreateObject< CSplineAnimationCompression >();
	m_compression->AddToRootSet();

	m_compressionSlider->Enable( true );
}

void CAnimBrowserCompressionPage::DestroyCompression()
{
	if ( m_compression )
	{
		m_compression->RemoveFromRootSet();
		m_compression->Discard();
		m_compression = NULL;

		m_compressionSlider->Enable( false );
	}
}

void CAnimBrowserCompressionPage::CreateCompressedAnimation()
{
	ASSERT( m_compressedAnimation == NULL );
	m_compressedAnimation = new CSkeletalAnimationSetEntry;
}

void CAnimBrowserCompressionPage::DestroyCompressedAnimation()
{
	if ( m_compressedAnimation )
	{
		delete m_compressedAnimation;
		m_compressedAnimation = NULL;
	}
}

void CAnimBrowserCompressionPage::SetupCompressionFromAnim( const CSkeletalAnimationSetEntry* animation )
{
	const CSkeletalAnimation* skeletalAnim = animation->GetAnimation();
	if ( skeletalAnim )
	{
		m_compression->SetCompressionFactor( 0.0f ); // was always 0.0
	}

	RefreshCompressionPage();
}

void CAnimBrowserCompressionPage::RefreshCompressionPage()
{
	// ustaw wszystko np.slidery
}

void CAnimBrowserCompressionPage::OnRefreshAnimation()
{
	RefreshLoadButton();

	if ( m_browser->m_playedAnimation->IsValid() && m_comprElem.m_component && m_uncomprElem.m_component )
	{
		RefreshAnimation( m_browser->m_playedAnimation->m_playedAnimation, m_comprElem, m_compressedAnimation );
		RefreshAnimation( m_browser->m_playedAnimation->m_playedAnimation, m_uncomprElem, m_uncompressedAnimation );
	}
}

void CAnimBrowserCompressionPage::OnSelectedAnimation()
{
	RefreshLoadButton();

	if ( m_browser->m_playedAnimation->IsValid() && m_comprElem.m_component && m_uncomprElem.m_component )
	{
		ASSERT( m_browser->m_playedAnimation->GetAnimationEntry() == m_browser->m_selectedAnimation );

		m_compressedAnimation->SetAnimation( m_browser->m_selectedAnimation->GetAnimation() );

		RecreateAnimation( m_browser->m_playedAnimation->m_playedAnimation, m_comprElem, m_compressedAnimation );
		RecreateAnimation( m_browser->m_playedAnimation->m_playedAnimation, m_uncomprElem, m_uncompressedAnimation );
	}
}

void CAnimBrowserCompressionPage::RecreateAnimation( const CPlayedSkeletalAnimation* browserAnim, 
												   PreviewElem& elem, 
												   CSkeletalAnimationSetEntry* animation )
{
	if ( browserAnim && elem.m_component )
	{
		const CName& animName = browserAnim->GetName();

		if ( elem.m_playedAnimation && elem.m_playedAnimation->GetName() != animName )
		{
			elem.m_component->StopAllAnimationsOnSkeleton();
			elem.m_playedAnimation = NULL;
		}

		if ( elem.m_playedAnimation == NULL && animation && animation->GetAnimation() )
		{
			elem.m_playedAnimation = elem.m_component->GetAnimatedSkeleton()->PlayAnimation( animation );
		}

		if ( elem.m_component->GetBehaviorStack() && elem.m_component->GetBehaviorStack()->IsActive() )
		{
			elem.m_component->GetBehaviorStack()->Deactivate();
		}

		RefreshAnimation( browserAnim, elem, animation );
	}
}

void CAnimBrowserCompressionPage::RefreshAnimation( const CPlayedSkeletalAnimation* browserAnim, 
												    PreviewElem& elem, 
													CSkeletalAnimationSetEntry* animation )
{
	if ( browserAnim && elem.m_component && elem.m_playedAnimation )
	{
		const CName& animName = browserAnim->GetName();
		Float time = browserAnim->GetTime();
		Float speed = browserAnim->GetSpeed();
		Bool paused = browserAnim->IsPaused();

		ASSERT(  elem.m_playedAnimation->GetName() == animName );
		ASSERT( elem.m_component->GetBehaviorStack()->IsActive() == false );

		elem.m_playedAnimation->SetTime( time );
		elem.m_playedAnimation->SetSpeed( speed );

		if ( paused && elem.m_playedAnimation->IsPaused() == false )
		{
			elem.m_playedAnimation->Pause();
		}
		else if ( paused == false && elem.m_playedAnimation->IsPaused() )
		{
			elem.m_playedAnimation->Unpause();
		}
	}
}

void CAnimBrowserCompressionPage::FillAnimationList( const CSkeletalAnimationSet* set )
{
	m_comprAnimationChoice->Freeze();
	m_comprAnimationChoice->Clear();

	if ( m_browser->IsAnimsetValid() )
	{
		TDynArray< CSkeletalAnimationSetEntry* > setAnims;
		m_browser->GetAnimationsFromAnimset( m_browser->m_selectedAnimSet, setAnims );

		for( Uint32 i=0; i<setAnims.Size(); ++i )
		{
			if ( setAnims[i]->GetAnimation() )
			{
				m_comprAnimationChoice->Append( setAnims[i]->GetAnimation()->GetName().AsString().AsChar(), setAnims[i] );
			}
		}

		if ( m_browser->IsAnimationValid() )
		{
			m_comprAnimationChoice->SetStringSelection( m_browser->m_selectedAnimation->GetAnimation()->GetName().AsString().AsChar() );
		}
		else
		{
			m_comprAnimationChoice->SetSelection( wxNOT_FOUND );
		}
	}

	m_comprAnimationChoice->Thaw();
}

void CAnimBrowserCompressionPage::DestroyEntities()
{
	if ( m_uncomprElem.m_entity )
	{
		m_uncomprElem.m_entity->Destroy();
	}
	if ( m_comprElem.m_entity )
	{
		m_comprElem.m_entity->Destroy();
	}

	m_uncomprElem.Clear();
	m_comprElem.Clear();
}

void CAnimBrowserCompressionPage::DestroyUncompressedAnimations()
{
	if ( m_uncompressedAnimation )
	{
		delete m_uncompressedAnimation;
		m_uncompressedAnimation = NULL;
	}
}

Bool CAnimBrowserCompressionPage::LoadUncompressedAnimation()
{
	ASSERT( m_compression );

	DestroyUncompressedAnimations();

	CSkeletalAnimationSetEntry* anim = m_browser->m_selectedAnimation;

	if ( anim && anim->GetAnimation() && anim->GetAnimation()->GetImportFile() != String::EMPTY )
	{
		CFilePath filePath( anim->GetAnimation()->GetImportFile() );

 		ISkeletalAnimationImporter* animImporter = ISkeletalAnimationImporter::FindImporter( filePath.GetExtension() );
 		if ( !animImporter )
 		{
 			return false;
		}

		AnimImporterParams params;
		params.m_animationSet = NULL;
		params.m_filePath = anim->GetAnimation()->GetImportFile();
		params.m_dontCompress = true;

		CSkeletalAnimation* newAnimation = animImporter->DoImport( params );
		if ( newAnimation )
		{
			m_uncompressedAnimation = new CSkeletalAnimationSetEntry;
			m_uncompressedAnimation->SetAnimation( newAnimation );
		}
		else
		{
			wxMessageBox( wxT("Couldn't load uncompressed animation"), wxT("Error") );
		}
	}

	OnRefreshAnimation();

	return false;
}

void CAnimBrowserCompressionPage::OnAnimChanged( wxCommandEvent& event )
{
	Int32 selection = m_comprAnimationChoice->GetSelection();

	if ( selection != wxNOT_FOUND )
	{
		CUndoAnimBrowserAnimChange::CreateStep( *m_undoManager, m_browser );
		m_browser->SelectAnimation( (CSkeletalAnimationSetEntry*)m_comprAnimationChoice->GetClientData( selection ) );
	}
}

void CAnimBrowserCompressionPage::OnLoadAnimation( wxCommandEvent& event )
{
	LoadUncompressedAnimation();
}

void CAnimBrowserCompressionPage::OnAnimUp( wxCommandEvent& event )
{
	Int32 selection = m_comprAnimationChoice->GetSelection();

	if ( selection != wxNOT_FOUND && selection - 1 >= 0 )
	{
		Int32 newAnimNum = selection - 1;

		m_comprAnimationChoice->SetSelection( newAnimNum );

		CSkeletalAnimationSetEntry* anim = (CSkeletalAnimationSetEntry*)m_comprAnimationChoice->GetClientData( newAnimNum );

		if ( anim )
		{
			CUndoAnimBrowserAnimChange::CreateStep( *m_undoManager, m_browser );
			m_browser->SelectAnimation( anim );
		}
		else
		{
			ASSERT( anim );
		}
	}
}

void CAnimBrowserCompressionPage::OnAnimDown( wxCommandEvent& event )
{
	Int32 selection = m_comprAnimationChoice->GetSelection();

	if ( selection != wxNOT_FOUND && selection + 1 < (Int32)m_comprAnimationChoice->GetCount() )
	{
		Int32 newAnimNum = selection + 1;

		m_comprAnimationChoice->SetSelection( newAnimNum );

		CSkeletalAnimationSetEntry* anim = (CSkeletalAnimationSetEntry*)m_comprAnimationChoice->GetClientData( newAnimNum );

		if ( anim )
		{
			CUndoAnimBrowserAnimChange::CreateStep( *m_undoManager, m_browser );
			m_browser->SelectAnimation( anim );
		}
		else
		{
			ASSERT( anim );
		}
	}
}

void CAnimBrowserCompressionPage::OnPosOffsetChanged( wxScrollEvent& event )
{
	wxSlider* slider = XRCCTRL( *this, "sliderOffsetPos", wxSlider );
	Float offset = (Float)slider->GetValue()/100.f;

	if ( m_comprElem.m_entity )
	{
		m_comprElem.m_entity->SetPosition( Vector( offset, 0.f, 0.f ) );
	}
	if ( m_uncomprElem.m_entity )
	{
		m_uncomprElem.m_entity->SetPosition( Vector( -offset, 0.f, 0.f ) );
	}
}

void CAnimBrowserCompressionPage::OnCompressSliderScrolled( wxScrollEvent& event )
{
	ASSERT( m_compression != NULL, TXT( "No Animation compression defined" ) );

	Float value = (Float)m_compressionSlider->GetValue() / (Float)m_compressionSlider->GetMax();

	m_compression->SetCompressionFactor( value );
}

void CAnimBrowserCompressionPage::OnShowMeshes( wxCommandEvent& event )
{
	Bool hide = !event.IsChecked();

	if ( m_comprElem.m_entity )
	{
		m_comprElem.m_entity->SetHideInGame( hide );
	}

	if ( m_uncomprElem.m_entity )
	{
		m_uncomprElem.m_entity->SetHideInGame( hide );
	}
}

void CAnimBrowserCompressionPage::OnApply( wxCommandEvent& event )
{
	if ( m_comprElem.m_playedAnimation )
	{
		ASSERT( m_compressedAnimation && m_compressedAnimation->GetAnimation() );
		ASSERT( m_uncompressedAnimation && m_uncompressedAnimation->GetAnimation() );
		ASSERT( m_compression );

#ifdef USE_HAVOK_ANIMATION
		m_compressedAnimation->GetAnimation()->Compress( m_uncompressedAnimation->GetAnimation(), m_compression );
#endif
	}
}

void CAnimBrowserCompressionPage::OnFpsChanged( wxCommandEvent& event )
{
	if ( event.GetId() == XRCID( "radioFps10" ) )
	{
		ChangedFpsTo( AF_10 );	
	}
	else if ( event.GetId() == XRCID( "radioFps15" ) )
	{
		ChangedFpsTo( AF_15 );	
	}
	else if ( event.GetId() == XRCID( "radioFps30" ) )
	{
		ChangedFpsTo( AF_30 );	
	}
}

void CAnimBrowserCompressionPage::ChangedFpsTo( EAnimationFps fps )
{
	if ( m_comprElem.m_playedAnimation )
	{
		ASSERT( m_compressedAnimation && m_compressedAnimation->GetAnimation() );
		ASSERT( m_uncompressedAnimation && m_uncompressedAnimation->GetAnimation() );
		ASSERT( m_compression );

#ifdef USE_HAVOK_ANIMATION
		// 1. FPS
		CAnimationFpsCompression* fpsCompression = CreateObject< CAnimationFpsCompression >();
		fpsCompression->SetFps( fps );
		
		CSkeletalAnimation* newAnimation = SafeCast< CSkeletalAnimation >( m_uncompressedAnimation->GetAnimation()->Clone( NULL ) );
		newAnimation->Compress( m_uncompressedAnimation->GetAnimation(), fpsCompression );

		// 2. Prev compression
		m_compressedAnimation->GetAnimation()->Compress( newAnimation, m_compression );
#endif
	}
}

Bool CAnimBrowserCompressionPage::CanLoadUncompressedAnimation() const
{
	CSkeletalAnimationSetEntry* anim = m_browser->m_selectedAnimation;

	if ( anim && anim->GetAnimation() && anim->GetAnimation()->GetImportFile() != String::EMPTY )
	{
		return GFileManager->GetFileSize( anim->GetAnimation()->GetImportFile() ) != 0;
	}

	return false;
}

void CAnimBrowserCompressionPage::RefreshLoadButton()
{
	wxButton* loadButt = XRCCTRL( *this, "loadButt", wxButton );
	loadButt->Enable( CanLoadUncompressedAnimation() );
}

Bool CAnimBrowserCompressionPage::SetEntityForElem( TDynArray< Uint8 >& buffer, const String& originalComponentName,
												    CAnimBrowserCompressionPage::PreviewElem& elem, 
													const Vector& offset )
{
	LayerEntitiesArray pastedEntities;

	// Paste
	m_browser->m_previewPanel->GetPreviewWorld()->GetDynamicLayer()->PasteSerializedEntities( buffer, pastedEntities, true, offset, EulerAngles( 0.0f, 0.0f, 0.0f ) );

	ASSERT( pastedEntities.Size() == 1 );
	if ( pastedEntities.Size() == 1 )
	{
		elem.m_entity = pastedEntities[0];

		// Find the cloned component
		elem.m_component = elem.m_entity->FindComponent< CAnimatedComponent >( originalComponentName );

		if ( elem.m_component )
		{
			elem.m_component->SetUseExtractedMotion( false );
		}

		CActor* actor = Cast< CActor >( elem.m_entity );
		if( actor != NULL )
		{
			if ( actor->GetInventoryComponent() )
			{
				actor->InitInventory();
				actor->GetInventoryComponent()->SpawnMountedItems();
			}
		}

		return elem.m_component != NULL;
	}
	else
	{
		elem.Clear();
		return false;
	}
}

Float CAnimBrowserCompressionPage::GetPreviewEntitiesOffset() const
{
	wxSlider* slider = XRCCTRL( *this, "sliderOffsetPos", wxSlider );
	return (Float)slider->GetValue()/100.f;
}

Bool CAnimBrowserCompressionPage::CloneEntitiesFromComponent( const CAnimatedComponent* originalComponent )
{
	TDynArray< Uint8 > buffer;
	CMemoryFileWriter writer( buffer );
	CDependencySaver saver( writer, NULL );

	CEntity *prototype = originalComponent->GetEntity();

	if( prototype && prototype->GetTemplate() )
	{
		CEntityTemplate* etemplate = Cast<CEntityTemplate>( prototype->GetTemplate() );
		SEvents::GetInstance().DispatchEvent( CNAME( FileEdited ), CreateEventData< String >( etemplate->GetFile()->GetDepotPath() ) );
	}

	// Save objects
	DependencySavingContext context( prototype );
	if ( !saver.SaveObjects( context ) )
	{
		WARN_EDITOR( TXT("Couldnt clone given entity!") );
		return false;
	}

	Float offset = GetPreviewEntitiesOffset();

	if ( SetEntityForElem( buffer, originalComponent->GetName(), m_uncomprElem, Vector( -offset, 0.f, 0.f ) ) == false )
	{
		return false;
	}

	if ( SetEntityForElem( buffer, originalComponent->GetName(), m_comprElem, Vector( offset, 0.f, 0.f ) ) == false )
	{
		return false;
	}

	CWorldTickInfo info( m_browser->m_previewPanel->GetPreviewWorld(), 0.1f );
	m_browser->m_previewPanel->GetPreviewWorld()->Tick( info );

	return true;
}