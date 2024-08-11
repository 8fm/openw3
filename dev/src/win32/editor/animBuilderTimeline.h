
#include "timeline.h"
#include "../../common/engine/virtualAnimation.h"

#pragma once

class CEdAnimationBuilder;
class VirtualAnimTimelineItem_Animation;

//////////////////////////////////////////////////////////////////////////

class CEdAnimBuilderTimeline : public CEdTimeline
{
	const IVirtualAnimationContainer*	m_container; // const is extremely important here!
	CEdAnimationBuilder*				m_builder;
	
	struct
	{
		struct
		{
			VirtualAnimation			m_virtualAnimation;
			VirtualAnimationID			m_animationID;
		};

		struct
		{
			VirtualAnimationMotion		m_virtualMotion;
			VirtualAnimationMotionID	m_virtualMotionID;
		};

		struct
		{
			VirtualAnimationPoseFK		m_virtualFK;
			VirtualAnimationPoseFKID	m_virtualFKID;
		};

		struct
		{
			VirtualAnimationPoseIK		m_virtualIK;
			VirtualAnimationPoseIKID	m_virtualIKID;
		};

	} m_prop;

	Bool								m_checkAutoBlendTimes;

	struct
	{
		Bool	m_pending;
		Int32		m_track;
	} m_dragInfo;

public:
	static const String	TRACK_BASE;
	static const String	TRACK_OVERRIDE;
	static const String	TRACK_ADDITIVE;
	static const String	TRACK_FK;
	static const String	TRACK_IK;
	static const String	TRACK_MOTION;

	static const Float	STATIC_OFFSET;

public:
	CEdAnimBuilderTimeline( wxPanel* parent, CEdAnimationBuilder* builder );
	virtual ~CEdAnimBuilderTimeline();

	void Fill( const IVirtualAnimationContainer* container );

	void RefreshTimeline();

	static String GetTrackName( EVirtualAnimationTrack track, Int32 trackNumber );

public:
	Bool SelectEvent( const CName& eventName );

	virtual void PaintCanvas( Int32 width, Int32 height );

	virtual void OnDrawItem( const ITimelineItem* item, const wxRect& rect );

	Float RequestVirtualAnimationStartTime( const VirtualAnimationID& animation, Float startTime ) const;
	Float RequestVirtualMotionStartTime( const VirtualAnimationMotionID& motion, Float startTime ) const;
	Float RequestVirtualFKStartTime( const VirtualAnimationPoseFKID& dataID, Float startTime ) const;
	Float RequestVirtualIKStartTime( const VirtualAnimationPoseIKID& dataID, Float startTime ) const;

public: // CDropTarget
	virtual Bool OnDropText( wxCoord x, wxCoord y, String &text );
	virtual wxDragResult OnDragOver( wxCoord x, wxCoord y, wxDragResult def );
	virtual void OnLeave();

protected:
	virtual void EditProperties( ITimelineItem* item, CEdPropertiesPage& propertiesPage ) const;

	virtual void ItemPropertyChanged( TDynArray< ITimelineItem* >& items, const CName& propertyName ) override;

	virtual Bool CanChangeDefaultTrack() const { return false; }

	virtual Bool CanResizeOverEnd() const { return false; }

	virtual void SerializeItem( ITimelineItem* item, IFile& file ) {}
	virtual ITimelineItem* DeserializeItem( IFile& file ) { return NULL; }

	virtual void SelectionChanged();

	virtual Bool CanFillDefaultCanvasMenu() const { return false; }
	virtual Bool CanFillDefaultTrackMenu() const { return false; }
	virtual Bool CanFillDefaultItemMenu() const { return false; }
	virtual Bool CanFillDefaultTrackButtonMenu() const { return false; }

	virtual void FillTrackMenu( const String& name, wxMenu* menu );
	virtual void FillItemMenu( ITimelineItem* item, wxMenu* menu, Bool& addDefaults );

	virtual void OnLeftMouseDoubleClick( Int32 x, Int32 y );

protected:
	Float GetMaxAnimationTime() const;

	void CreateItems( EVirtualAnimationTrack track );
	void CreateItems_Motion();
	void CreateItems_FK();
	void CreateItems_IK();

	EVirtualAnimationTrack GetTrackType( Int32 track ) const;
	EVirtualAnimationTrack GetTrackType( const String& name ) const;
	Int32 GetTrackNumber( Int32 track ) const;
	Int32 GetTrackNumber( const String& name ) const;
	static String GetTrackName( EVirtualAnimationTrack track );
	String GetNextTrackName( EVirtualAnimationTrack track ) const;

	void PaintEmptySpaces();

	void SortTracks();

	Bool ItemsOverlapsLeft( const VirtualAnimTimelineItem_Animation* refItem, const VirtualAnimTimelineItem_Animation* b, Float& val ) const;
	Bool ItemsOverlapsRight( const VirtualAnimTimelineItem_Animation* refItem, const VirtualAnimTimelineItem_Animation* b, Float& val ) const;
	Bool ItemsOverlaps( const VirtualAnimTimelineItem_Animation* a, const VirtualAnimTimelineItem_Animation* b ) const;

