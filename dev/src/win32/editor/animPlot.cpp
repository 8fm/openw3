/**
 * Copyright © 2008 CD Projekt Red. All Rights Reserved.
 */
#include "build.h"
#include "animPlot.h"
#include "../../common/engine/curve.h"
#include "../../common/engine/motionExtraction.h"
#include "../../common/engine/motionExtractionCompression.h"
#include "../../common/engine/skeleton.h"

BEGIN_EVENT_TABLE( CEdAnimPlot, wxSmartLayoutPanel )
	EVT_BUTTON( XRCID("btnOk"), CEdAnimPlot::OnOk )
END_EVENT_TABLE()

CEdAnimPlot::CEdAnimPlot( wxWindow* parent, const CSkeletalAnimationSetEntry* anim, const CSkeleton* skeleton )
	: wxSmartLayoutPanel( parent, TXT("AnimPlot"), false )
{
	wxString caption = wxString::Format( wxT("Anim Plot - %s"), anim->GetName().AsString().AsChar() );
	SetTitle( wxString::Format( caption ) );

	// Create Curve Editor Canvas panel
	wxPanel* rp = XRCCTRL( *this, "curvePanel", wxPanel );
	wxBoxSizer* sizer1 = new wxBoxSizer( wxVERTICAL );
	m_curveEditorCanvas = new CEdCurveEditorCanvas( rp );
	m_curveEditorCanvas->SetActiveRegion( 0.f, anim->GetDuration() + 0.0001f );
	sizer1->Add( m_curveEditorCanvas, 1, wxEXPAND, 0 );
	rp->SetSizer( sizer1 );
	rp->Layout();

	m_animation = anim;

	// Update and finalize layout
	Layout();
	Show();
}

CEdAnimPlot::~CEdAnimPlot()
{

}

const CSkeletalAnimationSetEntry* CEdAnimPlot::GetAnimEntry() const
{
	return m_animation;
}

const CSkeletalAnimation* CEdAnimPlot::GetAnim() const
{
	const CSkeletalAnimationSetEntry* entry = GetAnimEntry();
	return entry ? entry->GetAnimation() : NULL;
}

void CEdAnimPlot::OnCurveSelected( TPlotCurveData curveData )
{
	m_curveEditorCanvas->RemoveAllCurves();
	m_curveEditorCanvas->AddCurve( &curveData.m_second->GetCurveData(), curveData.m_first );
	m_curveEditorCanvas->SetZoomedRegionToFit();
	m_curveEditorCanvas->Repaint();
}

void CEdAnimPlot::OnCurvesSelected( TDynArray< TPlotCurveData >& curveDatas )
{
	m_curveEditorCanvas->RemoveAllCurves();

	for ( Uint32 i=0; i<curveDatas.Size(); ++i )
	{
		TPlotCurveData& curveData = curveDatas[ i ];
		m_curveEditorCanvas->AddCurve( &curveData.m_second->GetCurveData(), curveData.m_first );
	}

	m_curveEditorCanvas->SetZoomedRegionToFit();
	m_curveEditorCanvas->Repaint();
}

void CEdAnimPlot::OnOk( wxCommandEvent& event )
{
	Close();
}

//////////////////////////////////////////////////////////////////////////

#define ID_ANIM_PLOT_TREE 6000

BEGIN_EVENT_TABLE( CEdAnimPlotTree, wxPanel )
	EVT_TREE_ITEM_ACTIVATED( ID_ANIM_PLOT_TREE, CEdAnimPlotTree::OnItemActivated )
	EVT_TREE_SEL_CHANGED( ID_ANIM_PLOT_TREE, CEdAnimPlotTree::OnItemSelected )
END_EVENT_TABLE()

CEdAnimPlotTree::CEdAnimPlotTree( wxWindow* parent )
	: wxPanel( parent )
	, m_fps( 30.f )
	, m_hook( NULL )
{
	wxBoxSizer* bSizer1;
	bSizer1 = new wxBoxSizer( wxVERTICAL );

	m_wxTree = new wxTreeCtrl( this, ID_ANIM_PLOT_TREE, wxDefaultPosition, wxDefaultSize, wxTR_DEFAULT_STYLE );
	bSizer1->Add( m_wxTree, 1, wxEXPAND, 5 );

	this->SetSizer( bSizer1 );
	this->Layout();
}

CEdAnimPlotTree::~CEdAnimPlotTree()
{
	ClearCurves();
}

void CEdAnimPlotTree::SetHook( IAnimPlotTreeHook* hook )
{
	m_hook = hook;
}

void CEdAnimPlotTree::AddCurve( CCurve* curve )
{
	ASSERT( !curve->IsInRootSet() );
	curve->AddToRootSet();
	m_curves.PushBack( curve );
}

void CEdAnimPlotTree::AddCurves( CCurve** curve, Uint32 num )
{
	for ( Uint32 i=0; i<num; ++i )
	{
		AddCurve( curve[ i ] );
	}
}

void CEdAnimPlotTree::ClearCurves()
{
	for ( Uint32 i=0; i<m_curves.Size(); ++i )
	{
		m_curves[ i ]->RemoveFromRootSet();
	}
	m_curves.Clear();
}

void CEdAnimPlotTree::ClearAndFill( const CSkeletalAnimation* animation, const CSkeleton* skeleton )
{
	wxTreeItemId selItem = m_wxTree->GetSelection();

	Clear();
	Fill( animation, skeleton );

	if ( selItem.IsOk() )
	{
		m_wxTree->SelectItem( selItem, true );

		wxTreeEvent fake;
		OnItemActivated( fake );
	}
}

