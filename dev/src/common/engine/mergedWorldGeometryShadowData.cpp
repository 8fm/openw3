/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "layerInfo.h" // ehh
#include "layerGroup.h" // ehh
#include "mergedShadowMeshComponent.h"
#include "mergedMeshBuilder.h"
#include "mergedWorldGeometryShadowData.h"

IMPLEMENT_ENGINE_CLASS( CMergedWorldGeometryShadowData );

CMergedWorldGeometryShadowData::CMergedWorldGeometryShadowData()
	: m_minExtractMeshRadius( 0.0f )
	, m_minMergeMeshRadius( 0.3f )
	, m_maxMeshTriangles( 1024*32 )
	, m_mergeCascade1( false )
	, m_mergeCascade2( true )
	, m_mergeCascade3( true )
	, m_mergeCascade4( true )
	, m_useInCascade1( false )
	, m_useInCascade2( true )
	, m_useInCascade3( true )
	, m_useInCascade4( true )
	, m_killZ( -50.0f )
	, m_killAngle( 5.0f )
	, m_excludeProxies( true )
	, m_streamingDistance( 240.0f )
{
}

Bool CMergedWorldGeometryShadowData::CanMerge( const THandle< CComponent > component ) const
{
	// invalid component
	THandle< CMeshComponent > mc = Cast<CMeshComponent>( component );
	if ( !mc )
		return false;

	// do not merge shadow meshes again :)
	if ( mc->IsA< CMergedMeshComponent >() )
		return false;

	// no mesh
	THandle< CMesh > mesh = mc->GetMeshNow();
	if ( !mesh )
		return false;

	// proxy mesh
	if ( m_excludeProxies && mesh->IsEntityProxy() )
		return false;

	// component is not visible or is not casting shadows
	if ( !mc->IsVisible() || !mc->IsCastingShadows() )
		return false;

	// entity is not mergable
	if ( !mc->GetEntity() || mc->GetEntity()->CheckStaticFlag( ESF_NoCompMerge ) )
		return false;

	// check if the entity is on a layer we can merge
	CLayer* layer = mc->GetEntity()->GetLayer();
	if ( layer && layer->GetLayerInfo() )
	{
		Bool canProcess = false;

		// we cannot merge stuff from groups that can be hidden
		CLayerGroup* group = layer->GetLayerInfo()->GetLayerGroup();
		while ( group && group->GetParentGroup() )
		{
			if ( !group->IsVisibleOnStart() )
				return false;

			group = group->GetParentGroup();
		}

		auto mode = layer->GetLayerInfo()->GetMergedContentMode();
		if ( mode == LMC_Auto )
		{
			auto lgbt = layer->GetLayerInfo()->GetLayerBuildTag();
			canProcess = (lgbt != LBT_Quest) && (lgbt != LBT_Gameplay);
		}
		else if ( mode == LMC_ForceAlways )
		{
			canProcess = true;
		}
		else if ( mode == LMC_ForceNever )
		{
			canProcess = false;
		}

		// test flag
		if ( !canProcess )
			return false;
	}

	// mesh is not mergable
	if ( !mesh->CanMergeIntoGlobalShadowMesh() )
		return false;

	// mesh is to small to be extracted
	if ( mesh->GetGeneralizedMeshRadius() < m_minExtractMeshRadius )
		return false;

	// this mesh is casting shadows only from local lights, do not merge it
	if ( mc->IsCastingShadowsFromLocalLightsOnly() )
		return false;

	// component is animated ?
	if ( mc->GetTransformParent() != nullptr )
		return false;

	// get the LOD index to use
	TDynArray< Uint32 > chunksToUse;
	mesh->GetChunksForShadowMesh( chunksToUse );
	if ( chunksToUse.Empty() )
		return false;

	// count the triangles at given LOD
	Uint32 numTriangles = 0;
	for ( const Uint32 chunkIndex : chunksToUse )
	{
		numTriangles += (mesh->GetChunks()[ chunkIndex ].m_numIndices / 3);
	}

	// mesh to big to be merged ?
	if ( numTriangles > m_maxMeshTriangles )
		return false;

	// ok, we can try to merge this shit
	return true;
}

namespace Helper
{
	/// triangle filter & optimizer for shadows
	class CShadowMeshTriangleFilter : public CMeshData::IMergeGeometryModifier
	{
	public:
		CShadowMeshTriangleFilter( const Float killZ, const Float killDot )
			: m_killZ( killZ )
			, m_killDot( killDot )
		{}

