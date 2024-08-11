#include "build.h"

#include "behaviorEditorAnimationUsagePanel.h"
#include "behaviorEditor.h"

#include "../../common/engine/behaviorGraphInstance.h"
#include "../../common/engine/behaviorGraphStackSnapshot.h"

#include "timelineImpl/drawing.h"

#define ANIMATION_USAGE_PANEL_DEFAULT_TRACK_COUNT 4

class CDrawGroupVolatileAnimationUsagePanel: public TimelineImpl::CDrawGroupVolatile
{
public:
	CDrawGroupVolatileAnimationUsagePanel(CEdTimeline& owner, TimelineImpl::CDrawBuffer* drawBuffer)
		:	CDrawGroupVolatile( owner, drawBuffer)
	{
	}

protected:
	virtual void Draw()
	{
		CDrawGroupVolatile::Draw();
		CEdBehaviorEditorAnimationUsageTimeline* auTimeline = (CEdBehaviorEditorAnimationUsageTimeline *) &(GetOwner());
		TimelineImpl::CDrawBuffer* drawBuf = GetDrawBuffer();
		Int32 width, height;
		drawBuf->GetSize(width, height);
		PaintLines(drawBuf->GetGraphics(), width, height, auTimeline);
	}

	void PaintLines( Gdiplus::Graphics* g, Int32 width, Int32 height, CEdBehaviorEditorAnimationUsageTimeline* auTimeline )
	{
		if ( auTimeline->GetPanel()->GetPlayingPreviewing() )
		{
			PaintVerticalLineAt( g, auTimeline, auTimeline->GetPanel()->GetPreviewTimeStamp(), height, wxColour( 0, 255, 255 ) );
		}
		Float previewingSnapshotFromTimeStamp = auTimeline->GetPanel()->GetPreviewingSnapshotFromTimeStamp();
		if( previewingSnapshotFromTimeStamp >= 0.0f )
		{
			PaintVerticalLineAt( g, auTimeline, previewingSnapshotFromTimeStamp, height, wxColour( 255, 0, 255 ) );
		}
	}

	void PaintVerticalLineAt( Gdiplus::Graphics* g, CEdBehaviorEditorAnimationUsageTimeline* auTimeline, Float timeStamp, Int32 height, wxColour color )
	{
		Int32 i = auTimeline->CalculatePixelPos( timeStamp );

		Int32 animStart = auTimeline->CalculatePixelPos( 0.0f );
		Int32 animEnd =   auTimeline->CalculatePixelPos( auTimeline->GetActiveRangeDuration() );

		// Skip lines which are not in animation range
		if( i >= animStart && i <= animEnd )
		{
			// Skip invisible lines and these under track buttons
			if( i > auTimeline->TIMELINE_TRACK_BTN_WIDTH )
			{
				TimelineImpl::DrawLine(g, i, 1, i, height - 2, color, 2.0f );
			}
		}
	}

};

//////////////////////////////////////////////////////////////////////////

static String GetBehaviorEditorAnimationUsageTimelineTrackName(Uint32 trackNo)
{
	return String::Printf( TXT( "track %02i" ), trackNo + 1 );
}

//////////////////////////////////////////////////////////////////////////

struct SEdBehaviorEditorAnimationUsageEntry
{
public:
	CBehaviorGraphNode*				m_node;
	SBehaviorUsedAnimationData		m_animationUsage;
	Float							m_startTimeStamp;
	Float							m_endTimeStamp;
	Uint32							m_track;
	Bool							m_active; // still active, may extend it

	SEdBehaviorEditorAnimationUsageEntry(Float timeStamp, Int32 trackNo, CBehaviorGraphNode* node, const SBehaviorUsedAnimationData& animationUsage)
		:	m_node( node )
		,	m_animationUsage( animationUsage )
		,	m_startTimeStamp( timeStamp )
		,	m_endTimeStamp( timeStamp )
		,	m_track( trackNo )
		,	m_active( true )
	{}

	Bool CanBeExtended() const { return m_active; }
	void Close() { m_active = false; }

	Bool DoesContainTimeStamp(Float timeStamp) const { return m_startTimeStamp <= timeStamp && timeStamp <= m_endTimeStamp; }
	Bool IsAfterTimeStamp(Float timeStamp) const { return timeStamp < m_startTimeStamp; }

	void ClearAfterAndOpen(Float timeStamp)
	{
		if (m_endTimeStamp >= timeStamp && DoesContainTimeStamp(timeStamp))
		{
			m_endTimeStamp = timeStamp;
			m_active = true;
		}
	}

	void ExtendToContain(Float timeStamp)
	{
		m_startTimeStamp = Min(m_startTimeStamp, timeStamp);
		m_endTimeStamp = Max(m_endTimeStamp, timeStamp);
	}

	Float GetStartTime() const { return m_startTimeStamp; }
	Float GetEndTime() const { return m_endTimeStamp; }
	Float GetDuration() const { return m_endTimeStamp - m_startTimeStamp; }
	String GetAnimationName() const { return m_animationUsage.m_animation? m_animationUsage.m_animation->GetName().AsString() : TXT("??"); }
	String GetNodeName() const { return m_node? m_node->GetName() : String(TEXT("?")); }
	String GetNodeClassName() const { return m_node? m_node->GetClass()->GetName().AsString() : String(TEXT("?")); }

