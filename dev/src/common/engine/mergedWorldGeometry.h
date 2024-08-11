/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "layer.h"

//--------------------------------------------------------

/// General geometry suplier
class IMergedWorldGeometrySupplier
{
public:
	virtual ~IMergedWorldGeometrySupplier() {};

	/// Get world size
	virtual const Float GetWorldSize() const = 0;

	/// Project given position on the terrain (should work both from above and below terrain)
	virtual const Vector ProjectOnTerrain( const Vector& pos ) const = 0;

	/// Get entities from given area, they should be streamed in
	virtual void GetEntitiesFromArea( const Box& worldSpaceBox, TDynArray< THandle< CEntity > >& outEntities ) const = 0;
};

//----------------------

/// Merging processor
class IMergedWorldGeometryData : public CObject
{
	DECLARE_ENGINE_CLASS( IMergedWorldGeometryData, CObject, 0 );

public:
	IMergedWorldGeometryData();
	~IMergedWorldGeometryData();

	/// Can we merge this component
	virtual Bool CanMerge( const THandle< CComponent > component ) const = 0;

	/// Merge input components
	virtual Bool Merge( CDirectory* additionalContentDir, class CMergedWorldGeometryEntity* gridEntity, const TDynArray< THandle< CComponent > >& components, TDynArray<String> &outCorruptedMeshDepotPaths ) const = 0;
};

BEGIN_ABSTRACT_CLASS_RTTI( IMergedWorldGeometryData );
	PARENT_CLASS( CObject );
END_CLASS_RTTI();

//----------------------

/// Helper class that manages merged geometry for given world
/// It's always embedded inside the CWorld
class CMergedWorldGeometry : public CObject
{
	DECLARE_ENGINE_CLASS( CMergedWorldGeometry, CObject, 0 );

public:
	CMergedWorldGeometry();
	~CMergedWorldGeometry();

	/// Get/Create the special layer
	static THandle< CLayer > GetContentLayer( CWorld* world );

	/// Get/Create the special content directory
	static CDirectory* GetContentDirectory( CWorld* world );

#ifndef NO_RESOURCE_IMPORT
	/// Build grid data
	const Bool Build( CDirectory* additionalContentDirectory, CLayer* contentLayer, const IMergedWorldGeometrySupplier* worldDataSupplier, const Vector& worldCenter, const Float worldRadius );
#endif

private:
	static const Char* LAYER_NAME;
	static const Char* CONTENT_DIR_NAME;
	// W3 BOB HACK
	static const Char* BOB_DIR_NAME;
	// End of hack

	// general
	Int32										m_gridSize;		//!< Size of the grid
	TDynArray< IMergedWorldGeometryData* >		m_mergers;		//!< Content mergers
};

BEGIN_CLASS_RTTI( CMergedWorldGeometry );
	PARENT_CLASS( CObject );
	PROPERTY_EDIT( m_gridSize, TXT("Size of the grid with merged data") );
	PROPERTY_INLINED( m_mergers, TXT("Content mergers") );
END_CLASS_RTTI();

//----------------------
