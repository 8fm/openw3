
#include "build.h"
#include "controlRigBoneTransformPanel.h"
#include "controlRigPanel.h"
#include "../../common/engine/skeleton.h"
#include "../../common/game/storySceneEventPoseKey.h"

#ifdef DEBUG_SCENES_2
#pragma optimize("",off)
#endif

BEGIN_EVENT_TABLE( CEdControlRigBoneTransformPanel, wxPanel )
	EVT_TEXT_ENTER( XRCID( "boneName" ), CEdControlRigBoneTransformPanel::OnSelectBoneName )
	EVT_TREE_ITEM_ACTIVATED( XRCID( "skeletonTree"), CEdControlRigBoneTransformPanel::OnSelectTreeBone )
	EVT_TOGGLEBUTTON( XRCID( "Rot15"), CEdControlRigBoneTransformPanel::OnSliderRangeChanged )
	EVT_TOGGLEBUTTON( XRCID( "Rot90"), CEdControlRigBoneTransformPanel::OnSliderRangeChanged )
	EVT_TOGGLEBUTTON( XRCID( "Rot360"), CEdControlRigBoneTransformPanel::OnSliderRangeChanged )
	EVT_BUTTON( XRCID( "ResetRot"), CEdControlRigBoneTransformPanel::OnResetSliders )
	EVT_BUTTON( XRCID( "bhA"), CEdControlRigBoneTransformPanel::OnHistoryBtnSelected )
	EVT_BUTTON( XRCID( "bhB"), CEdControlRigBoneTransformPanel::OnHistoryBtnSelected )
	EVT_BUTTON( XRCID( "bhC"), CEdControlRigBoneTransformPanel::OnHistoryBtnSelected )
	EVT_BUTTON( XRCID( "bhD"), CEdControlRigBoneTransformPanel::OnHistoryBtnSelected )
	EVT_SLIDER( XRCID( "rotX"), CEdControlRigBoneTransformPanel::OnRotSlider )
	EVT_SLIDER( XRCID( "rotY"), CEdControlRigBoneTransformPanel::OnRotSlider )
	EVT_SLIDER( XRCID( "rotZ"), CEdControlRigBoneTransformPanel::OnRotSlider )
	EVT_SPINCTRL( XRCID( "rotXspin" ), CEdControlRigBoneTransformPanel::OnRotSpinCtrl )
	EVT_SPINCTRL( XRCID( "rotYspin" ), CEdControlRigBoneTransformPanel::OnRotSpinCtrl )
	EVT_SPINCTRL( XRCID( "rotZspin" ), CEdControlRigBoneTransformPanel::OnRotSpinCtrl )
END_EVENT_TABLE()

CEdControlRigBoneTransformPanel::CEdControlRigBoneTransformPanel( wxWindow* parent, CEdControlRigPanel* ed )
	: m_sliderRange( 15.f )
	, m_entity( nullptr )
	, m_parentWin( ed )
{
	wxXmlResource::Get()->LoadPanel( this, parent, wxT("BoneTransformPanel") );

	m_boneName = XRCCTRL( *this, "boneName", wxTextCtrl );
	m_boneTree = XRCCTRL( *this, "skeletonTree", wxTreeCtrl );

	m_sliderX = XRCCTRL( *this, "rotX", wxSlider );
	m_sliderY = XRCCTRL( *this, "rotY", wxSlider );
	m_sliderZ = XRCCTRL( *this, "rotZ", wxSlider );

	m_spinX = XRCCTRL( *this, "rotXspin", wxSpinCtrl );
	m_spinY = XRCCTRL( *this, "rotYspin", wxSpinCtrl );
	m_spinZ = XRCCTRL( *this, "rotZspin", wxSpinCtrl );

	m_sliderReference[ 0 ] = m_sliderReference[ 1 ] = m_sliderReference[ 2 ] = 0.f;
}

CEdControlRigBoneTransformPanel::~CEdControlRigBoneTransformPanel()
{

}

void CEdControlRigBoneTransformPanel::OnActorSelectionChange( const CEntity* e )
{
	m_entity = e;
	
	FillBoneTree();
	DeselectBone();
	MarkElementsModified();	
}

