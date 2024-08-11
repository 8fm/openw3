
#include "build.h"
#include "animBuilderTimeline.h"
#include "animBuilder.h"
#include "defaultColorsIterator.h"
#include "callbackData.h"

#include "../../common/core/depot.h"

const String	CEdAnimBuilderTimeline::TRACK_BASE( TXT("1 Base") );
const String	CEdAnimBuilderTimeline::TRACK_OVERRIDE( TXT("2 Override") );
const String	CEdAnimBuilderTimeline::TRACK_ADDITIVE( TXT("3 Additive") );
const String	CEdAnimBuilderTimeline::TRACK_FK( TXT("5 FK") );
const String	CEdAnimBuilderTimeline::TRACK_IK( TXT("6 IK") );
const String	CEdAnimBuilderTimeline::TRACK_MOTION( TXT("7 Motion") );
const Float		CEdAnimBuilderTimeline::STATIC_OFFSET = 2.f;

enum ETrackEvents
{
	ID_NEW_VANIM_TRACK							= 5001,
	ID_NEW_VANIM_TRACK_LAST						= 5010,
	ID_DEBUG_ITEM								= 5011,
	ID_DEBUG_ITEM_LAST							= 5100,
	ID_ITEM_SNAP_TO_ZERO						= 5101,
	ID_ITEM_SNAP_TO_PREV_TACK_ITEM				= 5102,
	ID_ITEM_SNAP_TO_PREV_ITEM					= 5102,
	ID_ITEM_SNAP_TO_PREV_TACK_ITEM_INPUT		= 5103,
	ID_ITEM_SNAP_TO_PREV_ITEM_INPUT				= 5104,
	ID_ITEM_SNAP_TO_NEXT_TACK_ITEM				= 5105,
	ID_ITEM_SNAP_TO_NEXT_ITEM					= 5106,
	ID_ITEM_SNAP_TO_NEXT_TACK_ITEM_INPUT		= 5107,
	ID_ITEM_SNAP_TO_NEXT_ITEM_INPUT				= 5108,
	ID_ITEM_SNAP_CLOSEST_EDGE_TRACK				= 5109,
	ID_ITEM_SNAP_CLOSEST_EDGE					= 5110,
	ID_ITEM_SNAP_CLOSEST_EDGE_TRACK_INPUT		= 5111,
	ID_ITEM_SNAP_CLOSEST_EDGE_INPUT				= 5112,
	ID_ITEM_MOVE								= 5113,
	ID_REMOVE_ITEM								= 5114,
	ID_EDIT_BONES								= 5115,
	ID_ITEM_DUPLICATE							= 5116,
	ID_CALC_AUTO_BLENDS							= 5117,
	ID_ADD_MOTION								= 5118,
	ID_ADD_FK									= 5119,
	ID_ADD_IK									= 5120,
};

namespace
{
	template< class T > T* ItemCast( VirtualAnimTimelineItem* obj )
	{
		return obj->ItemCast< T >();
	}

	template< class T > T* ItemCast( ITimelineItem* obj )
	{
		return ItemCast< T >( static_cast< VirtualAnimTimelineItem* >( obj ) );
	}

	template< class T > const T* ItemCast( const VirtualAnimTimelineItem* obj )
	{
		return obj->ItemCast< T >();
	}

	template< class T > const T* ItemCast( const ITimelineItem* obj )
	{
		return ItemCast< T >( static_cast< const VirtualAnimTimelineItem* >( obj ) );
	}
}

CEdAnimBuilderTimeline::CEdAnimBuilderTimeline( wxPanel* parent, CEdAnimationBuilder* builder )
	: CEdTimeline( parent )
	, m_builder( builder )
	, m_checkAutoBlendTimes( true )
{
	m_dragInfo.m_pending = false;
	m_activeRangeDuration = 1.f;

	RegisterDrawGroupTracksPinned(new TimelineImpl::CDrawGroupTracks(*this, &m_pinnedBuffer));
	RegisterDrawGroupTracksDefault(new TimelineImpl::CDrawGroupTracks(*this, &m_defaultBuffer));
	RegisterDrawGroupTimebar(new TimelineImpl::CDrawGroupTimebar(*this, &m_timebarBuffer));
	RegisterDrawGroupVolatile(new TimelineImpl::CDrawGroupVolatile(*this, &m_volatileBuffer));
}

CEdAnimBuilderTimeline::~CEdAnimBuilderTimeline()
{}

void CEdAnimBuilderTimeline::Fill( const IVirtualAnimationContainer* container )
{
	m_container = container;
	
	m_tracks.ClearFast();
	m_pinnedGroup->RemoveAllTracks();
	m_defaultGroup->RemoveAllTracks();

	AddTrack( TRACK_BASE );
	AddTrack( TRACK_OVERRIDE );
	AddTrack( TRACK_ADDITIVE );
	AddTrack( TRACK_FK );
	AddTrack( TRACK_IK );
	AddTrack( TRACK_MOTION );

	RefreshTimeline();
}

void CEdAnimBuilderTimeline::RefreshTimeline()
{
	ClearItems();

	CreateItems( VAT_Base );
	CreateItems( VAT_Override );
	CreateItems( VAT_Additive );

	CreateItems_Motion();
	CreateItems_FK();
	CreateItems_IK();

	Float maxTime = GetMaxAnimationTime();
	if ( maxTime + STATIC_OFFSET > m_activeRangeDuration )
	{
		m_activeRangeDuration = maxTime + 2.f * STATIC_OFFSET;
	}

	m_visibleRangeDuration = m_activeRangeDuration * 2;
	CenterPosition( m_activeRangeDuration / 2 );

	CalculateNewGrid();
}

Float CEdAnimBuilderTimeline::GetMaxAnimationTime() const
{
	Float min = 0.f;
	Float max = 0.1f;

	const Uint32 size = m_items.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		const VirtualAnimTimelineItem* item = static_cast< const VirtualAnimTimelineItem* >( m_items[ i ] );

		if ( item->GetStart() > min )
		{
			min = item->GetStart();
		}

		if ( item->GetStart() + item->GetDuration() > max )
		{
			max = item->GetStart() + item->GetDuration();
		}
	}

	return max;
}

void CEdAnimBuilderTimeline::CreateItems( EVirtualAnimationTrack track )
{
	const TDynArray< VirtualAnimation >& animations = m_container->GetVirtualAnimations( track );

	const Uint32 size = animations.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		VirtualAnimationID id;
		id.m_track = track;
		id.m_index = i;
		VirtualAnimTimelineItem_Animation* item = new VirtualAnimTimelineItem_Animation( this, m_container, id );
		AddItem( item );
	}
}

void CEdAnimBuilderTimeline::CreateItems_Motion()
{
	const TDynArray< VirtualAnimationMotion >& motions = m_container->GetVirtualMotions();

	const Uint32 size = motions.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		VirtualAnimationMotionID id;
		id = i;
		VirtualAnimTimelineItem_Motion* item = new VirtualAnimTimelineItem_Motion( this, m_container, id );
		AddItem( item );
	}
}

void CEdAnimBuilderTimeline::CreateItems_FK()
{
	const TDynArray< VirtualAnimationPoseFK >& data = m_container->GetVirtualFKs();

	const Uint32 size = data.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		VirtualAnimationPoseFKID id;
		id = i;
		VirtualAnimTimelineItem_FK* item = new VirtualAnimTimelineItem_FK( this, m_container, id );
		AddItem( item );
	}
}

void CEdAnimBuilderTimeline::CreateItems_IK()
{
	const TDynArray< VirtualAnimationPoseIK >& data = m_container->GetVirtualIKs();

	const Uint32 size = data.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		VirtualAnimationPoseIKID id;
		id = i;
		VirtualAnimTimelineItem_IK* item = new VirtualAnimTimelineItem_IK( this, m_container, id );
		AddItem( item );
	}
}

void CEdAnimBuilderTimeline::RemoveItemImpl( ITimelineItem* item )
{
	if ( VirtualAnimTimelineItem_Animation* vaitem = ItemCast< VirtualAnimTimelineItem_Animation >( item ) )
	{
		m_builder->RequestRemoveVirtualAnimation( vaitem->m_animationID );
		RefreshTimeline();
	}
	else if ( VirtualAnimTimelineItem_Motion* mitem = ItemCast< VirtualAnimTimelineItem_Motion >( item ) )
	{
		m_builder->RequestRemoveVirtualMotion( mitem->m_motionID );
		RefreshTimeline();
	}
	else if ( VirtualAnimTimelineItem_FK* fkitem = ItemCast< VirtualAnimTimelineItem_FK >( item ) )
	{
		m_builder->RequestRemoveVirtualFK( fkitem->m_dataID );
		RefreshTimeline();
	}
	else if ( VirtualAnimTimelineItem_IK* ikitem = ItemCast< VirtualAnimTimelineItem_IK >( item ) )
	{
		m_builder->RequestRemoveVirtualFK( ikitem->m_dataID );
		RefreshTimeline();
	}
}

void CEdAnimBuilderTimeline::StoreLayout()
{
	// empty - this functionality is not supported
}

void CEdAnimBuilderTimeline::RestoreLayout()
{
	// empty - this functionality is not supported
}

