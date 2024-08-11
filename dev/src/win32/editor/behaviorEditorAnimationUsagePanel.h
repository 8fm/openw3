#pragma once

#include "timeline.h"
#include "behaviorEditorPanel.h"

struct SEdBehaviorEditorAnimationUsageEntry;
struct SEdBehaviorEditorAnimationUsageData;

class CEdBehaviorEditorAnimationUsagePanel;

//////////////////////////////////////////////////////////////////////////

class CEdBehaviorEditorAnimationUsageTimeline : public CEdTimeline
{
public:
	CEdBehaviorEditorAnimationUsageTimeline( wxPanel* parentPanel, CEdBehaviorEditorAnimationUsagePanel* panel );
	virtual ~CEdBehaviorEditorAnimationUsageTimeline();

public:
	virtual void EditProperties( ITimelineItem* item, CEdPropertiesPage& propertiesPage ) const {}

	virtual Bool CanChangeDefaultTrack() const { return false; }
	virtual Bool CanResizeOverEnd() const { return false; }
	virtual Bool CanUseSelectionItemMenu() const { return false; }

	virtual void SelectionChanged();

	virtual void SerializeItem( ITimelineItem* item, IFile& file ) {}
	virtual ITimelineItem* DeserializeItem( IFile& file ) { return NULL; }

protected:
	virtual void OnLeftMouseDoubleClick( Int32 x, Int32 y );

public:
	void AddEntry( SEdBehaviorEditorAnimationUsageEntry* entry );
	void RemoveEntry( SEdBehaviorEditorAnimationUsageEntry* entry );
	void Clear();
	void UpdateTimes(Float timeStamp);
	void RecalculateTimes();

protected:
	void AddDefaultTracks();

public:
	void RefreshViewWithInterval(Bool forceNow = false);

	CEdBehaviorEditorAnimationUsagePanel* GetPanel() const { return m_panel; }

private:
	void OnMouseMove( wxMouseEvent& event );

	virtual void StoreLayout() override;
	virtual void RestoreLayout() override;

	CEdBehaviorEditorAnimationUsagePanel* m_panel;
	Int32 m_updateTickInterval;

	wxDECLARE_EVENT_TABLE();
};

//////////////////////////////////////////////////////////////////////////

class CEdBehaviorEditorAnimationUsagePanel : public CEdBehaviorEditorSimplePanel
{
	wxDECLARE_EVENT_TABLE();

	friend class CEdBehaviorEditorAnimationUsageTimeline;

public:
	CEdBehaviorEditorAnimationUsagePanel( CEdBehaviorEditor* editor );
	~CEdBehaviorEditorAnimationUsagePanel();

	virtual wxString	GetPanelName() const	{ return wxT( "Animation Usage" ); }
	virtual wxString	GetPanelCaption() const { return wxT( "Animation Usage" ); }
	virtual wxString	GetInfo() const			{ return wxT( "Animation Usage Configuration" ); }

	CEdBehaviorEditorAnimationUsageTimeline* GetAnimationUsageTimeLine() const { return m_timeline; }

public:
	void RestoreSnapshotAt(Float timeStamp);

private:
	void RestoreSnapshot(const struct SEdBehaviorEditorAnimationUsageSnapshot* snapshot);
	void CreateSnapshotIfNeeded();

private:
	virtual Bool RequiresCustomTick() const;
	virtual void OnCustomTick( Float dt );
	virtual void OnTick( Float dt );
	virtual void OnDebuggerPostSamplePose( const SBehaviorGraphOutput& pose );

	virtual void OnReset() { Reset(); }
	virtual void OnPanelClose() { Reset(); }
	virtual void OnLoadEntity() { Reset(); }
	virtual void OnUnloadEntity() { Reset(); }
	virtual void OnInstanceReload() { Reset(); }
	virtual void OnGraphModified() { Reset(); }

protected:
	void OnClearData( wxCommandEvent& event );
	void OnGatherData( wxCommandEvent& event );
	void OnGatherSnapshots( wxCommandEvent& event );
	void OnGatherSnapshotsWithInterval( wxCommandEvent& event );
	void OnPreview( wxCommandEvent& event );
	void OnPreviewPrev( wxCommandEvent& event );
	void OnPreviewNext( wxCommandEvent& event );

	void UpdateStateOfControls();
	void SetPreviewing( Bool previewing );

private:
	void Reset();

	CEntity* FindOwnerEntity();

public:
	Bool GetPlayingPreviewing() const { return m_playingPreviewing; }
	Float GetPreviewTimeStamp() const { return m_previewTimeStamp; }
	Float GetPreviewingSnapshotFromTimeStamp() const { return m_previewingSnapshotFromTimeStamp; }

private:
	Bool										m_playingPreviewing;
	Float										m_previewTimeStamp;
	Float										m_previewingSnapshotFromTimeStamp;

	Bool										m_gatherDataNow;

	Bool										m_gatherData;
	Bool										m_disableGatherDataTemporarily;
	Bool										m_gatherSnapshots;
	Bool										m_gatherSnapshotsWithInterval;
	Float										m_timeToNextSnapshot;

	Float										m_timeStamp;
	Float										m_deltaTime;
	SEdBehaviorEditorAnimationUsageData*		m_data;

	CEdBehaviorEditorAnimationUsageTimeline*	m_timeline;
	wxPanel*									m_timelinePanel;

	wxToolBar*									m_previewControlToolbar;
	wxBitmap									m_playIcon;
	wxBitmap									m_pauseIcon;

};

