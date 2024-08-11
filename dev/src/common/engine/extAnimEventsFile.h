/*
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include "../core/resource.h"
#include "extAnimEvent.h"
#include "../core/tagList.h"
#include "../core/classBuilder.h"
#include "../core/2darray.h"

// After resave, we could remove cutscene dependency on this class
// and remove it completely from game builds
class CExtAnimEventsFile : public CResource, public IEventsContainer
#ifdef USE_EXT_ANIM_EVENTS
	, public I2dArrayPropertyOwner
#endif
{
	DECLARE_ENGINE_RESOURCE_CLASS( CExtAnimEventsFile, CResource, "w2animev", "Events for animation set" );	

#ifdef USE_EXT_ANIM_EVENTS

public:
	CExtAnimEventsFile();
	virtual ~CExtAnimEventsFile();

	RED_INLINE const TDynArray< CExtAnimEvent* >& GetEvents() const
	{ return m_events; }

	RED_INLINE TDynArray< CExtAnimEvent* >& GetEvents()
	{ return m_events; }

	template < class EventType >
	void GetEventsOfType( const CName& animName, TDynArray< EventType* >& list ) const;

	template < class EventType, class Collector >
	void GetEventsOfType( const CName& animName, Collector& collector ) const;

	void GetEventsByTime( const CSkeletalAnimationSetEntry* animation, Float prevTime, Float currTime, Int32 numLoops, Float alpha, TDynArray< CAnimationEventFired >* events, SBehaviorGraphOutput* output, const CName& tag ) const;

	Uint32 GetNumEventsOfType( const CName& animName, const CClass* c ) const;

	// deprecated: Remove after resave
	virtual void OnSerialize( IFile& file );

	/// IEventsContainer interface
	virtual void AddEvent( CExtAnimEvent* event );
	virtual void RemoveEvent( CExtAnimEvent* event );
	virtual void GetEventsForAnimation( const CName& animName, TDynArray< CExtAnimEvent* >& events );
	virtual const CResource* GetParentResource() const;
	virtual Bool OnPropertyMissing( CName propertyName, const CVariant& readValue );

	Bool RenameAnimationInAllEvents( CName const & prevName, CName const & newName );

	virtual void CleanupSourceData();

	const CName& GetRequiredSfxTag() const { return m_requiredSfxTag; }

	virtual void Get2dArrayPropertyAdditionalProperties( IProperty *property, SConst2daValueProperties &valueProperties );

protected:
	TSortedArray< CExtAnimEvent*, CExtAnimEvent::Sorter >	m_events;
	CAnimEventSerializer*									m_crappySerializer;
	CName													m_requiredSfxTag;

#else

public:
	/// IEventsContainer interface
	virtual void AddEvent( CExtAnimEvent* event ) {}
	virtual void RemoveEvent( CExtAnimEvent* event ) {}
	virtual void GetEventsForAnimation( const CName& animName, TDynArray< CExtAnimEvent* >& events ) {}
	virtual const CResource* GetParentResource() const { return NULL; }

#endif
};

BEGIN_CLASS_RTTI( CExtAnimEventsFile )
	PARENT_CLASS( CResource )
#ifdef USE_EXT_ANIM_EVENTS
	PROPERTY_CUSTOM_EDIT( m_requiredSfxTag, TXT("Required sfx tag in template's GlobalAnimParam"), TXT("2daValueSelection") )
#endif
	
END_CLASS_RTTI()

#ifdef USE_EXT_ANIM_EVENTS

#include "extAnimEventsFile.inl"

#endif