void CEdAnimBuilderTimeline::SelectionChanged()
{
	const Int32 num = GetSelectedItemCount();
	if ( num == 1 )
	{
		ITimelineItem* item = GetSelectedItem( 0 );

		VirtualAnimTimelineItem_Animation* vaitem = ItemCast< VirtualAnimTimelineItem_Animation >( item );
		if ( vaitem )
		{
			m_builder->SelectAnimationEvent( vaitem->m_animationID );
			return;
		}

		VirtualAnimTimelineItem_Motion* mitem = ItemCast< VirtualAnimTimelineItem_Motion >( item );
		if ( mitem )
		{
			m_builder->SelectMotionEvent( mitem->m_motionID );
			return;
		}

		VirtualAnimTimelineItem_FK* fkitem = ItemCast< VirtualAnimTimelineItem_FK >( item );
		if ( fkitem )
		{
			m_builder->SelectFKEvent( fkitem->m_dataID );
			return;
		}

		VirtualAnimTimelineItem_IK* ikitem = ItemCast< VirtualAnimTimelineItem_IK >( item );
		if ( ikitem )
		{
			m_builder->SelectIKEvent( ikitem->m_dataID );
			return;
		}
	}
	else
	{
		m_builder->DeselectEvent();
	}
}

void CEdAnimBuilderTimeline::EditProperties( ITimelineItem* item, CEdPropertiesPage& propertiesPage ) const
{	
	if( item == NULL )
	{
		return;
	}

	CEdAnimBuilderTimeline* tl = const_cast< CEdAnimBuilderTimeline* >( this );

	VirtualAnimTimelineItem_Animation* vaitem = ItemCast< VirtualAnimTimelineItem_Animation >( item );
	if ( vaitem )
	{
		tl->m_prop.m_animationID = vaitem->m_animationID;
		tl->m_prop.m_virtualAnimation = m_container->GetVirtualAnimations( vaitem->m_animationID.m_track )[ vaitem->m_animationID.m_index ];

		propertiesPage.SetObject( &(tl->m_prop.m_virtualAnimation) );

		return;
	}

	VirtualAnimTimelineItem_Motion* mitem = ItemCast< VirtualAnimTimelineItem_Motion >( item );
	if ( mitem )
	{
		tl->m_prop.m_virtualMotionID = mitem->m_motionID;
		tl->m_prop.m_virtualMotion = m_container->GetVirtualMotions()[ mitem->m_motionID ];

		propertiesPage.SetObject( &(tl->m_prop.m_virtualMotion) );

		return;
	}

	VirtualAnimTimelineItem_FK* fkitem = ItemCast< VirtualAnimTimelineItem_FK >( item );
	if ( fkitem )
	{
		tl->m_prop.m_virtualFKID = fkitem->m_dataID;
		tl->m_prop.m_virtualFK = m_container->GetVirtualFKs()[ fkitem->m_dataID ];

		propertiesPage.SetObject( &(tl->m_prop.m_virtualFK) );

		return;
	}

	VirtualAnimTimelineItem_IK* ikitem = ItemCast< VirtualAnimTimelineItem_IK >( item );
	if ( ikitem )
	{
		tl->m_prop.m_virtualIKID = ikitem->m_dataID;
		tl->m_prop.m_virtualIK = m_container->GetVirtualIKs()[ ikitem->m_dataID ];

		propertiesPage.SetObject( &(tl->m_prop.m_virtualIK) );

		return;
	}
}

void CEdAnimBuilderTimeline::ItemPropertyChanged( TDynArray< ITimelineItem* >& items, const CName& propertyName )
{
	const Uint32 size = items.Size();

	ASSERT( size == 1 || size == 0 );

	if ( size == 1 )
	{
		ITimelineItem* item = items[ 0 ];

		VirtualAnimTimelineItem_Animation* vaitem = ItemCast< VirtualAnimTimelineItem_Animation >( item );
		if ( vaitem )
		{
			m_builder->RequestVirtualAnimationChange( m_prop.m_animationID, m_prop.m_virtualAnimation );

			RefreshTimeline();
			return;
		}

		VirtualAnimTimelineItem_Motion* mitem = ItemCast< VirtualAnimTimelineItem_Motion >( item );
		if ( mitem )
		{
			m_builder->RequestVirtualMotionChange( m_prop.m_virtualMotionID, m_prop.m_virtualMotion );

			RefreshTimeline();
			return;
		}

		VirtualAnimTimelineItem_FK* fkitem = ItemCast< VirtualAnimTimelineItem_FK >( item );
		if ( fkitem )
		{
			m_builder->RequestVirtualFKChange( m_prop.m_virtualFKID, m_prop.m_virtualFK );

			RefreshTimeline();
			return;
		}

		VirtualAnimTimelineItem_IK* ikitem = ItemCast< VirtualAnimTimelineItem_IK >( item );
		if ( ikitem )
		{
			m_builder->RequestVirtualIKChange( m_prop.m_virtualIKID, m_prop.m_virtualIK );

			RefreshTimeline();
			return;
		}
	}
}

Float CEdAnimBuilderTimeline::RequestVirtualAnimationStartTime( const VirtualAnimationID& animation, Float startTime ) const
{
	VirtualAnimation temp = m_container->GetVirtualAnimations( animation.m_track )[ animation.m_index ];
	
	temp.m_time = startTime;

	m_builder->RequestVirtualAnimationChange( animation, temp );

	CheckAutoBlendForAnims( animation );

	return startTime;
}

Float CEdAnimBuilderTimeline::RequestVirtualMotionStartTime( const VirtualAnimationMotionID& motion, Float startTime ) const
{
	VirtualAnimationMotion temp = m_container->GetVirtualMotions()[ motion ];
	const Float duration = temp.GetDuration();

	temp.m_startTime = startTime;
	temp.m_endTime = startTime + duration;

	m_builder->RequestVirtualMotionChange( motion, temp );

	return startTime;
}

Float CEdAnimBuilderTimeline::RequestVirtualFKStartTime( const VirtualAnimationPoseFKID& dataID, Float startTime ) const
{
	VirtualAnimationPoseFK temp = m_container->GetVirtualFKs()[ dataID ];
	temp.m_time = startTime;

	m_builder->RequestVirtualFKChange( dataID, temp );

	return startTime;
}

Float CEdAnimBuilderTimeline::RequestVirtualIKStartTime( const VirtualAnimationPoseIKID& dataID, Float startTime ) const
{
	VirtualAnimationPoseIK temp = m_container->GetVirtualIKs()[ dataID ];
	temp.m_time = startTime;

	m_builder->RequestVirtualIKChange( dataID, temp );

	return startTime;
}

Bool CEdAnimBuilderTimeline::SelectEvent( const CName& eventName )
{
	return false;
}

Bool CEdAnimBuilderTimeline::OnDropText( wxCoord x, wxCoord y, String &text )
{
	ASSERT( m_dragInfo.m_pending );

	Int32 selectedTrack = m_dragInfo.m_track;
	
	EVirtualAnimationTrack trackType = GetTrackType( selectedTrack );
	Int32 trackNumber = GetTrackNumber( selectedTrack );

	if ( trackType > VAT_None && trackNumber >= 0 )
	{
		size_t cutPlace;
		text.FindCharacter( ';', cutPlace );

		String path = text.LeftString( cutPlace );
		CName animationName = CName( text.RightString( text.GetLength() - cutPlace - 1 ) );

		ResourceLoadingContext context;
		CSkeletalAnimationSet* set = LoadResource< CSkeletalAnimationSet >( path, context );
		if ( set )
		{
			CSkeletalAnimationSetEntry* entry = set->FindAnimation( animationName );
			if ( entry )
			{
				Float time = Max( CalculateTimePos( x ), 0.f );

				VirtualAnimation anim;
				anim.m_name = animationName;
				anim.m_time = time;
				anim.m_startTime = 0.f;
				anim.m_endTime = entry->GetDuration();
				anim.m_speed = 1.f;
				anim.m_track = trackNumber;
				anim.m_animset = set;

				m_builder->RequestAddVirtualAnimation( anim, trackType );
			}
		}
	}

	m_dragInfo.m_pending = false;

	RefreshTimeline();

	return true;
}

EVirtualAnimationTrack CEdAnimBuilderTimeline::GetTrackType( Int32 track ) const
{
	const String trackName = m_tracks[ track ]->m_name;
	return GetTrackType( trackName );
}

EVirtualAnimationTrack CEdAnimBuilderTimeline::GetTrackType( const String& trackName ) const
{
	if ( trackName.BeginsWith( TRACK_BASE ) )
	{
		return VAT_Base;
	}
	else if ( trackName.BeginsWith( TRACK_ADDITIVE ) )
	{
		return VAT_Additive;
	}
	else if ( trackName.BeginsWith( TRACK_OVERRIDE ) )
	{
		return VAT_Override;
	}

	return VAT_None;
}

Int32 CEdAnimBuilderTimeline::GetTrackNumber( Int32 track ) const
{
	const String trackName = m_tracks[ track ]->m_name;
	return GetTrackNumber( trackName );
}

Int32 CEdAnimBuilderTimeline::GetTrackNumber( const String& track ) const
{
	size_t place;
	if (track.FindCharacter( '_', place ))
	{
		String numStr = track.RightString( track.GetLength() - place - 1 );

		Int32 num = 0;
		FromString( numStr, num );

		return num;
	}
	else
	{
		return 0;
	}
}

