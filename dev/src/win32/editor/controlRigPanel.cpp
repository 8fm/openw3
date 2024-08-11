 /**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "controlRigPanel.h"
#include "controlRigBoneTransformPanel.h"

#include "../../common/engine/controlRig.h"
#include "../../common/engine/controlRigDefinition.h"
#include "../../common/engine/skeletonUtils.h"
#include "../../common/game/storySceneEventControlRig.h"
#include "../../common/game/storySceneEventPoseKey.h"

#include "dialogEditorHelperEntitiesCtrl.h"
#include "dialogPreview.h"
#include "dialogTimeline.h"
#include "storyScenePreviewPlayer.h"
#include "../../common/engine/skeleton.h"
#include "controlRigIKPanel.h"
#include "controlRigPresetsPanel.h"

#ifdef DEBUG_SCENES_2
#pragma optimize("",off)
#endif

BEGIN_EVENT_TABLE( CEdControlRigPanel, wxPanel )
END_EVENT_TABLE()

CEdControlRigPanel::CEdControlRigPanel( wxPanel* parent, CEdSceneEditor* editor )
	: m_mediator( editor )
	, m_currentSectionTime( 0.f )
	, m_anyHelperSpawned( false )
	, m_anyHelperDirty( false )
	, m_showSkeletonMode_FK( false )
	, m_showSkeletonModePrev_FK( false )
	, m_showSkeletonMode_IK( false )
	, m_showSkeletonModePrev_IK( false )
	, m_event( nullptr )
	, m_forceCache( false )
{
	m_fkHelpers.Resize( 256 );
	m_ikHelpers.Resize( 64 );

	wxXmlResource::Get()->LoadPanel( this, parent, wxT("DialogBodyControlRig") );

	m_editing = XRCCTRL( *this, "btnControl", wxToggleButton );
	m_editing->Connect( wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler( CEdControlRigPanel::OnEditButtonClicked ), nullptr, this );

	//FK
	{
		wxPanel* panel = XRCCTRL( *this, "panelFK", wxPanel );
		wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );

		m_boneFKTranfromPanel = new CEdControlRigBoneTransformPanel( panel, this );

		sizer->Add( m_boneFKTranfromPanel, 1, wxEXPAND | wxALL, 5 );
		panel->SetSizer( sizer );
		panel->Layout();
	}

	// Hands
	{
		wxPanel* panel = XRCCTRL( *this, "panelHand", wxPanel );
		wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );

		m_handsTransformPanel = new CEdControlRigHandsTransformPanel( panel, this );

		sizer->Add( m_handsTransformPanel, 1, wxEXPAND | wxALL, 5 );
		panel->SetSizer( sizer );
		panel->Layout();
	}
	// IK
	{
		wxPanel* panel = XRCCTRL( *this, "panelIK", wxPanel );
		wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );

		m_boneIkTransformPanel = new CEdControlRigIKPanel( panel, this );

		sizer->Add( m_boneIkTransformPanel, 1, wxEXPAND | wxALL, 5 );
		panel->SetSizer( sizer );
		panel->Layout();
	}
	// Presets
	{
		wxPanel* panel = XRCCTRL( *this, "panelCRPresets", wxPanel );
		wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );

		m_presetsPanel = new CEdControlRigPresetPanel( panel, this );

		sizer->Add( m_presetsPanel, 1, wxEXPAND | wxALL, 5 );
		panel->SetSizer( sizer );
		panel->Layout();
	}

	CreateHandsMapping();

	RefreshEditButton();
	Layout();
}

void CEdControlRigPanel::ShowSkeletonFK()
{
	m_showSkeletonMode_FK = true;
}

void CEdControlRigPanel::HideSkeletonFK()
{
	m_showSkeletonMode_FK = false;
}

void CEdControlRigPanel::ShowSkeletonIK()
{
	m_showSkeletonMode_IK = true;
}

void CEdControlRigPanel::HideSkeletonIK()
{
	m_showSkeletonMode_IK = false;
}

void CEdControlRigPanel::OnActorSelectionChange( CActor* newSelection )
{
	if ( newSelection )
	{
		if ( newSelection != m_selectedActorH )
		{
			m_selectedActorH = newSelection;

			CacheHandsMapping();
			InitHelpers();
			LoadDataFromEvent();
			RefreshEditButton();
			ToggleEditModeIfNeeded();
			UpdateSpawn();
			UpdateTransforms();
			UpdateSlidersFK();
		}
	}
	else
	{
		m_selectedActorH = nullptr;

		m_editing->SetValue( false );
		RefreshEditButton();
		UpdateSpawn();
	}

	m_boneFKTranfromPanel->OnActorSelectionChange( newSelection );
	m_handsTransformPanel->OnActorSelectionChange( newSelection );
	m_boneIkTransformPanel->OnActorSelectionChange( newSelection );
}

void CEdControlRigPanel::RefreshEditButton()
{
	if ( CActor* selectedActor = m_selectedActorH.Get() )
	{
		const Bool isEditing = IsEditing();
		String txt = String::Printf( TXT("%sdit%s control rig of %s.%s"), 
			isEditing ? TXT("E") : TXT("Click here to e"), 
			isEditing ? TXT("ing") : TXT(""), 
			selectedActor->GetVoiceTag().AsChar(),
			isEditing ? TXT(" Click here to stop.") : TXT("") );
		m_editing->SetLabel( txt.AsChar() );
		m_editing->Enable( true );
	}
	else
	{
		m_editing->SetLabel( wxT("Please select an actor with contol rig in order to edit it.") );
		m_editing->Enable( false );
	}
}

void CEdControlRigPanel::OnEditButtonClicked( wxCommandEvent& event )
{
	event.Skip();
	RefreshEditButton();
	UpdateSpawn();
}

void CEdControlRigPanel::ToggleEditModeIfNeeded()
{
	const Bool isEditing = IsEditing();
	Bool shouldEdit( false );
	if ( m_selectedActorH )
	{
		for ( Uint32 i=0; i<m_fkHelpers.Size(); ++i )
		{
			if ( m_fkHelpers[ i ].m_isEditable )
			{
				shouldEdit = true;
				goto Decided;
			}
		}

		for ( Uint32 i=0; i<m_ikHelpers.Size(); ++i )
		{
			if ( m_ikHelpers[ i ].m_isEditable )
			{
				shouldEdit = true;
				goto Decided;
			}
		}
	}

Decided:
	if ( shouldEdit != isEditing )
	{
		m_editing->SetValue( shouldEdit );
		RefreshEditButton();
	}
}

void CEdControlRigPanel::InitHelpers()
{
	for ( Uint32 i=0; i<m_fkHelpers.Size(); ++i )
	{
		m_fkHelpers[ i ].Deinit();
	}

	for ( Uint32 i=0; i<m_ikHelpers.Size(); ++i )
	{
		m_ikHelpers[ i ].Deinit();
	}

	if ( CActor* selectedActor = m_selectedActorH.Get() )
	{
		if ( const CAnimatedComponent* ac = selectedActor->GetRootAnimatedComponent() )
		{
			if ( const CSkeleton* skeleton = ac->GetSkeleton() )
			{
				if ( const CControlRigSettings* crFkSettings = skeleton->GetControlRigSettings() )
				{
					const TDynArray< CName >& boneNames = crFkSettings->GetFkBoneNamesL1();
					const TDynArray< Int32 >& boneIdx = crFkSettings->GetFkBonesL1();

					SCENE_ASSERT( boneNames.Size() == boneIdx.Size() );

					const Uint32 numBones = boneNames.Size();
					for ( Uint32 i=0; i<numBones; ++i )
					{
						const Int32 parentBone = skeleton->GetParentBoneIndex( boneIdx[ i ] );

						m_fkHelpers[ i ].InitFK( boneNames[ i ].AsString(), boneIdx[ i ], parentBone );
					}
				}
				else
				{
					const Int32 numBones = skeleton->GetBonesNum();
					for ( Int32 i=0; i<numBones; ++i )
					{
						const Int32 parentBone = skeleton->GetParentBoneIndex( i );

						m_fkHelpers[ i ].InitFK( skeleton->GetBoneName( i ), i, parentBone );
					}
				}

				if ( const TCrDefinition* crIkDefinition = skeleton->GetControlRigDefinition() )
				{
					for ( Int32 id = TCrEffector_First; id<TCrEffector_Last; ++id )
					{
						m_ikHelpers[ id ].InitIK( Red::StringSearch( CEnum::ToString< ETCrEffectorId >( ETCrEffectorId( id ) ).AsChar(), TXT("_") ) + 1, id );
					}
				}
			}
		}
	}
}

CEdControlRigPanel::SHelper::SHelper()
	: m_isEditable( false )
	, m_bone( -1 )
	, m_parentBone( 0 )
	, m_selectedByUser( false )
{
}

void CEdControlRigPanel::SHelper::InitFK( const String& name, Int32 boneIdx, Int32 parentBoneIdx )
{
	m_guid = CGUID::Create();
	m_name = name;
	m_isSpawned = false;
	m_isDirty = false;
	m_helperDirtyFlag = false;
	m_justMoved = false;
	m_isEditable = true;
	m_nameAsName = CName( m_name );
	m_bone = boneIdx;
	m_parentBone = parentBoneIdx;
	m_selectedByUser = false;
}

void CEdControlRigPanel::SHelper::InitIK( const Char* name, Int32 idx )
{
	m_guid = CGUID::Create();
	m_name = name;
	m_isSpawned = false;
	m_isDirty = false;
	m_helperDirtyFlag = false;
	m_justMoved = false;
	m_isEditable = true;
	m_nameAsName = CName( m_name );
	m_bone = idx;
	m_selectedByUser = false;
}

void CEdControlRigPanel::SHelper::Deinit()
{
	m_isEditable = false;
	m_selectedByUser = false;
}

void CEdControlRigPanel::SHelper::Spawn( Bool enable, CEdSceneEditor* mediator )
{
	if ( enable )
	{
		const Bool isBone = m_parentBone != 0 && m_parentBone != -1;

		CEdSceneHelperShapeSettings s;
		s.m_nameOnlyWhenSelected = true;
		s.m_isBox = !isBone;
		s.m_isBone = isBone;
		s.m_boneScale = 2.f;
		s.m_selectionColorDiv = 1.25f;
		mediator->OnControlRig_HelperEntityCreate( m_guid, m_currentTransformWS, m_helperDirtyFlag, m_name, &s );	
	}
	else
	{
		mediator->OnControlRig_HelperEntityDestroy( m_guid );
	}

	m_isSpawned = enable;
}

void CEdControlRigPanel::UpdateSpawn( Int32 spawnHelperFK, Int32 spawnHelperIK )
{
	const Bool isEditing = IsEditing();
	m_anyHelperSpawned = false;

	for ( Uint32 i=0; i<m_fkHelpers.Size(); ++i )
	{
		const Bool shouldBeSpawned = isEditing && m_fkHelpers[ i ].m_isEditable && ( m_showSkeletonMode_FK || m_mediator->OnControlRig_HelperEntityIsSelected( m_fkHelpers[ i ].m_guid ) ) || Int32(i) == spawnHelperFK;
		if ( shouldBeSpawned != m_fkHelpers[ i ].m_isSpawned )
		{
			m_fkHelpers[ i ].Spawn( shouldBeSpawned, m_mediator );	
		}

		if ( shouldBeSpawned )
		{
			m_mediator->OnControlRig_HelperEntityVisible( m_fkHelpers[ i ].m_guid );
			m_anyHelperSpawned = true;
		}
	}

	for ( Uint32 i=0; i<m_ikHelpers.Size(); ++i )
	{
		const Bool shouldBeSpawned =( isEditing && m_ikHelpers[ i ].m_isEditable  && ( m_showSkeletonMode_IK || m_mediator->OnControlRig_HelperEntityIsSelected( m_ikHelpers[ i ].m_guid ) ) ) || Int32(i) == spawnHelperIK;

		if ( shouldBeSpawned != m_ikHelpers[ i ].m_isSpawned )
		{
			m_ikHelpers[ i ].Spawn( shouldBeSpawned, m_mediator );	
		}

		if ( shouldBeSpawned )
		{
			m_mediator->OnControlRig_HelperEntityVisible( m_ikHelpers[ i ].m_guid );
			m_anyHelperSpawned = true;
		}
	}
}

void CEdControlRigPanel::OnPreviewSelectionChanged( const TDynArray< CEdSceneHelperEntity* >& entities )
{
	if ( m_boneFKTranfromPanel )
	{
		const Uint32 num = entities.Size();
		for ( Uint32 i=0; i<num; ++i )
		{
			CEdSceneHelperEntity* e = entities[ i ];

			m_boneFKTranfromPanel->SelectBone( e->GetName().AsChar() );
		}
	}

	if ( m_boneIkTransformPanel )
	{
		const Uint32 num = entities.Size();
		for ( Uint32 i=0; i<num; ++i )
		{
			CEdSceneHelperEntity* e = entities[ i ];

			m_boneIkTransformPanel->SelectEffector( e->GetName().AsChar() );
		}
	}
}

void CEdControlRigPanel::OnGenerateFragments( CRenderFrame *frame )
{
	m_boneIkTransformPanel->OnGenerateFragments( frame );
}

void CEdControlRigPanel::OnTick( Float sectionTimeBeforePreviewTick, Float sectionTimeAfterPreviewTick, Float timeDelta )
{
	if ( m_showSkeletonMode_FK != m_showSkeletonModePrev_FK || m_showSkeletonMode_IK != m_showSkeletonModePrev_IK )
	{
		m_showSkeletonModePrev_FK = m_showSkeletonMode_FK;
		m_showSkeletonModePrev_IK = m_showSkeletonMode_IK;

		UpdateSpawn();
	}

	const Bool isEditing = IsEditing();

	UpdateDirty();

	if ( sectionTimeBeforePreviewTick != sectionTimeAfterPreviewTick )
	{
		if ( m_anyHelperDirty )
		{
			m_currentSectionTime = sectionTimeAfterPreviewTick;

			if( !m_selectedEvent )
			{	
				DiscardChanges();
				return;
			}					
		}
	}

	m_selectedEvent = false;

	if ( isEditing && m_selectedActorH.Get() && m_anyHelperSpawned )
	{
		m_mediator->OnControlRig_SetPreviewEditString( 
			String::Printf( TXT("Editing control rig of %s.%s"), 
				m_selectedActorH.Get()->GetVoiceTag().AsChar(), 
				m_anyHelperDirty ? TXT(" Press K to create an event.") : TXT("") )
			, EEditStringId::EDIT_ControlRig );
	}
	else
	{
		m_mediator->OnControlRig_SetPreviewEditString( String::EMPTY, EEditStringId::EDIT_ControlRig );
	}

	if ( !isEditing )
	{
		return;
	}

	UpdateTransforms();
	//UpdateSlidersFK();
}


void CEdControlRigPanel::UpdateDirty()
{
	m_anyHelperDirty = false;
	m_anyHelperJustMoved = false;

	for ( Uint32 i=0; i<m_fkHelpers.Size(); ++i )
	{
		if( m_fkHelpers[ i ].m_helperDirtyFlag )
		{
			m_fkHelpers[ i ].m_justMoved = m_fkHelpers[ i ].m_helperDirtyFlag;
			m_fkHelpers[ i ].m_helperDirtyFlag = false;
			m_fkHelpers[ i ].m_isDirty = true;
			m_anyHelperJustMoved = true;
		}
		if ( m_fkHelpers[ i ].m_isDirty )
		{
			m_anyHelperDirty = true;
		}
	}

	for ( Uint32 i=0; i<m_ikHelpers.Size(); ++i )
	{
		if( m_ikHelpers[ i ].m_helperDirtyFlag )
		{
			m_ikHelpers[ i ].m_justMoved = m_ikHelpers[ i ].m_helperDirtyFlag;
			m_ikHelpers[ i ].m_helperDirtyFlag = false;
			m_ikHelpers[ i ].m_isDirty = true;
			m_anyHelperJustMoved = true;
		}
		if ( m_ikHelpers[ i ].m_isDirty )
		{
			m_anyHelperDirty = true;
		}
	}
}


void CEdControlRigPanel::LoadDataFromEvent()
{
	if ( CActor* selectedActor = m_selectedActorH.Get() )
	{
		CStorySceneEventPoseKey* evt = static_cast< CStorySceneEventPoseKey* >( FindSelectedEvent( ClassID< CStorySceneEventPoseKey >(), selectedActor->GetVoiceTag() ) );
		if ( !evt )
		{
			return;
		}

		for ( Uint32 i = 0; i < evt->GetNumOfFkBones(); i++ )
		{
			EngineTransform transform;
			CName boneName;
			evt->GetFkBonesData( i, boneName, transform );

			if ( SHelper* h = FindFkHelper( boneName.AsString() ) )
			{
				Matrix toAddLS;
				transform.CalcLocalToWorld( toAddLS );		

				if ( const CAnimatedComponent* ac = selectedActor->GetRootAnimatedComponent() )
				{
					const Matrix boneWS = ac->GetBoneMatrixWorldSpace( h->m_bone );

					h->m_currentTransformWS.SetPosition( boneWS.GetTranslation() );
					h->m_currentTransformWS.SetRotation( boneWS.ToEulerAngles() );

					Matrix initMat = boneWS * toAddLS.FullInverted();

					h->m_initialTransformWS.SetPosition( initMat.GetTranslation() );
					h->m_initialTransformWS.SetRotation( initMat.ToEulerAngles() );
				}

				TransformFKHelperLS( *h, toAddLS );
			}
		}

		for ( Int32 id=TCrEffector_First; id<TCrEffector_Last; ++id )
		{
			Vector val( Vector::ZERO_3D_POINT );
			Float w( 0.f );

			if ( evt->LoadIkEffector( id, val, w ) )
			{
				if ( SHelper* h = FindIkHelper( id ) )
				{
					h->m_currentTransformWS.SetPosition( val );
					h->m_isDirty = true;
				}
			}
		}
	}
}

void CEdControlRigPanel::UpdateTransforms()
{
	CActor* selectedActor = m_selectedActorH.Get();
	CStorySceneEventPoseKey* evt = GetEvent();
	if ( !evt || !selectedActor )
	{
		return;
	}
	if ( const CAnimatedComponent* ac = selectedActor->GetRootAnimatedComponent() )
	{				
		// FK
		for ( Uint32 i=0; i<m_fkHelpers.Size(); ++i )
		{
			SHelper& helper = m_fkHelpers[ i ];
			if ( !helper.m_isDirty )
			{
				const Matrix boneWS = ac->GetBoneMatrixWorldSpace( helper.m_bone );

				helper.m_initialTransformWS.SetPosition( boneWS.GetTranslation() );
				helper.m_initialTransformWS.SetRotation( boneWS.ToEulerAngles() );

				helper.m_currentTransformWS = helper.m_initialTransformWS;

				if ( const SHelper* parentH = FindFkHelper( helper.m_parentBone ) )
				{
					m_mediator->OnControlRig_HelperEntityUpdateTransform( helper.m_guid, helper.m_currentTransformWS, parentH->m_currentTransformWS );
				}
				else
				{
					m_mediator->OnControlRig_HelperEntityUpdateTransform( helper.m_guid, helper.m_currentTransformWS );
				}
			}
			else
			{
				const Matrix boneWS = ac->GetBoneMatrixWorldSpace( helper.m_bone );

				helper.m_currentTransformWS.SetPosition( boneWS.GetTranslation() );

				if ( const SHelper* parentH = FindFkHelper( helper.m_parentBone ) )
				{
					m_mediator->OnControlRig_HelperEntityUpdateTransform( helper.m_guid, helper.m_currentTransformWS, parentH->m_currentTransformWS );
				}
				else
				{
					m_mediator->OnControlRig_HelperEntityUpdateTransform( helper.m_guid, helper.m_currentTransformWS );
				}

				SCENE_ASSERT( evt );
				if ( evt )
				{
					SetEventBoneTransformFromFkHelper( evt, helper );
				}
			}
		} 

		// IK
		for ( Uint32 i=0; i<m_ikHelpers.Size(); ++i )
		{
			SHelper& helper = m_ikHelpers[ i ];
			if ( helper.m_bone != -1 )
			{
				if ( !helper.m_isDirty )
				{
					const Matrix effectorWS = GetEffectorDefaultMatrixWorldSpace( helper.m_bone );

					helper.m_currentTransformWS.SetPosition( effectorWS.GetTranslation() );

					m_mediator->OnControlRig_HelperEntityUpdateTransform( helper.m_guid, helper.m_currentTransformWS );
				}
				else
				{
					if ( helper.m_justMoved )
					{
						m_boneIkTransformPanel->ActivateEffector( helper.m_bone );
					}
					m_boneIkTransformPanel->RefreshEffector( helper.m_bone, helper.m_currentTransformWS );
				}
			}

			if ( evt )
			{
				m_boneIkTransformPanel->OnUpdateTransforms( evt );
			}
		} 

		if ( m_anyHelperDirty || m_forceCache )
		{
			m_forceCache = false;
			evt->CacheBones( selectedActor );
		}
		if( m_anyHelperJustMoved )
		{
			m_mediator->OnControlRig_RefreshPlayer();
		}
	}
}

CStorySceneEvent* CEdControlRigPanel::FindSelectedEvent( const CClass* c, const CName& actorId )
{
	//TODO - this approach is too buggy, we use selected event directly
	//return m_mediator->OnControlRig_FindSelectedEvent( c, actorId );
	return m_event;
}

const CSkeleton* CEdControlRigPanel::GetActorsSkeleton() const
{
	if ( const CAnimatedComponent* ac = GetActorsAnimatedComponent() )
	{
		return ac->GetSkeleton();
	}

	return nullptr;
}

const CAnimatedComponent* CEdControlRigPanel::GetActorsAnimatedComponent() const
{
	const CActor* selectedActor = m_selectedActorH.Get();
	return selectedActor ? selectedActor->GetRootAnimatedComponent() : nullptr;
}

Bool CEdControlRigPanel::GetEvtParentAnimation( CStorySceneEventPoseKey* e, CName& animName, Float& animTime ) const
{
	if ( e->IsLinkedToDialogset() )
	{
		CActor* selectedActor = m_selectedActorH.Get();
		SCENE_ASSERT( selectedActor );
		return selectedActor && m_mediator->OnControlRig_GetCurrIdleAnimationName( selectedActor->GetVoiceTag(), animName, animTime );
	}
	else
	{
		return m_mediator->OnControlRig_GetParentAnimationName( e, animName, animTime );
	}
}

void CEdControlRigPanel::SetEventBoneTransformFromFkHelper( CStorySceneEventPoseKey* evt, const CEdControlRigPanel::SHelper& helper )
{
	if ( const SHelper* parentH = FindFkHelper( helper.m_bone ) )
	{
		Matrix matParentW2L;
		parentH->m_initialTransformWS.CalcWorldToLocal( matParentW2L );

		Matrix boneInitWS;
		Matrix boneCurrWS;
		helper.m_initialTransformWS.CalcLocalToWorld( boneInitWS );
		helper.m_currentTransformWS.CalcLocalToWorld( boneCurrWS );

		Matrix boneInitLS = Matrix::Mul( matParentW2L, boneInitWS );
		Matrix boneCurrLS = Matrix::Mul( matParentW2L, boneCurrWS );

		EngineTransform boneTransLS;
		boneTransLS.SetRotation( EulerAngles::AngleDistance( boneInitLS.ToEulerAngles(), boneCurrLS.ToEulerAngles() ) );
		boneTransLS.RemovePosition();
		boneTransLS.RemoveScale();

		evt->SetBoneTransformLS_FK( helper.m_nameAsName, boneTransLS );
	}
}

Matrix CEdControlRigPanel::GetEffectorDefaultMatrixWorldSpace( Int32 effectorId ) const
{
	return m_boneIkTransformPanel->GetEffectorDefaultMatrixWorldSpace( effectorId );
}

CEdControlRigPanel::SHelper* CEdControlRigPanel::FindIkHelper( Int32 effectorId )
{
	for ( Uint32 i=0; i<m_ikHelpers.Size(); ++i )
	{
		SHelper& helper = m_ikHelpers[ i ];
		if ( helper.m_bone == effectorId )
		{
			return &helper;
		}
	}

	return nullptr;
}

const CEdControlRigPanel::SHelper* CEdControlRigPanel::FindFkHelper( Int32 boneIndex ) const
{
	for ( Uint32 i=0; i<m_fkHelpers.Size(); ++i )
	{
		const SHelper& helper = m_fkHelpers[ i ];
		if ( helper.m_bone == boneIndex )
		{
			return &helper;
		}
	}

	return nullptr;
}

CEdControlRigPanel::SHelper* CEdControlRigPanel::FindFkHelper( const String& boneName )
{
	return const_cast<SHelper*>( static_cast< const CEdControlRigPanel* >( this )->FindFkHelper( boneName ) );
}

const CEdControlRigPanel::SHelper* CEdControlRigPanel::FindFkHelper( const String& boneName ) const
{
	for ( Uint32 i=0; i<m_fkHelpers.Size(); ++i )
	{
		const SHelper& helper = m_fkHelpers[ i ];
		if ( helper.m_name == boneName )
		{
			return &helper;
		}
	}

	return nullptr;
}

const CEdControlRigPanel::SHelper* CEdControlRigPanel::FindFkParentHelper( Int32 boneIndex ) const
{
	if ( const CSkeleton* s = GetActorsSkeleton() )
	{
		const Int32 parentBone = s->GetParentBoneIndex( boneIndex );
		if ( parentBone != -1 )
		{
			return FindFkHelper( parentBone );
		}
	}

	return nullptr;
}

void CEdControlRigPanel::SetData( CStorySceneEventPoseKey* e, CActor* actor )
{
	m_event = e;
	m_selectedEvent = true;

	OnActorSelectionChange( actor );
}

void CEdControlRigPanel::Reload()
{
	// ?Why do we need this?
	CActor* selectedActor = m_selectedActorH.Get();
	m_selectedActorH = nullptr;
	OnActorSelectionChange( selectedActor );
}


void CEdControlRigPanel::OnNewKeyframe()
{
	CommitChanges();
}

void CEdControlRigPanel::CommitChanges( const Float* time )
{
	DiscardChanges();
}

void CEdControlRigPanel::DiscardChanges()
{
	for ( Uint32 i=0; i<m_fkHelpers.Size(); ++i )
	{
		if ( m_fkHelpers[ i ].m_isDirty )
		{
			m_fkHelpers[ i ].m_isDirty = false;
			m_fkHelpers[ i ].m_currentTransformWS = m_fkHelpers[ i ].m_initialTransformWS; 
			m_mediator->OnControlRig_HelperEntityUpdateTransform( m_fkHelpers[ i ].m_guid, m_fkHelpers[ i ].m_currentTransformWS );
		}
	}

	for ( Uint32 i=0; i<m_ikHelpers.Size(); ++i )
	{
		if ( m_ikHelpers[ i ].m_isDirty )
		{
			m_ikHelpers[ i ].m_isDirty = false;
			m_ikHelpers[ i ].m_currentTransformWS = m_ikHelpers[ i ].m_initialTransformWS; 
			m_mediator->OnControlRig_HelperEntityUpdateTransform( m_ikHelpers[ i ].m_guid, m_ikHelpers[ i ].m_currentTransformWS );
		}
	}
}

void CEdControlRigPanel::UpdateSlidersFK()
{
	const String& boneName = m_boneFKTranfromPanel->GetSelectedBone();
	const CEdControlRigPanel::SHelper* h = FindFkHelper( boneName );
	if( h && h->m_isEditable && !boneName.Empty() )
	{
		m_boneFKTranfromPanel->DisableSliders( false );
		Matrix boneInit;
		h->m_initialTransformWS.CalcLocalToWorld( boneInit );

		Matrix boneCurr;
		h->m_currentTransformWS.CalcLocalToWorld( boneCurr );

		Matrix boneLS = Matrix::Mul( boneInit.FullInverted() , boneCurr );
		m_boneFKTranfromPanel->RefreshUI( boneLS );
	}
	else
	{
		m_boneFKTranfromPanel->DisableSliders( true );
	}
}

void CEdControlRigPanel::TransformFKHelperLS( CEdControlRigPanel::SHelper& h, const Matrix& transformLS )
{
	if ( const SHelper* parent = FindFkHelper( h.m_bone ) )
	{
		h.m_isDirty = true;

		Matrix boneInit;
		h.m_initialTransformWS.CalcLocalToWorld( boneInit );

		Matrix boneWS = Matrix::Mul( boneInit, transformLS );

		EngineTransform& trans = h.m_currentTransformWS;
		trans.SetRotation( boneWS.ToEulerAngles() );

		m_mediator->OnControlRig_HelperEntityUpdateTransform( h.m_guid, h.m_currentTransformWS );
		m_mediator->OnControlRig_RefreshPlayer();
	}
}

void CEdControlRigPanel::OnFkTransformPanel_BoneSelected( const String& boneName )
{
	Uint32 chosenHelper = -1;
	for ( Uint32 i=0; i<m_fkHelpers.Size(); ++i )
	{
		m_fkHelpers[i].m_selectedByUser = false;		
		if ( m_fkHelpers[i].m_name == boneName )
		{
			chosenHelper = i;
		}
	}
	if ( chosenHelper  != -1 )
	{	
		UpdateSpawn( (Int32)chosenHelper, -1 );		
		m_mediator->OnControlRig_HelperEntitySelect( m_fkHelpers[ chosenHelper ].m_guid );
	}
}

void CEdControlRigPanel::OnIkTransformPanel_EffectorSelected( Int32 id )
{
	Uint32 chosenHelper = -1;
	for ( Uint32 i=0; i<m_ikHelpers.Size(); ++i )
	{
		m_ikHelpers[i].m_selectedByUser = false;		
		if ( m_ikHelpers[i].m_bone == id )
		{
			chosenHelper = i;
		}
	}
	if ( chosenHelper  != -1 )
	{
		UpdateSpawn( -1, (Int32)chosenHelper );			
		m_mediator->OnControlRig_HelperEntitySelect( m_ikHelpers[ chosenHelper ].m_guid );
	}
}

void CEdControlRigPanel::OnFkTransformPanel_Rotate( Float valX, Float valY, Float valZ, const String& boneName )
{
	for ( Uint32 i=0; i<m_fkHelpers.Size(); ++i )
	{
		if ( m_fkHelpers[i].m_name == boneName )
		{
			Matrix toAddLS;
			EulerAngles ang;
			ang.Pitch = valX;
			ang.Roll = valY;
			ang.Yaw = valZ;
			toAddLS = ang.ToMatrix();

			TransformFKHelperLS( m_fkHelpers[i], toAddLS );

			break;
		}
	}
}

void CEdControlRigPanel::OnFkTransformPanel_UpdateSliders()
{
	UpdateSlidersFK();
}

Bool CEdControlRigPanel::OnFkTransformPanel_IsBoneModified( const String& name ) const
{
	if ( const SHelper* h = FindFkHelper( name ) )
	{
		return h->m_isDirty;
	}
	return false;
}

void CEdControlRigPanel::OnFkTransformPanel_ResetBone( const String& name )
{
	for ( Uint32 i=0; i<m_fkHelpers.Size(); ++i )
	{
		if ( m_fkHelpers[ i ].m_name == name && m_fkHelpers[ i ].m_isDirty )
		{
			m_fkHelpers[ i ].m_isDirty = false;
			if( m_event )
			{
				m_event->ResetBone_FK( m_fkHelpers[ i ].m_nameAsName );
			}
			return;
		}
	}
}

Bool CEdControlRigPanel::OnFkTransformPanel_HelperExists( const String& boneName ) const
{
	return FindFkHelper( boneName ) != nullptr;
}

void CEdControlRigPanel::CreateHandsMapping()
{
	m_handsMappingNames[ SSB_Hand_L ] = CName( TXT("hand_l") );
	m_handsMappingNames[ SSB_Hand_R ] = CName( TXT("hand_r") );
}

void CEdControlRigPanel::CacheHandsMapping()
{
	if ( const CSkeleton* s = GetActorsSkeleton() )
	{
		for ( Uint32 i=0; i<SSB_Last; ++i )
		{
			m_handsMappingIdx[ i ] = s->FindBoneByName( m_handsMappingNames[ i ] );
		}
	}
}

void CEdControlRigPanel::OnHandsTransformPanel_Rotate( Float valX, Float valY, Float valZ, CName boneName )
{
	if( m_event )
	{
		EngineTransform transform;
		transform.SetRotation( EulerAngles( valY, valX, valZ ) );
		m_event->SetBoneTransformLS_Hands( boneName, transform );
	}
	m_forceCache = true;
	m_mediator->OnControlRig_RefreshPlayer();
}

void CEdControlRigPanel::OnHandsTransformPanel_ResetBone( CName name )
{
	if( m_event )
	{
		m_event->ResetBone_Hands( name );
	}
	m_forceCache = true;
	m_mediator->OnControlRig_RefreshPlayer();
}

CStorySceneEventPoseKey* CEdControlRigPanel::GetEvent()
{
	return m_event;
}

void CEdControlRigPanel::OnIkTransformPanel_RefreshPlayer()
{
	m_mediator->OnControlRig_RefreshPlayer();
}

//////////////////////////////////////////////////////////////////////////

#ifdef DEBUG_SCENES_2
#pragma optimize("",on)
#endif
