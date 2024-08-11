/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

class CBrushComponent;
class CBrushFace;

/// Builder of brush geometry
class CBrushBuilder : public CObject
{
	DECLARE_ENGINE_ABSTRACT_CLASS( CBrushBuilder, CObject );

protected:
	CBrushComponent*		m_brush;			//!< Brush we are building
	CBrushFace*				m_face;				//!< Current face
	TDynArray< Uint32 >		m_tempIndices;		//!< Face indices
	Int32						m_tempFaceSign;		//!< Face sign ( winding sign )

public:
	//! Build brush geometry !
	virtual void Build( CBrushComponent* brush )=0;

public:
	//! Begin brush
	void BeginBrush( CBrushComponent* brush );

	//! End brush, will recompute some stuff
	void EndBrush();

	//! Add brush vertex
	Uint32 AddVertex( const Vector& pos, Float u, Float v );

	//! Add full brush vertex
	Uint32 AddVertex( const Vector& pos, Float u, Float v, const Vector& normal );

	//! Add brush vertex
	Uint32 AddVertex( Float x, Float y, Float z, Float u, Float v );

	//! Begin brush face
	void BeginFace();

	//! End brush face
	void EndFace( Bool buildNormals = true );

	//! Begin brush polygon
	void BeginPolygon( Int32 sideSign );

	//! End brush polygon
	void EndPolygon();

	//! Add triangle
	void AddTriangle( Int32 side, Uint32 a, Uint32 b, Uint32 c );

	//! Add quad
	void AddQuad( Int32 side, Uint32 a, Uint32 b, Uint32 c, Uint32 d );

	//! Add point to polygon
	void AddIndex( Uint32 index );
};

BEGIN_ABSTRACT_CLASS_RTTI( CBrushBuilder );
	PARENT_CLASS( CObject );
END_CLASS_RTTI();