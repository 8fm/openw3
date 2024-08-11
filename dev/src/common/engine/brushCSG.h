/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "localBSP.h"
#include "brushVertex.h"

class CBrushFace;
class CBrushComponent;
class BrushRenderData;

/// CSG face
class CCSGFace
{
public:
	CBrushFace*			m_face;		//!< Source face
	CBrushPolygon*		m_polygon;	//!< Clipping polygon
	Bool				m_flag;		//!< Clipping flag

public:
	CCSGFace( CBrushFace *face, Bool flag, CBrushPolygon *poly );
	CCSGFace( CCSGFace *face, Bool flag, CBrushPolygon *newPoly );
	~CCSGFace();
};

/// CSG brush
class CCSGBrush
{
public:
	CBrushComponent*		m_brush;			//!< Source brush
	Box						m_box;				//!< Bounding box
	Matrix					m_localToWorld;		//!< Cached local to world matrix
	Matrix					m_worldToLocal;		//!< Cached world to local matrix
	TDynArray< CCSGFace* >	m_faces;			//!< Faces
	CLocalBSP				m_clipBPS;			//!< Clipping BSP
	Bool					m_generateFaces;	//!< Generate new geometry
	Bool					m_subtractive;		//!< Subractive brush
	Bool					m_detail;			//!< Detail brush

public:
	CCSGBrush( CBrushComponent* brush, Bool generateFaces );
	~CCSGBrush();
};

/// CSG engine
class CCSGCompiler
{
public:
	//! Compile full CSG for all brushes in given brush list
	void CompileCSG( const TDynArray< CBrushComponent* > &brushes, BrushRenderData& renderData );

private:
	//! Compile full CSG for all brushes in given brush list
	void CompileCSG( const TDynArray< CCSGBrush* > &brushes, Bool useDetailBrushes, BrushRenderData& renderData );

	//! Clip face
	void ClipFace( Bool preClip, const CCSGBrush &srcBrush, const CCSGBrush &clipBrush, CCSGFace *face, TDynArray< CCSGFace* > &faces, Bool useDetailBrushes );

	//! Feed brush with new render data
	void GenerateRenderData( const CCSGBrush &brush, BrushRenderData& renderData );
};