namespace
{
	void MarkItem( wxTreeItemId& item, wxTreeCtrl* tree, Bool modified, Bool childMod )
	{
		wxString text = tree->GetItemText( item );

		text.Replace( wxT(" (#)"), wxT("") );
		text.Replace( wxT(" (*)"), wxT("") );

		if ( modified )
		{
			text += wxT(" (#)");
		}
		else if ( childMod )
		{
			text += wxT(" (*)");
		}

		tree->SetItemText( item, text );
	}
}

void CEdControlRigBoneTransformPanel::MarkElementsModified()
{
	MarkElementsModified( m_boneTree->GetRootItem(), m_boneTree );
} 

Bool CEdControlRigBoneTransformPanel::MarkElementsModified( wxTreeItemId& root, wxTreeCtrl* tree )
{
	Bool dirty = false;
	if ( !root.IsOk() )
	{
		return false;
	}
	wxString text = m_boneTree->GetItemText( root );
	text.Replace( wxT(" (#)"), wxT("") );
	text.Replace( wxT(" (*)"), wxT("") );

	if( m_parentWin->OnFkTransformPanel_IsBoneModified( text.wc_str() ) )
	{
		MarkItem( root, tree, true , false );
		dirty = true;
	}


	wxTreeItemIdValue cookie;
	wxTreeItemId item = tree->GetFirstChild( root, cookie );

	while( item.IsOk() )
	{
		if( MarkElementsModified( item, tree ) && !dirty )
		{
			MarkItem( root, tree, false , true );	
			dirty = true;
		}
		item = tree->GetNextChild( root, cookie );
	}

	if ( !dirty )
	{
		MarkItem( root, tree, false , false );	
	}
	else
	{
		tree->Expand( root );
	}
	return dirty;
}

void CEdControlRigBoneTransformPanel::FillBoneTree()
{
	if ( m_entity )
	{
		if ( const CAnimatedComponent* ac = m_entity->GetRootAnimatedComponent() )
		{
			if ( const CSkeleton* s = ac->GetSkeleton() )
			{
				m_boneTree->Freeze();
				m_boneTree->DeleteAllItems();

				const Uint32 numBones = s->GetBonesNum();
				if ( numBones > 0 )
				{
					wxString itemName = s->GetBoneNameAnsi(0);
					wxTreeItemId root = m_boneTree->AddRoot( itemName );
					FillBoneTree( s, 0, root, m_boneTree );
					m_boneTree->Expand( root );
				}

				m_boneTree->Thaw();
			}
		}		
	}
}

void CEdControlRigBoneTransformPanel::FillBoneTree( const CSkeleton* skeleton, Int32 parentIndex, wxTreeItemId& parent, wxTreeCtrl* tree )
{
	const Uint32 numBones = skeleton->GetBonesNum();
	for ( Uint32 i=0; i<numBones; i++ )
	{
		const Int32 boneParentIndex = skeleton->GetParentBoneIndex(i);
		if ( boneParentIndex == parentIndex )
		{
			wxString itemName = skeleton->GetBoneNameAnsi(i);
			wxTreeItemId item = parent;
			if ( m_parentWin->OnFkTransformPanel_HelperExists( itemName.wc_str() ) )
			{
				item = tree->AppendItem( parent, itemName, 0 );
				if ( !tree->ItemHasChildren( parent ) ) 
				{
					tree->SetItemHasChildren( parent, true );
				}

				m_boneTree->SetItemData( item, new SItemBoneData( i ) );
			}
			FillBoneTree( skeleton, i, item, tree );
		}
	}
}

wxTreeItemId CEdControlRigBoneTransformPanel::GetTreeBoneItemByName( const wxString& itemName ) const
{
	TQueue< wxTreeItemId >	items;
	items.Push( m_boneTree->GetRootItem() );

	while( !items.Empty() )
	{
		wxTreeItemId &currItem = items.Front();
		items.Pop();

		wxString text =  m_boneTree->GetItemText(currItem);
		text.Replace( wxT(" (#)"), wxT("") );
		text.Replace( wxT(" (*)"), wxT("") );

		if ( text == itemName )
		{
			return currItem;
		}

		wxTreeItemIdValue cookie = 0;
		wxTreeItemId child = m_boneTree->GetFirstChild( currItem, cookie );
		while( child.IsOk() )
		{
			items.Push( child );
			child = m_boneTree->GetNextChild( currItem, cookie );
		}
	}

	return wxTreeItemId();
}