String CEdAnimBuilderTimeline::GetTrackName( EVirtualAnimationTrack track )
{
	switch ( track )
	{
	case VAT_Base:
		return CEdAnimBuilderTimeline::TRACK_BASE;
	case VAT_Override:
		return CEdAnimBuilderTimeline::TRACK_OVERRIDE;
	case VAT_Additive:
		return CEdAnimBuilderTimeline::TRACK_ADDITIVE;
	case VAT_None:
		return String::EMPTY;
	}

	return TXT("DUPA");
}

String CEdAnimBuilderTimeline::GetTrackName( EVirtualAnimationTrack track, Int32 trackNumber )
{
	String str = GetTrackName( track );
	if ( trackNumber > 0 )
	{
		return String::Printf(TXT("%s_%d"), str.AsChar(), trackNumber );
	}
	else
	{
		return str;
	}
}

String CEdAnimBuilderTimeline::GetNextTrackName( EVirtualAnimationTrack track ) const
{
	String prefix = GetTrackName( track );

	Int32 numMax = -1;

	const Uint32 size = m_tracks.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		const String track = m_tracks[ i ]->m_name;

		if ( track.BeginsWith( prefix ) )
		{
			size_t place;
			if ( track.FindCharacter( '_', place ) )
			{
				String numStr = track.RightString( track.GetLength() - place - 1 );

				Int32 num = 0;
				FromString( numStr, num );
				
				if ( num > numMax )
				{
					numMax = num;
				}
			}
		}
	}

	if ( numMax >= 0 )
	{
		numMax++;
		String numStr = TXT("_") + ToString( numMax );
		return prefix + numStr;
	}
	else
	{
		return prefix + TXT("_1");
	}
}

wxDragResult CEdAnimBuilderTimeline::OnDragOver( wxCoord x, wxCoord y, wxDragResult def ) 
{
	Track* track = GetTrackAt( wxPoint( x, y) );
	Int32 selectedTrack = GetTrackIndex( track );

	if ( selectedTrack != -1 )
	{
		m_dragInfo.m_pending = true;
		m_dragInfo.m_track = selectedTrack;
	}
	else
	{
		m_dragInfo.m_pending = false;
	}

	Refresh();

	return def;
}

void CEdAnimBuilderTimeline::OnLeave()
{
	m_dragInfo.m_pending = false;

	Refresh();
}

void CEdAnimBuilderTimeline::FillTrackMenu( const String& name, wxMenu* menu )
{
	EVirtualAnimationTrack trackType = GetTrackType( name );
	if ( trackType != VAT_None )
	{
		String msg = String::Printf( TXT("Create new %s track"), name.AsChar() );

		menu->Append( ID_NEW_VANIM_TRACK + trackType, msg.AsChar() );
		menu->Connect( ID_NEW_VANIM_TRACK + trackType, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdAnimBuilderTimeline::OnAddVTrack ), NULL, this );
	}
	else if ( name == CEdAnimBuilderTimeline::TRACK_MOTION )
	{
		menu->Append( ID_ADD_MOTION, TXT("Add motion event") );
		menu->Connect( ID_ADD_MOTION, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdAnimBuilderTimeline::OnAddMotionEvent ), NULL, this );
	}
	else if ( name == CEdAnimBuilderTimeline::TRACK_FK )
	{
		menu->Append( ID_ADD_FK, TXT("Add FK event") );
		menu->Connect( ID_ADD_FK, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdAnimBuilderTimeline::OnAddFKEvent ), NULL, this );
	}
	else if ( name == CEdAnimBuilderTimeline::TRACK_IK )
	{
		menu->Append( ID_ADD_IK, TXT("Add IK event") );
		menu->Connect( ID_ADD_IK, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdAnimBuilderTimeline::OnAddIKEvent ), NULL, this );
	}
}

void CEdAnimBuilderTimeline::FillItemMenu( ITimelineItem* item, wxMenu* menu, Bool& addDefaults )
{
	addDefaults = false;

	const VirtualAnimTimelineItem_Animation* vitem = ItemCast< VirtualAnimTimelineItem_Animation >( item );
	if ( !vitem )
	{
		return;
	}

	if ( CanCalcAutoBlends( vitem ) )
	{
		menu->Append( ID_CALC_AUTO_BLENDS, TXT("Auto blends") );
		menu->Connect( ID_CALC_AUTO_BLENDS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdAnimBuilderTimeline::OnCalcAutoBlends ), new TCallbackData< ITimelineItem* >( item ), this );
		menu->AppendSeparator();
	}

	menu->Append( ID_ITEM_SNAP_TO_ZERO, TXT("Snap to zero") );
	menu->Connect( ID_ITEM_SNAP_TO_ZERO, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdAnimBuilderTimeline::OnSnapItemToZeroPosition ), new TCallbackData< ITimelineItem* >( item ), this );

	menu->Append( ID_ITEM_MOVE, TXT("Move") );
	menu->Connect( ID_ITEM_MOVE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdAnimBuilderTimeline::OnItemMoved ), new TCallbackData< ITimelineItem* >( item ), this );

	/*{
		wxMenu* subMenu = new wxMenu();

		subMenu->Append( ID_ITEM_SNAP_CLOSEST_EDGE_TRACK, TXT("Snap to closest edge (track)") );
		menu->Connect( ID_ITEM_SNAP_CLOSEST_EDGE_TRACK, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdAnimBuilderTimeline::OnSnapItemToClosestFromTrack ), new TCallbackData< ITimelineItem* >( item ), this );

		subMenu->Append( ID_ITEM_SNAP_CLOSEST_EDGE, TXT("Snap to closest edge (all)") );
		menu->Connect( ID_ITEM_SNAP_CLOSEST_EDGE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdAnimBuilderTimeline::OnSnapItemToClosest ), new TCallbackData< ITimelineItem* >( item ), this );

		subMenu->Append( ID_ITEM_SNAP_CLOSEST_EDGE_TRACK_INPUT, TXT("Snap to closest edge with offset (track)") );
		menu->Connect( ID_ITEM_SNAP_CLOSEST_EDGE_TRACK_INPUT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdAnimBuilderTimeline::OnSnapItemToClosestFromTrackWithOffset ), new TCallbackData< ITimelineItem* >( item ), this );

		subMenu->Append( ID_ITEM_SNAP_CLOSEST_EDGE_INPUT, TXT("Snap to closest edge with offset (all)") );
		menu->Connect( ID_ITEM_SNAP_CLOSEST_EDGE_INPUT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdAnimBuilderTimeline::OnSnapItemToClosestWithOffset ), new TCallbackData< ITimelineItem* >( item ), this );

		menu->Append( wxID_ANY, wxT("Closest"), subMenu );
	}*/
	
	{
		wxMenu* subMenu = new wxMenu();

		subMenu->Append( ID_ITEM_SNAP_TO_PREV_TACK_ITEM, TXT("Snap to prev (track)") );
		menu->Connect( ID_ITEM_SNAP_TO_PREV_TACK_ITEM, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdAnimBuilderTimeline::OnSnapItemToPrevTrackItem ), new TCallbackData< ITimelineItem* >( item ), this );

		subMenu->Append( ID_ITEM_SNAP_TO_PREV_TACK_ITEM_INPUT, TXT("Snap to prev with offset (track)") );
		menu->Connect( ID_ITEM_SNAP_TO_PREV_TACK_ITEM_INPUT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdAnimBuilderTimeline::OnSnapItemToPrevTrackItemWithOffset ), new TCallbackData< ITimelineItem* >( item ), this );

		subMenu->Append( ID_ITEM_SNAP_TO_PREV_ITEM, TXT("Snap to prev (all)") );
		menu->Connect( ID_ITEM_SNAP_TO_PREV_ITEM, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdAnimBuilderTimeline::OnSnapItemToPrevItem ), new TCallbackData< ITimelineItem* >( item ), this );

		subMenu->Append( ID_ITEM_SNAP_TO_PREV_ITEM_INPUT, TXT("Snap to prev with offset (all)") );
		menu->Connect( ID_ITEM_SNAP_TO_PREV_ITEM_INPUT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdAnimBuilderTimeline::OnSnapItemToPrevItemWithOffset ), new TCallbackData< ITimelineItem* >( item ), this );

		menu->Append( wxID_ANY, wxT("Prev"), subMenu );
	}

	{
		wxMenu* subMenu = new wxMenu();

		subMenu->Append( ID_ITEM_SNAP_TO_NEXT_TACK_ITEM, TXT("Snap to next (track)") );
		menu->Connect( ID_ITEM_SNAP_TO_NEXT_TACK_ITEM, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdAnimBuilderTimeline::OnSnapItemToNextTrackItem ), new TCallbackData< ITimelineItem* >( item ), this );

		subMenu->Append( ID_ITEM_SNAP_TO_NEXT_TACK_ITEM_INPUT, TXT("Snap to next with offset (track)") );
		menu->Connect( ID_ITEM_SNAP_TO_NEXT_TACK_ITEM_INPUT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdAnimBuilderTimeline::OnSnapItemToNextTrackItemWithOffset ), new TCallbackData< ITimelineItem* >( item ), this );

		subMenu->Append( ID_ITEM_SNAP_TO_NEXT_ITEM, TXT("Snap to next (all)") );
		menu->Connect( ID_ITEM_SNAP_TO_NEXT_ITEM, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdAnimBuilderTimeline::OnSnapItemToNextItem ), new TCallbackData< ITimelineItem* >( item ), this );

		subMenu->Append( ID_ITEM_SNAP_TO_NEXT_ITEM_INPUT, TXT("Snap to next with offet (all)") );
		menu->Connect( ID_ITEM_SNAP_TO_NEXT_ITEM_INPUT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdAnimBuilderTimeline::OnSnapItemToNextItemWithOffset ), new TCallbackData< ITimelineItem* >( item ), this );

		menu->Append( wxID_ANY, wxT("Next"), subMenu );
	}

	menu->AppendSeparator();

	if ( !m_builder->IsDebuggingAnimation( vitem->m_animationID ) )
	{
		{
			wxMenu* subMenu = new wxMenu();

			Uint32 counter = 0;

			for ( DefaultColorsIterator it; it; ++it )
			{
				Color c = *it;

				counter++;

				wxMenuItem* mitem = subMenu->Append( ID_DEBUG_ITEM + counter, wxString::Format( wxT("Color %d"), counter ) );
				menu->Connect( ID_DEBUG_ITEM + counter, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdAnimBuilderTimeline::OnDebugItemAnimationEnabled ), new TCallbackData< ITimelineItem* >( item ), this );

				mitem->SetTextColour( wxColour( c.R, c.G, c.B ) );
			}

			menu->Append( wxID_ANY, wxT("Set debug visualizer"), subMenu );
		}
	}
	else
	{
		menu->Append( ID_DEBUG_ITEM, TXT("Reset debug visualizer") );
		menu->Connect( ID_DEBUG_ITEM, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdAnimBuilderTimeline::OnDebugItemAnimationDisabled ), new TCallbackData< ITimelineItem* >( item ), this );
	}

	menu->AppendSeparator();

	menu->Append( ID_REMOVE_ITEM, TXT("Remove item") );
	menu->Connect( ID_REMOVE_ITEM, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdAnimBuilderTimeline::OnItemRemoved ), new TCallbackData< ITimelineItem* >( item ), this );

	menu->Append( ID_ITEM_DUPLICATE, TXT("Duplicate item") );
	menu->Connect( ID_ITEM_DUPLICATE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdAnimBuilderTimeline::OnItemDuplicated ), new TCallbackData< ITimelineItem* >( item ), this );

	menu->AppendSeparator();

	menu->Append( ID_EDIT_BONES, TXT("Choose bones") );
	menu->Connect( ID_EDIT_BONES, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdAnimBuilderTimeline::OnEditItemBones ), new TCallbackData< ITimelineItem* >( item ), this );
}

