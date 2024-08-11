/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "brushRenderData.h"

class CBrushComponent;

/// Brush world - a list of CSG brushes with generated geometry
class CBrushCompiledData : public CObject
{
	DECLARE_ENGINE_CLASS( CBrushCompiledData, CObject, 0 );

protected:
	TDynArray< CBrushComponent* >	m_brushes;			//!< All brushes ( in order )
	BrushRenderData					m_renderData;		//!< Rendering data

public:
	//! Get compiled brush rendering data
	RED_INLINE const BrushRenderData& GetRenderData() const { return m_renderData; }

	//! Get compiled brush rendering data
	RED_INLINE BrushRenderData& GetRenderData() { return m_renderData; }

public:
	//! Serialization
	virtual void OnSerialize( IFile& file );

public:
	//! Register in brush list
	void AddBrush( CBrushComponent* brush );

	//! Register brushes in brush list, they will be added to the back of the list
	void AddBrushes( const TDynArray< CBrushComponent* > &brushes );

	//! Remove brush from list
	void RemoveBrush( CBrushComponent* brush );

	//! Compile CSG geometry from the brushes
	void Compile();

	//! Layer with this brush data was attached to world
	void AttachedToWorld( CWorld* world );

	//! Layer with this brush data was detached from world
	void DetachedFromWorld( CWorld* world );

public:
	//! Sort brushes using the brush list index
	static void SortBrushes( TDynArray< CBrushComponent* >& brushes );
};

BEGIN_CLASS_RTTI( CBrushCompiledData );
	PARENT_CLASS( CObject );
	PROPERTY( m_brushes );
END_CLASS_RTTI();