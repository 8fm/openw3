
/*
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "storySceneIncludes.h"
#include "storySceneInterfaces.h"

class CStorySceneElement;
class CStoryScenePlayer;
class CStorySceneEventsCollector;
class CStorySceneAnimationContainer;

//////////////////////////////////////////////////////////////////////////

#define DECLARE_SCENE_EVENT_BASE_CLASS( _className ) DECLARE_RTTI_SIMPLE_CLASS( _className )

#define DECLARE_SCENE_EVENT_CLASS( _className, _parentName )\
	DECLARE_RTTI_SIMPLE_CLASS( _className )\
	typedef _parentName TBaseClass;

//////////////////////////////////////////////////////////////////////////

struct SStorySceneEventTimeInfo
{
	Float	m_timeLocal;
	Float	m_timeAbs;
	Float	m_timeDelta;
	Float	m_progress;

	SStorySceneEventTimeInfo() : m_timeLocal( 0.f ), m_timeAbs( 0.f ), m_timeDelta( 0.f ), m_progress( 0.f ) {}
};

//////////////////////////////////////////////////////////////////////////

/*
Wraps set of common args taken by scene event functions.

This is not yet used by most event classes but it will be.
*/
class CSceneEventFunctionArgs
{
public:
	CSceneEventFunctionArgs( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo );

	CStorySceneInstanceBuffer& m_data;
	CStoryScenePlayer* m_scenePlayer;
	CStorySceneEventsCollector& m_collector;
	const SStorySceneEventTimeInfo& m_timeInfo;
	const CStorySceneEvent*	m_destEvent;

private:
	CSceneEventFunctionArgs( const CSceneEventFunctionArgs& );					// cctor - not defined
	CSceneEventFunctionArgs& operator=( const CSceneEventFunctionArgs& );		// op= - not defined
};

//////////////////////////////////////////////////////////////////////////

// Simplified version of event function arguments
class CSceneEventFunctionSimpleArgs
{
public:
	CSceneEventFunctionSimpleArgs( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer );
	CSceneEventFunctionSimpleArgs( CSceneEventFunctionArgs& args );

	CStorySceneInstanceBuffer& m_data;
	CStoryScenePlayer* m_scenePlayer;

private:
	CSceneEventFunctionSimpleArgs( const CSceneEventFunctionSimpleArgs& );					// cctor - not defined
	CSceneEventFunctionSimpleArgs& operator=( const CSceneEventFunctionSimpleArgs& );		// op= - not defined
};

//////////////////////////////////////////////////////////////////////////

class	CStorySceneEventBlend;
struct	SEtorySceneEventGenericCreationData;

class CStorySceneEvent
{
	DECLARE_SCENE_EVENT_BASE_CLASS( CStorySceneEvent )

	RED_DECL_INTERFACE_SUPPORT()

protected:
	String					m_eventName;
	CStorySceneElement*		m_sceneElement;		//!< Scene element with which this scene event is associated.
	Float					m_startPosition;	//!< Start position, relative to scene element, in range <0.0f, 1.0f).
												//!< TODO: Some code assumes <0.0f, 1.0f> range but 1.0f means that
												//!< event should really be associated with next scene element.
	String					m_trackName;
	Bool					m_isMuted;
	Int32					m_contexID;			// TODO: This is to be removed. We keep this for now as it is used
												// when converting old style sections to variant sections.

	CGUID					m_GUID;
	CGUID					m_interpolationEventGUID;
	CGUID					m_blendParentGUID;	//!< GUID of the parent blend event (CStorySceneEventBlend instance)