Bool CEdControlRigBoneTransformPanel::SelectBoneForTree( const wxString& boneName, Bool callback )
{
	if ( m_boneTree->IsEmpty() )
	{
		return false;
	}
	wxTreeItemId treeItem = GetTreeBoneItemByName( boneName );
	if ( treeItem.IsOk() )
	{
		m_boneTree->SelectItem( treeItem );

		AddBoneSelectionToHistory( boneName );

		if ( callback )
		{
			CallSelectionCallback( boneName );
		}

		m_selectedBone = boneName.wc_str();

		return true;
	}

	return false;
}

void CEdControlRigBoneTransformPanel::SelectBoneForEdit( const wxString& boneName, Bool callback )
{
	m_boneName->SetLabel( boneName );

	AddBoneSelectionToHistory( boneName );

	if ( callback )
	{
		CallSelectionCallback( boneName );
	}

	m_selectedBone = boneName.wc_str();

	CopyPoseToSliders();
}

void CEdControlRigBoneTransformPanel::SelectBone( const wxString& boneName )
{
	if ( SelectBoneForTree( boneName, false ) )
	{
		SelectBoneForEdit( boneName, false );

		m_selectedBone = boneName.wc_str();
	}
}

const String& CEdControlRigBoneTransformPanel::GetSelectedBone() const
{
	return m_selectedBone;
}

void CEdControlRigBoneTransformPanel::RefreshUI( const Matrix& boneLS )
{
	const EulerAngles ang = boneLS.ToEulerAngles();

	const Float sX_new = ang.Pitch / m_sliderRange;
	const Float sY_new = ang.Roll / m_sliderRange;
	const Float sZ_new = ang.Yaw / m_sliderRange;

	m_sliderX->SetValue( Clamp< Int32 >( (Int32)( sX_new * 100.f ) + 50, 0, 100 ) );
	m_sliderY->SetValue( Clamp< Int32 >( (Int32)( sY_new * 100.f ) + 50, 0, 100 ) );
	m_sliderZ->SetValue( Clamp< Int32 >( (Int32)( sZ_new * 100.f ) + 50, 0, 100 ) );

	m_spinX->SetValue( Clamp< Int32 >( (Int32)( ang.Pitch ), -360, 360 ) );
	m_spinY->SetValue( Clamp< Int32 >( (Int32)( ang.Roll ), -360, 360 ) );
	m_spinZ->SetValue( Clamp< Int32 >( (Int32)( ang.Yaw ), -360, 360 ) );
}

void CEdControlRigBoneTransformPanel::InternalSelectBone( const wxString& boneName )
{
	SCENE_VERIFY( SelectBoneForTree( boneName, false ) );
	SelectBoneForEdit( boneName, false );

	CallSelectionCallback( boneName );

	m_selectedBone = boneName.wc_str();
}

void CEdControlRigBoneTransformPanel::DeselectBone()
{
	m_boneName->SetLabel( wxT("<None>") );
	m_boneTree->UnselectAll();
	m_selectedBone = String::EMPTY;
}

void CEdControlRigBoneTransformPanel::AddBoneSelectionToHistory( const wxString& boneName )
{
	wxButton* b[ 4 ];
	b[0] = XRCCTRL( *this, "bhA", wxButton );
	b[1] = XRCCTRL( *this, "bhB", wxButton );
	b[2] = XRCCTRL( *this, "bhC", wxButton );
	b[3] = XRCCTRL( *this, "bhD", wxButton );

	m_boneNameHistory.Remove( boneName );
	m_boneNameHistory.PushFront( boneName );

	if ( m_boneNameHistory.Size() > 4 )
	{
		m_boneNameHistory.PopBack();
	}

	Int32 c = 0;
	for ( auto it = m_boneNameHistory.Begin(), end = m_boneNameHistory.End(); it != end; ++it, ++c )
	{
		const wxString& str = *it;
		b[c]->SetLabel( str );
	}
}

void CEdControlRigBoneTransformPanel::CallSelectionCallback( const wxString& boneName )
{
	String b = boneName.wc_str();
	m_parentWin->OnFkTransformPanel_BoneSelected( b );
}

