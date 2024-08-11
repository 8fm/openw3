/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "reactionScene.h"

class CActor;
///////////////////////////////////////////////////////////////////////////////

class CBehTreeReactionEventData : public CObject
{
	DECLARE_ENGINE_CLASS( CBehTreeReactionEventData, CObject, 0 );

protected:
	THandle< CEntity >			m_invoker;
	THandle< CReactionScene >	m_reactionScene;
	CName						m_eventName;
	Float						m_lifetime;
	Float						m_broadcastInterval;
	Float						m_broadcastTime;
	Float						m_expirationTime;
	Float						m_distanceRange;
	Float						m_distanceRangeSq;
	Box							m_rangeBox;
	Int32						m_recipientCount;
	Int32						m_baseRecipientCount;
	Bool						m_skipInvoker				: 1;
	Bool						m_useCustomReactionCenter	: 1;
	Vector						m_customReactionCenter;

//	Bool						m_setActionTargetOnBroadcast;

public:
	CBehTreeReactionEventData()
		: m_invoker( NULL )
		, m_reactionScene( NULL )
		, m_eventName( CName::NONE )
		, m_lifetime( 0.0f )
		, m_broadcastInterval( 0.0f )
		, m_broadcastTime( 0.0f )
		, m_expirationTime( 0.0f )
		, m_distanceRange( 0.0f )
		, m_distanceRangeSq( 0.0f )
		, m_recipientCount( 0 )
		, m_baseRecipientCount( 0 )
		, m_skipInvoker( false )
		, m_useCustomReactionCenter( false )
		//, m_setActionTargetOnBroadcast( false )
	{

	}

	CBehTreeReactionEventData( CEntity* invoker, CName name, Float lifetime = 5.0f , Float distanceRange = 15.0f, Float broadcastInterval = 2.0f, Int32 recipientCount = -1, Bool skipInvoker = false, Bool setActionTargetOnBroadcast = false )
		: m_invoker( invoker )
		, m_eventName( name )
		, m_lifetime( lifetime )
		, m_broadcastInterval( broadcastInterval )
		, m_broadcastTime( 0.0f )
		, m_expirationTime( 0.0f )
		, m_distanceRange( distanceRange )
		, m_recipientCount( recipientCount )
		, m_baseRecipientCount( recipientCount )
		, m_skipInvoker( skipInvoker )
		, m_useCustomReactionCenter( false )
		//, m_setActionTargetOnBroadcast( setActionTargetOnBroadcast )
	{
	}

	void SetCutomReactionCenter( const Vector& customReactionCenter ){ m_customReactionCenter = customReactionCenter; m_useCustomReactionCenter = true; }
	void ResetCutomReactionCenter(  ){ m_useCustomReactionCenter = false; }

	void PostLoad();
	Bool HasExpired();
	Bool IsInRange( Vector3 pos );

	void InitializeData( CEntity* invoker, const CName& eventName, Float lifetime, Float broadcastInterval, Float distanceRange, Int32 recipientCount, Bool skipInvoker, Bool useCustomReactionCenter, const Vector& reactionCenter );
	void UpdateRecipietCount( Int32 recipientCount );
	void ReportRecipient();

	Bool CanBeBroadcasted() const { return m_recipientCount != 0;}

	void SetBroadcastTime( Float val) { m_broadcastTime = val; }
	void SetExpirationTime( Float val ) { m_expirationTime = val;}	

	CEntity* GetInvoker() const { return m_invoker.Get(); }
	void SetInvoker( CEntity* invoker ) { m_invoker = invoker; }

	const Vector& GetReactionCenter() const { return m_useCustomReactionCenter ? m_customReactionCenter : m_invoker.Get()->GetWorldPositionRef();}

	CName GetEventName() const { return m_eventName; }
	Float GetLifetime() const { return m_lifetime; }
	Float GetBroadcastInterval() const { return m_broadcastInterval; }
	Float GetBroadcastTime() const { return m_broadcastTime; }
	Float GetExpirationTime() const { return m_expirationTime; }
	Float GetDistanceRange() const { return m_distanceRange; }
	Float GetDistanceRangeSqrt() const { return m_distanceRangeSq; }
	Box GetRangeBox() const { return m_rangeBox; }
	Bool GetSkipInvoker() const { return m_skipInvoker; }
	//Bool IfSetActionTargetOnBroadcast() const { return m_setActionTargetOnBroadcast; }
	void IncrementRecipientCounter();

	RED_INLINE CReactionScene* GetReactionScene(){ return m_reactionScene.Get(); }
	RED_INLINE void SetReactionScene( CReactionScene* reactionScene ){ m_reactionScene = reactionScene; }
	RED_INLINE void ForceExpiration(){ m_expirationTime = GGame->GetEngineTime(); m_lifetime = 0.1f; }
};

BEGIN_CLASS_RTTI( CBehTreeReactionEventData );
PARENT_CLASS( CObject );
PROPERTY_EDIT( m_eventName, TXT("Name of the reaction event") );
PROPERTY_EDIT( m_lifetime, TXT("Lifetime in seconds") );
PROPERTY_EDIT( m_broadcastInterval, TXT("Interval before the next broadcast of the event") );
PROPERTY_EDIT( m_distanceRange, TXT("Activation range") );
PROPERTY_EDIT( m_recipientCount, TXT("Number of actors that can receive and act on the event. Negative numbers are treated as unlimited count.") );
END_CLASS_RTTI();