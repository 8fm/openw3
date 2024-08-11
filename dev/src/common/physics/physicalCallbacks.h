#pragma once

struct SActorShapeIndex
{
	struct 
	{
		Int16 m_actorIndex;
		Int16 m_shapeIndex;
	};

	SActorShapeIndex() : m_shapeIndex( -1 ), m_actorIndex( -1 ) {}
	SActorShapeIndex( Int16 actorIndex, Int16 shapeIndex ) : m_actorIndex( actorIndex ), m_shapeIndex( shapeIndex ) {}

	Bool operator==(const SActorShapeIndex& other) const
	{
		return other.m_actorIndex == m_actorIndex && other.m_shapeIndex == m_shapeIndex;
	}

	Bool IsValid() const { return ( m_actorIndex != -1 ) && ( m_shapeIndex != -1 ); }
};

struct STriggeringInfo
{
	THandle< IScriptable > m_triggerObject;
	class CPhysicsWrapperInterface* m_triggerWrapper;
	SActorShapeIndex m_triggerBodyIndex;

	THandle< IScriptable > m_triggeredObject;
	class CPhysicsWrapperInterface* m_triggeredWrapper;
	SActorShapeIndex m_triggeredBodyIndex;
	void* m_triggeredBodyId;

	Bool operator==(const STriggeringInfo& other) const
	{
		return ( other.m_triggerBodyIndex == m_triggerBodyIndex ) && ( other.m_triggerWrapper == m_triggerWrapper ) && ( other.m_triggeredBodyIndex == m_triggeredBodyIndex ) && ( other.m_triggeredWrapper == m_triggeredWrapper );
	}

	Bool TriggeredBodyWasRemoved() const { return !m_triggeredBodyIndex.IsValid(); }

	STriggeringInfo() : m_triggerObject(), m_triggerWrapper( 0 ), m_triggerBodyIndex(), m_triggeredObject(), m_triggeredWrapper( 0 ), m_triggeredBodyIndex(), m_triggeredBodyId() {}
	STriggeringInfo( const THandle< IScriptable >& triggerComponent, class CPhysicsWrapperInterface* triggerWrapper, SActorShapeIndex triggerBodyIndex, const THandle< IScriptable >& triggeredComponent, class CPhysicsWrapperInterface* triggeredWrapper, SActorShapeIndex triggeredBodyIndex, void* triggeredBodyId ) : m_triggerObject( triggerComponent ), m_triggerWrapper( triggerWrapper ), m_triggerBodyIndex( triggerBodyIndex ), m_triggeredObject( triggeredComponent ), m_triggeredBodyIndex( triggeredBodyIndex ), m_triggeredWrapper( triggeredWrapper ), m_triggeredBodyId( triggeredBodyId ) {}
	STriggeringInfo( const THandle< IScriptable >& triggerComponent, class CPhysicsWrapperInterface* triggerWrapper, SActorShapeIndex triggerBodyIndex, void* triggeredBodyId ) : m_triggerObject( triggerComponent ), m_triggerWrapper( triggerWrapper ), m_triggerBodyIndex( triggerBodyIndex ), m_triggeredObject( 0 ), m_triggeredBodyIndex(), m_triggeredWrapper( 0 ), m_triggeredBodyId( triggeredBodyId ) {}
};

struct SPhysicalCollisionInfo
{
	class CPhysicsWrapperInterface* m_callbackBody;
	SActorShapeIndex m_callbackBodyIndex;
	class CPhysicsWrapperInterface* m_otherBody;
	SActorShapeIndex m_otherBodyIndex;
	Vector m_position;
	Vector m_force;

	SPhysicalCollisionInfo() : m_callbackBody( 0 ), m_callbackBodyIndex(), m_otherBody( 0 ), m_otherBodyIndex() {}
	SPhysicalCollisionInfo( class CPhysicsWrapperInterface* callbackBody, const SActorShapeIndex& callbackBodyIndex, class CPhysicsWrapperInterface* otherBody, const SActorShapeIndex& otherBodyIndex ) : m_callbackBody( callbackBody ), m_callbackBodyIndex( callbackBodyIndex ), m_otherBody( otherBody ), m_otherBodyIndex( otherBodyIndex ) {}
	SPhysicalCollisionInfo( class CPhysicsWrapperInterface* callbackBody, const SActorShapeIndex& callbackBodyIndex, class CPhysicsWrapperInterface* otherBody, const SActorShapeIndex& otherBodyIndex, const Vector& position, const Vector& force ) : m_callbackBody( callbackBody ), m_callbackBodyIndex( callbackBodyIndex ), m_otherBody( otherBody ), m_otherBodyIndex( otherBodyIndex ), m_position( position ), m_force( force ) {}
};

class IPhysicalCollisionTriggerCallback
{
public:
	virtual void onCollision( const SPhysicalCollisionInfo& info ) {}
	virtual void onTriggerEntered( const STriggeringInfo& info ) {}
	virtual void onTriggerExited( const STriggeringInfo& info ) {}
	virtual void onCharacterTouch( THandle< IScriptable > m_triggeredComponent, SActorShapeIndex& m_triggeredBodyIndex ) {}
};