	void CheckAutoBlendForAnims( const VirtualAnimationID& animation ) const;
	Bool CanCalcAutoBlends( const VirtualAnimTimelineItem_Animation* item ) const;
	void CalcAutoBlends( VirtualAnimTimelineItem_Animation* item );
	Bool FindItemsForAutoBlend( const VirtualAnimTimelineItem_Animation* refItem, const VirtualAnimTimelineItem_Animation*& left, const VirtualAnimTimelineItem_Animation*& right, Float& lVal, Float& rVal ) const;

	const VirtualAnimTimelineItem_Animation* FindPrevItemFromTrack( const VirtualAnimTimelineItem_Animation* item, EVirtualAnimationTrack track ) const;
	const VirtualAnimTimelineItem_Animation* FindPrevItem( const VirtualAnimTimelineItem_Animation* item ) const;
	const VirtualAnimTimelineItem_Animation* FindNextItemFromTrack( const VirtualAnimTimelineItem_Animation* item, EVirtualAnimationTrack track ) const;
	const VirtualAnimTimelineItem_Animation* FindNextItem( const VirtualAnimTimelineItem_Animation* item ) const;
	const VirtualAnimTimelineItem_Animation* FindClosestItemFromTrack( const VirtualAnimTimelineItem_Animation* item, EVirtualAnimationTrack track, Bool& isPrev ) const;
	const VirtualAnimTimelineItem_Animation* FindClosestItem( const VirtualAnimTimelineItem_Animation* item, Bool& isPrev ) const;

protected:
	void OnAddVTrack( wxCommandEvent& event );
	void OnAddMotionEvent( wxCommandEvent& event );
	void OnAddFKEvent( wxCommandEvent& event );
	void OnAddIKEvent( wxCommandEvent& event );
	void OnItemMoved( wxCommandEvent& event );
	void OnSnapItemToZeroPosition( wxCommandEvent& event );
	void OnSnapItemToPrevTrackItem( wxCommandEvent& event );
	void OnSnapItemToPrevItem( wxCommandEvent& event );
	void OnSnapItemToPrevTrackItemWithOffset( wxCommandEvent& event );
	void OnSnapItemToPrevItemWithOffset( wxCommandEvent& event );
	void OnSnapItemToNextTrackItem( wxCommandEvent& event );
	void OnSnapItemToNextItem( wxCommandEvent& event );
	void OnSnapItemToNextTrackItemWithOffset( wxCommandEvent& event );
	void OnSnapItemToNextItemWithOffset( wxCommandEvent& event );
	void OnSnapItemToClosest( wxCommandEvent& event );
	void OnSnapItemToClosestFromTrack( wxCommandEvent& event );
	void OnSnapItemToClosestWithOffset( wxCommandEvent& event );
	void OnSnapItemToClosestFromTrackWithOffset( wxCommandEvent& event );
	void OnDebugItemAnimationEnabled( wxCommandEvent& event );
	void OnDebugItemAnimationDisabled( wxCommandEvent& event );
	void OnItemRemoved( wxCommandEvent& event );
	void OnItemDuplicated( wxCommandEvent& event );
	void OnEditItemBones( wxCommandEvent& event );
	void OnCalcAutoBlends( wxCommandEvent& event );

private:
	virtual void RemoveItemImpl( ITimelineItem* item ) override;

	virtual void StoreLayout() override;
	virtual void RestoreLayout() override;
};

//////////////////////////////////////////////////////////////////////////

#define VANIM_ITEM_CAST_IMPLEMENT( type, _class ) template<> _class* ItemCast() { return m_type == type ? reinterpret_cast< _class* >( this ) : NULL; } template<> const _class* ItemCast() const { return m_type == type ? reinterpret_cast< const _class* >( this ) : NULL; }

class VirtualAnimTimelineItem_Animation;
class VirtualAnimTimelineItem_Motion;
class VirtualAnimTimelineItem_FK;
class VirtualAnimTimelineItem_IK;

// Wrapper for virtual animation for timeline
class VirtualAnimTimelineItem : public ITimelineItem
{
public:
	enum EItemType
	{
		IT_Animation,
		IT_Motion,
		IT_FK,
		IT_IK,
	};

protected:
	const CEdAnimBuilderTimeline*		m_timeline;
	const IVirtualAnimationContainer*	m_container;
	EItemType							m_type;

public:
	VirtualAnimTimelineItem( const CEdAnimBuilderTimeline* timeline, const IVirtualAnimationContainer* container, EItemType type );

	template< class T > T* ItemCast();
	template< class T > const T* ItemCast() const;

	VANIM_ITEM_CAST_IMPLEMENT( IT_Animation, VirtualAnimTimelineItem_Animation );
	VANIM_ITEM_CAST_IMPLEMENT( IT_Motion, VirtualAnimTimelineItem_Motion );
	VANIM_ITEM_CAST_IMPLEMENT( IT_FK, VirtualAnimTimelineItem_FK );
	VANIM_ITEM_CAST_IMPLEMENT( IT_IK, VirtualAnimTimelineItem_IK );