void CEdAnimBuilderTimeline::OnLeftMouseDoubleClick( Int32 x, Int32 y )
{
	if ( !IsAnySelected() )
	{
		m_builder->TogglePause();
	}
}

void CEdAnimBuilderTimeline::PaintCanvas( Int32 width, Int32 height )
{
	CEdTimeline::PaintCanvas( width, height );

	if ( m_dragInfo.m_pending )
	{
		Track* track = m_tracks[ m_dragInfo.m_track ];

		// get track y position in timeline space
		TimelineImpl::CDrawGroupTracks* drwGrp = GetTrackDrawGroup( track );
		wxPoint targetPos;
		drwGrp->GetDrawBuffer()->GetTargetPos(targetPos);
		Int32 trackPosY = drwGrp->GetDispTrackLocalPos( track ).y + targetPos.y;

		FillRect( 1, trackPosY, TIMELINE_TRACK_BTN_WIDTH, drwGrp->GetTrackHeight( track ), wxColour( 255, 255, 255, 128 ) );
	}

	PaintEmptySpaces();
}

void CEdAnimBuilderTimeline::PaintEmptySpaces()
{
	// Paint empty spaces - red rectangles...
	// TODO
}

void CEdAnimBuilderTimeline::SortTracks()
{

}

const VirtualAnimTimelineItem_Animation* CEdAnimBuilderTimeline::FindPrevItemFromTrack( const VirtualAnimTimelineItem_Animation* item, EVirtualAnimationTrack track ) const
{
	const VirtualAnimTimelineItem_Animation* best = NULL;
	Float dist = NumericLimits< Float >::Max();

	const Float refTime = item->GetStart();

	const Uint32 size = m_items.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		const VirtualAnimTimelineItem_Animation* vitem = ItemCast< VirtualAnimTimelineItem_Animation >( m_items[ i ] );
		if ( !vitem || vitem->m_animationID.m_track != track )
		{
			continue;
		}
		if ( vitem == item )
		{
			continue;
		}

		Float stime = vitem->GetStart();
		Float endTime = stime + vitem->GetDuration();

		if ( refTime - stime >= 0.f  && MAbs( refTime - endTime ) < dist )
		{
			dist = MAbs( refTime - endTime );
			best = vitem;
		}
	}

	return best;
}

const VirtualAnimTimelineItem_Animation* CEdAnimBuilderTimeline::FindPrevItem( const VirtualAnimTimelineItem_Animation* item ) const
{
	const VirtualAnimTimelineItem_Animation* best = NULL;
	Float dist = NumericLimits< Float >::Max();

	const Float refTime = item->GetStart();

	const Uint32 size = m_items.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		const VirtualAnimTimelineItem_Animation* vitem = ItemCast< VirtualAnimTimelineItem_Animation >( m_items[ i ] );
		if ( !vitem || vitem == item )
		{
			continue;
		}

		Float stime = vitem->GetStart();
		Float endTime = stime + vitem->GetDuration();

		if ( refTime - stime >= 0.f  && MAbs( refTime - endTime ) < dist )
		{
			dist = MAbs( refTime - endTime );
			best = vitem;
		}
	}

	return best;
}

const VirtualAnimTimelineItem_Animation* CEdAnimBuilderTimeline::FindNextItemFromTrack( const VirtualAnimTimelineItem_Animation* item, EVirtualAnimationTrack track ) const
{
	const VirtualAnimTimelineItem_Animation* best = NULL;
	Float dist = NumericLimits< Float >::Max();

	const Float refTime = item->GetStart() + item->GetDuration();

	const Uint32 size = m_items.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		const VirtualAnimTimelineItem_Animation* vitem = ItemCast< VirtualAnimTimelineItem_Animation >( m_items[ i ] );
		if ( !vitem || vitem->m_animationID.m_track != track )
		{
			continue;
		}
		if ( vitem == item )
		{
			continue;
		}

		Float endTime = vitem->GetStart();
		Float stime = endTime + vitem->GetDuration();

		if ( stime - refTime >= 0.f  && MAbs( refTime - endTime ) < dist )
		{
			dist = MAbs( refTime - endTime );
			best = vitem;
		}
	}

	return best;
}

const VirtualAnimTimelineItem_Animation* CEdAnimBuilderTimeline::FindNextItem( const VirtualAnimTimelineItem_Animation* item ) const
{
	const VirtualAnimTimelineItem_Animation* best = NULL;
	Float dist = NumericLimits< Float >::Max();

	const Float refTime = item->GetStart() + item->GetDuration();

	const Uint32 size = m_items.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		const VirtualAnimTimelineItem_Animation* vitem = ItemCast< VirtualAnimTimelineItem_Animation >( m_items[ i ] );
		if ( !vitem || vitem == item )
		{
			continue;
		}

		Float endTime = vitem->GetStart();

		if ( endTime - refTime >= 0.f  && MAbs( refTime - endTime ) < dist )
		{
			dist = MAbs( refTime - endTime );
			best = vitem;
		}
	}

	return best;
}

const VirtualAnimTimelineItem_Animation* CEdAnimBuilderTimeline::FindClosestItemFromTrack( const VirtualAnimTimelineItem_Animation* item, EVirtualAnimationTrack track, Bool& isPrev ) const
{
	const VirtualAnimTimelineItem_Animation* prev = FindPrevItemFromTrack( item, track );
	const VirtualAnimTimelineItem_Animation* next = FindNextItemFromTrack( item, track );

	if ( !prev )
	{
		return next;
	}
	else if ( !next )
	{
		return prev;
	}

	Float distToPrev = MAbs( item->GetAnimation().m_time - prev->GetAnimation().m_time + prev->GetAnimation().GetDuration() );
	Float distToNext = MAbs( item->GetAnimation().m_time - ( next->GetAnimation().m_time - item->GetAnimation().GetDuration() ) );

	if ( distToPrev < distToNext )
	{
		isPrev = true;
		return prev;
	}
	else
	{
		isPrev = true;
		return next;
	}
}

const VirtualAnimTimelineItem_Animation* CEdAnimBuilderTimeline::FindClosestItem( const VirtualAnimTimelineItem_Animation* item, Bool& isPrev ) const
{
	const VirtualAnimTimelineItem_Animation* prev = FindPrevItem( item );
	const VirtualAnimTimelineItem_Animation* next = FindNextItem( item );

	if ( !prev )
	{
		return next;
	}
	else if ( !next )
	{
		return prev;
	}

	Float distToPrev = MAbs( item->GetAnimation().m_time - prev->GetAnimation().m_time + prev->GetAnimation().GetDuration() );
	Float distToNext = MAbs( item->GetAnimation().m_time - ( next->GetAnimation().m_time - item->GetAnimation().GetDuration() ) );

	if ( distToPrev < distToNext )
	{
		isPrev = true;
		return prev;
	}
	else
	{
		isPrev = true;
		return next;
	}
}

