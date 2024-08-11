/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "animBrowserTrajectory.h"
#include "../../common/engine/animatedSkeleton.h"
#include "../../common/core/mathUtils.h"
#include "../../common/engine/renderFrame.h"

BEGIN_EVENT_TABLE( CAnimBrowserTrajectoryPage, wxPanel )
	EVT_BUTTON( XRCID( "loadButt" ), CAnimBrowserTrajectoryPage::OnLoadUncompressedAnimation )
	EVT_BUTTON( XRCID( "createTrajButt" ), CAnimBrowserTrajectoryPage::OnCreateRawTraj )
	EVT_BUTTON( XRCID( "createSplineButt" ), CAnimBrowserTrajectoryPage::OnCreateSplineTraj )
	EVT_TOGGLEBUTTON( XRCID( "editSplineButt" ), CAnimBrowserTrajectoryPage::OnEditSplineTraj )
	EVT_TOGGLEBUTTON( XRCID( "extractTrajButt" ), CAnimBrowserTrajectoryPage::OnRefreshSettings )
	EVT_TOGGLEBUTTON( XRCID( "showTrajButt" ), CAnimBrowserTrajectoryPage::OnRefreshSettings )
	EVT_TOGGLEBUTTON( XRCID( "settAxisButt" ), CAnimBrowserTrajectoryPage::OnRefreshSettings )
	EVT_TOGGLEBUTTON( XRCID( "settLineButt" ), CAnimBrowserTrajectoryPage::OnRefreshSettings )
	EVT_TOGGLEBUTTON( XRCID( "settVelButt" ), CAnimBrowserTrajectoryPage::OnRefreshSettings )
	EVT_TOGGLEBUTTON( XRCID( "settCurrButt" ), CAnimBrowserTrajectoryPage::OnRefreshSettings )
	EVT_BUTTON( XRCID( "splineRemove" ), CAnimBrowserTrajectoryPage::OnSplinePointRemoved )
	EVT_BUTTON( XRCID( "splineAddAfter" ), CAnimBrowserTrajectoryPage::OnSplinePointAddedAfter )
	EVT_BUTTON( XRCID( "splineAddBefore" ), CAnimBrowserTrajectoryPage::OnSplinePointAddedBefore )
	EVT_TOGGLEBUTTON( XRCID( "IkHandsButt" ), CAnimBrowserTrajectoryPage::OnIkHandsPressed )
END_EVENT_TABLE()

CAnimBrowserTrajectoryPage::CAnimBrowserTrajectoryPage( wxWindow* parent, CEdAnimBrowser* browser )
	: wxPanel( parent )
	, m_browser( browser )
	, m_uncompressedAnimation( NULL )
	, m_active( false )
	, m_extractTrajectory( true )
	, m_boneIdx( -1 )
	, m_showRawTrajectory( true )
	, m_showRawRot( false )
	, m_showRawLine( true )
	, m_showRawCurrent( true )
	, m_showSplineTrajectory( true )
	, m_ikHandsId( 0xFFFFFFFF )
	, m_refershIconsRequest()
{
	// Load designed frame from resource
	wxPanel* innerPanel = wxXmlResource::Get()->LoadPanel( this, wxT("AnimBrowserTrajPanel") );

	m_notebook = XRCCTRL( *this, "notebook", wxNotebook );

	XRCCTRL( *this, "editBone", wxTextCtrl )->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( CAnimBrowserTrajectoryPage::OnBoneSelected ), NULL, this );

	wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );
	sizer->Add( innerPanel, 1, wxEXPAND|wxALL, 0 );

	SetSizer( sizer );
	Layout();

	RefreshAll();
}

CAnimBrowserTrajectoryPage::~CAnimBrowserTrajectoryPage()
{
	EditSplineTrajectory( false );
}

void CAnimBrowserTrajectoryPage::DestroyPanel()
{
	DestroyUncompressedAnimations();
}

