/**
 * Copyright © 2009 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#if USE_HAVOK

#include "havokDataBuffer.h"

enum ERagdollBodyFlag
{
	RBF_Leg								= FLAG( 1 ),	// Body is part of a leg
	RBF_Obstacle						= FLAG( 2 ),	// Body is an obstacle for ragdoll attached dynamic bodies
};

BEGIN_BITFIELD_RTTI( ERagdollBodyFlag, 1 );
	BITFIELD_OPTION( RBF_Leg );
	BITFIELD_OPTION( RBF_Obstacle );
END_BITFIELD_RTTI();


////////////////////////////////////////////////////////////
// CRagdollBody
////////////////////////////////////////////////////////////

class CRagdollBody : public CObject
{
	DECLARE_ENGINE_CLASS( CRagdollBody, CObject, 0 )

public:
	CRagdollBody() 
		: m_inertiaFactor( 1.f )
		, m_flags( 0 )
	{}

public:
	String						m_bodyName;			// Preview body name - editor only
	Float						m_inertiaFactor;	// Inertia factor
	Uint8						m_flags;			// Any special custom flags

};

BEGIN_CLASS_RTTI( CRagdollBody );
	PARENT_CLASS( CObject );
	PROPERTY_RO_NOT_COOKED( m_bodyName, TXT("Body name") );
	PROPERTY_EDIT_RANGE( m_inertiaFactor, TXT("Inertia factor"), 1.f, 1000.f );
	PROPERTY_BITFIELD_EDIT( m_flags, ERagdollBodyFlag, TXT("Special flags") );
END_CLASS_RTTI();


////////////////////////////////////////////////////////////
// CRagdollConstraint
////////////////////////////////////////////////////////////

class CRagdollConstraint : public CObject
{
	DECLARE_ENGINE_CLASS( CRagdollConstraint, CObject, 0 )

public:
	CRagdollConstraint()
		: m_springConstant( 0.f )
		, m_springDamping( 0.f )
	{}

public:
	String						m_constraintName;	// Preview constraint name - editor only
	Float						m_springConstant;	// Spring constant parameter (spring damping motor must be selected in Havok Content Tools!)
	Float						m_springDamping;	// Spring damping parameter (spring damping motor must be selected in Havok Content Tools!)
};

BEGIN_CLASS_RTTI( CRagdollConstraint );
	PARENT_CLASS( CObject );
	PROPERTY_RO_NOT_COOKED( m_constraintName, TXT("Constraint name") );
	PROPERTY_EDIT_RANGE( m_springConstant, TXT("Spring constant"), 0.f, 1000.f );
	PROPERTY_EDIT_RANGE( m_springDamping, TXT("Spring damping"), 0.f, 1000.f );
END_CLASS_RTTI();


////////////////////////////////////////////////////////////
// CRagdoll
////////////////////////////////////////////////////////////

/// Physical ragdoll
class CRagdoll : public CResource
{
	DECLARE_ENGINE_RESOURCE_CLASS( CRagdoll, CResource, "w2ragdoll", "Ragdoll" )

protected:
	THavokDataBuffer< hkaRagdollInstance >	m_ragdollInstance;
	THavokDataBuffer< hkaSkeletonMapper >	m_lowToHighMapper;
	THavokDataBuffer< hkaSkeletonMapper >	m_highToLowMapper;
	CName									m_lowResSkeletonName;
	CName									m_highResSkeletonName;
	TDynArray< CRagdollBody* >				m_ragdollBodies;			// Additional rigid bodies information
	TDynArray< CRagdollConstraint* >		m_ragdollConstraints;		// Additional constraints information

public:
	//! Ragdoll editor import  data
	struct FactoryInfo : public CResource::FactoryInfo< CRagdoll >
	{				
		hkaRagdollInstance*		m_ragdollInstance;
		hkaSkeletonMapper*		m_lowToHighMapper;
		hkaSkeletonMapper*		m_highToLowMapper;
		CName					m_lowResSkeletonName;
		CName					m_highResSkeletonName;
	};

public:
	//! Get name of low res skeleton
	FORCE_INLINE const CName& GetLowResSkeletonName() const { return m_lowResSkeletonName; }

	//! Get name of high res skeleton
	FORCE_INLINE const CName& GetHighResSkeletonName() const { return m_highResSkeletonName; }

public:
	CRagdoll();

	//! Serialize the object
	virtual void OnSerialize( IFile& file );
	
public:
	//! Get number of bodies in ragdoll
	Int GetNumBodies() const;

	//! Get original ragdoll rigid body
	const class hkpRigidBody* GetOriginalBody( Int index ) const;

	const class hkpRigidBody* GetOriginalBodyOfBone( Int index ) const;

	//! Get parent of given rigid body
	const class hkpRigidBody* GetParentOfBody( Int index ) const;

	//! Find index of given rigid body in the ragdoll, returns -1 if not found
	Int FindRigidBodyIndex( const class hkpRigidBody* body ) const;

	//! Get intertia scale for given rigid body
	Float GetInertiaFactor( Int rigidBodyIndex ) const;
	
	//! Check if body is part of a leg
	Bool GetIsLeg( Int rigidBodyIndex, const AnsiChar* bodyName ) const;
	
	//! Check if body is an obstacle for other dynamic ragdoll parts
	Bool GetIsObstacle( Int rigidBodyIndex ) const;

	//! Returns true if any spring dumper parametrs should be enabled
	Bool HasAnySpringParameters() const;
	
	//! Get spring constant for given constraint
	Float GetSpringConstant( Int constraintIndex ) const;

	//! Get spring damping scale for given constraint
	Float GetSpringDamping( Int constraintIndex ) const;

	//! Get data buffers
	void GetHavokDataBuffers( TDynArray< const HavokDataBuffer* >& buffers ) const;

	//! Assign skeletons to this ragodll
	void AssignSkeletons( const hkaSkeleton* hiRes, const hkaSkeleton* lowRes );

public:
	const hkaSkeletonMapper* GetLowToHighMapper() const;

	//! Create copy of Havok low to high skeleton mapping
	hkaSkeletonMapper* CloneLowToHighMapper() const;

	//! Create copy of Havok high to low skeleton mapping
	hkaSkeletonMapper* CloneHighToLowMapper() const;

	//! Create copy of Havok ragdoll instance
	THavokDataBuffer< hkaRagdollInstance >* CloneInstance() const;

public:
	//! Create resource from factory definition
	static CRagdoll* Create( const FactoryInfo& data );

};

////////////////////////////////////////////////////////////////////////////////////////

BEGIN_CLASS_RTTI( CRagdoll );
	PARENT_CLASS( CResource );
	PROPERTY_RO( m_lowResSkeletonName, TXT("Low res skeleton name") );
	PROPERTY_RO( m_highResSkeletonName, TXT("High res skeleton name") );
	PROPERTY_INLINED( m_ragdollBodies, TXT("Ragdoll bodies") );
	PROPERTY_INLINED( m_ragdollConstraints, TXT("Ragdoll constraints") );
END_CLASS_RTTI();

///////////////////////////////////////////////////////////////////////////////////////

#endif