	Uint32 GetTrackNumber() const { return m_track; }
	String GetTrackName() const { return GetBehaviorEditorAnimationUsageTimelineTrackName( m_track ); }
	void SetTrackName(const String& trackName ) { /* can't be changed */ }
};

struct SEdBehaviorEditorAnimationUsageSnapshot
{
private:
	Float m_timeStamp;
	const CBehaviorGraph* m_forGraph;
	CBehaviorGraphInstanceSnapshot* m_snapshot;
	Vector m_worldPosition;
	EulerAngles m_worldRotation;

public:
	Float GetTimeStamp() const { return m_timeStamp; }
	CBehaviorGraphInstanceSnapshot* GetSnapshot(const CBehaviorGraph* forGraph) const { return m_forGraph == forGraph? m_snapshot : NULL; }

	Bool IsAfterTimeStamp(Float timeStamp) const { return m_timeStamp > timeStamp; }

public:
	SEdBehaviorEditorAnimationUsageSnapshot(Float timeStamp, const CBehaviorGraph* forGraph, CBehaviorGraphInstanceSnapshot* snapshot)
		: m_timeStamp( timeStamp )
		, m_forGraph( forGraph )
		, m_snapshot( nullptr )
		, m_worldPosition( Vector::ZEROS )
		, m_worldRotation( EulerAngles::ZEROS )
	{
		// set snapshot with method (as it does a little bit more than just pure set)
		SetSnapshot( snapshot );
	}

	~SEdBehaviorEditorAnimationUsageSnapshot()
	{
		Clear();
	}

	void SetSnapshot(CBehaviorGraphInstanceSnapshot* snapshot)
	{
		Clear();
		m_snapshot = snapshot;
		// we don't want it to be deleted when garbage collection time comes
		m_snapshot->AddToRootSet();
	}

	void SetPositionAndRotation(const CEntity* fromEntity)
	{
		if (fromEntity)
		{
			m_worldPosition = fromEntity->GetWorldPosition();
			m_worldRotation = fromEntity->GetWorldRotation();
		}
	}

	void RestorePositionAndRotation(CEntity* toEntity) const
	{
		if (toEntity &&
			m_worldPosition != Vector::ZEROS &&
			m_worldRotation != EulerAngles::ZEROS)
		{
			toEntity->SetRawPlacement( &m_worldPosition, &m_worldRotation, NULL );
		}
	}

	void Clear()
	{
		if (m_snapshot)
		{
			m_snapshot->RemoveFromRootSet();
			m_snapshot->ReleaseAndDiscard();
			m_snapshot = NULL;
		}
	}
};

//////////////////////////////////////////////////////////////////////////

struct SEdBehaviorEditorAnimationUsageData
{
public:
	TDynArray< SEdBehaviorEditorAnimationUsageEntry* > m_entries;
	TDynArray< SEdBehaviorEditorAnimationUsageSnapshot* > m_snapshots;

	CEdBehaviorEditorAnimationUsagePanel* m_panel;

	SEdBehaviorEditorAnimationUsageData(CEdBehaviorEditorAnimationUsagePanel* panel)
	:	m_panel( panel )
	{}
	~SEdBehaviorEditorAnimationUsageData();

	void AddEntry(Float timeStamp, CBehaviorGraphNode* node, const SBehaviorUsedAnimationData& animationUsage);
	void AddSnapshot(Float timeStamp, const CEntity* ownerEntity, const CBehaviorGraph* forGraph, CBehaviorGraphInstanceSnapshot* snapshot);
	void Clear();
	void ClearAfter(Float timeStamp);

	void CloseNotAtTimeStamp(Float timeStamp);

	Uint32 FindEmptyTrack(Float timeStamp) const;

	Bool Empty() const { return m_entries.Size() == 0; }

	const SEdBehaviorEditorAnimationUsageSnapshot* FindClosestSnapshot(Float timeStamp, Int32 offset = 0) const;
	const SEdBehaviorEditorAnimationUsageSnapshot* FindSnapshotArOtBefore(Float timeStamp) const;
};

SEdBehaviorEditorAnimationUsageData::~SEdBehaviorEditorAnimationUsageData()
{
	Clear();
}

void SEdBehaviorEditorAnimationUsageData::Clear()
{
	m_panel->GetAnimationUsageTimeLine()->Clear();
	for ( TDynArray< SEdBehaviorEditorAnimationUsageEntry* >::iterator it = m_entries.Begin(); it != m_entries.End(); ++ it )
	{
		delete *it;
	}
	m_entries.Clear();
	for ( TDynArray< SEdBehaviorEditorAnimationUsageSnapshot* >::iterator it = m_snapshots.Begin(); it != m_snapshots.End(); ++ it )
	{
		delete *it;
	}
	m_snapshots.Clear();
}