	CGUID					m_linkParentGUID;
	Float					m_linkParentTimeOffset;
#ifndef  NO_EDITOR
	TDynArray< CGUID >		m_linkChildrenGUID;
#endif

#ifndef  NO_EDITOR
	String					m_debugString;
#endif

protected:
	TInstanceVar< Bool >	i_valid;
	TInstanceVar< Bool >	i_started;			// True - event instance has started, false - otherwise.
	TInstanceVar< Bool >	i_finished;			// True - event instance has finished, false - otherwise.
	TInstanceVar< Float >	i_startTimeMS;
	TInstanceVar< Float >	i_durationMS;
	TInstanceVar< Float >	i_scalingFactor;

public:
	CStorySceneEvent();
	CStorySceneEvent( const String& eventName, CStorySceneElement* sceneElement, Float startTime, const String& trackName );
	CStorySceneEvent( const CStorySceneEvent& other );
	virtual ~CStorySceneEvent();

	virtual CStorySceneEvent* Clone() const;

	virtual void Serialize( IFile& file );

	void BakeScale( Float scalingFactor );
	virtual void  OnPostLoad() {}
protected:
	virtual void DoBakeScaleImpl( Float scalingFactor );

public:
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CStorySceneInstanceBuffer& instance ) const;
	virtual void OnReleaseInstance( CStorySceneInstanceBuffer& instance ) const;

public:
	void GetEventInstanceData( const CStorySceneInstanceBuffer& data, Float& s, Float& e, Float& d ) const;
	
	void MarkInstanceDataSet( CStorySceneInstanceBuffer& instanceBuffer ) const;
	Bool IsEventInstanceDataSet( const CStorySceneInstanceBuffer& instanceBuffer ) const;

	void SetInstanceStartTime( CStorySceneInstanceBuffer& instanceBuffer, Float startTime ) const;
	Float GetInstanceStartTime( const CStorySceneInstanceBuffer& instanceBuffer ) const;

	void SetInstanceDuration( CStorySceneInstanceBuffer& instanceBuffer, Float duration ) const;
	Float GetInstanceDuration( const CStorySceneInstanceBuffer& instanceBuffer ) const;

	void SetInstanceScalingFactor( CStorySceneInstanceBuffer& instanceBuffer, Float scalingFactor ) const;
	Float GetInstanceScalingFactor( const CStorySceneInstanceBuffer& instanceBuffer ) const;

	void SetInstanceStarted( CStorySceneInstanceBuffer& instanceBuffer, Bool state ) const;
	Bool HasInstanceStarted( CStorySceneInstanceBuffer& instanceBuffer ) const;

	void SetInstanceFinished( CStorySceneInstanceBuffer& instanceBuffer, Bool state ) const;
	Bool HasInstanceFinished( CStorySceneInstanceBuffer& instanceBuffer ) const;

