/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "drawableComponent.h"

class CBrushFace;
class CBrushList;
class CBrushBuilder;
class CCSGBrush;
class CCSGCompiler;

/// Brush CSG type
enum EBrushCSGType : CEnum::TValueType
{
	CSG_Edit,
	CSG_Addtive,
	CSG_Subtractive,
	CSG_Detail,
};

BEGIN_ENUM_RTTI( EBrushCSGType );
	ENUM_OPTION( CSG_Edit );
	ENUM_OPTION( CSG_Addtive );
	ENUM_OPTION( CSG_Subtractive );
	ENUM_OPTION( CSG_Detail );
END_ENUM_RTTI();

/// Basic geometry brush component
class CBrushComponent : public CDrawableComponent
{
	DECLARE_ENGINE_CLASS( CBrushComponent, CDrawableComponent, 0 );

	friend class CBrushBuilder;
	friend class CBrushCompiledData;

protected:
	TDynArray< CBrushFace* >	m_faces;		//!< Faces
	EBrushCSGType				m_csgType;		//!< Type of CSG
	Int32							m_brushIndex;	//!< Index of brush in the layer brush list

public:
	//! Get brush index
	RED_INLINE Int32 GetBrushIndex() const { return m_brushIndex; }

	//! Get CSG type
	RED_INLINE EBrushCSGType GetCSGType() const { return m_csgType; }

	//! Get the faces
	RED_INLINE const TDynArray< CBrushFace* >& GetFaces() const { return m_faces; }

public:
	CBrushComponent();

	//! Set brush CSG type
	void SetCSGType( EBrushCSGType csgType );

	// Called when component is attached to world ( layer gets visible, etc )
	virtual void OnAttached( CWorld* world );

	// Called when component is detached from world ( layer gets hidden, etc )
	virtual void OnDetached( CWorld* world );

	// Update brush bounds
	virtual void OnUpdateBounds();

	// Generate editor rendering fragments
	virtual void OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag );

	// Generate hit proxy fragments for editor
	virtual void OnGenerateEditorHitProxies( CHitProxyMap& map );

public:
	//! Remove all faces and verticesfrom brush
	void RemoveGeometry(); 

	//! Copy data ( vertices, faces, etc )
	Bool CopyData( const CBrushComponent* sourceData, Int32 flipSideFlag, IMaterial* materialToUse, EBrushCSGType csgType );
};

BEGIN_CLASS_RTTI( CBrushComponent );
	PARENT_CLASS( CDrawableComponent );
	PROPERTY_RO( m_brushIndex, TXT("Index in brush list") );
	PROPERTY_EDIT( m_csgType, TXT("Type of CSG") );
	PROPERTY( m_faces );
END_CLASS_RTTI();