		virtual Bool ProcessTriangle( const Matrix& referenceMatrix, SMeshVertex& a, SMeshVertex& b, SMeshVertex& c ) const override
		{
			// prepare vertices
			PrepareVertex( referenceMatrix, a );
			PrepareVertex( referenceMatrix, b );
			PrepareVertex( referenceMatrix, c );

			// are we below the kill Z ? (this cuts triangles that are below water level or sth)
			if ( a.m_position[2] < m_killZ && b.m_position[2] < m_killZ && c.m_position[2] < m_killZ )
				return false;

			// compute triangle's normal
			const Vector vab = b.GetPosition() - a.GetPosition();
			const Vector vac = c.GetPosition() - a.GetPosition();
			Vector n = Vector::Cross( vac, vab );
			if ( n.Normalize3() < 0.0000001f )
				return false; // triangle is to small

			// check triangle facing in the Z direction
			if ( n.Z <= m_killDot )
				return false;

			// triangle seems valid for shadow mesh
			return true;
		}

		RED_INLINE void PrepareVertex( const Matrix& referenceMatrix, SMeshVertex& v ) const
		{
			// transform position only
			const Vector pos( v.m_position[0], v.m_position[1], v.m_position[2] );
			const Vector newPos = referenceMatrix.TransformPoint( pos );

			// setup new vertex - only position matters
			Red::MemoryZero( &v, sizeof(v) );
			v.m_position[0] = newPos.X;
			v.m_position[1] = newPos.Y;
			v.m_position[2] = newPos.Z;
		}

	private:
		Float		m_killZ;
		Float		m_killDot;
	};
}

Bool CMergedWorldGeometryShadowData::Merge( CDirectory* additionalContentDir, class CMergedWorldGeometryEntity* gridEntity, const TDynArray< THandle< CComponent > >& components, TDynArray<String> &outCorruptedMeshDepotPaths ) const
{
#ifndef NO_RESOURCE_IMPORT
	// assemble the merge mask
	Uint8 mergeMask = 0;
	if ( m_mergeCascade1 ) mergeMask |= MCR_Cascade1;
	if ( m_mergeCascade2 ) mergeMask |= MCR_Cascade2;
	if ( m_mergeCascade3 ) mergeMask |= MCR_Cascade3;
	if ( m_mergeCascade4 ) mergeMask |= MCR_Cascade4;

	// assemble the render mask (can be different)
	Uint8 renderMask = 0;
	if ( m_useInCascade1 ) renderMask |= MCR_Cascade1;
	if ( m_useInCascade2 ) renderMask |= MCR_Cascade2;
	if ( m_useInCascade3 ) renderMask |= MCR_Cascade3;
	if ( m_useInCascade4 ) renderMask |= MCR_Cascade4;

	// prepare source data
	MergedMeshSourceData sourceData( mergeMask );

	// add stuff from components
	TDynArray< GlobalVisID > mergedObjects;
	for ( THandle< CComponent > comp : components )
	{
		THandle< CMeshComponent > mc = Cast<CMeshComponent>( comp );
		if ( mc )
		{
			// merge only meshes bigger than certain radius, the rest is ignored from the shadow mesh
			THandle< CMesh > mesh = mc->GetMeshNow();
			if ( mesh && mesh->GetGeneralizedMeshRadius() >= m_minMergeMeshRadius )
			{
				TDynArray< Uint32 > shadowChunks;
				mesh->GetChunksForShadowMesh( shadowChunks );

				if ( !shadowChunks.Empty() )
				{
					sourceData.AddComponent( mc, shadowChunks );
				}
			}

			// store ID of the merged object
			mergedObjects.PushBack( GlobalVisID( mc->GetMeshNow(), mc->GetLocalToWorld() ) );
		}
	}

	// get the target mesh
	THandle< CMesh > mergedMesh = CreateMergedMeshResource( additionalContentDir, gridEntity->GetGridCoordinates() );
	if ( !mergedMesh )
	{
		ERR_ENGINE( TXT("Failed to prepare merged mesh for grid cell [%d,%d]"), 
			gridEntity->GetGridCoordinates().m_x, gridEntity->GetGridCoordinates().m_y );
		return false;
	}

	// prepare geometry filter
	const Float killDot = -cosf( DEG2RAD(m_killAngle) );
	const Helper::CShadowMeshTriangleFilter geometryFilter( m_killZ, killDot );

	Vector mergedMeshOrigin( 0.0f, 0.0f, 0.0f, 0.0f );

	// build shadow mesh
	MergedMeshBuilder meshBuilder( mergedMesh, MVT_StaticMesh );
	meshBuilder.AddData( sourceData, &geometryFilter, outCorruptedMeshDepotPaths, mergedMeshOrigin );
	meshBuilder.SetRenderMaskForAllChunks( renderMask );
	if ( meshBuilder.Flush() )
	{
		// save the mesh
		RED_ASSERT( mergedMesh->GetFile() != nullptr, TXT("Merged mesh with no file") );
		if ( mergedMesh->GetFile() != nullptr )
		{
			mergedMesh->GetFile()->SilentCheckOut();

			// Calc bounding volume diagonal bounding sphere
			const Float diagonal = Max<Float>( sourceData.GetWorldBox().CalcSize().Mag3() * 0.5f , 100.0f );

			mergedMesh->SetGeneralizedMeshRadius( diagonal ); // make sure we don't cull it from the cascades
			mergedMesh->SetAutoHideDistance( 1000.0f + diagonal ); // make sure it's visible way longer than the mesh streaming distance

			mergedMesh->GetFile()->Save();
		}

		// create component that will hold the final data
		CMergedShadowMeshComponent* mergedComponent = new CMergedShadowMeshComponent( mergedMesh, mergedObjects, m_streamingDistance, renderMask );
		mergedComponent->SetDrawableFlags( DF_CastShadows | DF_CastShadowsWhenNotVisible );
		mergedComponent->SetParent( gridEntity );
		mergedComponent->SetName( TXT("shadow_mesh") );
		mergedComponent->SetStreamed( true );

		// Override entity position so it will be fit better to the content it holds
		gridEntity->SetPosition( mergedMeshOrigin );

		gridEntity->AddComponent( mergedComponent );

		// compute stats for the entity
		{
			const Uint32 numTriangles = mergedMesh->CountLODTriangles(0);
			const Uint32 numVertices = mergedMesh->CountLODVertices(0);
			const Uint32 cookedDataSize = numTriangles*6 + numVertices*8;
			gridEntity->AccumulatePayloadStats( cookedDataSize, numTriangles, numVertices );
		}
	}

	// done
	return true;
#else
	return false;
#endif
}

