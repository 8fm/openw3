#pragma once

class CPhysicsContactListener
{
	Uint32 m_mask;
	Double m_markedTime;
	Float m_definedTimeInterval;
	Float m_definedMaximalDistanceSquared;
	Float m_definedMinimalVelocity;

public:
	CPhysicsContactListener( Uint32 mask, Float definedTimeInterval, Float definedMaximalDistanceSquared, Float definedMinimalVelocity ) : m_mask( mask ), m_markedTime( 0.0f ), m_definedTimeInterval( definedTimeInterval ), m_definedMaximalDistanceSquared( definedMaximalDistanceSquared ), m_definedMinimalVelocity( definedMinimalVelocity ) {}
	virtual ~CPhysicsContactListener() {}
	enum EPhysicsContactListener
	{
		EPCL_THRESHOLD_FOUND = 32,
		EPCL_THRESHOLD_LOST = 128
	};

	Uint32 GetMask() const { return m_mask; }

	Float GetDefinedMinimalVelocity() const { return m_definedMinimalVelocity; }
	Float GetDefinedTimeInterval() const { return m_definedTimeInterval; }
	Float GetDefinedDistanceSquared() const { return m_definedMaximalDistanceSquared; }

	void MarkTime( Double time ) { m_markedTime = time; }
	Double GetMarkedTime() const { return m_markedTime; }

	virtual void OnContactThresholdFound( const Vector& position, const char* materialName0, const char* materialName1, Float mass, Float velocity ) {}
	virtual void OnContactThresholdLost() {}

};