void CEdAnimPlotTree::Clear()
{
	m_wxTree->Freeze();

	ClearCurves();

	m_wxTree->DeleteAllItems();
	m_wxTree->Thaw();
}

wxTreeItemId CEdAnimPlotTree::AddNode( wxTreeItemId treeRoot, wxString nodeName, ETreeDataType nodeType, TDynArray< AnimQsTransform >& data, TDynArray< Float >* times )
{
	wxTreeItemId root = m_wxTree->AppendItem( treeRoot, nodeName, -1, -1, new TreeData( nodeType, NULL ) );

	CCurve* curves[ CT_Size ];

	curves[ CT_PosX ] = CreateObject< CCurve >(); curves[ CT_PosX ]->SetColor( Color::RED );
	curves[ CT_PosY ] = CreateObject< CCurve >(); curves[ CT_PosY ]->SetColor( Color::GREEN );
	curves[ CT_PosZ ] = CreateObject< CCurve >(); curves[ CT_PosZ ]->SetColor( Color::BLUE );

	curves[ CT_RotX ] = CreateObject< CCurve >(); curves[ CT_RotX ]->SetColor( Color::RED );
	curves[ CT_RotY ] = CreateObject< CCurve >(); curves[ CT_RotY ]->SetColor( Color::GREEN );
	curves[ CT_RotZ ] = CreateObject< CCurve >(); curves[ CT_RotZ ]->SetColor( Color::BLUE );

	AddCurves( curves, CT_Size );
	FillCurves( curves, data, times );

	wxTreeItemId rootTrans = m_wxTree->AppendItem( root, wxT("Trans"), -1, -1, new TreeData( TDT_TransNode, NULL ) );
	wxTreeItemId rootRot = m_wxTree->AppendItem( root, wxT("Rot"), -1, -1, new TreeData( TDT_RotNode, NULL ) );

	wxTreeItemId itemTX = m_wxTree->AppendItem( rootTrans, wxT("X"), -1, -1, new TreeData( TDT_TransX, curves[ CT_PosX ]  ) );
	wxTreeItemId itemTY = m_wxTree->AppendItem( rootTrans, wxT("Y"), -1, -1, new TreeData( TDT_TransY, curves[ CT_PosY ]  ) );
	wxTreeItemId itemTZ = m_wxTree->AppendItem( rootTrans, wxT("Z"), -1, -1, new TreeData( TDT_TransZ, curves[ CT_PosZ ]  ) );

	wxTreeItemId itemRX = m_wxTree->AppendItem( rootRot, wxT("X"), -1, -1, new TreeData( TDT_RotX, curves[ CT_RotX ]  ) );
	wxTreeItemId itemRY = m_wxTree->AppendItem( rootRot, wxT("Y"), -1, -1, new TreeData( TDT_RotY, curves[ CT_RotY ]  ) );
	wxTreeItemId itemRZ = m_wxTree->AppendItem( rootRot, wxT("Z"), -1, -1, new TreeData( TDT_RotZ, curves[ CT_RotZ ]  ) );

	m_wxTree->SetItemTextColour( itemTX, *wxRED );
	m_wxTree->SetItemTextColour( itemTY, *wxGREEN );
	m_wxTree->SetItemTextColour( itemTZ, *wxBLUE );

	m_wxTree->SetItemTextColour( itemRX, *wxRED );
	m_wxTree->SetItemTextColour( itemRY, *wxGREEN );
	m_wxTree->SetItemTextColour( itemRZ, *wxBLUE );

	return root;
}

