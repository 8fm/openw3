#pragma once

/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#include "behTreeMachine.h"
#include "storySceneDialogset.h"

enum EBehaviorTreeEventType
{
	BTET_AET_Tick = AET_Tick,
	BTET_AET_DurationStart = AET_DurationStart,
	BTET_AET_DurationStartInTheMiddle = AET_DurationStartInTheMiddle,
	BTET_AET_DurationEnd = AET_DurationEnd,
	BTET_AET_Duration = AET_Duration,
	BTET_GameplayEvent
};

BEGIN_ENUM_RTTI( EBehaviorTreeEventType );
	ENUM_OPTION( BTET_AET_Tick );
	ENUM_OPTION( BTET_AET_DurationStart );
	ENUM_OPTION( BTET_AET_DurationStartInTheMiddle );
	ENUM_OPTION( BTET_AET_DurationEnd );
	ENUM_OPTION( BTET_AET_Duration );
	ENUM_OPTION( BTET_GameplayEvent );
END_ENUM_RTTI();

///////////////////////////////////////////////////////////////////////////////
struct SBehTreeAnimEventData
{
	const CExtAnimEvent*			m_animEventData;
	const SAnimationEventAnimInfo*	m_eventAnimInfo;
};

///////////////////////////////////////////////////////////////////////////////
// class representing AI system event data
class CBehTreeEvent
{
public:
	EBehaviorTreeEventType			m_eventType;
	CName							m_eventName;
	union
	{
		SBehTreeAnimEventData		m_animEventData;
		SGameplayEventData			m_gameplayEventData;
	};
	RED_INLINE Bool operator<( const CBehTreeEvent& e ) const;
	RED_INLINE Bool operator==( const CBehTreeEvent& e ) const;
	RED_INLINE Bool operator!=( const CBehTreeEvent& e ) const;
	CBehTreeEvent()														{}		// notice no initialization
	CBehTreeEvent( const CExtAnimEvent* event, EAnimationEventType eventType, const SAnimationEventAnimInfo* eventAnimInfo )
		: m_eventType( ConvertEvent( eventType ) )
		, m_eventName( event->GetEventName() )							{ m_animEventData.m_animEventData = event; m_animEventData.m_eventAnimInfo = eventAnimInfo; }
	CBehTreeEvent( EBehaviorTreeEventType eventType, CName name )
		: m_eventType( eventType )
		, m_eventName( name )											{}
	CBehTreeEvent( EAnimationEventType eventType, CName name )
		: m_eventType( ConvertEvent( eventType ) )
		, m_eventName( name )											{ m_animEventData.m_animEventData = NULL; m_animEventData.m_eventAnimInfo = NULL; }
	CBehTreeEvent( CName name, void* additionalData = NULL, IRTTIType* additionalDataType = NULL )
		: m_eventType( BTET_GameplayEvent )
		, m_eventName( name )											{ m_gameplayEventData.m_customData = additionalData; m_gameplayEventData.m_customDataType = additionalDataType; }

	static EAnimationEventType ConvertToAnimationEvent( EBehaviorTreeEventType e )
	{
		return EAnimationEventType( e );
	}
	static EBehaviorTreeEventType ConvertEvent( EAnimationEventType e )
	{
		return EBehaviorTreeEventType( e );
	}
};

///////////////////////////////////////////////////////////////////////////////
// class that represent data used to register for ai event listening
struct SBehTreeEvenListeningData
{
public:
	CName				m_eventName;
	enum EType
	{
		TYPE_ANIM,
		TYPE_GAMEPLAY
	}					m_eventType;
	RED_INLINE Bool operator<( const SBehTreeEvenListeningData& e ) const;
	RED_INLINE Bool operator==( const SBehTreeEvenListeningData& e ) const;
	RED_INLINE Bool operator!=( const SBehTreeEvenListeningData& e ) const;
};



RED_INLINE Bool CBehTreeEvent::operator<( const CBehTreeEvent& e ) const
{
	return
		(m_eventName < e.m_eventName) ? true :
		(m_eventName > e.m_eventName) ? false :
		(m_eventType < e.m_eventType) ? true : false;
}
RED_INLINE Bool CBehTreeEvent::operator==( const CBehTreeEvent& e ) const
{
	return e.m_eventName == m_eventName && e.m_eventType == m_eventType;
}
RED_INLINE Bool CBehTreeEvent::operator!=( const CBehTreeEvent& e ) const
{
	return e.m_eventName != m_eventName || e.m_eventType != m_eventType;
}
RED_INLINE Bool SBehTreeEvenListeningData::operator<( const SBehTreeEvenListeningData& e ) const
{
	return
		(m_eventName < e.m_eventName) ? true :
		(m_eventName > e.m_eventName) ? false :
		(m_eventType < e.m_eventType) ? true : false;
}
RED_INLINE Bool SBehTreeEvenListeningData::operator==( const SBehTreeEvenListeningData& e ) const
{
	return e.m_eventName == m_eventName && e.m_eventType == m_eventType;
}
RED_INLINE Bool SBehTreeEvenListeningData::operator!=( const SBehTreeEvenListeningData& e ) const
{
	return e.m_eventName != m_eventName || e.m_eventType != m_eventType;
}