void SEdBehaviorEditorAnimationUsageData::ClearAfter(Float timeStamp)
{
	// entries
	TDynArray< SEdBehaviorEditorAnimationUsageEntry* > entriesToRemove;
	for ( TDynArray< SEdBehaviorEditorAnimationUsageEntry* >::iterator it = m_entries.Begin(); it != m_entries.End(); ++ it )
	{
		SEdBehaviorEditorAnimationUsageEntry* entry = *it;
		if (entry->IsAfterTimeStamp(timeStamp))
		{
			entriesToRemove.PushBack(*it);
		}
		else
		{
			entry->ClearAfterAndOpen(timeStamp);
		}
	}
	for ( TDynArray< SEdBehaviorEditorAnimationUsageEntry* >::iterator it = entriesToRemove.Begin(); it != entriesToRemove.End(); ++ it )
	{
		m_panel->GetAnimationUsageTimeLine()->RemoveEntry(*it);
		m_entries.Remove(*it);
		delete *it;
	}
	m_panel->GetAnimationUsageTimeLine()->RecalculateTimes();
	// snapshots
	TDynArray< SEdBehaviorEditorAnimationUsageSnapshot* > snapshotsToRemove;
	for ( TDynArray< SEdBehaviorEditorAnimationUsageSnapshot* >::iterator it = m_snapshots.Begin(); it != m_snapshots.End(); ++ it )
	{
		SEdBehaviorEditorAnimationUsageSnapshot* snapshot = *it;
		if (snapshot->IsAfterTimeStamp(timeStamp))
		{
			snapshotsToRemove.PushBack(*it);
		}
	}
	for ( TDynArray< SEdBehaviorEditorAnimationUsageSnapshot* >::iterator it = snapshotsToRemove.Begin(); it != snapshotsToRemove.End(); ++ it )
	{
		m_snapshots.Remove(*it);
		delete *it;
	}
}

void SEdBehaviorEditorAnimationUsageData::AddEntry(Float timeStamp, CBehaviorGraphNode* node, const SBehaviorUsedAnimationData& animationUsage)
{
	for ( TDynArray< SEdBehaviorEditorAnimationUsageEntry* >::iterator it = m_entries.Begin(); it != m_entries.End(); ++ it )
	{
		SEdBehaviorEditorAnimationUsageEntry* entry = *it;
		if (entry->CanBeExtended() &&
			entry->m_node == node &&
			entry->m_animationUsage.m_animation == animationUsage.m_animation &&
			! animationUsage.m_firstUpdate)
		{
			entry->ExtendToContain(timeStamp);
			return;
		}
	}

	SEdBehaviorEditorAnimationUsageEntry* newEntry = new SEdBehaviorEditorAnimationUsageEntry(timeStamp, FindEmptyTrack(timeStamp), node, animationUsage);
	m_panel->GetAnimationUsageTimeLine()->AddEntry(newEntry);
	m_entries.PushBack(newEntry);
}

void SEdBehaviorEditorAnimationUsageData::AddSnapshot(Float timeStamp, const CEntity* ownerEntity, const CBehaviorGraph* forGraph, CBehaviorGraphInstanceSnapshot* snapshot)
{
	for ( TDynArray< SEdBehaviorEditorAnimationUsageSnapshot* >::iterator it = m_snapshots.Begin(); it != m_snapshots.End(); ++ it )
	{
		SEdBehaviorEditorAnimationUsageSnapshot* snapshotEntry = *it;
		if (snapshotEntry->GetTimeStamp() == timeStamp)
		{
			snapshotEntry->SetSnapshot(snapshot);
			snapshotEntry->SetPositionAndRotation(ownerEntity);
			return;
		}
	}

	SEdBehaviorEditorAnimationUsageSnapshot* newSnapshotEntry = new SEdBehaviorEditorAnimationUsageSnapshot(timeStamp, forGraph, snapshot);
	newSnapshotEntry->SetPositionAndRotation(ownerEntity);
	m_snapshots.PushBack(newSnapshotEntry);
}

void SEdBehaviorEditorAnimationUsageData::CloseNotAtTimeStamp(Float timeStamp)
{
	for ( TDynArray< SEdBehaviorEditorAnimationUsageEntry* >::iterator it = m_entries.Begin(); it != m_entries.End(); ++ it )
	{
		SEdBehaviorEditorAnimationUsageEntry* entry = *it;
		if (entry->CanBeExtended() &&
			! entry->DoesContainTimeStamp(timeStamp))
		{
			entry->Close();
		}
	}
}

Uint32 SEdBehaviorEditorAnimationUsageData::FindEmptyTrack(Float timeStamp) const
{
	TDynArray< Bool > tracksUsed;
	for ( TDynArray< SEdBehaviorEditorAnimationUsageEntry* >::const_iterator it = m_entries.Begin(); it != m_entries.End(); ++ it )
	{
		SEdBehaviorEditorAnimationUsageEntry* entry = *it;
		if (entry->DoesContainTimeStamp(timeStamp) ||
			entry->CanBeExtended()) // if it can be extended, it means that it still uses this track
		{
			while (tracksUsed.Size() <= entry->m_track)
			{
				tracksUsed.PushBack(false);
			}
			tracksUsed[entry->m_track] = true;
		}
	}
	Uint32 availableTrack = 0;
	while (availableTrack < tracksUsed.Size() && tracksUsed[availableTrack])
	{
		++ availableTrack;
	}
	return availableTrack;
}

const SEdBehaviorEditorAnimationUsageSnapshot* SEdBehaviorEditorAnimationUsageData::FindClosestSnapshot(Float timeStamp, Int32 offset) const
{
	SEdBehaviorEditorAnimationUsageSnapshot* closestSnapshot = NULL;
	Float distToClosestSnapshot = 0.0f;
	for ( TDynArray< SEdBehaviorEditorAnimationUsageSnapshot* >::const_iterator it = m_snapshots.Begin(); it != m_snapshots.End(); ++ it )
	{
		SEdBehaviorEditorAnimationUsageSnapshot* snapshot = *it;
		Float distToSnapshot = Abs(timeStamp - snapshot->GetTimeStamp());
		if (distToSnapshot < distToClosestSnapshot ||
			closestSnapshot == NULL)
		{
			closestSnapshot = snapshot;
			distToClosestSnapshot = distToSnapshot;
		}
	}
	if (closestSnapshot)
	{
		Int32 snapshotIdx = m_snapshots.GetIndex(closestSnapshot);
		snapshotIdx = Clamp(snapshotIdx + offset, 0, m_snapshots.SizeInt() - 1);
		closestSnapshot = m_snapshots[snapshotIdx];
	}
	return closestSnapshot;
}