wxTreeItemId CEdAnimPlotTree::AddNodeWithScale( wxTreeItemId treeRoot, wxString nodeName, ETreeDataType nodeType, TDynArray< AnimQsTransform >& data, TDynArray< Float >* times )
{
	wxTreeItemId root = m_wxTree->AppendItem( treeRoot, nodeName, -1, -1, new TreeData( nodeType, NULL ) );

	CCurve* curves[ CT_SizeWithScale ];

	curves[ CT_PosX ] = CreateObject< CCurve >(); curves[ CT_PosX ]->SetColor( Color::RED );
	curves[ CT_PosY ] = CreateObject< CCurve >(); curves[ CT_PosY ]->SetColor( Color::GREEN );
	curves[ CT_PosZ ] = CreateObject< CCurve >(); curves[ CT_PosZ ]->SetColor( Color::BLUE );

	curves[ CT_RotX ] = CreateObject< CCurve >(); curves[ CT_RotX ]->SetColor( Color::RED );
	curves[ CT_RotY ] = CreateObject< CCurve >(); curves[ CT_RotY ]->SetColor( Color::GREEN );
	curves[ CT_RotZ ] = CreateObject< CCurve >(); curves[ CT_RotZ ]->SetColor( Color::BLUE );

	curves[ CT_ScaleX ] = CreateObject< CCurve >(); curves[ CT_ScaleX ]->SetColor( Color::RED );
	curves[ CT_ScaleY ] = CreateObject< CCurve >(); curves[ CT_ScaleY ]->SetColor( Color::GREEN );
	curves[ CT_ScaleZ ] = CreateObject< CCurve >(); curves[ CT_ScaleZ ]->SetColor( Color::BLUE );

	AddCurves( curves, CT_SizeWithScale );
	FillCurvesWithScale( curves, data, times );

	wxTreeItemId rootTrans = m_wxTree->AppendItem( root, wxT("Trans"), -1, -1, new TreeData( TDT_TransNode, NULL ) );
	wxTreeItemId rootRot = m_wxTree->AppendItem( root, wxT("Rot"), -1, -1, new TreeData( TDT_RotNode, NULL ) );
	wxTreeItemId rootScale = m_wxTree->AppendItem( root, wxT("Scale"), -1, -1, new TreeData( TDT_ScaleNode, NULL ) );

	wxTreeItemId itemTX = m_wxTree->AppendItem( rootTrans, wxT("X"), -1, -1, new TreeData( TDT_TransX, curves[ CT_PosX ]  ) );
	wxTreeItemId itemTY = m_wxTree->AppendItem( rootTrans, wxT("Y"), -1, -1, new TreeData( TDT_TransY, curves[ CT_PosY ]  ) );
	wxTreeItemId itemTZ = m_wxTree->AppendItem( rootTrans, wxT("Z"), -1, -1, new TreeData( TDT_TransZ, curves[ CT_PosZ ]  ) );

	wxTreeItemId itemRX = m_wxTree->AppendItem( rootRot, wxT("X"), -1, -1, new TreeData( TDT_RotX, curves[ CT_RotX ]  ) );
	wxTreeItemId itemRY = m_wxTree->AppendItem( rootRot, wxT("Y"), -1, -1, new TreeData( TDT_RotY, curves[ CT_RotY ]  ) );
	wxTreeItemId itemRZ = m_wxTree->AppendItem( rootRot, wxT("Z"), -1, -1, new TreeData( TDT_RotZ, curves[ CT_RotZ ]  ) );

	wxTreeItemId itemSX = m_wxTree->AppendItem( rootScale, wxT("X"), -1, -1, new TreeData( TDT_ScaleX, curves[ CT_ScaleX ]  ) );
	wxTreeItemId itemSY = m_wxTree->AppendItem( rootScale, wxT("Y"), -1, -1, new TreeData( TDT_ScaleY, curves[ CT_ScaleY ]  ) );
	wxTreeItemId itemSZ = m_wxTree->AppendItem( rootScale, wxT("Z"), -1, -1, new TreeData( TDT_ScaleZ, curves[ CT_ScaleZ ]  ) );

	m_wxTree->SetItemTextColour( itemTX, *wxRED );
	m_wxTree->SetItemTextColour( itemTY, *wxGREEN );
	m_wxTree->SetItemTextColour( itemTZ, *wxBLUE );

	m_wxTree->SetItemTextColour( itemRX, *wxRED );
	m_wxTree->SetItemTextColour( itemRY, *wxGREEN );
	m_wxTree->SetItemTextColour( itemRZ, *wxBLUE );

	m_wxTree->SetItemTextColour( itemSX, *wxRED );
	m_wxTree->SetItemTextColour( itemSY, *wxGREEN );
	m_wxTree->SetItemTextColour( itemSZ, *wxBLUE );

	return root;
}

void CEdAnimPlotTree::Fill( const CSkeletalAnimation* animation, const CSkeleton* skeleton )
{
	if ( animation->GetUncompressedMotion() )
	{
		m_fps =  (Float)(animation->GetUncompressedMotion()->GetUncompressedFrames().Size()-1) / animation->GetUncompressedMotion()->GetDuration();
	}
}

void CEdAnimPlotTree::FillSkeletonData( const CSkeleton* skeleton, wxTreeItemId parent, Int32 parentIndex, TDynArray< SBehaviorGraphOutput >& data )
{
	for ( Int32 i=0; i<skeleton->GetBonesNum(); i++ )
	{
		Int32 boneParentIndex = skeleton->GetParentIndices()[ i ];

		if ( boneParentIndex == parentIndex )
		{
			wxTreeItemId child = m_wxTree->AppendItem( parent, skeleton->GetBoneName( i ).AsChar(), -1, -1, new TreeData( TDT_BoneNode, NULL ) );
			TDynArray< AnimQsTransform > boneData;
			boneData.Resize( data.Size() );

			for ( Uint32 b=0; b<data.Size(); ++b )
			{
#ifdef USE_HAVOK_ANIMATION
				boneData[ b ] = data[ b ].m_outputPose ? data[ b ].m_outputPose[ i ] : hkQsTransform::getIdentity();
#else
				boneData[ b ] = data[ b ].m_outputPose ? data[ b ].m_outputPose[ i ] : RedQsTransform::IDENTITY;
#endif
			}

			AddNodeWithScale( child, wxT("Data"), TDT_BoneData, boneData );

			if ( !m_wxTree->ItemHasChildren( parent ) ) 
			{
				m_wxTree->SetItemHasChildren( parent, true );
			}

			FillSkeletonData( skeleton, child, i, data );
		}
	}
}

void CEdAnimPlotTree::FillSkeletonTrackData( const CSkeleton* skeleton, wxTreeItemId parent, Int32 parentIndex, TDynArray< SBehaviorGraphOutput >& data )
{
	wxTreeItemId tracksItem = m_wxTree->AppendItem( parent, wxT("Tracks"), -1, -1, new TreeData( TDT_TrackNodes, NULL ) );
	for ( Int32 i=0; i<skeleton->GetTracksNum(); i++ )
	{
		CCurve* curve = CreateObject< CCurve >(); curve->SetColor( GetColorFromTrackNum( i ) );
		AddCurve( curve );

		const Float factor = 1.f / m_fps;

		for ( Uint32 b=0; b<data.Size(); ++b )
		{
			Float x = b * factor;
			Float y = data[ b ].m_floatTracks ? data[ b ].m_floatTracks[ i ] : 0.f;

			curve->GetCurveData().AddPoint( x, y );
		}

		wxTreeItemId track = m_wxTree->AppendItem( tracksItem, skeleton->GetTrackName( i ).AsChar(), -1, -1, new TreeData( TDT_Track, curve ) );
	}
}