Bool CEdAnimBuilderTimeline::ItemsOverlapsLeft( const VirtualAnimTimelineItem_Animation* refItem, const VirtualAnimTimelineItem_Animation* b, Float& val ) const
{
	val = 0.f;

	const Float timeStartA = refItem->GetStart();
	const Float timeEndA = refItem->GetStart() + refItem->GetDuration();

	const Float timeStartB = b->GetStart();
	const Float timeEndB = b->GetStart() + b->GetDuration();

	if ( timeStartB < timeStartA && timeEndB < timeEndA && timeEndB > timeStartA )
	{
		val = timeEndB - timeStartA;

		ASSERT( val >= 0.f );
		ASSERT( val < refItem->GetDuration() );

		return true;
	}

	return false;
}

Bool CEdAnimBuilderTimeline::ItemsOverlapsRight( const VirtualAnimTimelineItem_Animation* refItem, const VirtualAnimTimelineItem_Animation* b, Float& val ) const
{
	val = 0.f;

	const Float timeStartA = refItem->GetStart();
	const Float timeEndA = refItem->GetStart() + refItem->GetDuration();

	const Float timeStartB = b->GetStart();
	const Float timeEndB = b->GetStart() + b->GetDuration();

	if ( timeStartA < timeStartB && timeEndA < timeEndB && timeEndA > timeStartB )
	{
		val = timeEndA - timeStartB;

		ASSERT( val >= 0.f );
		ASSERT( val < refItem->GetDuration() );

		return true;
	}

	return false;
}

Bool CEdAnimBuilderTimeline::ItemsOverlaps( const VirtualAnimTimelineItem_Animation* a, const VirtualAnimTimelineItem_Animation* b ) const
{
	Float val = 0.f;
	return ItemsOverlapsLeft( a, b, val ) || ItemsOverlapsRight( a, b, val );
}

Bool CEdAnimBuilderTimeline::FindItemsForAutoBlend( const VirtualAnimTimelineItem_Animation* refItem, const VirtualAnimTimelineItem_Animation*& left, const VirtualAnimTimelineItem_Animation*& right, Float& lVal, Float& rVal ) const
{
	left = NULL;
	right = NULL;

	lVal = 0.f;
	rVal = 0.f;

	const Uint32 size = m_items.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		const VirtualAnimTimelineItem_Animation* temp = ItemCast< VirtualAnimTimelineItem_Animation >( m_items[ i ] );
		if ( temp && temp != refItem && temp->m_animationID.m_track == refItem->m_animationID.m_track )
		{	
			Float tempL, tempR;

			if ( ItemsOverlapsLeft( refItem, temp, tempL ) )
			{
				left = temp;
				lVal = tempL;
			}

			if ( ItemsOverlapsRight( refItem, temp, tempR ) )
			{
				right = temp;
				rVal = tempR;
			}
		}
	}

	return left || right;
}

void CEdAnimBuilderTimeline::CheckAutoBlendForAnims( const VirtualAnimationID& animation ) const
{
	if ( !m_checkAutoBlendTimes )
	{
		return;
	}

	const Uint32 size = m_items.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		const VirtualAnimTimelineItem_Animation* item = ItemCast< VirtualAnimTimelineItem_Animation >( m_items[ i ] );
		if ( item && item->m_animationID == animation )
		{
			const VirtualAnimTimelineItem_Animation* left, *right;
			Float lVal, rVal;

			if ( FindItemsForAutoBlend( item, left, right, lVal, rVal ) )
			{
				if ( left )
				{
					const VirtualAnimTimelineItem_Animation* ll, *rr;
					Float lv, rv;

					if ( FindItemsForAutoBlend( left, ll, rr, lv, rv ) && rr == item )
					{
						VirtualAnimation tempA = item->GetAnimation();
						VirtualAnimation tempB = left->GetAnimation();
							
						tempA.m_blendIn = lVal;
						tempB.m_blendOut = rv;

						m_builder->RequestVirtualAnimationChange( item->m_animationID, tempA );
						m_builder->RequestVirtualAnimationChange( left->m_animationID, tempB );
					}
				}

				if ( right )
				{
					const VirtualAnimTimelineItem_Animation* ll, *rr;
					Float lv, rv;

					if ( FindItemsForAutoBlend( right, ll, rr, lv, rv ) && ll == item )
					{
						VirtualAnimation tempA = item->GetAnimation();
						VirtualAnimation tempB = right->GetAnimation();

						tempA.m_blendOut = rVal;
						tempB.m_blendIn = lv;

						m_builder->RequestVirtualAnimationChange( item->m_animationID, tempA );
						m_builder->RequestVirtualAnimationChange( right->m_animationID, tempB );
					}
				}
			}
		}
	}
}

Bool CEdAnimBuilderTimeline::CanCalcAutoBlends( const VirtualAnimTimelineItem_Animation* item ) const
{
	const VirtualAnimTimelineItem_Animation* left, *right;
	Float lVal, rVal;
	return FindItemsForAutoBlend( item, left, right, lVal, rVal );
}

void CEdAnimBuilderTimeline::CalcAutoBlends( VirtualAnimTimelineItem_Animation* item )
{
	const VirtualAnimTimelineItem_Animation* left, *right;
	Float lVal, rVal;

	VERIFY( FindItemsForAutoBlend( item, left, right, lVal, rVal ) );

	if ( left || right )
	{
		VirtualAnimation temp = item->GetAnimation();

		if ( left )
		{
			temp.m_blendIn = lVal;
		}

		if ( right )
		{
			temp.m_blendOut = rVal;
		}

		m_builder->RequestVirtualAnimationChange( item->m_animationID, temp );
	}
}

void CEdAnimBuilderTimeline::OnDrawItem( const ITimelineItem* item, const wxRect& inputRect )
{
	const VirtualAnimTimelineItem_Animation* vitem = ItemCast< VirtualAnimTimelineItem_Animation >( item );
	if ( !vitem )
	{
		return;
	}

	const VirtualAnimation& anim = vitem->GetAnimation();

	Float blendIn = anim.m_blendIn;
	Float blendOut = anim.m_blendOut;

	Track* track = m_tracks[ GetTrackIndex( item->GetTrackName() ) ] ;

	// get track y position in timeline space
	TimelineImpl::CDrawGroupTracks* drwGrp = GetTrackDrawGroup( track );
	wxPoint targetPos;
	drwGrp->GetDrawBuffer()->GetTargetPos(targetPos);
	Int32 trackPosY = drwGrp->GetDispTrackLocalPos( track ).y + targetPos.y;

	Int32 startPos = CalculatePixelPos( item->GetStart() );
	Int32 endPos = CalculatePixelPos( Min( item->GetStart() + item->GetDuration(), m_activeRangeDuration ) );

	wxRect rect( startPos, trackPosY + 20, endPos - startPos, 24 );

	Color debugColor;
	if ( m_builder->IsDebuggingAnimation( vitem->m_animationID, debugColor ) )
	{
		wxRect drec( 0, 0, 9, 9 );
		drec = drec.CenterIn( rect );

		drwGrp->GetDrawBuffer()->FillRect( drec, wxColor(debugColor.R, debugColor.G, debugColor.B) );
		drwGrp->GetDrawBuffer()->DrawRect( drec, wxColor(0,0,0) );
	}

	if ( blendIn > 0.f || blendOut > 0.f )
	{
		if ( blendIn > 0.f )
		{
			Float p = blendIn / anim.GetDuration();
			Int32 x1 = rect.GetLeft();
			Int32 y1 = rect.GetBottom();
			Int32 x2 = x1 + (Int32)(p * rect.GetWidth());
			int y2 = rect.GetTop();
			Int32 x3 = x2;
			Int32 y3 = y1;

			drwGrp->GetDrawBuffer()->DrawLine( x1, y1, x2, y2, wxColor( 0,0,0 ) );

			//drwGrp->GetDrawBuffer()->DrawTriangle( x1, y1, x2, y2, x3, y3, wxColor( 0, 0, 0, 128 ) );
			//drwGrp->GetDrawBuffer()->FillTriangle...
		}

		if ( blendOut > 0.f )
		{
			Float p = blendOut / anim.GetDuration();
			Int32 x1 = rect.GetRight() - (Int32)(p * rect.GetWidth());
			Int32 y1 = rect.GetTop();
			Int32 x2 = rect.GetRight();
			int y2 = rect.GetBottom();
			Int32 x3 = x1;
			Int32 y3 = y2;

			drwGrp->GetDrawBuffer()->DrawLine( x1, y1, x2, y2, wxColor( 0,0,0 ) );

			//drwGrp->GetDrawBuffer()->DrawTriangle( x1, y1, x2, y2, x3, y3, wxColor( 0, 0, 0, 128 ) );
		}
	}
}