	virtual String				GetTypeName() const;
	virtual void				UpdatePresentation();
	//virtual Float				GetStart() const;
	//virtual Float				SetStart( Float start, Bool deepUpdate ) override;
	//virtual Bool				IsDuration() const;
	//virtual Float				GetDuration() const;
	//virtual Float				SetDuration( Float duration );
	virtual Bool				GetTopText( String& text ) const;
	//virtual Bool				GetMiddleText( String& text ) const;
	//virtual Bool				GetTooltip( String& text ) const;
	//virtual String			GetTrackName() const;
	virtual void				SetTrackName( const String& trackName );
	virtual Bool				IsRightResizable() const;
	virtual Bool				IsLeftResizable() const;
	virtual Bool				IsMovable() const;
	virtual Bool				IsCopyable() const;
	//virtual wxColor			GetColor() const;
	virtual Gdiplus::Bitmap*	GetIcon() const;
	virtual const wxBitmap*		GetWxIcon() const override;
	virtual Bool				IsEditable() const;
	virtual Bool				IsRemovable() const;
	virtual void				SetProperty( IProperty* property, ITimelineItem* sourceItem );
	virtual void				CustomDraw( CEdTimeline* timeline, const wxRect& rect ) const {}
	virtual void				SetState( Int32 value ) {}
	virtual Int32				GetState() const { return 0; }
};

class VirtualAnimTimelineItem_Animation : public VirtualAnimTimelineItem
{
public:
	VirtualAnimationID					m_animationID;

	VirtualAnimTimelineItem_Animation( const CEdAnimBuilderTimeline* timeline, const IVirtualAnimationContainer* container, const VirtualAnimationID& animation );

	virtual String				GetTypeName() const;
	virtual Bool				IsDuration() const;
	virtual Float				GetStart() const;
	virtual Float				SetStart( Float start, Bool deepUpdate ) override;
	virtual Float				GetDuration() const;
	virtual Float				SetDuration( Float duration );
	virtual Bool				GetMiddleText( String& text ) const;
	virtual Bool				GetTooltip( String& text ) const;
	virtual String				GetTrackName() const;
	virtual wxColor				GetColor() const;

	const VirtualAnimation& GetAnimation() const;
};

class VirtualAnimTimelineItem_Motion : public VirtualAnimTimelineItem
{
public:
	VirtualAnimationMotionID			m_motionID;

	VirtualAnimTimelineItem_Motion( const CEdAnimBuilderTimeline* timeline, const IVirtualAnimationContainer* container, const VirtualAnimationMotionID& motion );

	virtual String				GetTypeName() const;
	virtual Bool				IsDuration() const;
	virtual Float				GetStart() const;
	virtual Float				SetStart( Float start, Bool deepUpdate ) override;
	virtual Float				GetDuration() const;
	virtual Float				SetDuration( Float duration );
	virtual Bool				GetMiddleText( String& text ) const;
	virtual Bool				GetTooltip( String& text ) const;
	virtual String				GetTrackName() const;
	virtual wxColor				GetColor() const;

	const VirtualAnimationMotion& GetMotion() const;
};

class VirtualAnimTimelineItem_FK : public VirtualAnimTimelineItem
{
public:
	VirtualAnimationPoseFKID			m_dataID;

	VirtualAnimTimelineItem_FK( const CEdAnimBuilderTimeline* timeline, const IVirtualAnimationContainer* container, const VirtualAnimationPoseFKID& id );

	virtual String				GetTypeName() const;
	virtual Bool				IsDuration() const;
	virtual Float				GetStart() const;
	virtual Float				SetStart( Float start, Bool deepUpdate ) override;
	virtual Float				GetDuration() const;
	virtual Float				SetDuration( Float duration );
	virtual Bool				GetMiddleText( String& text ) const;
	virtual Bool				GetTooltip( String& text ) const;
	virtual String				GetTrackName() const;
	virtual wxColor				GetColor() const;

	const VirtualAnimationPoseFK&	GetData() const;
};

class VirtualAnimTimelineItem_IK : public VirtualAnimTimelineItem
{
public:
	VirtualAnimationPoseIKID			m_dataID;

	VirtualAnimTimelineItem_IK( const CEdAnimBuilderTimeline* timeline, const IVirtualAnimationContainer* container, const VirtualAnimationPoseIKID& id );

	virtual String				GetTypeName() const;
	virtual Bool				IsDuration() const;
	virtual Float				GetStart() const;
	virtual Float				SetStart( Float start, Bool deepUpdate ) override;
	virtual Float				GetDuration() const;
	virtual Float				SetDuration( Float duration );
	virtual Bool				GetMiddleText( String& text ) const;
	virtual Bool				GetTooltip( String& text ) const;
	virtual String				GetTrackName() const;
	virtual wxColor				GetColor() const;

	const VirtualAnimationPoseIK&	GetData() const;
};

//////////////////////////////////////////////////////////////////////////