void CEdControlRigBoneTransformPanel::SetSliderRange( Int32 range )
{
	const Float valX_prev = m_sliderReference[ 0 ] + ( (m_sliderX->GetValue()-50) / 100.f ) * m_sliderRange;
	const Float valY_prev = m_sliderReference[ 1 ] + ( (m_sliderY->GetValue()-50) / 100.f ) * m_sliderRange;
	const Float valZ_prev = m_sliderReference[ 2 ] + ( (m_sliderZ->GetValue()-50) / 100.f ) * m_sliderRange;

	m_sliderRange = range;
	
	const Float sX_new = valX_prev / m_sliderRange;
	const Float sY_new = valY_prev / m_sliderRange;
	const Float sZ_new = valZ_prev / m_sliderRange;

	m_sliderX->SetValue( Clamp< Int32 >( (Int32)( sX_new * 100.f ) + 50, 0, 100 ) );
	m_sliderY->SetValue( Clamp< Int32 >( (Int32)( sY_new * 100.f ) + 50, 0, 100 ) );
	m_sliderZ->SetValue( Clamp< Int32 >( (Int32)( sZ_new * 100.f ) + 50, 0, 100 ) );

	wxCommandEvent fakeEvt;
	OnRotSlider( fakeEvt );
}

void CEdControlRigBoneTransformPanel::CopyPoseToSliders()
{
	if ( !m_selectedBone.Empty() )
	{
		m_parentWin->OnFkTransformPanel_UpdateSliders();
	}
}

void CEdControlRigBoneTransformPanel::Rotate( Float valX, Float valY, Float valZ )
{
	m_parentWin->OnFkTransformPanel_Rotate( valX, valY, valZ, m_selectedBone );
}

void CEdControlRigBoneTransformPanel::OnSelectBoneName( wxCommandEvent& event )
{
	wxString val = m_boneName->GetValue();
	SelectBoneForTree( val, true );
}

void CEdControlRigBoneTransformPanel::OnSelectTreeBone( wxTreeEvent& event )
{
	wxTreeItemId item = event.GetItem();
	if ( item.IsOk() )
	{
		wxString text = m_boneTree->GetItemText( item );
		text.Replace( wxT(" (#)"), wxT("") );
		text.Replace( wxT(" (*)"), wxT("") );
		SelectBoneForEdit( text, true );
	}
}

void CEdControlRigBoneTransformPanel::OnSliderRangeChanged( wxCommandEvent& event )
{
	wxToggleButton* b[3];
	b[0] = XRCCTRL( *this, "Rot15", wxToggleButton );
	b[1] = XRCCTRL( *this, "Rot90", wxToggleButton );
	b[2] = XRCCTRL( *this, "Rot360", wxToggleButton );

	if ( event.GetId() == XRCID( "Rot15") )
	{
		SetSliderRange( 15 );

		b[1]->SetValue( false );
		b[2]->SetValue( false );
	}
	else if ( event.GetId() == XRCID( "Rot90") )
	{
		SetSliderRange( 90 );

		b[0]->SetValue( false );
		b[2]->SetValue( false );
	}
	else if ( event.GetId() == XRCID( "Rot360") )
	{
		SetSliderRange( 360 );

		b[0]->SetValue( false );
		b[1]->SetValue( false );
	}
}

void CEdControlRigBoneTransformPanel::OnResetSliders( wxCommandEvent& event )
{
	m_sliderX->SetValue( 50 );
	m_sliderY->SetValue( 50 );
	m_sliderZ->SetValue( 50 );

	m_spinX->SetValue( 0 );
	m_spinY->SetValue( 0 );
	m_spinZ->SetValue( 0 );

	Rotate( 0.f, 0.f, 0.f );
	m_parentWin->OnFkTransformPanel_ResetBone( m_selectedBone );
	MarkElementsModified();
}

void CEdControlRigBoneTransformPanel::OnHistoryBtnSelected( wxCommandEvent& event )
{
	Int32 sel = -1;

	if ( event.GetId() == XRCID( "bhA" ) )
	{
		sel = 0;
	}
	else if ( event.GetId() == XRCID( "bhB" ) )
	{
		sel = 1;
	}
	else if ( event.GetId() == XRCID( "bhC" ) )
	{
		sel = 2;
	}
	else if ( event.GetId() == XRCID( "bhD" ) )
	{
		sel = 3;
	}

	if ( sel != -1 && sel < (Int32)m_boneNameHistory.Size() )
	{
		auto it = m_boneNameHistory.Begin();
		while ( sel > 0 )
		{
			--sel;
			++it;

			SCENE_ASSERT( it != m_boneNameHistory.End() );
		}

		SCENE_ASSERT( it != m_boneNameHistory.End() );

		wxString str( *it );
		InternalSelectBone( str );
	}
}