void CAnimBrowserTrajectoryPage::EnablePanel( Bool flag )
{
	if ( flag == m_active )
	{
		return;
	}

	if ( flag )
	{
		wxCommandEvent fake;
		OnBoneSelected( fake );

		RefreshAll();
	}
	else
	{
		EditSplineTrajectory( false );

		EnableIkHands( false );
	}

	m_active = flag;
}


void CAnimBrowserTrajectoryPage::OnRefreshAnimation()
{
	m_refershIconsRequest.Increment();
}

void CAnimBrowserTrajectoryPage::OnSelectedAnimation()
{
	m_refershIconsRequest.Increment();
}

void CAnimBrowserTrajectoryPage::OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame )
{
	CAnimatedComponent* ac = m_browser->GetAnimatedComponent();
	if ( ac )
	{
		if ( m_itemContainer )
		{
			m_itemContainer->SetPosition( ac->GetWorldPosition() );
		}

		if ( m_rawData.IsValid() )
		{
			DrawBoneTrajectory( frame, m_rawData );
		}

		if ( m_splineData.IsValid() )
		{
			DrawBoneTrajectory( frame, m_splineData );
		}

		if ( m_showRawCurrent && m_boneIdx != -1 )
		{
			Matrix mat = ac->GetBoneMatrixWorldSpace( m_boneIdx );
			frame->AddDebugAxis( mat.GetTranslation(), mat, 0.1f, true );
		}
	}
}

void CAnimBrowserTrajectoryPage::Tick( Float timeDelta )
{
	if( m_refershIconsRequest.GetValue() > 0 )
	{
		RefreshAll();
		m_refershIconsRequest.Decrement();
	}
}

void CAnimBrowserTrajectoryPage::OnSelectItem( IPreviewItem* item )
{
	m_browser->m_previewPanel->EnableWidgets( true );
}

void CAnimBrowserTrajectoryPage::HandleSelection( const TDynArray< CHitProxyObject* >& objects )
{
	HandleItemSelection( objects );
}

void CAnimBrowserTrajectoryPage::DestroyUncompressedAnimations()
{
	if ( m_uncompressedAnimation )
	{
		delete m_uncompressedAnimation;
		m_uncompressedAnimation = NULL;
	}
}