void CEdAnimBuilderTimeline::OnAddVTrack( wxCommandEvent& event )
{
	EVirtualAnimationTrack trackType = (EVirtualAnimationTrack)(event.GetId() - ID_NEW_VANIM_TRACK);

	String trackName = GetNextTrackName( trackType );

	AddTrack( trackName );

	SortTracks();
}

void CEdAnimBuilderTimeline::OnAddMotionEvent( wxCommandEvent& event )
{
	const Float place = m_cursorTimePos;

	VirtualAnimationMotion motion;
	motion.m_startTime = place;
	motion.m_endTime = place + 1.f;

	m_builder->RequestAddVirtualMotion( motion );

	RefreshTimeline();
}

void CEdAnimBuilderTimeline::OnAddFKEvent( wxCommandEvent& event )
{
	const Float place = m_cursorTimePos;

	VirtualAnimationPoseFK data;
	data.m_time = place;

	m_builder->RequestAddVirtualFK( data );

	RefreshTimeline();
}

void CEdAnimBuilderTimeline::OnAddIKEvent( wxCommandEvent& event )
{
	const Float place = m_cursorTimePos;

	VirtualAnimationPoseIK data;
	data.m_time = place;

	m_builder->RequestAddVirtualIK( data );

	RefreshTimeline();
}

void CEdAnimBuilderTimeline::OnSnapItemToZeroPosition( wxCommandEvent& event )
{
	TCallbackData< ITimelineItem* >* callData = static_cast< TCallbackData< ITimelineItem* >* >( event.m_callbackUserData );
	VirtualAnimTimelineItem_Animation* item = ItemCast< VirtualAnimTimelineItem_Animation >( callData->GetData() );
	if ( !item )
	{
		return;
	}

	RequestVirtualAnimationStartTime( item->m_animationID, 0.f );
}

void CEdAnimBuilderTimeline::OnItemMoved( wxCommandEvent& event )
{
	TCallbackData< ITimelineItem* >* callData = static_cast< TCallbackData< ITimelineItem* >* >( event.m_callbackUserData );
	VirtualAnimTimelineItem_Animation* item = ItemCast< VirtualAnimTimelineItem_Animation >( callData->GetData() );
	if ( !item )
	{
		return;
	}

	String valStr = TXT("0.0");
	if ( InputBox( this, wxT("Move"), wxT("Offset:"), valStr ) )
	{
		Float val = 0.f;
		FromString( valStr, val );

		RequestVirtualAnimationStartTime( item->m_animationID, item->GetAnimation().m_time + val );
	}
}

void CEdAnimBuilderTimeline::OnSnapItemToPrevTrackItem( wxCommandEvent& event )
{
	TCallbackData< ITimelineItem* >* callData = static_cast< TCallbackData< ITimelineItem* >* >( event.m_callbackUserData );
	VirtualAnimTimelineItem_Animation* item = ItemCast< VirtualAnimTimelineItem_Animation >( callData->GetData() );
	if ( !item )
	{
		return;
	}

	const VirtualAnimTimelineItem_Animation* prevItem = FindPrevItemFromTrack( item, item->m_animationID.m_track );
	if ( prevItem )
	{
		RequestVirtualAnimationStartTime( item->m_animationID, prevItem->GetAnimation().m_time + prevItem->GetAnimation().GetDuration() );
	}
}

void CEdAnimBuilderTimeline::OnSnapItemToPrevTrackItemWithOffset( wxCommandEvent& event )
{
	TCallbackData< ITimelineItem* >* callData = static_cast< TCallbackData< ITimelineItem* >* >( event.m_callbackUserData );
	VirtualAnimTimelineItem_Animation* item = ItemCast< VirtualAnimTimelineItem_Animation >( callData->GetData() );
	if ( !item )
	{
		return;
	}

	String valStr = TXT("0.0");
	if ( InputBox( this, wxT("Snapping"), wxT("Offset:"), valStr ) )
	{
		Float val = 0.f;
		FromString( valStr, val );

		const VirtualAnimTimelineItem_Animation* prevItem = FindPrevItemFromTrack( item, item->m_animationID.m_track );
		if ( prevItem )
		{
			RequestVirtualAnimationStartTime( item->m_animationID, prevItem->GetAnimation().m_time + val + prevItem->GetAnimation().GetDuration() );
		}
	}
}

void CEdAnimBuilderTimeline::OnSnapItemToPrevItem( wxCommandEvent& event )
{
	TCallbackData< ITimelineItem* >* callData = static_cast< TCallbackData< ITimelineItem* >* >( event.m_callbackUserData );
	VirtualAnimTimelineItem_Animation* item = ItemCast< VirtualAnimTimelineItem_Animation >( callData->GetData() );
	if ( !item )
	{
		return;
	}

	const VirtualAnimTimelineItem_Animation* prevItem = FindPrevItem( item );
	if ( prevItem )
	{
		RequestVirtualAnimationStartTime( item->m_animationID, prevItem->GetAnimation().m_time + prevItem->GetAnimation().GetDuration() );
	}
}

void CEdAnimBuilderTimeline::OnSnapItemToPrevItemWithOffset( wxCommandEvent& event )
{
	TCallbackData< ITimelineItem* >* callData = static_cast< TCallbackData< ITimelineItem* >* >( event.m_callbackUserData );
	VirtualAnimTimelineItem_Animation* item = ItemCast< VirtualAnimTimelineItem_Animation >( callData->GetData() );
	if ( !item )
	{
		return;
	}

	String valStr = TXT("0.0");
	if ( InputBox( this, wxT("Snapping"), wxT("Offset:"), valStr ) )
	{
		Float val = 0.f;
		FromString( valStr, val );

		const VirtualAnimTimelineItem_Animation* prevItem = FindPrevItem( item );
		if ( prevItem )
		{
			RequestVirtualAnimationStartTime( item->m_animationID, prevItem->GetAnimation().m_time + val + prevItem->GetAnimation().GetDuration() );
		}
	}
}

void CEdAnimBuilderTimeline::OnSnapItemToNextTrackItem( wxCommandEvent& event )
{
	TCallbackData< ITimelineItem* >* callData = static_cast< TCallbackData< ITimelineItem* >* >( event.m_callbackUserData );
	VirtualAnimTimelineItem_Animation* item = ItemCast< VirtualAnimTimelineItem_Animation >( callData->GetData() );
	if ( !item )
	{
		return;
	}

	const VirtualAnimTimelineItem_Animation* nextItem = FindNextItemFromTrack( item, item->m_animationID.m_track );
	if ( nextItem )
	{
		RequestVirtualAnimationStartTime( item->m_animationID, nextItem->GetAnimation().m_time - item->GetAnimation().GetDuration() );
	}
}

void CEdAnimBuilderTimeline::OnSnapItemToNextTrackItemWithOffset( wxCommandEvent& event )
{
	TCallbackData< ITimelineItem* >* callData = static_cast< TCallbackData< ITimelineItem* >* >( event.m_callbackUserData );
	VirtualAnimTimelineItem_Animation* item = ItemCast< VirtualAnimTimelineItem_Animation >( callData->GetData() );
	if ( !item )
	{
		return;
	}

	String valStr = TXT("0.0");
	if ( InputBox( this, wxT("Snapping"), wxT("Offset:"), valStr ) )
	{
		Float val = 0.f;
		FromString( valStr, val );

		const VirtualAnimTimelineItem_Animation* nextItem = FindNextItemFromTrack( item, item->m_animationID.m_track );
		if ( nextItem )
		{
			RequestVirtualAnimationStartTime( item->m_animationID, nextItem->GetAnimation().m_time + val - item->GetAnimation().GetDuration() );
		}
	}
}

void CEdAnimBuilderTimeline::OnSnapItemToNextItem( wxCommandEvent& event )
{
	TCallbackData< ITimelineItem* >* callData = static_cast< TCallbackData< ITimelineItem* >* >( event.m_callbackUserData );
	VirtualAnimTimelineItem_Animation* item = ItemCast< VirtualAnimTimelineItem_Animation >( callData->GetData() );
	if ( !item )
	{
		return;
	}

	const VirtualAnimTimelineItem_Animation* nextItem = FindNextItem( item );
	if ( nextItem )
	{
		RequestVirtualAnimationStartTime( item->m_animationID, nextItem->GetAnimation().m_time - item->GetAnimation().GetDuration() );
	}
}

void CEdAnimBuilderTimeline::OnSnapItemToNextItemWithOffset( wxCommandEvent& event )
{
	TCallbackData< ITimelineItem* >* callData = static_cast< TCallbackData< ITimelineItem* >* >( event.m_callbackUserData );
	VirtualAnimTimelineItem_Animation* item = ItemCast< VirtualAnimTimelineItem_Animation >( callData->GetData() );
	if ( !item )
	{
		return;
	}

	String valStr = TXT("0.0");
	if ( InputBox( this, wxT("Snapping"), wxT("Offset:"), valStr ) )
	{
		Float val = 0.f;
		FromString( valStr, val );

		const VirtualAnimTimelineItem_Animation* nextItem = FindNextItem( item );
		if ( nextItem )
		{
			RequestVirtualAnimationStartTime( item->m_animationID, nextItem->GetAnimation().m_time + val - item->GetAnimation().GetDuration() );
		}
	}
}