void CEdControlRigBoneTransformPanel::OnRotSlider( wxCommandEvent& event )
{
	const Float valX = m_sliderReference[ 0 ] + ( (m_sliderX->GetValue()-50) / 100.f ) * m_sliderRange;
	const Float valY = m_sliderReference[ 1 ] + ( (m_sliderY->GetValue()-50) / 100.f ) * m_sliderRange;
	const Float valZ = m_sliderReference[ 2 ] + ( (m_sliderZ->GetValue()-50) / 100.f ) * m_sliderRange;

	m_spinX->SetValue( valX );
	m_spinY->SetValue( valY );
	m_spinZ->SetValue( valZ );

	Rotate( valX, valY, valZ );
	MarkElementsModified();
}

void CEdControlRigBoneTransformPanel::OnRotSpinCtrl( wxSpinEvent& event )
{
	const Float valX = m_spinX->GetValue();
	const Float valY = m_spinY->GetValue();
	const Float valZ = m_spinZ->GetValue();

	const Float sX_new = valX / m_sliderRange;
	const Float sY_new = valY / m_sliderRange;
	const Float sZ_new = valZ / m_sliderRange;

	m_sliderX->SetValue( Clamp< Int32 >( (Int32)( sX_new * 100.f ) + 50, 0, 100 ) );
	m_sliderY->SetValue( Clamp< Int32 >( (Int32)( sY_new * 100.f ) + 50, 0, 100 ) );
	m_sliderZ->SetValue( Clamp< Int32 >( (Int32)( sZ_new * 100.f ) + 50, 0, 100 ) );

	Rotate( valX, valY, valZ );
	MarkElementsModified();
}

void CEdControlRigBoneTransformPanel::DisableSliders( Bool param1 )
{
	if( param1 )
	{
		m_sliderX->SetValue( 50.f );
		m_sliderX->Disable();

		m_sliderY->SetValue( 50.f );
		m_sliderY->Disable();

		m_sliderZ->SetValue( 50.f );
		m_sliderZ->Disable();

		m_spinX->SetValue( 0.f );
		m_spinX->Disable();

		m_spinY->SetValue( 0.f );
		m_spinY->Disable();

		m_spinZ->SetValue( 0.f );
		m_spinZ->Disable();
	}
	else
	{
		m_sliderX->Enable();
		m_sliderY->Enable();
		m_sliderZ->Enable();
		m_spinX->Enable();
		m_spinY->Enable();
		m_spinZ->Enable();
	}
}

//////////////////////////////////////////////////////////////////////////

namespace
{
	void AddBoneData( CEdControlRigHandsTransformPanel::CorrectionData& data, CName bone, Float ratio, CEdControlRigHandsTransformPanel::BoneRotationAxis axis = CEdControlRigHandsTransformPanel::EA_AxisZ )
	{
		data.m_bones.PushBack( bone );
		data.m_ratios.PushBack( ratio );
		data.m_rotAxis = axis;
	}

	void AddFingerData( TDynArray< CEdControlRigHandsTransformPanel::CorrectionData >& handData, CEdControlRigHandsTransformPanel::Part handPart, const String& boneNameStr,  Float twistRatio, Float allFingersRatio = 1.f, Float baseRatio = 1.f, Int32 bonesNr = 3 )
	{
		const Int32 LastBoneIndex = 3;
		Float ratio[4];
		ratio[0] = baseRatio*0.3f; 
		ratio[1] = baseRatio;
		ratio[2] = baseRatio*0.6f;
		ratio[3] = baseRatio*0.3f;
		String bone = boneNameStr;		
		for ( Int32 j = 0; j < bonesNr; ++j  )
		{
			CName boneName = CName( bone + ToString( LastBoneIndex - j ) );
			AddBoneData( handData[handPart], boneName, ratio[j] );  
			AddBoneData( handData[CEdControlRigHandsTransformPanel::EP_All], boneName, allFingersRatio * ratio[j] );  
			AddBoneData( handData[CEdControlRigHandsTransformPanel::EP_Twist], boneName, twistRatio * ratio[j] );  
		}
	}
}


