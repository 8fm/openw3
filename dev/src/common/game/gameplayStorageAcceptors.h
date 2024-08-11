/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

namespace GameplayStorageAcceptors
{

struct DefaultAcceptor
{
	Bool operator()( const CGameplayEntity* entity ) const
	{
		return true;
	}

	Bool operator()( const AACylinder& cylinder ) const
	{
		return true;
	}
};

struct SphereAcceptor
{
    SphereAcceptor( const Vector& position, Float radius );
    Bool operator()( const CGameplayEntity* entity ) const;
	Bool operator()( const AACylinder& cylinder ) const;

	Vector				m_position;
	Float				m_radius;
    Float				m_radiusSquare;
};

struct CylinderAcceptor
{
	CylinderAcceptor( const Vector& position, Float radius, Float height );
	Bool operator()( const CGameplayEntity* entity ) const;
	Bool operator()( const AACylinder& cylinder ) const;

	Vector				m_position;			//!< position is located at the botrom of cylinder Z axis
	Float				m_radius;
	Float				m_radiusSquare;
	Float				m_height;
};

struct ConeAcceptor
{
    ConeAcceptor( const Vector& position, Float coneDir, Float coneAngle, Float radius, Float height = NumericLimits< Float >::Max() );
    Bool operator()( const CGameplayEntity* entity ) const;
	Bool operator()( const AACylinder& cylinder ) const;

    Vector				m_position;			//!< position is located in the middle of cone Z axis
    Vector				m_heading;
	Vector				m_leftEdgeNormal;
	Vector				m_rightEdgeNormal;
    Float				m_halfAngleCos;
	Float				m_radius;
    Float				m_radiusSquare;
	Float				m_height;
};

struct BoxAcceptor
{
    BoxAcceptor( const Vector& position, const Box& boxLS, Float boxYaw = 0.0f /* box yaw */ );
    Bool operator()( const CGameplayEntity* entity ) const;
	Bool operator()( const AACylinder& cylinder ) const;

	Vector				m_position;
    Box					m_boxLS;			//!< box coords in respect to its position, boxWS = boxLS + position
	Vector				m_right;
	Vector				m_forward;
};

};