void CEdAnimBuilderTimeline::OnSnapItemToClosest( wxCommandEvent& event )
{
	TCallbackData< ITimelineItem* >* callData = static_cast< TCallbackData< ITimelineItem* >* >( event.m_callbackUserData );
	VirtualAnimTimelineItem_Animation* item = ItemCast< VirtualAnimTimelineItem_Animation >( callData->GetData() );
	if ( !item )
	{
		return;
	}

	Bool isPrev = false;

	const VirtualAnimTimelineItem_Animation* closestItem = FindClosestItem( item, isPrev );
	if ( closestItem )
	{
		if ( isPrev )
		{
			RequestVirtualAnimationStartTime( item->m_animationID, closestItem->GetAnimation().m_time + closestItem->GetAnimation().GetDuration() );
		}
		else
		{
			RequestVirtualAnimationStartTime( item->m_animationID, closestItem->GetAnimation().m_time - item->GetAnimation().GetDuration() );
		}
	}
}

void CEdAnimBuilderTimeline::OnSnapItemToClosestFromTrack( wxCommandEvent& event )
{
	TCallbackData< ITimelineItem* >* callData = static_cast< TCallbackData< ITimelineItem* >* >( event.m_callbackUserData );
	VirtualAnimTimelineItem_Animation* item = ItemCast< VirtualAnimTimelineItem_Animation >( callData->GetData() );
	if ( !item )
	{
		return;
	}

	Bool isPrev = false;

	const VirtualAnimTimelineItem_Animation* closestItem = FindClosestItemFromTrack( item, item->m_animationID.m_track, isPrev );
	if ( closestItem )
	{
		if ( isPrev )
		{
			RequestVirtualAnimationStartTime( item->m_animationID, closestItem->GetAnimation().m_time + closestItem->GetAnimation().GetDuration() );
		}
		else
		{
			RequestVirtualAnimationStartTime( item->m_animationID, closestItem->GetAnimation().m_time - item->GetAnimation().GetDuration() );
		}
	}
}

void CEdAnimBuilderTimeline::OnSnapItemToClosestWithOffset( wxCommandEvent& event )
{
	TCallbackData< ITimelineItem* >* callData = static_cast< TCallbackData< ITimelineItem* >* >( event.m_callbackUserData );
	VirtualAnimTimelineItem_Animation* item = ItemCast< VirtualAnimTimelineItem_Animation >( callData->GetData() );
	if ( !item )
	{
		return;
	}

	String valStr = TXT("0.0");
	if ( InputBox( this, wxT("Snapping"), wxT("Offset:"), valStr ) )
	{
		Float val = 0.f;
		FromString( valStr, val );

		Bool isPrev = false;

		const VirtualAnimTimelineItem_Animation* closestItem = FindClosestItem( item, isPrev );
		if ( closestItem )
		{
			if ( isPrev )
			{
				RequestVirtualAnimationStartTime( item->m_animationID, closestItem->GetAnimation().m_time + closestItem->GetAnimation().GetDuration() + val );
			}
			else
			{
				RequestVirtualAnimationStartTime( item->m_animationID, closestItem->GetAnimation().m_time - item->GetAnimation().GetDuration() + val );
			}
		}
	}
}

void CEdAnimBuilderTimeline::OnSnapItemToClosestFromTrackWithOffset( wxCommandEvent& event )
{
	TCallbackData< ITimelineItem* >* callData = static_cast< TCallbackData< ITimelineItem* >* >( event.m_callbackUserData );
	VirtualAnimTimelineItem_Animation* item = ItemCast< VirtualAnimTimelineItem_Animation >( callData->GetData() );
	if ( !item )
	{
		return;
	}

	String valStr = TXT("0.0");
	if ( InputBox( this, wxT("Snapping"), wxT("Offset:"), valStr ) )
	{
		Float val = 0.f;
		FromString( valStr, val );
		
		Bool isPrev = false;

		const VirtualAnimTimelineItem_Animation* closestItem = FindClosestItemFromTrack( item, item->m_animationID.m_track, isPrev );
		if ( closestItem )
		{
			if ( isPrev )
			{
				RequestVirtualAnimationStartTime( item->m_animationID, closestItem->GetAnimation().m_time + closestItem->GetAnimation().GetDuration() + val );
			}
			else
			{
				RequestVirtualAnimationStartTime( item->m_animationID, closestItem->GetAnimation().m_time - item->GetAnimation().GetDuration() + val );
			}
		}
	}
}

void CEdAnimBuilderTimeline::OnDebugItemAnimationEnabled( wxCommandEvent& event )
{
	TCallbackData< ITimelineItem* >* callData = static_cast< TCallbackData< ITimelineItem* >* >( event.m_callbackUserData );
	VirtualAnimTimelineItem_Animation* item = ItemCast< VirtualAnimTimelineItem_Animation >( callData->GetData() );
	if ( !item )
	{
		return;
	}

	Int32 index = event.GetId() - ID_DEBUG_ITEM - 1;
	ASSERT( index >= 0 );

	Color color = DefaultColorsIterator::GetColor( index );

	m_builder->StartDebuggingAnimation( item->m_animationID, color );
}

void CEdAnimBuilderTimeline::OnDebugItemAnimationDisabled( wxCommandEvent& event )
{
	TCallbackData< ITimelineItem* >* callData = static_cast< TCallbackData< ITimelineItem* >* >( event.m_callbackUserData );
	VirtualAnimTimelineItem_Animation* item = ItemCast< VirtualAnimTimelineItem_Animation >( callData->GetData() );
	if ( !item )
	{
		return;
	}

	m_builder->StopDebuggingAnimation( item->m_animationID );
}

void CEdAnimBuilderTimeline::OnItemRemoved( wxCommandEvent& event )
{
	TCallbackData< ITimelineItem* >* callData = static_cast< TCallbackData< ITimelineItem* >* >( event.m_callbackUserData );
	VirtualAnimTimelineItem_Animation* item = ItemCast< VirtualAnimTimelineItem_Animation >( callData->GetData() );
	if ( !item )
	{
		return;
	}

	String str = String::Printf( TXT("Are you sure you want to remove animation '%s' (time %1.2f)?"), item->GetAnimation().m_name.AsString().AsChar(), item->GetStart() );
	if ( YesNo( str.AsChar() ) )
	{
		RemoveItem( callData->GetData() );
		RefreshTimeline();
	}
	else
	{
		static Int32 stupidity = 0;

		if ( stupidity == 0 )
		{
			wxMessageBox( wxT("So why do you click?"), wxT("Message") );
		}
		else if ( stupidity == 1 )
		{
			wxMessageBox( wxT("Mistake again??"), wxT("Message") );
		}
		else if ( stupidity == 2 )
		{
			wxMessageBox( wxT("...and again a mistake"), wxT("Message") );
		}
		else
		{
			wxMessageBox( wxT("That's going too far!!!"), wxT("Message") );
		}

		stupidity++;
	}
}

void CEdAnimBuilderTimeline::OnItemDuplicated( wxCommandEvent& event )
{
	TCallbackData< ITimelineItem* >* callData = static_cast< TCallbackData< ITimelineItem* >* >( event.m_callbackUserData );
	VirtualAnimTimelineItem_Animation* item = ItemCast< VirtualAnimTimelineItem_Animation >( callData->GetData() );
	if ( !item )
	{
		return;
	}

	m_builder->RequestVirtualAnimationDupication( item->m_animationID );

	RefreshTimeline();
}

void CEdAnimBuilderTimeline::OnEditItemBones( wxCommandEvent& event )
{
	TCallbackData< ITimelineItem* >* callData = static_cast< TCallbackData< ITimelineItem* >* >( event.m_callbackUserData );
	VirtualAnimTimelineItem_Animation* item = ItemCast< VirtualAnimTimelineItem_Animation >( callData->GetData() );
	if ( !item )
	{
		return;
	}

	m_builder->SelectBonesAndWeightsForAnimation( item->m_animationID );
}

void CEdAnimBuilderTimeline::OnCalcAutoBlends( wxCommandEvent& event )
{
	TCallbackData< ITimelineItem* >* callData = static_cast< TCallbackData< ITimelineItem* >* >( event.m_callbackUserData );
	VirtualAnimTimelineItem_Animation* item = ItemCast< VirtualAnimTimelineItem_Animation >( callData->GetData() );
	if ( !item )
	{
		return;
	}

	CalcAutoBlends( item );
}

//////////////////////////////////////////////////////////////////////////

VirtualAnimTimelineItem::VirtualAnimTimelineItem( const CEdAnimBuilderTimeline* timeline, const IVirtualAnimationContainer* container, VirtualAnimTimelineItem::EItemType type )
	: m_type( type )
	, m_timeline( timeline )
	, m_container( container )
{

}

String VirtualAnimTimelineItem::GetTypeName() const
{
	return TXT("VirtualAnimTimelineItem");
}

void VirtualAnimTimelineItem::UpdatePresentation()
{

}

Bool VirtualAnimTimelineItem::GetTopText( String& text ) const
{
	return false;
}

void VirtualAnimTimelineItem::SetTrackName( const String& trackName )
{
	ASSERT( 0 );
}

Bool VirtualAnimTimelineItem::IsRightResizable() const
{
	return false;
}

Bool VirtualAnimTimelineItem::IsLeftResizable() const
{
	return false;
}