THandle< CMesh > CMergedWorldGeometryShadowData::CreateMergedMeshResource( CDirectory* additionalContentDir, const CMergedWorldGeometryGridCoordinates& gridCoordinates ) const
{
	RED_FATAL_ASSERT( gridCoordinates.IsValid(), "Invalid grid coordinates" );

#ifndef NO_RESOURCE_IMPORT
	// no content directory specified - we will not be able to create the meshes
	if ( !additionalContentDir )
		return nullptr;

	// find existing file
	const String fileName = GetMeshFileName( gridCoordinates );
	CDiskFile* file = additionalContentDir->FindLocalFile( fileName );
	if ( file != nullptr )
	{
		// checkout file
		file->SilentCheckOut();

		// get mesh
		THandle< CMesh > retMesh = Cast< CMesh >( file->Load() );
		RED_ASSERT( retMesh != nullptr, TXT("Failed to load existing mesh from '%ls'"), file->GetDepotPath().AsChar() );
		return retMesh;
	}

	// create empty resource
	THandle< CMesh > newMesh = new CMesh();
	if ( !newMesh->SaveAs( additionalContentDir, fileName ) )
	{
		return nullptr;
	}

	// return created file
	return newMesh;
#else
	return nullptr;
#endif
}

String CMergedWorldGeometryShadowData::GetMeshFileName( const CMergedWorldGeometryGridCoordinates& gridCoordinates )
{
	RED_FATAL_ASSERT( gridCoordinates.IsValid(), "Invalid grid coordinates" );

	Char buf[128];
	Red::SNPrintF( buf, ARRAY_COUNT(buf), TXT("shadow_mesh_%dx%d.%ls"), 
		gridCoordinates.m_x, gridCoordinates.m_y,
		ResourceExtension< CMesh >() );
	return String(buf);
}