void CEdAnimPlotTree::FillCurves( CCurve** curve, const TDynArray< AnimQsTransform >& data, TDynArray< Float >* times )
{
	Matrix mat;

	Float factor = 1.f / m_fps;

	for ( Uint32 i=0; i<data.Size(); ++i )
	{
#ifdef USE_HAVOK_ANIMATION
		const hkQsTransform& trans = data[ i ];
		HavokTransformToMatrix_Renormalize( trans, &mat );
#else
		const RedQsTransform& trans = data[ i ];
		RedMatrix4x4 conversionMatrix = trans.ConvertToMatrixNormalized();
		mat = reinterpret_cast< const Matrix& >( conversionMatrix );
#endif
		const Vector& pos = mat.GetTranslationRef();
		const EulerAngles rot = mat.ToEulerAngles();

		Float x = times ? (*times)[ i ] : i * factor;

		curve[ CT_PosX ]->GetCurveData().AddPoint( x, pos.X );
		curve[ CT_PosY ]->GetCurveData().AddPoint( x, pos.Y );
		curve[ CT_PosZ ]->GetCurveData().AddPoint( x, pos.Z );

		curve[ CT_RotX ]->GetCurveData().AddPoint( x, rot.Pitch );
		curve[ CT_RotY ]->GetCurveData().AddPoint( x, rot.Roll );
		curve[ CT_RotZ ]->GetCurveData().AddPoint( x, rot.Yaw );
	}
}

void CEdAnimPlotTree::FillCurvesWithScale( CCurve** curve, const TDynArray< AnimQsTransform >& data, TDynArray< Float >* times )
{
	Matrix mat;

	Float factor = 1.f / m_fps;

	for ( Uint32 i=0; i<data.Size(); ++i )
	{
#ifdef USE_HAVOK_ANIMATION
		const hkQsTransform& trans = data[ i ];
		HavokTransformToMatrix_Renormalize( trans, &mat );
#else
		const RedQsTransform& trans = data[ i ];
		RedMatrix4x4 conversionMatrix = trans.ConvertToMatrixNormalized();
		mat = reinterpret_cast< const Matrix& >( conversionMatrix );
		
#endif

		const Vector& pos = mat.GetTranslationRef();
		const EulerAngles rot = mat.ToEulerAngles();
#ifdef USE_HAVOK_ANIMATION
		const hkVector4& scale = trans.m_scale;
#else
		const RedVector4& scale = trans.Scale;
#endif
		Float x = times ? (*times)[ i ] : i * factor;

		curve[ CT_PosX ]->GetCurveData().AddPoint( x, pos.X );
		curve[ CT_PosY ]->GetCurveData().AddPoint( x, pos.Y );
		curve[ CT_PosZ ]->GetCurveData().AddPoint( x, pos.Z );

		curve[ CT_RotX ]->GetCurveData().AddPoint( x, rot.Pitch );
		curve[ CT_RotY ]->GetCurveData().AddPoint( x, rot.Roll );
		curve[ CT_RotZ ]->GetCurveData().AddPoint( x, rot.Yaw );
#ifdef USE_HAVOK_ANIMATION
		curve[ CT_ScaleX ]->GetCurveData().AddPoint( x, scale( 0 ) );
		curve[ CT_ScaleY ]->GetCurveData().AddPoint( x, scale( 1 ) );
		curve[ CT_ScaleZ ]->GetCurveData().AddPoint( x, scale( 2 ) );
#else
		curve[ CT_ScaleX ]->GetCurveData().AddPoint( x, scale.X );
		curve[ CT_ScaleY ]->GetCurveData().AddPoint( x, scale.Y );
		curve[ CT_ScaleZ ]->GetCurveData().AddPoint( x, scale.Z );
#endif
	}
}

void CEdAnimPlotTree::ExtractMotionExtraction( const CSkeletalAnimation* animation, TDynArray< AnimQsTransform >& data, TDynArray< Float >* times )
{
	Float timeDelta = 1.f / (Float)m_fps;
	Float dur = animation->GetDuration();

	Uint32 num = (Uint32)(dur / timeDelta) + 2;
	data.Resize( num );

	if ( times )
	{
		times->Resize( num );
	}

	for ( Uint32 i=0; i<data.Size()-1; ++i )
	{
		if ( animation->GetUncompressedMotion() )
		{
			animation->GetUncompressedMotion()->GetMovementAtTime( timeDelta * i, data[ i ] );
		}
		else
		{
			data[ i ] = animation->GetMovementAtTime( timeDelta * i );
		}

		if ( times )
		{
			(*times)[ i ] = timeDelta * i;
		}
	}

	if ( animation->GetUncompressedMotion() )
	{
		animation->GetUncompressedMotion()->GetMovementAtTime( dur, data[ data.Size()-1 ] );
	}
	else
	{
		data[ data.Size()-1 ] = animation->GetMovementAtTime( dur );
	}

	if ( times )
	{
		(*times)[ data.Size()-1 ] = dur;
	}
}

void CEdAnimPlotTree::PrepareBoneData( const CSkeletalAnimation* animation, TDynArray< SBehaviorGraphOutput >& data )
{
	Float timeDelta = 1.f / (Float)m_fps;
	Float dur = animation->GetDuration();

	Uint32 num = (Uint32)(dur / timeDelta) + 2;
	data.Resize( num );

	for ( Uint32 i=0; i<data.Size(); ++i )
	{
		SBehaviorGraphOutput& pose = data[ i ];
		pose.Init( animation->GetBonesNum(), animation->GetTracksNum() );
	}
}

