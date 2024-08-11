/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "entityHandle.h"

// Saves reference to entity or just position/rotation
struct PersistentRef
{
	DECLARE_RTTI_STRUCT( PersistentRef )

private:
	EntityHandle	m_entityHandle;		//!< Entity handle
	Vector			m_position;			//!< Position
	EulerAngles		m_rotation;			//!< Rotation

public:
	//! Default constructor
	PersistentRef()
		: m_position( Vector::ZEROS )
		, m_rotation( EulerAngles::ZEROS )
	{}

	//! Create from CNode
	PersistentRef( CNode* node )
	{
		Set( node );
	}

	//! Create from position and rotation
	PersistentRef( const Vector& pos, const EulerAngles& rot )
		: m_position( pos )
		, m_rotation( rot )
	{
	}


	//! Set using node
	void Set( CNode* node );

	//! Set using orientation
	void Set( const Vector& pos, const EulerAngles& rot );

	//! Get entity
	CEntity* GetEntity();

	//! Get entity
	const CEntity* GetEntity() const;

	//! Get full world position
	Vector GetWorldPosition() const;

	//! Get full world rotation
	EulerAngles GetWorldRotation() const;

	//! Get full world orientation
	void GetWorldOrientation( Vector& position, EulerAngles& rotation ) const;
};

BEGIN_CLASS_RTTI( PersistentRef );
	PROPERTY_SAVED( m_entityHandle );
	PROPERTY_SAVED( m_position );
	PROPERTY_SAVED( m_rotation );
END_CLASS_RTTI();