Float CEdControlRigHandsTransformPanel::BASE_ROT_ANGLE = 120.f;

CEdControlRigHandsTransformPanel::CEdControlRigHandsTransformPanel( wxWindow* parent, CEdControlRigPanel* ed )
	: m_parentWin( ed )
{
	wxXmlResource::Get()->LoadPanel( this, parent, wxT("HandTransformPanel") );

	m_hardResetBtn = XRCCTRL( *this, "btnHandReset", wxButton );
	m_hardResetBtn->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdControlRigHandsTransformPanel::OnHardReset ), nullptr, this  );			
	
	const StringAnsi sides[] = { "L", "R" };
	const String	 boneSidePref[] = { TXT("l_"), TXT("r_") };
	const StringAnsi names[] = { "all", "A", "B", "C", "D", "E", "twist", "handX", "handY", "handZ" };
	const Int32 namesSize = sizeof( names ) / sizeof( names[0] );
	const Int32 sidesSize = sizeof( sides ) / sizeof( sides[0] );

	ASSERT( namesSize == EP_PartSize );
	ASSERT( sidesSize == ES_SideSize );

	for ( Int32 i = 0; i < sidesSize; ++i )
	{	
		m_sliders.PushBack( TDynArray< wxSlider* >( size_t( namesSize ) ) );
		m_buttons.PushBack( TDynArray< wxButton* >( size_t( namesSize ) ) );		
		m_data.PushBack( TDynArray< CorrectionData >( size_t( namesSize ) ) );		

		for ( Int32 j = 0; j < namesSize; ++j )
		{
			StringAnsi sliderName = "m_slider_" + sides[i] + "_" + names[j];
			StringAnsi buttonName = "b_" + sides[i] + "_" + names[j];
			m_sliders[i][j] = XRCCTRL( *this, sliderName.AsChar(), wxSlider );		
			m_sliders[i][j]->Connect( wxEVT_COMMAND_SLIDER_UPDATED, wxCommandEventHandler( CEdControlRigHandsTransformPanel::OnSlider ), new PartInfo( Side( i ), Part( j ) ), this  );				
			m_buttons[i][j] = XRCCTRL( *this, buttonName.AsChar(), wxButton );
			m_buttons[i][j]->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdControlRigHandsTransformPanel::OnReset ), new PartInfo( Side( i ), Part( j ) ) , this  );			
		}

		TDynArray< CorrectionData >& handData = m_data[i];
		
		AddFingerData( handData, EP_FingerA, boneSidePref[i] + TXT("pinky") , 1.0f );
		AddFingerData( handData, EP_FingerB, boneSidePref[i] + TXT("ring")  , 0.9f );
		AddFingerData( handData, EP_FingerC, boneSidePref[i] + TXT("middle"), 0.8f );
		AddFingerData( handData, EP_FingerD, boneSidePref[i] + TXT("index") , 0.7f );
		AddFingerData( handData, EP_FingerE, boneSidePref[i] + TXT("thumb") , 0.1f, 0.1f );	

		AddBoneData( handData[EP_HandX], CName( boneSidePref[i] + TXT("hand") ), 1.f, EA_AxisX );
		AddBoneData( handData[EP_HandY], CName( boneSidePref[i] + TXT("hand") ), 1.f, EA_AxisY );
		AddBoneData( handData[EP_HandZ], CName( boneSidePref[i] + TXT("hand") ), 1.f, EA_AxisZ );

		String bone;
		CName boneName;
	}
}

void CEdControlRigHandsTransformPanel::OnActorSelectionChange( const CActor* actor )
{
	if( actor )
	{
		CStorySceneEventPoseKey* evt = m_parentWin->GetEvent();
		for ( Int32 i = 0; i < ES_SideSize; ++i )
		{	
			for ( Int32 j = 0; j < EP_PartSize; ++j )
			{
				Float val;
				evt->LoadHandTrackVals( j + i * EP_PartSize, val );
				m_sliders[i][j]->SetValue( (val + 1.f)*50.f );
				m_data[i][j].m_currentVal = val;
				m_dirtyBones.PushBackUnique( m_data[i][j].m_bones );
			}
		}			
		CalculatePoses();
	}
	else
	{
		wxCommandEvent event;
		OnHardReset( event );
	}
}

