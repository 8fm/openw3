/*
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "timeline.h"

class CEdPropertiesPage;

class CEdAnimEventsTimeline : public CEdTimeline
{
public:
	CEdAnimEventsTimeline( wxPanel* parentPanel );
	virtual ~CEdAnimEventsTimeline();

	// Called when animation is selected from the list
	void SetAnimation( const CName& animationName, Float animationDuration,
		const TDynArray< IEventsContainer* >& eventContainers );

	// TEMPSHIT!
	CResource* GetSelectedResource() const
	{ return ( m_eventsContainer != NULL ) ? m_eventsContainer->GetParentResource() : NULL; }

	CExtAnimEvent* GetEvent( int n ) const;
	CExtAnimEvent* GetSelectedEvent( int n ) const;

public:
	static CExtAnimEvent* DeserializeAsEvent( IFile& file );
	static void SerializeEvent( CExtAnimEvent* event, IFile& writer );

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

	virtual void ItemPropertyChanged( TDynArray< ITimelineItem* >& items, const CName& propertyName ) override;

private:
	class CTimelineItemEvent;
	class CTimelineItemAnimation;
	class CTimelineItemFootstep;
	class CTimelineItemItem;
	class CTimelineItemExploration;
	class CTimelineItemSound;
	class CTimelineItemSoundHit;
	class CTimelineItemCombo;
	class CTimelineItemItemEffect;
	class CTimelineItemHit;
	class CTimelineItemLookAt;
	class CTimelineItemProjectile;
	class CTimelineItemItemAnimation;
	class CTimelineItemItemBehavior;
	class CTimelineItemItemDrop;
	class CTimelineItemGhostEvent;
	class CTimelineItemSyncEvent;
	class CTimelineItemMorphing;

	CName								m_animationName;
	IEventsContainer*					m_eventsContainer;
	Bool								m_cutsceneMode;

	// Events logic
	void OnNewEvent( wxCommandEvent& event );
	void OnShowAllTracksToggled( wxCommandEvent& event );

	CTimelineItemEvent* CreateTimelineItem( CExtAnimEvent* event, const IEventsContainer* container );

	virtual void RemoveItemImpl( ITimelineItem* item ) override;

	virtual void StoreLayout() override;
	virtual void RestoreLayout() override;
};