public:
	void EventInit( CStorySceneInstanceBuffer& data, const CStoryScenePlayer* scenePlayer ) const;
	void EventDeinit( CStorySceneInstanceBuffer& data, const CStoryScenePlayer* scenePlayer ) const;

	void EventStart( CStorySceneInstanceBuffer& data, const CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const;
	void EventProcess( CStorySceneInstanceBuffer& data, const CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const;
	void EventEnd( CStorySceneInstanceBuffer& data, const CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo )	const;

public: // SCENE_TOMSIN_TODO - to jest publiczne tylko przez hacki w  CStorySceneEventGroup
	virtual void OnInit( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer ) const {}
	virtual void OnDeinit( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer ) const {}

public: // SCENE_TOMSIN_TODO - to jest publiczne tylko przez hacki w CStorySceneEventGroup
	virtual void OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const { }
	virtual void OnEnd( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo )	const { }

	virtual void OnProcess( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const {}

public:
	void RegenerateGUID();
	virtual void OnGuidChanged( CGUID oldGuid, CGUID newGuid );

	void SetGUID( const CGUID& guid ) { m_GUID = guid; }
	const CGUID& GetGUID() const { return m_GUID; }

	Bool IsInterpolationEventKey() const;
	CGUID GetInterpolationEventGUID() const;
	void SetInterpolationEventGUID( CGUID interpolationEventGUID );

	Bool HasBlendParent() const { return !m_blendParentGUID.IsZero(); }
	const CGUID& GetBlendParentGUID() const { return m_blendParentGUID; }
	Bool HasLinkParent() const { return !m_linkParentGUID.IsZero(); }
	const CGUID& GetLinkParentGUID() const { return m_linkParentGUID; }
	Float GetLinkParentTimeOffset() const { return m_linkParentTimeOffset; }
	void SetBlendParentGUID( const CGUID& blendParentGUID ) { m_blendParentGUID = blendParentGUID; }
	void SetLinkParent( const CGUID& linkParentGUID, Float timeOffset ) { m_linkParentGUID = linkParentGUID; m_linkParentTimeOffset = timeOffset; }
#ifndef  NO_EDITOR
	void ResetLinkParent() { m_linkParentGUID = CGUID::ZERO; m_linkParentTimeOffset = 0.f; }
	void SetLinkParentTimeOffset( Float timeOffset ) { m_linkParentTimeOffset = timeOffset; }

	void AddLinkChildGUID( const CGUID& childGUID ) { m_linkChildrenGUID.PushBackUnique( childGUID ); }
	Bool RemoveLinkChildGUID( const CGUID& childGUID ) { return m_linkChildrenGUID.Remove( childGUID ); }
	void RemoveAllLinkChildren() { m_linkChildrenGUID.Clear(); }
	Bool HasLinkChildGUID( const CGUID& childGUID ) const { return m_linkChildrenGUID.Exist( childGUID ); }
	Bool HasLinkChildren() const { return m_linkChildrenGUID.Size() > 0; }
	const TDynArray< CGUID >& GetLinkChildrenGUID() const { return m_linkChildrenGUID; }
#endif

public:
	void SetStartPosition( Float startPosition );
	Float GetStartPosition() const					{ return m_startPosition; }
	const String& GetEventName() const				{ return m_eventName; }
	const String& GetTrackName() const				{ return m_trackName; }
	CStorySceneElement* GetSceneElement() const	{ return m_sceneElement; }
	void SetSceneElement( CStorySceneElement* element )			{ m_sceneElement = element; }

	Int32 GetContexID() const { return m_contexID; }

	String GetName() const;

	virtual CName GetSubject() const { return CName::NONE; }

	void SetTrackName( const String& trackName )		{ m_trackName = trackName; }
	void Mute( Bool flag )								{ m_isMuted = flag; }
	Bool IsMuted() const								{ return m_isMuted; }

	virtual void CollectUsedAnimations( CStorySceneAnimationContainer& container ) const {}

#ifndef  NO_EDITOR
public:
	virtual void OnPreviewPropertyChanged( const CStoryScenePlayer* previewPlayer, const CName& propertyName ) {}
	void SetDebugString( const String & debug )	{ m_debugString = debug; }
#endif

#ifndef  NO_EDITOR
public:
	virtual Bool	GenericEventCreation() const { return false; }
	virtual SEtorySceneEventGenericCreationData* GetGenericCreationData() const { return nullptr; }
#endif

protected:
	virtual void OnSerialize( IFile& file ) {}

protected:
	virtual void OnInterpolationEventGUIDSet() {}
};

BEGIN_CLASS_RTTI( CStorySceneEvent )
	PROPERTY_EDIT( m_eventName, TXT( "Event name" ) )
	PROPERTY_EDIT( m_startPosition, TXT( "Start time" ) )
	PROPERTY_EDIT( m_isMuted, TXT("Is event muted") )
	PROPERTY_RO( m_contexID, TXT("Contex ID") );
	PROPERTY( m_sceneElement )
	PROPERTY( m_GUID )
	PROPERTY( m_interpolationEventGUID )
	PROPERTY( m_blendParentGUID )
	PROPERTY( m_linkParentGUID )
	PROPERTY( m_linkParentTimeOffset )
#ifndef  NO_EDITOR
	PROPERTY( m_linkChildrenGUID )
	PROPERTY_NOT_COOKED( m_trackName )
	PROPERTY_EDIT_NOSERIALIZE( m_debugString, TXT( "Camera generator debug info" ) );
#endif
END_CLASS_RTTI()

#ifndef  NO_EDITOR
struct SEtorySceneEventGenericCreationData
{
	const Char*		m_timelineIconName;
	String			m_menuCategory;
	String			m_menuEntry;
	const CClass*	m_eventClass;

	SEtorySceneEventGenericCreationData() : m_timelineIconName( nullptr ), m_eventClass( nullptr )
	{}
	SEtorySceneEventGenericCreationData( const String& menuCat, const String& menuEntry, const Char* iconName, CClass* evtClass  )
		: m_menuCategory( menuCat ), m_menuEntry( menuEntry ), m_timelineIconName( iconName ) 
	{}

	struct SGenericCreationArgs
	{
		SGenericCreationArgs( const String& eventName, CStorySceneElement* sceneElement, Float startTime, CName actor, const String& trackName )
			: m_eventName( eventName), m_sceneElement( sceneElement ), m_startTime( startTime ), m_actor( actor ), m_trackName( trackName )
		{}
		String				m_eventName;
		CStorySceneElement* m_sceneElement;
		Float				m_startTime;
		CName				m_actor;
		String				m_trackName;
	};

	virtual CStorySceneEvent* CreateEvent( SGenericCreationArgs& args ) = 0;
};

#define DECLARE_STORY_SCENE_EVENT_GENERIC_CREATION( event, category, menuName, iconName )									\
private:																													\
	struct StorySceneEventGenericCreationData##event : public SEtorySceneEventGenericCreationData							\
	{																														\
		StorySceneEventGenericCreationData##event()																			\
		{																													\
			m_timelineIconName = iconName;																					\
			m_menuCategory = category;																						\
			m_menuEntry = menuName;																							\
			m_eventClass = event::GetStaticClass();																			\
		}																													\
		virtual CStorySceneEvent* CreateEvent( SEtorySceneEventGenericCreationData::SGenericCreationArgs& args ) override	\
		{																													\
			return new event( args.m_eventName, args.m_sceneElement, args.m_startTime, args.m_actor, args.m_trackName );	\
		}																													\
	};																														\
	static StorySceneEventGenericCreationData##event m_genericCreationData;													\
public:																														\
	virtual Bool	GenericEventCreation() const override { return true; }													\
	virtual SEtorySceneEventGenericCreationData* GetGenericCreationData() const override { return &m_genericCreationData; }	\

#define IMPLEMENT_STORY_SCENE_EVENT_GENERIC_CREATION( event )							\
	event::StorySceneEventGenericCreationData##event event::m_genericCreationData;		\
																				
#else
	#define DECLARE_STORY_SCENE_EVENT_GENERIC_CREATION(...)
	#define IMPLEMENT_STORY_SCENE_EVENT_GENERIC_CREATION(...)
#endif

RED_INLINE Bool CStorySceneEvent::IsInterpolationEventKey() const
{
	return !m_interpolationEventGUID.IsZero();
}

RED_INLINE CGUID CStorySceneEvent::GetInterpolationEventGUID() const
{
	return m_interpolationEventGUID;
}

RED_INLINE void CStorySceneEvent::SetInterpolationEventGUID( CGUID interpolationEventGUID )
{
	m_interpolationEventGUID = interpolationEventGUID;

	OnInterpolationEventGUIDSet();
}

RED_INLINE void CStorySceneEvent::SetInstanceStarted( CStorySceneInstanceBuffer& instanceBuffer, Bool state ) const
{
	instanceBuffer[ i_started ] = state;
}

RED_INLINE Bool CStorySceneEvent::HasInstanceStarted( CStorySceneInstanceBuffer& instanceBuffer ) const
{
	return instanceBuffer[ i_started ];
}

RED_INLINE void CStorySceneEvent::SetInstanceFinished( CStorySceneInstanceBuffer& instanceBuffer, Bool state ) const
{
	instanceBuffer[ i_finished ] = state;
}

RED_INLINE Bool CStorySceneEvent::HasInstanceFinished( CStorySceneInstanceBuffer& instanceBuffer ) const
{
	return instanceBuffer[ i_finished ];
}