void CEdControlRigHandsTransformPanel::CalculatePoses()
{
	TDynArray<Vector> dirtyBonesRot( m_dirtyBones.Size() );
	for ( Uint32 i = 0; i < dirtyBonesRot.Size(); ++i )
	{
		dirtyBonesRot[i].SetZeros();
	}

	for ( Int32 i = 0; i < ES_SideSize; ++i )
	{	
		for ( Int32 j = 0; j < EP_PartSize; ++j )
		{
			for ( Uint32 k = 0; k < m_dirtyBones.Size(); ++k )
			{
				const CorrectionData& data = m_data[i][j];
				Int32 index = data.m_bones.GetIndex( m_dirtyBones[k] );
				if( index != -1 )
				{
					Float angle = data.m_currentVal * data.m_ratios[ index ] * BASE_ROT_ANGLE;
					if ( data.m_rotAxis == EA_AxisX )
					{
						dirtyBonesRot[k].X += angle;
					}
					if ( data.m_rotAxis == EA_AxisY )
					{
						dirtyBonesRot[k].Y += angle;
					}
					if ( data.m_rotAxis == EA_AxisZ )
					{
						dirtyBonesRot[k].Z += angle;
					}					
				}
			}				
		}
	}

	for ( Uint32 i = 0; i < m_dirtyBones.Size(); ++i )
	{
		if( dirtyBonesRot[i].SquareMag3() < 0.0001 )
		{
			m_parentWin->OnHandsTransformPanel_ResetBone( m_dirtyBones[i] );
		}
		else
		{
			m_parentWin->OnHandsTransformPanel_Rotate( dirtyBonesRot[i].X, dirtyBonesRot[i].Y, dirtyBonesRot[i].Z, m_dirtyBones[i] );
		}		
	}
}


void CEdControlRigHandsTransformPanel::OnHardReset( wxCommandEvent& event )
{
	CStorySceneEventPoseKey* evt = m_parentWin->GetEvent();
	for ( Int32 i = 0; i < ES_SideSize; ++i )
	{	
		for ( Int32 j = 0; j < EP_PartSize; ++j )
		{
			CorrectionData& data = m_data[i][j];
			m_sliders[i][j]->SetValue( 50.f );
			data.m_currentVal = 0.f;
			m_dirtyBones.PushBackUnique( data.m_bones );
			if ( evt )
			{
				evt->CacheHandTrackVals( j + i * EP_PartSize, data.m_currentVal );
			}
		}
	}		
	CalculatePoses();
}


void CEdControlRigHandsTransformPanel::OnReset( wxCommandEvent& event )
{
	PartInfo* partInfo = static_cast<PartInfo*>( event.m_callbackUserData );
	Side side = partInfo->m_side;
	Part part = partInfo->m_part;

	m_sliders[side][part]->SetValue( 50.f );
	CorrectionData& data = m_data[side][part];
	data.m_currentVal = 0.f;
	m_dirtyBones.PushBackUnique( data.m_bones );

	CStorySceneEventPoseKey* evt = m_parentWin->GetEvent();
	if ( evt )
	{
		evt->CacheHandTrackVals( part + side * EP_PartSize, data.m_currentVal );
	}

	CalculatePoses();
}

void CEdControlRigHandsTransformPanel::OnSlider( wxCommandEvent& event )
{
	PartInfo* partInfo = static_cast<PartInfo*>( event.m_callbackUserData );
	Side side = partInfo->m_side;
	Part part = partInfo->m_part;
	CorrectionData& data = m_data[side][part];
	m_dirtyBones.PushBackUnique( data.m_bones );
	data.m_currentVal = m_sliders[side][part]->GetValue() / 50.f - 1.f;
	CStorySceneEventPoseKey* evt = m_parentWin->GetEvent();
	if ( evt )
	{
		evt->CacheHandTrackVals( part + side * EP_PartSize, data.m_currentVal );
	}
	CalculatePoses();
}

#ifdef DEBUG_SCENES_2
#pragma optimize("",on)
#endif
