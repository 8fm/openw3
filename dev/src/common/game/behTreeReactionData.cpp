#include "build.h"
#include "behTreeReactionData.h"

IMPLEMENT_ENGINE_CLASS( CBehTreeReactionEventData );

Bool CBehTreeReactionEventData::HasExpired()
{
	if( m_invoker.Get() )
	{
		Float currTime = GGame->GetEngineTime();
		return m_expirationTime <= currTime && m_lifetime > 0.0f;
	}
	
	return true;
}

Bool CBehTreeReactionEventData::IsInRange( Vector3 pos )
{
	CEntity* entity = m_invoker.Get();
	if( entity )
	{
		return GetReactionCenter().DistanceSquaredTo( pos ) <= m_distanceRangeSq;
	}
	return false;
}

void CBehTreeReactionEventData::PostLoad()
{
	m_distanceRangeSq = m_distanceRange*m_distanceRange;
	Vector minBound( -m_distanceRange, -m_distanceRange, -m_distanceRange );
	Vector maxBound( m_distanceRange, m_distanceRange, m_distanceRange );
	m_rangeBox = Box( minBound, maxBound );
}

void CBehTreeReactionEventData::InitializeData( CEntity* invoker, const CName& eventName, Float lifetime, Float broadcastInterval, Float distanceRange, Int32 recipientCount, Bool skipInvoker, Bool useCustomReactionCenter, const Vector& reactionCenter )
{
	m_invoker			= invoker;
	m_eventName			= eventName;
	m_lifetime			= lifetime;
	m_broadcastInterval = broadcastInterval;
	m_distanceRange		= distanceRange;
	m_recipientCount	= recipientCount;
	m_baseRecipientCount= recipientCount;
	m_skipInvoker		= skipInvoker;
	m_useCustomReactionCenter	= useCustomReactionCenter;
	m_customReactionCenter		= reactionCenter;
}

void CBehTreeReactionEventData::UpdateRecipietCount( Int32 recipientCount )
{
	if( recipientCount > m_baseRecipientCount )
	{
		Int32 diff = recipientCount - m_baseRecipientCount;
		m_recipientCount += diff;			
		m_baseRecipientCount = recipientCount;
	}
	else if( recipientCount < m_baseRecipientCount )
	{
		Int32 diff = m_baseRecipientCount - recipientCount;
		m_recipientCount = m_recipientCount - diff > 0 ? m_recipientCount - diff : 0 ;		
		m_baseRecipientCount = recipientCount;
	}	

}

void CBehTreeReactionEventData::ReportRecipient()
{
	if( m_recipientCount > 0 )
	{
		--m_recipientCount;
	}
}

void CBehTreeReactionEventData::IncrementRecipientCounter()
{
	if( m_recipientCount >= 0 && m_recipientCount < m_baseRecipientCount )
	{
		++m_recipientCount;
	}
}