void CEdAnimPlotTree::ReleaseBoneData( const CSkeletalAnimation* animation, TDynArray< SBehaviorGraphOutput >& data )
{
	for ( Uint32 i=0; i<data.Size(); ++i )
	{
		SBehaviorGraphOutput& pose = data[ i ];
		pose.Deinit();
	}
}

void CEdAnimPlotTree::SampleBones( const CSkeletalAnimation* animation, TDynArray< SBehaviorGraphOutput >& data, const CSkeleton* skeleton )
{
	Float timeDelta = 1.f / (Float)m_fps;
	Float dur = animation->GetDuration();

	Float time;

	for ( Uint32 i=0; i<data.Size(); ++i )
	{
		time = Min( timeDelta * i, dur );

		SBehaviorGraphOutput& pose = data[ i ];
		animation->Sample( time, pose.m_numBones, pose.m_numFloatTracks, pose.m_outputPose, pose.m_floatTracks );
	}

	ASSERT( MAbs( dur - time ) < 0.01f );
}

String CEdAnimPlotTree::GetNameFromType( ETreeDataType type ) const
{
	switch ( type )
	{
	case TDT_TransX:
		return TXT( "TransX" );
	case TDT_TransY:
		return TXT( "TransY" );
	case TDT_TransZ:
		return TXT( "TransZ" );
	case TDT_RotX:
		return TXT( "RotX" );
	case TDT_RotY:
		return TXT( "RotY" );
	case TDT_RotZ:
		return TXT( "RotZ" );
	case TDT_ScaleX:
		return TXT( "ScaleX" );
	case TDT_ScaleY:
		return TXT( "ScaleY" );
	case TDT_ScaleZ:
		return TXT( "ScaleZ" );
	case TDT_Track:
		return TXT( "Track" );
	default:
		return TXT( "Invalid" );
	}
}

Color CEdAnimPlotTree::GetColorFromTrackNum( Int32 num ) const
{
	return Color::RED;
}

void CEdAnimPlotTree::OnItemSelected( wxTreeEvent& event )
{
	
}

void CEdAnimPlotTree::OnItemActivated( wxTreeEvent& event )
{
	if ( m_hook )
	{
		wxTreeItemId item = m_wxTree->GetSelection();
		if ( item.IsOk() )
		{
			TreeData* treeData = static_cast< TreeData* >( m_wxTree->GetItemData( item ) );

			if ( treeData->GetCurve() )
			{
				TPlotCurveData data;

				data.m_first = GetNameFromType( treeData->GetType() );
				data.m_second = treeData->GetCurve();

				m_hook->OnCurveSelected( data );
			}
			else if ( treeData->GetType() == TDT_TransNode || treeData->GetType() == TDT_RotNode || treeData->GetType() == TDT_ScaleNode )
			{
				TDynArray< TPlotCurveData > datas;

				wxTreeItemIdValue cookie;
				for ( wxTreeItemId currItem = m_wxTree->GetFirstChild( item, cookie ); currItem.IsOk(); currItem = m_wxTree->GetNextChild( currItem, cookie ) )
				{
					TreeData* treeChildData = static_cast< TreeData* >( m_wxTree->GetItemData( currItem ) );

					if ( treeChildData->GetCurve() )
					{
						TPlotCurveData childData;

						childData.m_first = GetNameFromType( treeChildData->GetType() );
						while ( ExistsInList( datas, childData.m_first ) )
						{
							childData.m_first += wxT(".");
						}

						childData.m_second = treeChildData->GetCurve();

						datas.PushBack( childData );
					}
				}

				m_hook->OnCurvesSelected( datas );
			}
		}
	}
}