const SEdBehaviorEditorAnimationUsageSnapshot* SEdBehaviorEditorAnimationUsageData::FindSnapshotArOtBefore(Float timeStamp) const
{
	const SEdBehaviorEditorAnimationUsageSnapshot* closestSnapshot = NULL;
	Float distToClosestSnapshot = 0.0f;
	for ( TDynArray< SEdBehaviorEditorAnimationUsageSnapshot* >::const_iterator it = m_snapshots.Begin(); it != m_snapshots.End(); ++ it )
	{
		const SEdBehaviorEditorAnimationUsageSnapshot* snapshot = *it;
		Float distToSnapshot = timeStamp - snapshot->GetTimeStamp();
		if (distToSnapshot >= 0.0f &&
			(distToSnapshot < distToClosestSnapshot ||
			 closestSnapshot == NULL))
		{
			closestSnapshot = snapshot;
			distToClosestSnapshot = distToSnapshot;
		}
	}
	return closestSnapshot;
}

//////////////////////////////////////////////////////////////////////////

class CEdBehaviorEditorAnimationUsageTimelineEvent : public ITimelineItem
{
public:
	CEdBehaviorEditorAnimationUsageTimelineEvent( SEdBehaviorEditorAnimationUsageEntry* usageEntry )
		: m_usageEntry( usageEntry )
	{
		ASSERT( m_usageEntry != NULL );
	}

	SEdBehaviorEditorAnimationUsageEntry* GetEntry() const
	{ return m_usageEntry; }

	virtual Bool IsLeftResizable() const
	{ return false; }

	virtual Bool IsRightResizable() const
	{ return false; }

	virtual Bool IsMovable() const
	{ return false; }

	virtual Float GetStart() const
	{ return m_usageEntry->GetStartTime(); }

	virtual Float SetStart( Float start, Bool deepUpdate ) override
	{ return GetStart(); }

	virtual Bool IsDuration() const
	{ return true; }

	virtual Float GetDuration() const
	{ return m_usageEntry->GetDuration(); }

	virtual Float SetDuration( Float duration )
	{ return GetDuration(); }

	virtual Bool GetTopText( String& text ) const
	{ 
		text = m_usageEntry->GetAnimationName();
		return true;
	}

	virtual Bool GetTooltip( String& text ) const
	{ 
		text = String::Printf( TXT( "%s in %s node %s" ), m_usageEntry->GetAnimationName().AsChar(), m_usageEntry->GetNodeClassName().AsChar(), m_usageEntry->GetNodeName().AsChar() );
		return true;
	}

	virtual Bool GetMiddleText( String& text ) const
	{ return false; }

	virtual String GetTrackName() const
	{ return m_usageEntry->GetTrackName(); }

	virtual void SetTrackName( const String& trackName )
	{ m_usageEntry->SetTrackName( trackName ); }

	virtual const Char* GetIconName() const
	{ return NULL; }

	virtual Gdiplus::Bitmap* GetIcon() const
	{ return NULL; }

	virtual const wxBitmap* GetWxIcon() const override
	{ return nullptr; }

	virtual void UpdatePresentation()
	{}

	virtual Bool IsEditable() const
	{ return false;	}

	virtual Bool IsRemovable() const
	{ return false; }

	virtual Bool IsCopyable() const
	{ return false; }

	virtual void SetProperty( IProperty* property, ITimelineItem* sourceItem )
	{ return; }

	virtual void CustomDraw( CEdTimeline* timeline, const wxRect& rect ) const
	{}

	virtual void SetState( Int32 value ) 
	{}

	virtual Int32 GetState() const 
	{ return 0; }

	virtual String GetTypeName() const
	{ return TXT( "CEdBehaviorEditorAnimationUsageTimelineEvent" ); }

	virtual wxColor GetColor() const
	{ return wxColor( 100, 100, 100, 150 ); }

private:
	SEdBehaviorEditorAnimationUsageEntry* m_usageEntry;
};

//////////////////////////////////////////////////////////////////////////

wxBEGIN_EVENT_TABLE( CEdBehaviorEditorAnimationUsagePanel, CEdBehaviorEditorSimplePanel )
	EVT_BUTTON( XRCID( "ClearData" ), CEdBehaviorEditorAnimationUsagePanel::OnClearData )
	EVT_CHECKBOX( XRCID( "GatherData" ), CEdBehaviorEditorAnimationUsagePanel::OnGatherData )
	EVT_CHECKBOX( XRCID( "Snapshots" ), CEdBehaviorEditorAnimationUsagePanel::OnGatherSnapshots )
	EVT_CHECKBOX( XRCID( "SnapshotsWithInterval" ), CEdBehaviorEditorAnimationUsagePanel::OnGatherSnapshotsWithInterval )
	EVT_MENU( XRCID( "PreviewPlay" ), CEdBehaviorEditorAnimationUsagePanel::OnPreview )
	EVT_MENU( XRCID( "PreviewPrev" ), CEdBehaviorEditorAnimationUsagePanel::OnPreviewPrev )
	EVT_MENU( XRCID( "PreviewNext" ), CEdBehaviorEditorAnimationUsagePanel::OnPreviewNext )
