/**
* Copyright c 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "aiProfile.h"
#include "gameplayStorageAcceptors.h"

class CAIAttackRange : public CObject								
	// Change of name would be nice, because it is no longer so strongly connected to AI
	// Make it abstract after E3, right now its unnecessary and might cause crashes is CAIAttackRange objects are used in some entities 
{
	DECLARE_ENGINE_CLASS( CAIAttackRange, CObject, 0 );	
	//DECLARE_ENGINE_ABSTRACT_CLASS( CAIAttackRange, CObject );

public:

	Float	m_rangeMax;
	Float	m_height;
	Vector	m_position;
	CName	m_name;
	Float	m_angleOffset;
	Bool	m_checkLineOfSight;
	Float	m_lineOfSightHeight;
	Bool	m_useHeadOrientation;

	CAIAttackRange();
	
	virtual void GatherEntities( const CGameplayEntity* parentEntity, TDynArray< THandle< CGameplayEntity > > & output ) const;
	virtual Bool Test( const CGameplayEntity* parentEntity, const CGameplayEntity* testedEntity, Float predictPositionInTime = -1 ) const;
	
	virtual String GetFriendlyName() const;
	virtual void OnGenerateDebugFragments( const CEntity* parentActor, CRenderFrame* frame ) const;

	static const CAIAttackRange* Get( const CEntity* entity, CName attackRange );

protected:

	virtual Float GetStorageMaxRange() const;

	template < typename CustomAcceptor >
	void GatherEntities( const CGameplayEntity* parentEntity, TDynArray< THandle< CGameplayEntity > > & output, const CustomAcceptor& acceptor ) const;
	template < typename CustomAcceptor >
	Bool Test( const CGameplayEntity* parentEntity, const CGameplayEntity* testedEntity, Float predictPositionInTime, const CustomAcceptor& acceptor) const;

	void GetAttackL2W( const CEntity* parentEntity, Matrix& l2W ) const
	{
		if ( m_useHeadOrientation && parentEntity->IsA< CActor >() )
		{
			const CActor* parentActor = static_cast< const CActor* >( parentEntity );
			Int32 headBone = parentActor->GetHeadBone();
			if ( headBone != -1 )
			{
				CAnimatedComponent* anim = parentActor->GetRootAnimatedComponent();
				if ( anim )
				{
					Matrix headMat = anim->GetBoneMatrixWorldSpace( headBone );
					l2W = headMat.SetRotZ33( headMat.GetYaw() );
				}
			}
		}
		l2W = parentEntity->GetLocalToWorld();
	}

private:

	Vector GetLosTestPosition( const CEntity* parent ) const;

	void funcTest( CScriptStackFrame& stack, void* result );
	void funcGatherEntities( CScriptStackFrame& stack, void* result );
};

//BEGIN_ABSTRACT_CLASS_RTTI( CAIAttackRange );
BEGIN_CLASS_RTTI( CAIAttackRange );
	PARENT_CLASS( CObject );
	PROPERTY_EDIT( m_name, TXT( "Attack range name" ) );
	PROPERTY_EDIT_RANGE( m_rangeMax, TXT("Maximum range"), 0.0f, NumericLimits< Float >::Max() );
	PROPERTY_EDIT_RANGE( m_height, TXT("Height"), 0.0f, NumericLimits< Float >::Max() );
	PROPERTY_EDIT_RANGE( m_angleOffset, TXT("offset angle"), 0.0f, 360.f );
	PROPERTY_EDIT( m_position, TXT("Position") );
	PROPERTY_EDIT( m_checkLineOfSight, TXT("Perform line of sight test"));
	PROPERTY_EDIT( m_lineOfSightHeight, TXT("Height used for line of sight test for non-actors."));
	PROPERTY_EDIT( m_useHeadOrientation, TXT("Use entity head orientation instead of entity orientation"));
	NATIVE_FUNCTION( "Test", funcTest );
	NATIVE_FUNCTION( "GatherEntities", funcGatherEntities );
END_CLASS_RTTI();

///////////////////////////////////////////////////////////////////////////////

class CSphereAttackRange : public CAIAttackRange
{
	DECLARE_ENGINE_CLASS( CSphereAttackRange, CAIAttackRange, 0 );

public:

	CSphereAttackRange();

	GameplayStorageAcceptors::SphereAcceptor CreateAcceptor( const CGameplayEntity* parentEntity ) const;
	virtual void GatherEntities( const CGameplayEntity* parentEntity, TDynArray< THandle< CGameplayEntity > > & output ) const override;
	virtual Bool Test( const CGameplayEntity* parentEntity, const CGameplayEntity* testedEntity, Float predictPositionInTime = -1 ) const override;

	virtual String GetFriendlyName() const override;
	virtual void OnGenerateDebugFragments( const CEntity* parentActor, CRenderFrame* frame ) const override;
};

BEGIN_CLASS_RTTI( CSphereAttackRange )
	PARENT_CLASS( CAIAttackRange )
END_CLASS_RTTI()

///////////////////////////////////////////////////////////////////////////////

class CCylinderAttackRange : public CAIAttackRange
{
	DECLARE_ENGINE_CLASS( CCylinderAttackRange, CAIAttackRange, 0 );

public:

	CCylinderAttackRange();

	GameplayStorageAcceptors::CylinderAcceptor CreateAcceptor( const CGameplayEntity* parentEntity ) const;
	virtual void GatherEntities( const CGameplayEntity* parentEntity, TDynArray< THandle< CGameplayEntity > > & output ) const override;
	virtual Bool Test( const CGameplayEntity* parentEntity, const CGameplayEntity* testedEntity, Float predictPositionInTime = -1 ) const override;

	virtual String GetFriendlyName() const override;
	virtual void OnGenerateDebugFragments( const CEntity* parentActor, CRenderFrame* frame ) const override;
};

BEGIN_CLASS_RTTI( CCylinderAttackRange )
	PARENT_CLASS( CAIAttackRange )
END_CLASS_RTTI()

///////////////////////////////////////////////////////////////////////////////

class CConeAttackRange : public CAIAttackRange
{
	DECLARE_ENGINE_CLASS( CConeAttackRange, CAIAttackRange, 0 );

public:

	Float	m_rangeAngle;

	CConeAttackRange();
	
	GameplayStorageAcceptors::ConeAcceptor CreateAcceptor( const CGameplayEntity* parentEntity ) const;
	virtual void GatherEntities( const CGameplayEntity* parentEntity, TDynArray< THandle< CGameplayEntity > > & output ) const override;
	virtual Bool Test( const CGameplayEntity* parentEntity, const CGameplayEntity* testedEntity, Float predictPositionInTime = -1 ) const override;

	virtual String GetFriendlyName() const override;
	virtual void OnGenerateDebugFragments( const CEntity* parentActor, CRenderFrame* frame ) const override;
};

BEGIN_CLASS_RTTI( CConeAttackRange )
	PARENT_CLASS( CAIAttackRange )
	PROPERTY_EDIT_RANGE( m_rangeAngle, TXT("Angle range"), 0.0f, 360.0f );
END_CLASS_RTTI()

//////////////////////////////////////////////////////////////////////////

class CBoxAttackRange : public CAIAttackRange
{
	DECLARE_ENGINE_CLASS( CBoxAttackRange, CAIAttackRange, 0 );

public:

	Float	m_rangeWidth;

	CBoxAttackRange();

	GameplayStorageAcceptors::BoxAcceptor CreateAcceptor( const CGameplayEntity* parentEntity ) const;
	virtual void GatherEntities( const CGameplayEntity* parentEntity, TDynArray< THandle< CGameplayEntity > > & output ) const override;
	virtual Bool Test( const CGameplayEntity* parentEntity, const CGameplayEntity* testedEntity, Float predictPositionInTime = -1 ) const override;

	virtual String GetFriendlyName() const override;
	virtual void OnGenerateDebugFragments( const CEntity* parentActor, CRenderFrame* frame ) const override;

protected:

	virtual Float GetStorageMaxRange() const override;
	void CreateLocalBox( Box& box ) const;
};

BEGIN_CLASS_RTTI( CBoxAttackRange )
	PARENT_CLASS( CAIAttackRange )
	PROPERTY_EDIT( m_rangeWidth, TXT( "Width of the box (max distance from box axis)" ) );
END_CLASS_RTTI()