Bool CEdAnimPlotTree::ExistsInList( const TDynArray< TPlotCurveData >& datas, const String& str ) const
{
	for ( Uint32 i=0; i<datas.Size(); ++i )
	{
		if ( datas[ i ].m_first == str )
		{
			return true;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////

CEdAnimBoneMSPlotTree::CEdAnimBoneMSPlotTree( wxWindow* parent )
	: CEdAnimBonePlotTree( parent )
{

}

void CEdAnimBoneMSPlotTree::SampleBones( const CSkeletalAnimation* animation, TDynArray< SBehaviorGraphOutput >& data, const CSkeleton* skeleton )
{
	CEdAnimPlotTree::SampleBones( animation, data, skeleton );

	const Uint32 bonesNum = data.Size() > 0 ? data[ 0 ].m_numBones : 0;
	TDynArray< AnimQsTransform > bonesMS;
	bonesMS.Resize( bonesNum );

	for ( Uint32 i=0; i<data.Size(); ++i )
	{
		SBehaviorGraphOutput& pose = data[ i ];
		pose.GetBonesModelSpace( skeleton, bonesMS );

		for ( Uint32 j=0; j<pose.m_numBones; ++j )
		{
			pose.m_outputPose[ j ] = bonesMS[ j ];
		}
	}
}

//////////////////////////////////////////////////////////////////////////

CEdAnimBonePlotTree::CEdAnimBonePlotTree( wxWindow* parent )
	: CEdAnimPlotTree( parent )
{

}

void CEdAnimBonePlotTree::Fill( const CSkeletalAnimation* animation, const CSkeleton* skeleton )
{
	CEdAnimPlotTree::Fill( animation, skeleton );

	Clear();

	m_wxTree->Freeze();

	if ( !animation )
	{
		m_wxTree->AddRoot( wxT("Empty") );
		m_wxTree->Thaw();
		return;
	}

	wxTreeItemId treeRoot = m_wxTree->AddRoot( animation->GetName().AsString().AsChar() );

	Uint32 bones = animation->GetBonesNum();
	Uint32 tracks = animation->GetTracksNum();

	// Motion extraction
	{
		TDynArray< AnimQsTransform > data;
		ExtractMotionExtraction( animation, data );
		AddNode( treeRoot, wxT("Motion extraction"), TDT_MotionExNode, data );
	}

	// Bones
	if ( skeleton )
	{
		TDynArray< SBehaviorGraphOutput > data;
		PrepareBoneData( animation, data );

		SampleBones( animation, data, skeleton );

		FillSkeletonData( skeleton, treeRoot, -1, data );
		FillSkeletonTrackData( skeleton, treeRoot, -1, data );

		ReleaseBoneData( animation, data );
	}

	m_wxTree->Expand( treeRoot );
	m_wxTree->Thaw();
}

//////////////////////////////////////////////////////////////////////////

CEdAnimMotionTree::CEdAnimMotionTree( wxWindow* parent )
	: CEdAnimPlotTree( parent )
{
	m_compression = CreateObject< CMotionExtractionLineCompression >();
	m_compression2 = CreateObject< CMotionExtractionLineCompression2 >();
	m_compression->AddToRootSet();
	m_compression2->AddToRootSet();
}

CEdAnimMotionTree::~CEdAnimMotionTree()
{
	m_compression->RemoveFromRootSet();
	m_compression2->RemoveFromRootSet();
}

void CEdAnimMotionTree::Fill( const CSkeletalAnimation* animation, const CSkeleton* skeleton )
{
	CEdAnimPlotTree::Fill( animation, skeleton );

	Clear();

	m_wxTree->Freeze();

	if ( !animation )
	{
		m_wxTree->AddRoot( wxT("Empty") );
		m_wxTree->Thaw();
		return;
	}

	wxTreeItemId treeRoot = m_wxTree->AddRoot( animation->GetName().AsString().AsChar() );

	// Motion extraction
	{
		TDynArray< AnimQsTransform > data1;
		TDynArray< AnimQsTransform > data2;
		TDynArray< Float > times1;
		TDynArray< Float > times2;

		ExtractMotionExtraction( animation, data1, &times1 );
		ExtractCompressedMotionExtraction( animation, data2, times2 );

		AddDoubleNode( treeRoot, wxT("Motion extraction"), TDT_MotionExNode, data1, data2, &times1, &times2 );
	}

	m_wxTree->Expand( treeRoot );
	m_wxTree->Thaw();
}

void CEdAnimMotionTree::ExtractCompressedMotionExtraction( const CSkeletalAnimation* animation, TDynArray< AnimQsTransform >& data, TDynArray< Float >& times )
{
	// Uncompressed animation
	const CUncompressedMotionExtraction* uncompressedMotion = animation->GetUncompressedMotion();
	if ( !uncompressedMotion )
	{
		return;
	}

	// Compressed animation
	ASSERT( m_compression );
	ASSERT( m_compression2 );

	static Bool X = false;
	if ( X )
	{
		CLineMotionExtraction* lineMotion = SafeCast< CLineMotionExtraction >( m_compression->Compress( uncompressedMotion ) );
		if ( lineMotion )
		{
			// Sample compressed animation
			const Float timeDelta = 1.f / 30.f;
			const Float dur = lineMotion->GetDuration();

			const TDynArray< Float >& frameTimes = lineMotion->GetFrameTimes();
			times = frameTimes;

			ASSERT( MAbs( uncompressedMotion->GetDuration() - times.Back() ) < 0.001f );

			data.Resize( frameTimes.Size() );

			for ( Uint32 i=0; i<frameTimes.Size(); ++i )
			{
				Float time = frameTimes[ i ];
				AnimQsTransform trans;
				lineMotion->GetMovementAtTime( time, trans );
				data[ i ] = trans;
			}

			delete lineMotion;
		}
	}
	else
	{
		CLineMotionExtraction2* lineMotion = SafeCast< CLineMotionExtraction2 >( m_compression2->Compress( uncompressedMotion ) );
		if ( lineMotion )
		{
			// Sample compressed animation
			lineMotion->GetFrameTimes( times );

			//ASSERT( MAbs( uncompressedMotion->m_duration / ( uncompressedMotion->m_frames.Size() - 1 ) - CLineMotionExtraction2::TIME_SLICE ) < 0.001f );
			ASSERT( MAbs( uncompressedMotion->GetDuration() - times.Back() ) < 0.001f );

			data.Resize( times.Size() );

			for ( Uint32 i=0; i<times.Size(); ++i )
			{
				Float time = times[ i ];
				AnimQsTransform trans;
				lineMotion->GetMovementAtTime( time, trans );

				data[ i ] = trans;
			}

			delete lineMotion;
		}
	}
}

wxTreeItemId CEdAnimMotionTree::AddDoubleNode(	wxTreeItemId treeRoot, wxString nodeName, ETreeDataType nodeType, 
												TDynArray< AnimQsTransform >& data1, TDynArray< AnimQsTransform >& data2,
												TDynArray< Float >* times1, TDynArray< Float >* times2 )
{
	wxTreeItemId root = m_wxTree->AppendItem( treeRoot, nodeName, -1, -1, new TreeData( nodeType, NULL ) );

	CCurve* curves1[ CT_Size ];
	CCurve* curves2[ CT_Size ];

	curves1[ CT_PosX ] = CreateObject< CCurve >(); curves1[ CT_PosX ]->SetColor( Color::RED );
	curves1[ CT_PosY ] = CreateObject< CCurve >(); curves1[ CT_PosY ]->SetColor( Color::GREEN );
	curves1[ CT_PosZ ] = CreateObject< CCurve >(); curves1[ CT_PosZ ]->SetColor( Color::BLUE );

	curves1[ CT_RotX ] = CreateObject< CCurve >(); curves1[ CT_RotX ]->SetColor( Color::RED );
	curves1[ CT_RotY ] = CreateObject< CCurve >(); curves1[ CT_RotY ]->SetColor( Color::GREEN );
	curves1[ CT_RotZ ] = CreateObject< CCurve >(); curves1[ CT_RotZ ]->SetColor( Color::BLUE );

	const Float factor = 0.6f;

	curves2[ CT_PosX ] = CreateObject< CCurve >(); curves2[ CT_PosX ]->SetColor( Color::Mul3( Color::RED, factor ) );
	curves2[ CT_PosY ] = CreateObject< CCurve >(); curves2[ CT_PosY ]->SetColor( Color::Mul3( Color::GREEN, factor ) );
	curves2[ CT_PosZ ] = CreateObject< CCurve >(); curves2[ CT_PosZ ]->SetColor( Color::Mul3( Color::BLUE, factor ) );

	curves2[ CT_RotX ] = CreateObject< CCurve >(); curves2[ CT_RotX ]->SetColor( Color::Mul3( Color::RED, factor ));
	curves2[ CT_RotY ] = CreateObject< CCurve >(); curves2[ CT_RotY ]->SetColor( Color::Mul3( Color::GREEN, factor ) );
	curves2[ CT_RotZ ] = CreateObject< CCurve >(); curves2[ CT_RotZ ]->SetColor( Color::Mul3( Color::BLUE, factor ) );

	AddCurves( curves1, CT_Size );
	FillCurves( curves1, data1, times1 );

	AddCurves( curves2, CT_Size );
	FillCurves( curves2, data2, times2 );

	wxTreeItemId rootTrans = m_wxTree->AppendItem( root, wxT("Trans"), -1, -1, new TreeData( TDT_TransNode, NULL ) );
	wxTreeItemId rootRot = m_wxTree->AppendItem( root, wxT("Rot"), -1, -1, new TreeData( TDT_RotNode, NULL ) );

	wxTreeItemId itemTX1 = m_wxTree->AppendItem( rootTrans, wxT("X - uncompressed"), -1, -1, new TreeData( TDT_TransX, curves1[ CT_PosX ]  ) );
	wxTreeItemId itemTY1 = m_wxTree->AppendItem( rootTrans, wxT("Y - uncompressed"), -1, -1, new TreeData( TDT_TransY, curves1[ CT_PosY ]  ) );
	wxTreeItemId itemTZ1 = m_wxTree->AppendItem( rootTrans, wxT("Z - uncompressed"), -1, -1, new TreeData( TDT_TransZ, curves1[ CT_PosZ ]  ) );

	wxTreeItemId itemRX1 = m_wxTree->AppendItem( rootRot, wxT("X - uncompressed"), -1, -1, new TreeData( TDT_RotX, curves1[ CT_RotX ]  ) );
	wxTreeItemId itemRY1 = m_wxTree->AppendItem( rootRot, wxT("Y - uncompressed"), -1, -1, new TreeData( TDT_RotY, curves1[ CT_RotY ]  ) );
	wxTreeItemId itemRZ1 = m_wxTree->AppendItem( rootRot, wxT("Z - uncompressed"), -1, -1, new TreeData( TDT_RotZ, curves1[ CT_RotZ ]  ) );

	wxTreeItemId itemTX2 = m_wxTree->AppendItem( rootTrans, wxT("X - compressed"), -1, -1, new TreeData( TDT_TransX, curves2[ CT_PosX ]  ) );
	wxTreeItemId itemTY2 = m_wxTree->AppendItem( rootTrans, wxT("Y - compressed"), -1, -1, new TreeData( TDT_TransY, curves2[ CT_PosY ]  ) );
	wxTreeItemId itemTZ2 = m_wxTree->AppendItem( rootTrans, wxT("Z - compressed"), -1, -1, new TreeData( TDT_TransZ, curves2[ CT_PosZ ]  ) );

	wxTreeItemId itemRX2 = m_wxTree->AppendItem( rootRot, wxT("X - compressed"), -1, -1, new TreeData( TDT_RotX, curves2[ CT_RotX ]  ) );
	wxTreeItemId itemRY2 = m_wxTree->AppendItem( rootRot, wxT("Y - compressed"), -1, -1, new TreeData( TDT_RotY, curves2[ CT_RotY ]  ) );
	wxTreeItemId itemRZ2 = m_wxTree->AppendItem( rootRot, wxT("Z - compressed"), -1, -1, new TreeData( TDT_RotZ, curves2[ CT_RotZ ]  ) );

	m_wxTree->SetItemTextColour( itemTX1, *wxRED );
	m_wxTree->SetItemTextColour( itemTY1, *wxGREEN );
	m_wxTree->SetItemTextColour( itemTZ1, *wxBLUE );

	m_wxTree->SetItemTextColour( itemRX1, *wxRED );
	m_wxTree->SetItemTextColour( itemRY1, *wxGREEN );
	m_wxTree->SetItemTextColour( itemRZ1, *wxBLUE );

	m_wxTree->SetItemTextColour( itemTX2, *wxRED );
	m_wxTree->SetItemTextColour( itemTY2, *wxGREEN );
	m_wxTree->SetItemTextColour( itemTZ2, *wxBLUE );

	m_wxTree->SetItemTextColour( itemRX2, *wxRED );
	m_wxTree->SetItemTextColour( itemRY2, *wxGREEN );
	m_wxTree->SetItemTextColour( itemRZ2, *wxBLUE );

	return root;
}

//////////////////////////////////////////////////////////////////////////

CEdAnimBonePlot::CEdAnimBonePlot( wxWindow* parent, const CSkeletalAnimationSetEntry* anim, const CSkeleton* skeleton )
	: CEdAnimPlot( parent, anim, skeleton )
{
	// Create tree
	wxPanel* rp2 = XRCCTRL( *this, "treePanel", wxPanel );
	wxBoxSizer* sizer2 = new wxBoxSizer( wxVERTICAL );		
	m_tree = new CEdAnimBonePlotTree( rp2 );
	m_tree->SetHook( this );
	sizer2->Add( m_tree, 1, wxEXPAND, 0 );
	rp2->SetSizer( sizer2 );		
	rp2->Layout();

	// Fill tree
	m_tree->Fill( anim->GetAnimation(), skeleton );

	// Update and finalize layout
	Layout();
	Show();
}

CEdAnimBonePlot::~CEdAnimBonePlot()
{

}

//////////////////////////////////////////////////////////////////////////

CEdAnimBoneMSPlot::CEdAnimBoneMSPlot( wxWindow* parent, const CSkeletalAnimationSetEntry* anim, const CSkeleton* skeleton )
	: CEdAnimPlot( parent, anim, skeleton )
{
	// Create tree
	wxPanel* rp2 = XRCCTRL( *this, "treePanel", wxPanel );
	wxBoxSizer* sizer2 = new wxBoxSizer( wxVERTICAL );		
	m_tree = new CEdAnimBoneMSPlotTree( rp2 );
	m_tree->SetHook( this );
	sizer2->Add( m_tree, 1, wxEXPAND, 0 );
	rp2->SetSizer( sizer2 );		
	rp2->Layout();

	// Fill tree
	m_tree->Fill( anim->GetAnimation(), skeleton );

	// Update and finalize layout
	Layout();
	Show();
}

CEdAnimBoneMSPlot::~CEdAnimBoneMSPlot()
{

}

//////////////////////////////////////////////////////////////////////////

CEdMotionExtractionPreview::CEdMotionExtractionPreview( wxWindow* parent, const CSkeletalAnimationSetEntry* anim, const CSkeleton* skeleton )
	: CEdAnimPlot( parent, anim, skeleton )
{
	// Create tree
	wxPanel* treePanel = XRCCTRL( *this, "treePanel", wxPanel );
	wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );

	{
		wxPanel* panel93 = new wxPanel( treePanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
		wxBoxSizer* bSizer209 = new wxBoxSizer( wxVERTICAL );

		m_tree = new CEdAnimMotionTree( panel93 );
		m_tree->SetHook( this );

		bSizer209->Add( m_tree, 1, wxALL|wxEXPAND, 5 );

		panel93->SetSizer( bSizer209 );
		panel93->Layout();
		bSizer209->Fit( panel93 );

		sizer->Add( panel93, 1, wxEXPAND | wxALL, 5 );
	}
	
	{
		wxPanel* panel94 = new wxPanel( treePanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
		wxBoxSizer* bSizer210 = new wxBoxSizer( wxVERTICAL );

		// Properties panel
		{
			PropertiesPageSettings settings;
			CEdPropertiesPage* panel95 = new CEdPropertiesPage( panel94, settings, nullptr );
			panel95->SetObject( m_tree->GetCompression() );
			bSizer210->Add( panel95, 1, wxEXPAND | wxALL, 5 );
			panel95->SetMaxSize( wxSize( -1, 100 ) );
		}
		{
			PropertiesPageSettings settings;
			CEdPropertiesPage* panel95 = new CEdPropertiesPage( panel94, settings, nullptr );
			panel95->SetObject( m_tree->GetCompression2() );
			bSizer210->Add( panel95, 1, wxEXPAND | wxALL, 5 );
			panel95->SetMaxSize( wxSize( -1, 100 ) );
		}

		wxButton* button69 = new wxButton( panel94, wxID_ANY, wxT("Recompress"), wxDefaultPosition, wxDefaultSize, 0 );
		button69->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdMotionExtractionPreview::OnRecompress ), NULL, this );
		bSizer210->Add( button69, 0, wxALL|wxEXPAND, 5 );

		panel94->SetSizer( bSizer210 );
		panel94->Layout();
		bSizer210->Fit( panel94 );

		sizer->Add( panel94, 1, wxEXPAND | wxALL, 5 );
	}

	m_skeleton = skeleton;
	m_animation = anim;

	treePanel->SetSizer( sizer );
	treePanel->Layout();

	// Fill tree
	m_tree->Fill( anim->GetAnimation(), skeleton );

	// Update and finalize layout
	Layout();
	Show();
}

CEdMotionExtractionPreview::~CEdMotionExtractionPreview()
{

}

void CEdMotionExtractionPreview::OnRecompress( wxCommandEvent& event )
{
	const CSkeletalAnimationSetEntry* anim = m_animation;
	const CSkeleton* skeleton = m_skeleton.Get();

	if ( anim && skeleton )
	{
		m_curveEditorCanvas->RemoveAllCurves();
		
		m_tree->ClearAndFill( anim->GetAnimation(), skeleton );

		m_curveEditorCanvas->Repaint();
	}
}
