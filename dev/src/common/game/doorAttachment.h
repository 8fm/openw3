/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "../engine/umbraStructures.h"

struct SDoorSoundsEvents 
{
	DECLARE_RTTI_STRUCT( SDoorSoundsEvents )

	SDoorSoundsEvents(){}

	String	m_open;
	String	m_openFully;
	String	m_openingStart;
	String	m_openingStop;

	String	m_close;
	String	m_closeFully;
	String	m_closingStart;
	String	m_closingStop;
};

BEGIN_CLASS_RTTI( SDoorSoundsEvents )
	PROPERTY_EDIT( m_open			, TXT("") )
	PROPERTY_EDIT( m_openFully		, TXT("") )
	PROPERTY_EDIT( m_openingStart	, TXT("") )
	PROPERTY_EDIT( m_openingStop	, TXT("") )

	PROPERTY_EDIT( m_close			, TXT("") )
	PROPERTY_EDIT( m_closeFully		, TXT("") )
	PROPERTY_EDIT( m_closingStart	, TXT("") )
	PROPERTY_EDIT( m_closingStop	, TXT("") )
END_CLASS_RTTI()

//////////////////////////////////////////////////////////////////////////////////////////////
// Door attachments are means by which a door component "open" a door mesh
//////////////////////////////////////////////////////////////////////////////////////////////
class IDoorAttachment : public CHardAttachment
{
	DECLARE_ENGINE_CLASS( IDoorAttachment, CHardAttachment, 0 );

private:
	SDoorSoundsEvents	m_soundsEvents;
	Bool				m_isOpeningPlaying				: 1;
	Bool				m_isClosingPlaying				: 1;	

protected:
	Bool				m_isTrapdoor;
	Float				m_originalAngle;
	Bool				m_cachedState;
#ifdef USE_UMBRA
	TObjectIdType		m_umbraGateId;
#endif // USE_UMBRA

public:
	IDoorAttachment()
		: m_isOpeningPlaying ( false )
		, m_isClosingPlaying( false )
		, m_originalAngle( 0 )
		, m_isTrapdoor( false )
		, m_cachedState( true )
#ifdef USE_UMBRA
		, m_umbraGateId( INVALID_UMBRA_OBJECT_ID )
#endif // USE_UMBRA
	{

	}

	// init attachment (returns true if we can hook up parent & child)
	virtual Bool Init( CNode* parent, CNode* child, const AttachmentSpawnInfo* info ) override;

	// Get parent (as door component)
	RED_INLINE CDoorComponent* GetDoor() const				{ return Cast<CDoorComponent>( GetParent() ); }
	RED_INLINE CGameplayEntity* GetGameplayEntity() const		{ return Cast<CGameplayEntity>( GetParent()->AsComponent()->GetEntity() ); }

	void UpdateIsTrapdoor();

	// initialize / deinitialize door attachment
	virtual void OnAttached();
	virtual void OnDetached() {}

	// let doors react to impacts etc.
	virtual void AddForceImpulse( const Vector& origin, Float force )	{}

	// update the door component, returns true if more ticks are needed
	virtual Bool Update( Float timeDelta )		{ RED_FATAL_ASSERT( 0, "Base virtual function call - IDoorAttachment"); return false; }

	virtual void InstantClose()					{ RED_FATAL_ASSERT( 0, "Base virtual function call - IDoorAttachment"); }
	virtual void InstantOpen()					{ RED_FATAL_ASSERT( 0, "Base virtual function call - IDoorAttachment"); }
	virtual void SetOpenAngle( Float angle )	{ RED_FATAL_ASSERT( 0, "Base virtual function call - IDoorAttachment"); }
	virtual bool IfNeedsInteraction() { return true; }
	virtual void Unsuppressed(){}
	virtual Bool IsInteractive(){ return true; }	
	virtual void SetStateForced() {}

	virtual void UpdateDoorState();
	virtual Bool IsClosed() const				{ RED_FATAL_ASSERT( 0, "Base virtual function call - IDoorAttachment"); return true; }
	virtual Bool IsOpened() const				{ RED_FATAL_ASSERT( 0, "Base virtual function call - IDoorAttachment"); return true; }

protected:
	virtual void InstantClose( CDoorComponent* door, CComponent* child )	{ RED_FATAL_ASSERT( 0, "Base virtual function call - IDoorAttachment"); }
	virtual void InstantOpen( CDoorComponent* door, CComponent* child )		{ RED_FATAL_ASSERT( 0, "Base virtual function call - IDoorAttachment"); }

	void PlayStartOpeningSounds();
	void PlayStopOpeningSounds( Bool _onlyLoop = false );
	void PlayStartClosingSounds();
	void PlayStopClosingSounds( Bool _onlyLoop = false );
};

