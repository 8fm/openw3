/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "polygon.h"

/// Brush vertex
class BrushVertex
{
public:
	Vector		m_position;		//!< Vertex position
	Vector		m_normal;		//!< Vertex normal
	Vector		m_mapping;		//!< Texture mapping

public:
	//! Default constructor
	RED_INLINE BrushVertex()
		: m_position( Vector::ZERO_3D_POINT )
		, m_normal( Vector::ZEROS )
		, m_mapping( Vector::ZEROS )
	{};

	//! Initialize from vector
	RED_INLINE BrushVertex( const Vector& position )
		: m_position( position )
		, m_normal( Vector::ZEROS )
		, m_mapping( Vector::ZEROS )
	{};

	//! Initialize from data
	RED_INLINE BrushVertex( const Vector& position, const Vector& normal, const Vector& mapping )
		: m_position( position )
		, m_normal( normal )
		, m_mapping( mapping )
	{};

public:
	//! Position access, used by TPolygon
	RED_INLINE Vector& Position() { return m_position; };
	RED_INLINE const Vector& Position() const { return m_position; };

public:
	//! Serialization
	RED_INLINE friend IFile& operator<<( IFile& file, BrushVertex& vertex )
	{
		file << vertex.m_position;
		file << vertex.m_normal;
		file << vertex.m_mapping;
		return file;
	}
};

// Brush vertex interpolation
template <>
RED_INLINE BrushVertex Lerp( Float frac, const BrushVertex& a, const BrushVertex& b )
{
	BrushVertex vertex;
	vertex.m_position = Lerp( frac, a.m_position, b.m_position );
	vertex.m_normal = Lerp( frac, a.m_normal, b.m_normal ).Normalized3();
	vertex.m_mapping =  Lerp( frac, a.m_mapping, b.m_mapping );
	return vertex;
}

// Polygon definition using brush vertex
typedef TPolygon< BrushVertex > CBrushPolygon;
