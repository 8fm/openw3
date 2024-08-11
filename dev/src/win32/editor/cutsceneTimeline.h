/*
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "timeline.h"

class CEdPropertiesPage;
class CEdCutsceneEditor;

class CEdCutsceneTimeline : public CEdTimeline
{
	wxDECLARE_EVENT_TABLE();

private:
	class CTimelineItemBlocking;
	class CTimelineItemEvent;
	class CTimelineItemEffect;
	class CTimelineItemBodypart;
	class CTimelineItemDialog;
	class CTimelineItemEnvironment;
	class CTimelineItemDof;
	class CTimelineItemSound;
	class CTimelineItemAnimation;
	class CTimelineItemItem;
	class CTimelineItemFootstep;
	class CTimelineItemMusic;
	class CTimelineItemQuest;
	class CTimelineItemFade;
	class CTimelineItemAmbVolume;
	class CTimelineItemReattachItem;
	class CTimelineItemMorphing;
	class CTimelineItemClippingPlanes;
	class CTimelineItemResetCloth;
	class CTimelineItemDisableCloth;
	class CTimelineItemDisableDangle;
	class CTimelineItemSlowMo;

private:
	static CCutsceneTemplate*	m_currentCutscene;
	static CName				m_currentEntity;

	CName						m_trackName;
	IEventsContainer*			m_eventsSource;

	CEdCutsceneEditor*			m_editor;

public:
	// Very nasty hack for property editors
	static CCutsceneTemplate* GetEditedCutscene();
	static CName GetCurrentEntity();

	CEdCutsceneTimeline( CEdCutsceneEditor* ed, wxPanel* parentPanel, CEdPropertiesPage* propertyPage = nullptr );
	virtual ~CEdCutsceneTimeline();

	void SetTrack( const CName& trackName, Float trackDuration,
		IEventsContainer* eventsSource, const CSkeletalAnimationSetEntry* animation, CCutsceneTemplate* editedCutscene );

	Bool SelectEvent( const CName& eventName );

	RED_INLINE void Recreate()
	{ RecreateTracks();	}

protected:
	virtual void FillCanvasMenu( wxMenu* menu );
	virtual void FillTrackMenu( const String& name, wxMenu* menu );

	virtual void EditProperties( ITimelineItem* item, CEdPropertiesPage& propertiesPage ) const;

	virtual Bool CanChangeDefaultTrack() const
	{ return true; }

	virtual Bool CanResizeOverEnd() const
	{ return false; }

	virtual void SerializeItem( ITimelineItem* item, IFile& file );
	virtual ITimelineItem* DeserializeItem( IFile& file );

private:
	ITimelineItem* CreateTimelineItem( CExtAnimEvent* event );

	virtual void RemoveItemImpl( ITimelineItem* item ) override;

	virtual void StoreLayout() override;
	virtual void RestoreLayout() override;

protected:
	void OnNewEvent( wxCommandEvent& event );
	void OnTogglePause( wxCommandEvent& event );
	void OnCsToGameplay( wxCommandEvent& event );
};