BEGIN_CLASS_RTTI( IDoorAttachment );
	PARENT_CLASS( CHardAttachment );
	PROPERTY_EDIT( m_soundsEvents, TXT("Sounds events") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////////////////////////
// Simple yaw-rotation-over-time-animation to swing a door mesh open/closed
//////////////////////////////////////////////////////////////////////////////////////////////
class CDoorAttachment_AngleAnimation : public IDoorAttachment
{
	DECLARE_ENGINE_CLASS( CDoorAttachment_AngleAnimation, IDoorAttachment, 0 );

private:	
	Float		m_openAngle;
	Float		m_openTime;
	Float		m_openPercentage;
	Bool		m_stateForced;

public:
	CDoorAttachment_AngleAnimation();	
	virtual void OnAttached() override;
	virtual Bool Update( Float timeDelta ) override;
	virtual void InstantClose() override;
	virtual void InstantOpen() override;	
	virtual void SetOpenAngle( Float angle ) override { m_openAngle = angle; }

	virtual Bool IsClosed() const;
	virtual Bool IsOpened() const;

	void SetStateForced() override { m_stateForced = true; }
protected:
	void InstantClose( CDoorComponent* door, CComponent* child ) override;
	void InstantOpen( CDoorComponent* door, CComponent* child )	override;

private:
	Bool CanBeClosed( CDoorComponent* door );
};

BEGIN_CLASS_RTTI( CDoorAttachment_AngleAnimation );
	PARENT_CLASS( IDoorAttachment );
	PROPERTY_EDIT_RANGE( m_openTime, TXT("Open Time"), 0.1f, 5.0f );
	PROPERTY( m_originalAngle );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////////////////////////
// Using a property animation in the gameplay entity to open the door
//////////////////////////////////////////////////////////////////////////////////////////////
class CDoorAttachment_PropertyAnimation : public IDoorAttachment
{
	DECLARE_ENGINE_CLASS( CDoorAttachment_PropertyAnimation, IDoorAttachment, 0 );

private:	
	Float					m_animDuration;
	Float					m_openPercentage;
	
public:
	CDoorAttachment_PropertyAnimation();

	virtual Bool Init( CNode* parent, CNode* child, const AttachmentSpawnInfo* info ) override;
	virtual void OnAttached() override;
	virtual Bool Update( Float timeDelta ) override;
	virtual void InstantClose() override;
	virtual void InstantOpen() override;
	virtual void SetOpenAngle( Float angle ) override {}

	virtual Bool IsClosed() const;
	virtual Bool IsOpened() const;
	
protected:
	SPropertyAnimation* GetAnimationByName( const CName& name ) const;

	virtual void InstantClose( CDoorComponent* door, CComponent* child ) override;
	virtual void InstantOpen( CDoorComponent* door, CComponent* child )	override;
};

BEGIN_CLASS_RTTI( CDoorAttachment_PropertyAnimation );
	PARENT_CLASS( IDoorAttachment );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////////////////////////
// Allowing gameplay entities to push open the door
//////////////////////////////////////////////////////////////////////////////////////////////
class CDoorAttachment_GameplayPush : public IDoorAttachment
{
	DECLARE_ENGINE_CLASS( CDoorAttachment_GameplayPush, IDoorAttachment, 0 );

	static const Float YAW_EPS;
	static const Float ANG_EPS;
	static const Float COMBAT_LOCK_DISTANCE_SQRT;
	static const Float HORSE_BONUS_RADIUS;

private:
	Float				m_openAngle;		
	Float				m_accumulatedYaw;
	Float				m_yawSpeed;
	Float				m_autoCloseForce;
	Float				m_openingSpeed;
	Bool				m_stateForced;

	Bool				m_openningNotificationCalled	: 1;
	Bool				m_playerNotificationkCalled		: 1;	
	Bool				m_flipForward					: 1;
	Bool				m_invertedPivot;
public:
	CDoorAttachment_GameplayPush();

	virtual Bool Init( CNode* parent, CNode* child, const AttachmentSpawnInfo* info ) override;	
	virtual void AddForceImpulse( const Vector& origin, Float force ) override;
	virtual Bool Update( Float timeDelta ) override;
	void InstantClose() override;
	void InstantOpen() override;
	void SetOpenAngle( Float angle ) override { m_openAngle = angle; }
	bool IfNeedsInteraction() override { return false; }
	void Unsuppressed() override { m_playerNotificationkCalled = false; m_openningNotificationCalled = false; }
	Bool IsInteractive() override { return false; }
	void SetStateForced() override { m_stateForced = true; }
	Float GetAccumulatedYaw() const { return m_accumulatedYaw; }
	void ForceAccumulatedYaw( Float yaw );

protected:
	Bool GetDoorPlane( Plane& doorPlane );
	Bool GetDoorPlane( Plane& doorPlane, Vector& sideDirection );
	void InstantClose( CDoorComponent* door, CComponent* child ) override;
	void InstantOpen( CDoorComponent* door, CComponent* child )	override;
	virtual Bool IsClosed() const;
	virtual Bool IsOpened() const;
	Bool IsDoorMoving();
	Bool IsInDesiredState();
	Bool IsAngleValidForState( Float angle, Uint32 doorState ) const;

private:
	void NotifyPlayerOpened( CActor* doorUser );
	void NotifyOpenningDoors();	
};

BEGIN_CLASS_RTTI( CDoorAttachment_GameplayPush );
	PARENT_CLASS( IDoorAttachment );
//	PROPERTY_EDIT_RANGE( m_openAngle, TXT("Open Angle"), -180.0f, 180.0f );
	PROPERTY_EDIT_RANGE( m_autoCloseForce, TXT("Auto Close Force (in Angles per second)"), 0.0f, 500.0f );
	PROPERTY_EDIT( m_openingSpeed, TXT("Opening speed") );	
	PROPERTY_EDIT( m_invertedPivot, TXT("Mesh pivot inverted") );	
END_CLASS_RTTI();
