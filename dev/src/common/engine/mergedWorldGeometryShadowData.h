/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "component.h"
#include "mergedWorldGeometry.h"
#include "mergedWorldGeometryEntity.h"

/// Shadow data generator
class CMergedWorldGeometryShadowData : public IMergedWorldGeometryData
{
	DECLARE_ENGINE_CLASS( CMergedWorldGeometryShadowData, IMergedWorldGeometryData, 0 );

public:
	CMergedWorldGeometryShadowData();

	/// IMergedWorldGeometryData interface
	virtual Bool CanMerge( const THandle< CComponent > component ) const;
	virtual Bool Merge( CDirectory* additionalContentDir, class CMergedWorldGeometryEntity* gridEntity, const TDynArray< THandle< CComponent > >& components, TDynArray<String> &outCorruptedMeshDepotPaths ) const;

private:
	Float		m_minExtractMeshRadius;	// minimal size of mesh that is removed from the scene and considered a merge candidate
	Float		m_minMergeMeshRadius;	// minimal size of mesh that is actually merged (the gap is simply ignored)
	Uint32		m_maxMeshTriangles;		// maximum number of triangles in mesh that can be still merged (at the merge LOD)
	Float		m_killZ;				// filter out triangles below that Z
	Float		m_killAngle;			// filter out triangle facing down with this angle threshold

	Bool		m_mergeCascade1;		// generate geometry for cascade 1
	Bool		m_mergeCascade2;		// generate geometry for cascade 2
	Bool		m_mergeCascade3;		// generate geometry for cascade 3
	Bool		m_mergeCascade4;		// generate geometry for cascade 4

	Float		m_streamingDistance;	// streaming distance for created meshes
	Bool		m_excludeProxies;		// exclude entity proxies for merging

	Bool		m_useInCascade1;		// use merged geometry in cascade 1
	Bool		m_useInCascade2;		// use merged geometry in cascade 2
	Bool		m_useInCascade3;		// use merged geometry in cascade 3
	Bool		m_useInCascade4;		// use merged geometry in cascade 4

	// create automatic mesh
	THandle< CMesh > CreateMergedMeshResource( CDirectory* additionalContentDir, const CMergedWorldGeometryGridCoordinates& gridCoordinates ) const;

	// assemble mesh file name
	static String GetMeshFileName( const CMergedWorldGeometryGridCoordinates& gridCoordinates );
};

BEGIN_CLASS_RTTI( CMergedWorldGeometryShadowData );
	PARENT_CLASS( IMergedWorldGeometryData );
	PROPERTY_EDIT( m_minExtractMeshRadius, TXT("Minimal size of mesh that is removed from the scene and considered a merge candidate") );
	PROPERTY_EDIT( m_minMergeMeshRadius, TXT("Minimal size of mesh that is actually merged (the gap is simply ignored)") );
	PROPERTY_EDIT( m_maxMeshTriangles, TXT("Maximum number of triangles in mesh that can be still merged") );
	PROPERTY_EDIT( m_mergeCascade1, TXT("Generate geometry for cascade 1") );
	PROPERTY_EDIT( m_mergeCascade2, TXT("Generate geometry for cascade 2") );
	PROPERTY_EDIT( m_mergeCascade3, TXT("Generate geometry for cascade 3") );
	PROPERTY_EDIT( m_mergeCascade4, TXT("Generate geometry for cascade 4") );
	PROPERTY_EDIT( m_excludeProxies, TXT("Eexclude entity proxies for merging") );
	PROPERTY_EDIT( m_streamingDistance, TXT("Streaming distance for created shadow meshes") );
	PROPERTY_EDIT( m_useInCascade1, TXT("Use merged geometry in cascade 1") );
	PROPERTY_EDIT( m_useInCascade2, TXT("Use merged geometry in cascade 2") );
	PROPERTY_EDIT( m_useInCascade3, TXT("Use merged geometry in cascade 3") );
	PROPERTY_EDIT( m_useInCascade4, TXT("Use merged geometry in cascade 4") );
	PROPERTY_EDIT( m_killZ, TXT("Filter out triangles below that Z") );
	PROPERTY_EDIT( m_killAngle, TXT("Filter out triangle facing down with this angle threshold") );
END_CLASS_RTTI();