wxEND_EVENT_TABLE()

#define DEFAULT_FRAME_DELTA ( 1.0f / 30.0f )
#define MAX_FRAME_DELTA ( 1.0f )

CEdBehaviorEditorAnimationUsagePanel::CEdBehaviorEditorAnimationUsagePanel( CEdBehaviorEditor* editor )
:	CEdBehaviorEditorSimplePanel( editor )
,	m_playingPreviewing( false )
,	m_previewTimeStamp( 0.0f )
,	m_gatherData( true )
,	m_disableGatherDataTemporarily( false )
,	m_gatherSnapshots( true )
,	m_gatherSnapshotsWithInterval( false )
{
	m_data = new SEdBehaviorEditorAnimationUsageData( this );

//	VERIFY( wxXmlResource::Get()->LoadPanel( this, editor, wxT( "BehaviourEditorAnimationUsagePanel" ) ) );

	// Load icons
	m_playIcon	= SEdResources::GetInstance().LoadBitmap( TEXT( "IMG_CONTROL_PLAY" ) );
	m_pauseIcon = SEdResources::GetInstance().LoadBitmap( TEXT( "IMG_CONTROL_PAUSE" ) );

	// Load designed frame from resource
	wxPanel* innerPanel = wxXmlResource::Get()->LoadPanel( this, wxT( "BehaviourEditorAnimationUsagePanel" ) );
	SetMinSize( innerPanel->GetMinSize() );

	wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );
	sizer->Add( innerPanel, 1, wxEXPAND|wxALL, 0 );

	SetSizer( sizer );	
	Layout();

	{
		// Create timeline
		m_timelinePanel = XRCCTRL( *this, "timelinePanel", wxPanel );
		ASSERT( m_timelinePanel != NULL );

		m_timeline = new CEdBehaviorEditorAnimationUsageTimeline( m_timelinePanel, this );
		//m_timeline->Connect( usrEVT_TIMELINE_REQUEST_SET_TIME, wxCommandEventHandler( CEdAnimEventsEditor::OnRequestSetTime ), NULL, this );
		//m_timeline->Connect( usrEVT_REFRESH_PREVIEW, wxCommandEventHandler( CEdAnimEventsEditor::OnRefreshPreview ), NULL, this );
		//m_timeline->Connect( usrEVT_TIMELINE_RESIZED, wxCommandEventHandler( CEdAnimEventsEditor::OnTimelineResized ), NULL, this );
		wxBoxSizer* sizer1 = new wxBoxSizer( wxVERTICAL );
		sizer1->Add( m_timeline, 1, wxEXPAND, 0 );
		m_timelinePanel->SetSizer( sizer1 );
		m_timelinePanel->Layout();

		m_previewControlToolbar = XRCCTRL( *this, "playbackControlToolbar", wxToolBar );
	}

	Reset();

	UpdateStateOfControls();
}

CEdBehaviorEditorAnimationUsagePanel::~CEdBehaviorEditorAnimationUsagePanel()
{
	delete m_data;
}

void CEdBehaviorEditorAnimationUsagePanel::OnClearData( wxCommandEvent& event )
{
	Reset();
	GetAnimationUsageTimeLine()->RefreshViewWithInterval(true);
}

void CEdBehaviorEditorAnimationUsagePanel::OnGatherData( wxCommandEvent& event )
{
	m_gatherData = XRCCTRL( *this, "GatherData", wxCheckBox )->GetValue();
	UpdateStateOfControls();
}

void CEdBehaviorEditorAnimationUsagePanel::OnGatherSnapshots( wxCommandEvent& event )
{
	m_gatherSnapshots = XRCCTRL( *this, "Snapshots", wxCheckBox )->GetValue();
	UpdateStateOfControls();
}

void CEdBehaviorEditorAnimationUsagePanel::OnGatherSnapshotsWithInterval( wxCommandEvent& event )
{
	m_gatherSnapshotsWithInterval = XRCCTRL( *this, "SnapshotsWithInterval", wxCheckBox )->GetValue();
	UpdateStateOfControls();
}

void CEdBehaviorEditorAnimationUsagePanel::OnPreview( wxCommandEvent& event )
{
	SetPreviewing( ! m_playingPreviewing );
}

void CEdBehaviorEditorAnimationUsagePanel::OnPreviewPrev( wxCommandEvent& event )
{
	m_playingPreviewing = false;
	RestoreSnapshot(m_data->FindClosestSnapshot(m_previewTimeStamp, -1));
	GetAnimationUsageTimeLine()->RefreshViewWithInterval(true);
}

void CEdBehaviorEditorAnimationUsagePanel::OnPreviewNext( wxCommandEvent& event )
{
	m_playingPreviewing = false;
	RestoreSnapshot(m_data->FindClosestSnapshot(m_previewTimeStamp, 1));
	GetAnimationUsageTimeLine()->RefreshViewWithInterval(true);
}

