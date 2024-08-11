/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "brushComponent.h"
#include "brushVertex.h"

class CBrushBuilder;

/// Mapping mode
enum EBrushFaceMapping
{
	BFM_World,
	BFM_Local,
	BFM_Face,
};

BEGIN_ENUM_RTTI( EBrushFaceMapping );
	ENUM_OPTION( BFM_World );
	ENUM_OPTION( BFM_Face );
	ENUM_OPTION( BFM_Local );
END_ENUM_RTTI();

/// Brush face mapping info
class BrushFaceMapping
{
public:
	EBrushFaceMapping	m_mapping;		//!< Mapping mode
	Float				m_scaleU;		//!< Face mapping scale in U direction
	Float				m_scaleV;		//!< Face mapping scale in V direction
	Float				m_offsetU;		//!< Face mapping offset in U direction
	Float				m_offsetV;		//!< Face mapping offset in V direction
	Float				m_rotation;		//!< Face mapping rotation

public:
	RED_INLINE BrushFaceMapping()
		: m_scaleU( 1.0f )
		, m_scaleV( 1.0f )
		, m_offsetU( 0.0f )
		, m_offsetV( 0.0f )
		, m_rotation( 0.0f )
		, m_mapping( BFM_Local )
	{};
};

/// Single face in a brush, does not need to be planar
class CBrushFace : public CObject
{
	DECLARE_ENGINE_CLASS( CBrushFace, CObject, 0 );

	friend class CBrushBuilder;
	friend class CBrushComponent;
	friend class CCSGCompiler;

protected:
	struct Polygon
	{
		TDynArray< Uint32 >		m_indices;		//!< Polygon indices
		Plane					m_plane;		//!< Local plane

		// Serialization
		RED_INLINE friend IFile& operator<<( IFile& file, Polygon& poly )
		{
			file << poly.m_indices;
			file << poly.m_plane;
			return file;
		}
	};

protected:
	TDynArray< BrushVertex >	m_vertices;			//!< Brush vertices
	TDynArray< Polygon >		m_polygons;			//!< Brush face polygons, each is planar
	THandle< IMaterial >		m_material;			//!< Face material
	BrushFaceMapping			m_mapping;			//!< Face material mapping data
	Bool						m_isSelected;		//!< Face is selected
	Int32						m_renderFaceID;		//!< ID of compiled render face

public:
	//! Get brush
	RED_INLINE CBrushComponent* GetBrush() const { return SafeCast< CBrushComponent >( GetParent() ); }

	//! Get face material
	RED_INLINE IMaterial* GetMaterial() const { return m_material.Get(); }

	//! Get face mapping
	RED_INLINE const BrushFaceMapping& GetMapping() const { return m_mapping; }

	//! Get selection status
	RED_INLINE Bool IsSelected() const { return m_isSelected; }

public:
	CBrushFace();

	//! Serialize data
	virtual void OnSerialize( IFile& file );

public:
	//! Rebuild vertex normals
	void RebuildNormals();

	//! Set face mapping
	void SetMapping( const BrushFaceMapping& mapping );

	//! Set face material
	void SetMaterial( IMaterial* material );

	//! Set selection flag
	void SetSelection( Bool isSelected );

	//! Generate polygons from this face
	void GeneratePolygons( TDynArray< CPolygon* >& polygons ) const;

	//! Generate brush polygons from this face
	void GeneratePolygons( TDynArray< CBrushPolygon* >& polygons ) const;

	// Generate hit proxy fragments for editor
	void GenerateHitProxies( CHitProxyMap& map );

public:
	// Create face copy
	CBrushFace* CreateCopy( CObject* owner, Int32 faceFlipFlag, IMaterial* material ) const;
};

BEGIN_CLASS_RTTI( CBrushFace );
	PARENT_CLASS( CObject );
	PROPERTY_EDIT( m_material, TXT("Face material") );
	PROPERTY_EDIT_IN( m_mapping, m_mapping, TXT("Face mapping mode") );
	PROPERTY_EDIT_IN( m_mapping, m_scaleU, TXT("Face mapping scale") );
	PROPERTY_EDIT_IN( m_mapping, m_scaleV, TXT("Face mapping scale") );
	PROPERTY_EDIT_IN( m_mapping, m_offsetU, TXT("Face mapping offset") );
	PROPERTY_EDIT_IN( m_mapping, m_offsetV, TXT("Face mapping offset") );
	PROPERTY_EDIT_IN( m_mapping, m_rotation, TXT("Face mapping rotation") );
	PROPERTY( m_renderFaceID );
END_CLASS_RTTI();