Bool CAnimBrowserTrajectoryPage::LoadUncompressedAnimation()
{
	DestroyUncompressedAnimations();

	CSkeletalAnimationSetEntry* anim = m_browser->GetSelectedAnimationEntry();

	if ( anim && anim->GetAnimation() && anim->GetAnimation()->GetImportFile() != String::EMPTY )
	{
		CFilePath filePath( anim->GetAnimation()->GetImportFile() );

		ISkeletalAnimationImporter* animImporter = ISkeletalAnimationImporter::FindImporter( filePath.GetExtension() );
		if ( !animImporter )
		{
			return false;
		}

		AnimImporterParams params;
		params.m_dontCompress = true;
		params.m_filePath = anim->GetAnimation()->GetImportFile();

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

	RefreshAll();

	return false;
}

Bool CAnimBrowserTrajectoryPage::CanLoadUncompressedAnimation() const
{
	CSkeletalAnimationSetEntry* anim = m_browser->GetSelectedAnimationEntry();

	if ( anim && anim->GetAnimation() && anim->GetAnimation()->GetImportFile() != String::EMPTY )
	{
		return GFileManager->GetFileSize( anim->GetAnimation()->GetImportFile() ) != 0;
	}

	return false;
}

void CAnimBrowserTrajectoryPage::RefreshAll()
{
	RefreshSettings();
	RefreshAnimNameLabel();
	RefreshLoadButton();
	RefreshTrajButton();
	RefreshSplineButtons();
}

void CAnimBrowserTrajectoryPage::RefreshSettings()
{
	m_extractTrajectory = XRCCTRL( *this, "extractTrajButt", wxToggleButton )->GetValue();

	m_showRawTrajectory = XRCCTRL( *this, "showTrajButt", wxToggleButton )->GetValue();
	m_showRawRot = XRCCTRL( *this, "settAxisButt", wxToggleButton )->GetValue();
	m_showRawLine = XRCCTRL( *this, "settLineButt", wxToggleButton )->GetValue();
	m_showRawVel = XRCCTRL( *this, "settVelButt", wxToggleButton )->GetValue();
	m_showRawCurrent = XRCCTRL( *this, "settCurrButt", wxToggleButton )->GetValue();
}

void CAnimBrowserTrajectoryPage::RefreshLoadButton()
{
	Bool ret = CanLoadUncompressedAnimation();

	wxButton* loadButt = XRCCTRL( *this, "loadButt", wxButton );
	loadButt->Enable( ret );

	wxStaticText* text = XRCCTRL( *this, "loadButtInfo", wxStaticText );
	if ( ret )
	{
		if ( m_uncompressedAnimation )
		{
			text->SetLabelText( wxT("2") );
		}
		else
		{
			text->SetLabelText( wxT("3") );
		}
	}
	else
	{
		text->SetLabelText( wxT("1") );
	}
}

void CAnimBrowserTrajectoryPage::RefreshAnimNameLabel()
{
	wxStaticText* text = XRCCTRL( *this, "animNameText", wxStaticText );
	
	CSkeletalAnimationSetEntry* anim = m_browser->GetSelectedAnimationEntry();

	if ( anim )
	{
		text->SetLabelText( anim->GetName().AsString().AsChar() );
	}
	else
	{
		text->SetLabelText( wxT("<none>") );
	}
}

void CAnimBrowserTrajectoryPage::RefreshTrajButton()
{
	Bool state = m_uncompressedAnimation != NULL && m_boneIdx != -1;

	wxButton* button = XRCCTRL( *this, "createTrajButt", wxButton );
	button->Enable( state );

	wxToggleButton* toggle = XRCCTRL( *this, "showTrajButt", wxToggleButton );
	toggle->Enable( state );
}

void CAnimBrowserTrajectoryPage::RefreshSplineButtons()
{
	Bool state = m_rawData.IsValid();

	wxButton* button = XRCCTRL( *this, "createSplineButt", wxButton );
	button->Enable( state );

	wxToggleButton* toggle = XRCCTRL( *this, "editSplineButt", wxToggleButton );
	toggle->Enable( state );
}

void CAnimBrowserTrajectoryPage::CreateRawTrajectory()
{
#ifdef USE_HAVOK_ANIMATION
	// Get hkaInterleavedUncompressedAnimation - this will be long way...
	if ( !m_uncompressedAnimation )
	{
		ASSERT( m_uncompressedAnimation );
		return;
	}

	CAnimatedComponent* ac = m_browser->GetAnimatedComponent();
	if ( !ac )
	{
		return;
	}

	CSkeleton* skeleton = ac->GetSkeleton();
	if ( !skeleton )
	{
		return;
	}

	CSkeletalAnimation* skAnimation = m_uncompressedAnimation->GetAnimation();
	if ( !skAnimation )
	{
		return;
	}

	const AnimBuffer* animBuff = skAnimation->GetAnimBuffer();
	if ( !animBuff )
	{
		ASSERT( animBuff );
		return;
	}

	if ( animBuff->m_animNum != 1 )
	{
		return;
	}

	const hkaAnimation* hkAnim = animBuff->GetHavokAnimation( 0 );
	if ( !hkAnim )
	{
		ASSERT( hkAnim );
		return;
	}

	if ( hkAnim->getType() != hkaAnimation::HK_INTERLEAVED_ANIMATION )
	{
		ASSERT( hkAnim->getType() != hkaAnimation::HK_INTERLEAVED_ANIMATION );
		return;
	}

	// Uff
	const hkaInterleavedUncompressedAnimation* rawAnimation = static_cast< const hkaInterleavedUncompressedAnimation* >( hkAnim );

	const Uint32 numBones = skeleton->GetBonesNum();
	const Uint32 numTracks = skeleton->GetTracksNum();

	if ( numBones != rawAnimation->m_numberOfTransformTracks )
	{
		ASSERT( numBones != rawAnimation->m_numberOfTransformTracks );
		return;
	}

	Float minVel = NumericLimits< Float >::Max();
	Float maxVel = NumericLimits< Float >::Min();

	// Write frames to output

	const Int32 numberOfPoses = rawAnimation->getNumOriginalFrames();
	const Int32 numberOfTransformTracks = rawAnimation->m_numberOfTransformTracks;

	hkReal invTimeStep = 1.f / ( rawAnimation->m_duration / ( numberOfPoses - 1 ) );

	m_rawData.Set( numberOfPoses );

	hkQsTransform prevTran( hkQsTransform::IDENTITY );

	for ( Int32 p=0; p<numberOfPoses; ++p )
	{
		hkQsTransform& buffer = rawAnimation->m_transforms[ p * numberOfTransformTracks ];

		SBehaviorGraphOutput pose;
		pose.Init( numberOfTransformTracks, 0, &buffer, NULL, false );
		
		pose.m_deltaReferenceFrameLocal.setIdentity();
		
		if ( m_extractTrajectory )
		{
			pose.ExtractTrajectory( ac );
		}

		ASSERT( m_boneIdx != -1 );

		hkQsTransform boneTran = pose.GetBoneModelTransform( ac, m_boneIdx );

		m_rawData.m_trans[ p ] = boneTran;

		Matrix boneMat;
		HavokTransformToMatrix_Renormalize( boneTran, &boneMat );

		m_rawData.m_mats[ p ] = boneMat;

		hkVector4 vel; vel.setSub4( boneTran.m_translation, prevTran.m_translation );
		vel.mul4( invTimeStep );

		m_rawData.m_vels[ p ] = vel;

		hkReal velMag = vel.length3();
		if ( velMag > maxVel )
		{
			maxVel = velMag;
		}
		if ( velMag < minVel )
		{
			minVel = velMag;
		}

		prevTran = boneTran;
	}

	m_rawData.m_minVel = minVel;
	m_rawData.m_maxVel = maxVel;

	RefreshSplineButtons();
#else
	HALT( "DEX TODO - Needs to be implemented" );
#endif
}

void CAnimBrowserTrajectoryPage::CreateSplineTrajectory()
{
	if ( !m_itemContainer )
	{
		EditSplineTrajectory( true );
	}

	m_splineData.Set( m_rawData.m_trans.Size() );

	CreateItems( m_rawData.m_trans.Size() );

	for ( Uint32 i=0; i<m_splineData.m_pos.Size(); ++i )
	{
#ifdef USE_HAVOK_ANIMATION
		m_splineData.m_pos[ i ] = TO_CONST_VECTOR_REF( m_rawData.m_trans[ i ].m_translation );
#else
		m_splineData.m_pos[ i ] = reinterpret_cast< const Vector& >( m_rawData.m_trans[ i ].Translation );
#endif
	}

	RefreshItems();
}

void CAnimBrowserTrajectoryPage::SaveSplineTrajectoryToSelAnim()
{

}

void CAnimBrowserTrajectoryPage::EditSplineTrajectory( Bool state )
{
	if ( state )
	{
		if ( !m_itemContainer )
		{
			InitItemContainer();
		}

		//...
	}
	else
	{
		if ( m_itemContainer )
		{
			DestroyItemContainer();
		}

		//...
	}
}

void CAnimBrowserTrajectoryPage::CreateItems( Uint32 size )
{
	ClearItems();

	for ( Uint32 i=0; i<size; ++i )
	{
		CSplineKnotItem* item = new CSplineKnotItem( this, m_splineData );
		item->Init( ToString( i ) );
		item->SetSize( IPreviewItem::PS_Tiny );

		AddItem( item );
	}
}

void CAnimBrowserTrajectoryPage::DrawBoneTrajectory( CRenderFrame *frame, const SRawBoneTrajectoryData& data )
{
	if ( m_showRawTrajectory && m_rawData.IsValid() )
	{
		const CAnimatedComponent* ac = m_browser->GetAnimatedComponent();
		const Matrix& localToWorld = ac->GetLocalToWorld();

		Color color( 100, 100, 100 );
		Color color2( 200, 200, 200 );

		const Uint32 size = m_rawData.m_trans.Size();

		Vector prev;

		for ( Uint32 i=0; i<size; ++i )
		{
			Matrix tempMat = m_rawData.m_mats[ i ];
			tempMat = tempMat * localToWorld;

			if ( m_showRawRot )
			{
				frame->AddDebugAxis( tempMat.GetTranslation(), tempMat, 0.1f, color, true );
			}

			if ( i > 0 && m_showRawVel )
			{
				Vector vel = reinterpret_cast< const Vector& >( m_rawData.m_vels[ i ] );
				Float val = vel.Mag3();

				Float p = ( val - m_rawData.m_minVel ) / ( m_rawData.m_maxVel - m_rawData.m_minVel );

				color2.R = (Uint8)Lerp< Float >( p*p, 80.f, 255.f );
				color2.G = 80;
				color2.B = 80;
			}

			if ( i > 0 && m_showRawLine )
			{
				frame->AddDebugLine( prev, tempMat.GetTranslation(), color2, true );
			}

			prev = tempMat.GetTranslation();
		}
	}
}

void CAnimBrowserTrajectoryPage::DrawBoneTrajectory( CRenderFrame *frame, const SSplineBoneTrajectoryData& data )
{
	if ( m_showSplineTrajectory && m_splineData.IsValid() )
	{
		const CAnimatedComponent* ac = m_browser->GetAnimatedComponent();
		const Matrix& localToWorld = ac->GetLocalToWorld();

		Color color2( 0, 255, 0 );

		const Uint32 size = m_splineData.m_pos.Size();

		TDynArray< Vector > out;
		MathUtils::InterpolationUtils::InterpolatePoints( m_splineData.m_pos, out, 0.05f, false );

		Vector prev;

		for ( Uint32 i=0; i<out.Size(); ++i )
		{
			Vector pos = localToWorld.TransformPoint( out[ i ] );

			if ( i > 0 )
			{
				frame->AddDebugLine( prev, pos, color2, true );
			}

			prev = pos;
		}
	}
}

//////////////////////////////////////////////////////////////////////////

class AnimatedSkeletonIkArms : public IAnimatedSkeletonConstraint
{
public:
	virtual void Update( const CAnimatedComponent* ac, Float dt ) {}
	virtual void PreSample( const CAnimatedComponent* ac, SBehaviorSampleContext* context, SBehaviorGraphOutput& pose ) {}
	virtual void PostSample( const CAnimatedComponent* ac, SBehaviorSampleContext* context, SBehaviorGraphOutput& pose )
	{
		
	}
};

void CAnimBrowserTrajectoryPage::EnableIkHands( Bool status )
{
	CAnimatedComponent* ac = m_browser->GetAnimatedComponent();

	if ( status )
	{
		if ( m_ikHandsId != 0xFFFFFFFF )
		{
			ac->GetAnimatedSkeleton()->RemoveConstraint( m_ikHandsId );
		}

		m_ikHandsId = ac->GetAnimatedSkeleton()->AddConstraint( new AnimatedSkeletonIkArms() );
	}
	else
	{
		if ( m_ikHandsId != 0xFFFFFFFF )
		{
			ac->GetAnimatedSkeleton()->RemoveConstraint( m_ikHandsId );
		}

		m_ikHandsId = 0xFFFFFFFF;
	}
}

//////////////////////////////////////////////////////////////////////////

void CAnimBrowserTrajectoryPage::OnLoadUncompressedAnimation( wxCommandEvent& event )
{
	LoadUncompressedAnimation();
}

void CAnimBrowserTrajectoryPage::OnCreateRawTraj( wxCommandEvent& event )
{
	CreateRawTrajectory();
}

void CAnimBrowserTrajectoryPage::OnCreateSplineTraj( wxCommandEvent& event )
{
	CreateSplineTrajectory();
}

void CAnimBrowserTrajectoryPage::OnEditSplineTraj( wxCommandEvent& event )
{
	EditSplineTrajectory( event.IsChecked() );
}

void CAnimBrowserTrajectoryPage::OnRefreshSettings( wxCommandEvent& event )
{
	RefreshSettings();
}

void CAnimBrowserTrajectoryPage::OnBoneSelected( wxCommandEvent& event )
{
	m_boneIdx = -1;

	wxTextCtrl* edit = XRCCTRL( *this, "editBone", wxTextCtrl );
	wxStaticText* text = XRCCTRL( *this, "editBoneStatus", wxStaticText );

	CAnimatedComponent* ac = m_browser->GetAnimatedComponent();
	if ( ac )
	{
		String str = edit->GetValue().c_str();
		m_boneIdx = ac->FindBoneByName( str.AsChar() );
	}

	if ( m_boneIdx == -1 )
	{
		text->SetLabelText( wxT("") );
	}
	else
	{
		text->SetLabelText( wxT("Ok") );
	}
}

void CAnimBrowserTrajectoryPage::OnSplinePointRemoved( wxCommandEvent& event )
{
	TDynArray< CNode* > nodes;
	m_browser->m_previewPanel->GetSelectionManager()->GetSelectedNodes( nodes );

	if ( nodes.Size() == 1 )
	{
		CPreviewHelperComponent* component = SafeCast< CPreviewHelperComponent >( nodes[ 0 ] );
		if ( component )
		{
			String str = component->GetItem()->GetName();

			Int32 index = -1;
			FromString( str, index );

			ASSERT( index != -1 );
			ASSERT( m_splineData.m_pos.SizeInt() > index );

			if ( index != -1 && m_splineData.m_pos.SizeInt() > index )
			{
				m_splineData.m_pos.RemoveAt( index );

				RefreshItems();
			}
		}
	}
}

void CAnimBrowserTrajectoryPage::OnSplinePointAddedAfter( wxCommandEvent& event )
{

}

void CAnimBrowserTrajectoryPage::OnSplinePointAddedBefore( wxCommandEvent& event )
{

}

void CAnimBrowserTrajectoryPage::OnIkHandsPressed( wxCommandEvent& event )
{
	EnableIkHands( event.IsChecked() );
}

//////////////////////////////////////////////////////////////////////////

CSplineKnotItem::CSplineKnotItem( CAnimBrowserTrajectoryPage* page, SSplineBoneTrajectoryData& data )
	: m_data( data )
	, m_page( page )
{
	
}

Bool CSplineKnotItem::IsValid() const
{
	Int32 varId = GetIdx();
	return varId != -1 && m_data.m_pos.SizeInt() > varId;
}

void CSplineKnotItem::Refresh()
{
	Int32 varId = GetIdx();
	if ( varId != -1 && m_data.m_pos.SizeInt() > varId )
	{
		Vector newPos = m_data.m_pos[ varId ];
		SetPosition( newPos );
	}
}

void CSplineKnotItem::SetPositionFromPreview( const Vector& prevPos, Vector& newPos ) 
{
	Int32 varId = GetIdx();
	if ( varId != -1 && m_data.m_pos.SizeInt() > varId )
	{
		m_data.m_pos[ varId ] = newPos;
	}
}

IPreviewItemContainer* CSplineKnotItem::GetItemContainer() const
{
	return m_page;
}

Int32 CSplineKnotItem::GetIdx() const
{
	Int32 num = -1;
	FromString( GetName(), num );
	return num;
}

//////////////////////////////////////////////////////////////////////////