Bool VirtualAnimTimelineItem::IsMovable() const
{
	return true;
}

Bool VirtualAnimTimelineItem::IsCopyable() const
{
	return false;
}

Gdiplus::Bitmap* VirtualAnimTimelineItem::GetIcon() const
{
	return NULL;
}

const wxBitmap* VirtualAnimTimelineItem::GetWxIcon() const
{
	return nullptr;
}

Bool VirtualAnimTimelineItem::IsEditable() const
{
	return true;
}

Bool VirtualAnimTimelineItem::IsRemovable() const
{
	return true;
}

void VirtualAnimTimelineItem::SetProperty( IProperty* property, ITimelineItem* sourceItem )
{

}

//////////////////////////////////////////////////////////////////////////

VirtualAnimTimelineItem_Animation::VirtualAnimTimelineItem_Animation( const CEdAnimBuilderTimeline* timeline, const IVirtualAnimationContainer* container, const VirtualAnimationID& animation )
	: VirtualAnimTimelineItem( timeline, container, IT_Animation )
	, m_animationID( animation )
{

}

const VirtualAnimation& VirtualAnimTimelineItem_Animation::GetAnimation() const
{
	return m_container->GetVirtualAnimations( m_animationID.m_track )[ m_animationID.m_index ];
}

String VirtualAnimTimelineItem_Animation::GetTypeName() const
{
	return TXT("VirtualAnimTimelineItem_Animation");
}

Bool VirtualAnimTimelineItem_Animation::IsDuration() const
{
	return true;
}

Float VirtualAnimTimelineItem_Animation::GetStart() const
{
	return GetAnimation().m_time;
}

Float VirtualAnimTimelineItem_Animation::SetStart( Float start, Bool deepUpdate )
{
	Float var = m_timeline->RequestVirtualAnimationStartTime( m_animationID, start );
	return var;
}

Float VirtualAnimTimelineItem_Animation::GetDuration() const
{
	return GetAnimation().GetDuration();
}

Float VirtualAnimTimelineItem_Animation::SetDuration( Float duration )
{
	ASSERT( 0 );
	return duration;
}

Bool VirtualAnimTimelineItem_Animation::GetMiddleText( String& text ) const
{
	text = GetAnimation().m_name.AsString();
	return true;
}

Bool VirtualAnimTimelineItem_Animation::GetTooltip( String& text ) const
{
	const VirtualAnimation& a = GetAnimation();

	text = String::Printf( TXT("time: 1.2f; start %1.2f; end %1.2f; dur. %1.2f; speed %1.2f"), a.m_time, a.m_startTime, a.m_endTime, a.GetDuration(), a.m_speed );
	if ( a.m_useMotion )
	{
		text += TXT("; use motion");
	}
	//if ( a.m_useEvents )
	//{
	//	text += TXT("; use events");
	//}
	return true;
}

String VirtualAnimTimelineItem_Animation::GetTrackName() const
{
	return CEdAnimBuilderTimeline::GetTrackName( m_animationID.m_track, GetAnimation().m_track );
}

wxColor	VirtualAnimTimelineItem_Animation::GetColor() const
{
	return wxColor( 171, 207, 255, 128 );
}

//////////////////////////////////////////////////////////////////////////

VirtualAnimTimelineItem_Motion::VirtualAnimTimelineItem_Motion( const CEdAnimBuilderTimeline* timeline, const IVirtualAnimationContainer* container, const VirtualAnimationMotionID& motion )
	: VirtualAnimTimelineItem( timeline, container, IT_Motion )
	, m_motionID( motion )
{

}

const VirtualAnimationMotion& VirtualAnimTimelineItem_Motion::GetMotion() const
{
	return m_container->GetVirtualMotions()[ m_motionID ];
}

String VirtualAnimTimelineItem_Motion::GetTypeName() const
{
	return TXT("VirtualAnimTimelineItem_Motion");
}

Bool VirtualAnimTimelineItem_Motion::IsDuration() const
{
	return true;
}

Float VirtualAnimTimelineItem_Motion::GetStart() const
{
	return GetMotion().m_startTime;
}

Float VirtualAnimTimelineItem_Motion::SetStart( Float start, Bool deepUpdate )
{
	Float var = m_timeline->RequestVirtualMotionStartTime( m_motionID, start );
	return var;
}

Float VirtualAnimTimelineItem_Motion::GetDuration() const
{
	return GetMotion().GetDuration();
}

Float VirtualAnimTimelineItem_Motion::SetDuration( Float duration )
{
	ASSERT( 0 );
	return duration;
}

Bool VirtualAnimTimelineItem_Motion::GetMiddleText( String& text ) const
{
	text = TXT("Motion"); //GetMotion().m_name.AsString();
	return true;
}

Bool VirtualAnimTimelineItem_Motion::GetTooltip( String& text ) const
{
	const VirtualAnimationMotion& m = GetMotion();

	text = String::Printf( TXT("start %1.2f; end %1.2f; dur. %1.2f;"), m.m_startTime, m.m_endTime, m.GetDuration() );

	return true;
}

String VirtualAnimTimelineItem_Motion::GetTrackName() const
{
	return CEdAnimBuilderTimeline::TRACK_MOTION;
}

wxColor	VirtualAnimTimelineItem_Motion::GetColor() const
{
	return wxColor( 191, 255, 255, 128 );
}

//////////////////////////////////////////////////////////////////////////

VirtualAnimTimelineItem_FK::VirtualAnimTimelineItem_FK( const CEdAnimBuilderTimeline* timeline, const IVirtualAnimationContainer* container, const VirtualAnimationPoseFKID& dataID )
	: VirtualAnimTimelineItem( timeline, container, IT_FK )
	, m_dataID( dataID )
{

}

const VirtualAnimationPoseFK& VirtualAnimTimelineItem_FK::GetData() const
{
	return m_container->GetVirtualFKs()[ m_dataID ];
}

String VirtualAnimTimelineItem_FK::GetTypeName() const
{
	return TXT("VirtualAnimTimelineItem_FK");
}

Bool VirtualAnimTimelineItem_FK::IsDuration() const
{
	return false;
}

Float VirtualAnimTimelineItem_FK::GetStart() const
{
	return GetData().m_time;
}

Float VirtualAnimTimelineItem_FK::SetStart( Float start, Bool deepUpdate )
{
	Float var = m_timeline->RequestVirtualFKStartTime( m_dataID, start );
	return var;
}

Float VirtualAnimTimelineItem_FK::GetDuration() const
{
	return 0.f;
}

Float VirtualAnimTimelineItem_FK::SetDuration( Float duration )
{
	ASSERT( 0 );
	return duration;
}

Bool VirtualAnimTimelineItem_FK::GetMiddleText( String& text ) const
{
	text = TXT("FK"); //GetMotion().m_name.AsString();
	return true;
}

Bool VirtualAnimTimelineItem_FK::GetTooltip( String& text ) const
{
	const VirtualAnimationPoseFK& d = GetData();

	text = String::Printf( TXT("time %1.2f"), d.m_time );

	return true;
}

String VirtualAnimTimelineItem_FK::GetTrackName() const
{
	return CEdAnimBuilderTimeline::TRACK_FK;
}

wxColor	VirtualAnimTimelineItem_FK::GetColor() const
{
	return wxColor( 191, 255, 255, 128 );
}

//////////////////////////////////////////////////////////////////////////

VirtualAnimTimelineItem_IK::VirtualAnimTimelineItem_IK( const CEdAnimBuilderTimeline* timeline, const IVirtualAnimationContainer* container, const VirtualAnimationPoseFKID& dataID )
	: VirtualAnimTimelineItem( timeline, container, IT_IK )
	, m_dataID( dataID )
{

}

const VirtualAnimationPoseIK& VirtualAnimTimelineItem_IK::GetData() const
{
	return m_container->GetVirtualIKs()[ m_dataID ];
}

String VirtualAnimTimelineItem_IK::GetTypeName() const
{
	return TXT("VirtualAnimTimelineItem_IK");
}

Bool VirtualAnimTimelineItem_IK::IsDuration() const
{
	return false;
}

Float VirtualAnimTimelineItem_IK::GetStart() const
{
	return GetData().m_time;
}

Float VirtualAnimTimelineItem_IK::SetStart( Float start, Bool deepUpdate )
{
	Float var = m_timeline->RequestVirtualIKStartTime( m_dataID, start );
	return var;
}

Float VirtualAnimTimelineItem_IK::GetDuration() const
{
	return 0.f;
}

Float VirtualAnimTimelineItem_IK::SetDuration( Float duration )
{
	ASSERT( 0 );
	return duration;
}

Bool VirtualAnimTimelineItem_IK::GetMiddleText( String& text ) const
{
	text = TXT("IK"); //GetMotion().m_name.AsString();
	return true;
}

Bool VirtualAnimTimelineItem_IK::GetTooltip( String& text ) const
{
	const VirtualAnimationPoseIK& d = GetData();

	text = String::Printf( TXT("time %1.2f"), d.m_time );

	return true;
}

String VirtualAnimTimelineItem_IK::GetTrackName() const
{
	return CEdAnimBuilderTimeline::TRACK_IK;
}

wxColor	VirtualAnimTimelineItem_IK::GetColor() const
{
	return wxColor( 191, 255, 255, 128 );
}