void CEdBehaviorEditorAnimationUsagePanel::UpdateStateOfControls()
{
	XRCCTRL( *this, "GatherData", wxCheckBox )->SetValue( m_gatherData );
	XRCCTRL( *this, "Snapshots", wxCheckBox )->SetValue( m_gatherSnapshots );
	XRCCTRL( *this, "SnapshotsWithInterval", wxCheckBox )->SetValue( m_gatherSnapshotsWithInterval );
	if ( m_playingPreviewing )
	{
		m_previewControlToolbar->SetToolNormalBitmap( XRCID( "PreviewPlay" ), m_pauseIcon );
	}
	else
	{
		m_previewControlToolbar->SetToolNormalBitmap( XRCID( "PreviewPlay" ), m_playIcon );
	}
}

void CEdBehaviorEditorAnimationUsagePanel::SetPreviewing( Bool previewing )
{
	if (m_playingPreviewing != previewing)
	{
		m_playingPreviewing = previewing;
		m_previewTimeStamp = Clamp( m_previewTimeStamp, 0.0f, GetAnimationUsageTimeLine()->GetActiveRangeDuration() );
		UpdateStateOfControls();
	}
}

void CEdBehaviorEditorAnimationUsagePanel::Reset()
{
	m_timeStamp = 0.0f;
	m_previewTimeStamp = Clamp( m_previewTimeStamp, 0.0f, GetAnimationUsageTimeLine()->GetActiveRangeDuration() );
	m_previewingSnapshotFromTimeStamp = -1.0f;
	m_data->Clear();
}

Bool CEdBehaviorEditorAnimationUsagePanel::RequiresCustomTick() const
{
	return m_playingPreviewing;
}

void CEdBehaviorEditorAnimationUsagePanel::OnCustomTick( Float dt )
{
	Float prevTimeStamp = m_previewTimeStamp;
	m_previewTimeStamp += dt;
	const SEdBehaviorEditorAnimationUsageSnapshot* restoreSnapshot = NULL;
	if (m_previewTimeStamp >= GetAnimationUsageTimeLine()->GetActiveRangeDuration())
	{
		m_previewTimeStamp = 0.0f;
		restoreSnapshot = m_data->FindClosestSnapshot(0.0f);
	}
	else
	{
		const SEdBehaviorEditorAnimationUsageSnapshot* prevSnapshot = m_data->FindSnapshotArOtBefore(prevTimeStamp);
		const SEdBehaviorEditorAnimationUsageSnapshot* currSnapshot = m_data->FindSnapshotArOtBefore(m_previewTimeStamp);
		if (currSnapshot != prevSnapshot)
		{
			restoreSnapshot = currSnapshot;
		}
	}
	if (restoreSnapshot)
	{
		RestoreSnapshot(restoreSnapshot);
	}
	GetAnimationUsageTimeLine()->RefreshViewWithInterval();
}

void CEdBehaviorEditorAnimationUsagePanel::OnTick( Float dt )
{
	if ( ! GetEditor()->IsToolActive( this ) )
	{
		return;
	}

	m_gatherDataNow = false;
	if (m_disableGatherDataTemporarily || ! m_gatherData)
	{
		return;
	}

	const CBehaviorGraph* graph = GetBehaviorGraph();
	const CBehaviorGraphInstance* instance = GetBehaviorGraphInstance();

	if (! graph || ! instance || (GetEditor()->IsPaused() && ! GetEditor()->IsDebugMode()))
	{
		return;
	}

	m_gatherDataNow	= true;
	m_deltaTime = dt;

	SetPreviewing( false );

	m_data->ClearAfter(m_timeStamp);

	if (! m_data->Empty())
	{
		m_timeStamp += dt;
	}
	m_previewingSnapshotFromTimeStamp = -1.0f;

	// Force activation alpha processing
	if (! GetEditor()->IsActivationAlphaEnabled())
	{
		CBehaviorGraphInstance* instance = GetBehaviorGraphInstance();
		if ( instance )
		{
			instance->ProcessActivationAlphas();
		}
	}

	TDynArray< CBehaviorGraphNode* > nodes;
	graph->GetAllNodes( nodes );

	Int32 activeCount = 0;
	Float activeWeight = 0.0f;
	for ( Uint32 i=0; i<nodes.Size(); i++ )
	{
		Float nodesActivationWeight = nodes[i]->GetActivationAlpha( *instance );
		if (nodesActivationWeight > 0.0f || nodes[i]->IsActive( *instance ))
		{
			TDynArray<SBehaviorUsedAnimationData> collectorArray;
			++ activeCount;
			activeWeight += nodesActivationWeight;
			nodes[i]->CollectAnimationUsageData( *instance, collectorArray );
			for (TDynArray<SBehaviorUsedAnimationData>::iterator it = collectorArray.Begin(); it != collectorArray.End(); ++ it)
			{
				if (it->m_animation)
				{
					m_data->AddEntry(m_timeStamp, nodes[i], *it);
				}
			}
		}
	}
	

	// close all entries that were not updated during this frame
	m_data->CloseNotAtTimeStamp(m_timeStamp);

	// update times for timeline
	m_timeline->UpdateTimes(m_timeStamp);

	if (! GetEditor()->IsDebugMode())
	{
		CreateSnapshotIfNeeded();
	}
}

void CEdBehaviorEditorAnimationUsagePanel::OnDebuggerPostSamplePose( const SBehaviorGraphOutput& pose )
{
	if (! m_gatherDataNow)
	{
		return;
	}

	if (GetEditor()->IsDebugMode())
	{
		CreateSnapshotIfNeeded();
	}
}

void CEdBehaviorEditorAnimationUsagePanel::CreateSnapshotIfNeeded()
{
	// store snapshot after "sample", as during "sample" it could be modified
	if (m_gatherSnapshots && ! m_disableGatherDataTemporarily)
	{
		Bool createSnapshot = true;
		if (m_gatherSnapshotsWithInterval)
		{
			createSnapshot = false;
			m_timeToNextSnapshot -= m_deltaTime;
			if (m_timeToNextSnapshot <= 0.0f)
			{
				m_timeToNextSnapshot += 1.0f;
				createSnapshot = true;
			}
		}
		if (createSnapshot)
		{
			m_data->AddSnapshot(m_timeStamp, FindOwnerEntity(), GetBehaviorGraphInstance()->GetGraph(), GetBehaviorGraphInstance()->CreateSnapshot(GetBehaviorGraph(), true));
		}
	}
}

CEntity* CEdBehaviorEditorAnimationUsagePanel::FindOwnerEntity()
{
	if (CAnimatedComponent* animComp = GetAnimatedComponent())
	{
		return animComp->GetEntity();
	}
	else
	{
		return NULL;
	}
}

void CEdBehaviorEditorAnimationUsagePanel::RestoreSnapshotAt(Float timeStamp)
{
	RestoreSnapshot(m_data->FindClosestSnapshot(timeStamp));
}

void CEdBehaviorEditorAnimationUsagePanel::RestoreSnapshot(const SEdBehaviorEditorAnimationUsageSnapshot* snapshot)
{
	if (snapshot)
	{
		CBehaviorGraphInstance* instance = GetBehaviorGraphInstance();
		CEdBehaviorEditor* behaviorEditor = GetEditor();
		if (instance && behaviorEditor)
		{
			if (CBehaviorGraphInstanceSnapshot* snapshotToRestore = snapshot->GetSnapshot(instance->GetGraph()))
			{
				instance->RestoreSnapshot(snapshotToRestore);
				snapshot->RestorePositionAndRotation(FindOwnerEntity());
				m_timeStamp = snapshot->GetTimeStamp();
				m_previewingSnapshotFromTimeStamp = m_timeStamp;
				if (! m_playingPreviewing)
				{
					m_previewTimeStamp = m_timeStamp;
				}
				// pause it now
				m_disableGatherDataTemporarily = true;
				if (! behaviorEditor->IsDebugMode())
				{
					behaviorEditor->Tick(0.0001f); // tick it just a little bit
				}
				else
				{
					if (CEntity* entity = FindOwnerEntity())
					{
						entity->ForceUpdateTransformNodeAndCommitChanges();
					}
					// TODO just force pose - this will update base component, but everything else will get updated in next frame - it is glitch that could be fixed, but for debugging it is still fine, just as it is
					if (CAnimatedComponent* animatedComponent = instance->GetAnimatedComponentUnsafe())
					{
						animatedComponent->ForceBehaviorPose();
						animatedComponent->ForceUpdateTransformNodeAndCommitChanges();
					}
				}
				instance->ProcessActivationAlphas();
				behaviorEditor->SetPause(true);
				if (CEdBehaviorGraphEditor* behaviorGraphEditor = behaviorEditor->GetGraphEditor())
				{
					behaviorGraphEditor->Refresh(true);
				}
				m_disableGatherDataTemporarily = false;
			}
		}
	}	
}

//////////////////////////////////////////////////////////////////////////

wxBEGIN_EVENT_TABLE( CEdBehaviorEditorAnimationUsageTimeline, CEdTimeline )
	EVT_MOTION( CEdBehaviorEditorAnimationUsageTimeline::OnMouseMove )
wxEND_EVENT_TABLE()

CEdBehaviorEditorAnimationUsageTimeline::CEdBehaviorEditorAnimationUsageTimeline( wxPanel* parentPanel, CEdBehaviorEditorAnimationUsagePanel* panel )
	: CEdTimeline( parentPanel )
	, m_panel( panel )
	, m_updateTickInterval( 0 )
{
	RegisterDrawGroupTracksPinned(new TimelineImpl::CDrawGroupTracks(*this, &m_pinnedBuffer));
	RegisterDrawGroupTracksDefault(new TimelineImpl::CDrawGroupTracks(*this, &m_defaultBuffer));
	RegisterDrawGroupTimebar(new TimelineImpl::CDrawGroupTimebar(*this, &m_timebarBuffer));
	RegisterDrawGroupVolatile(new CDrawGroupVolatileAnimationUsagePanel(*this, &m_volatileBuffer));

	AddDefaultTracks();
}

CEdBehaviorEditorAnimationUsageTimeline::~CEdBehaviorEditorAnimationUsageTimeline()
{}

void CEdBehaviorEditorAnimationUsageTimeline::AddEntry( SEdBehaviorEditorAnimationUsageEntry* entry )
{
	if (m_items.Size() == 0)
	{
		// initialize to first entry
		SetTimeLimits(entry->GetStartTime(), entry->GetEndTime());
	}
	AddItem( new CEdBehaviorEditorAnimationUsageTimelineEvent( entry ) );
	while ( entry->GetTrackNumber() >= m_tracks.Size() )
	{
		AddTrack( GetBehaviorEditorAnimationUsageTimelineTrackName( m_tracks.Size() ) );
	}
}

void CEdBehaviorEditorAnimationUsageTimeline::RemoveEntry( SEdBehaviorEditorAnimationUsageEntry* entry )
{
	for ( TDynArray< ITimelineItem* >::iterator it = m_items.Begin(); it != m_items.End(); ++ it )
	{
		if (CEdBehaviorEditorAnimationUsageTimelineEvent* timelineEntry = (CEdBehaviorEditorAnimationUsageTimelineEvent*)(*it))
		{
			if (timelineEntry->GetEntry() == entry)
			{
				m_items.Remove(*it);
				return;
			}
		}
	}
}

void CEdBehaviorEditorAnimationUsageTimeline::Clear()
{
	ClearItems();
	RecalculateTimes();
	DeleteTracks();
	AddDefaultTracks();
}

void CEdBehaviorEditorAnimationUsageTimeline::AddDefaultTracks()
{
	for ( Uint32 trackNo = 0; trackNo < ANIMATION_USAGE_PANEL_DEFAULT_TRACK_COUNT; ++ trackNo )
	{
		AddTrack( GetBehaviorEditorAnimationUsageTimelineTrackName( trackNo ) );
	}
}

void CEdBehaviorEditorAnimationUsageTimeline::UpdateTimes(Float timeStamp)
{
	m_activeRangeDuration = Max(timeStamp, m_activeRangeDuration);
	SetTimeLimits(Min(timeStamp, m_timeLimitMin), Max(timeStamp, m_timeLimitMax));

	Float startTime = m_activeRangeTimeOffset;
	Float endTime = m_visibleRangeDuration - m_activeRangeTimeOffset;
	m_activeRangeTimeOffset = Min(m_activeRangeTimeOffset, Min(m_timeLimitMin, timeStamp));
	m_visibleRangeDuration = m_activeRangeTimeOffset + Max(endTime, Max(m_timeLimitMax, timeStamp));

	RefreshViewWithInterval();
}

void CEdBehaviorEditorAnimationUsageTimeline::RecalculateTimes()
{
	Float newLimitMin = 0.0f;
	Float newLimitMax = 0.0f;
	for ( TDynArray< ITimelineItem* >::iterator it = m_items.Begin(); it != m_items.End(); ++ it )
	{
		if (CEdBehaviorEditorAnimationUsageTimelineEvent* timelineEntry = (CEdBehaviorEditorAnimationUsageTimelineEvent*)(*it))
		{
			newLimitMin = Min(newLimitMin, timelineEntry->GetEntry()->GetStartTime());
			newLimitMax = Max(newLimitMax, timelineEntry->GetEntry()->GetEndTime());
		}
	}
	SetTimeLimits(newLimitMin, newLimitMax);
	m_activeRangeDuration = newLimitMax;

	RefreshViewWithInterval(true);
}

void CEdBehaviorEditorAnimationUsageTimeline::RefreshViewWithInterval(Bool forceNow)
{
	-- m_updateTickInterval;
	if (m_updateTickInterval <= 0 || forceNow)
	{
		m_updateTickInterval = 5;
		NotifyBufferIsInvalid();
		CenterPosition(m_visibleRangeDuration * 0.5f - m_activeRangeTimeOffset);
		CalculateNewGrid();
		Refresh();
	}
}

void CEdBehaviorEditorAnimationUsageTimeline::SelectionChanged()
{
	if (m_selectedItems.Size() > 0 &&
		m_panel &&
		m_panel->GetEditor() &&
		m_panel->GetEditor()->GetGraphEditor())
	{
		CBehaviorGraphNode* nodeToSelect = ((CEdBehaviorEditorAnimationUsageTimelineEvent*)(m_selectedItems[0]))->GetEntry()->m_node;
		CEdBehaviorGraphEditor* behaviorGraphEditor = m_panel->GetEditor()->GetGraphEditor();

		CUndoBehaviorGraphSetRoot::CreateStep( *(behaviorGraphEditor->GetUndoManager()), behaviorGraphEditor );
		behaviorGraphEditor->DisplayBlockContent( Cast<CBehaviorGraphNode>(nodeToSelect->GetParent()) );
		behaviorGraphEditor->DeselectAllBlocks();
		behaviorGraphEditor->SelectBlock( nodeToSelect, true );
		//behaviorGraphEditor->ZoomExtents();
		behaviorGraphEditor->FocusOnBlock( nodeToSelect );
	}
	NotifyBufferIsInvalid();
	Refresh();
}

void CEdBehaviorEditorAnimationUsageTimeline::OnLeftMouseDoubleClick(Int32 x, Int32 y)
{
	m_panel->RestoreSnapshotAt( CalculateTimePos(x) );
	NotifyBufferIsInvalid();
	Refresh();
}

void CEdBehaviorEditorAnimationUsageTimeline::OnMouseMove( wxMouseEvent& event )
{
	// CEdBehaviorEditor doesn't update CEdBehaviorEditorAnimationUsagePanel
	// (and hence CEdBehaviorEditorAnimationUsageTimeline) on a regular basis
	// unless animation is being played. This is why we request repaint here.
	Repaint();

	// We're not done with this event yet. Let it propagate to CEdTimeline.
	event.Skip();
}

void CEdBehaviorEditorAnimationUsageTimeline::StoreLayout()
{
	// empty - this functionality is not supported
}

void CEdBehaviorEditorAnimationUsageTimeline::RestoreLayout()
{
	// empty - this functionality is not supported
}

//////////////////////////////////////////////////////////////////////////

#undef ANIMATION_USAGE_PANEL_DEFAULT_TRACK_COUNT
