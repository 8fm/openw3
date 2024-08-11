/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "apexDestructionResource.h"
#include "../core/dataError.h"
#include "../physics/physicsEngine.h"
#include "NxDestructibleAsset.h"
#include "NxParamUtils.h"
#include "NxRenderMeshAsset.h"
#include "../core/2darray.h"

IMPLEMENT_ENGINE_CLASS( CApexDestructionResource );


CApexDestructionResource::CApexDestructionResource()
	: m_unfracturedDensityScaler( 1.0f )
	, m_fracturedDensityScaler( 1.0f )
{
}


CApexDestructionResource::~CApexDestructionResource()
{
}


void CApexDestructionResource::RestoreDefaults()
{
	m_neighborPadding = GetDefaultParameterF32( "neighborPadding" );
	m_maxDepth = GetDefaultParameterU32( "depthCount" ) - 1;
	m_originalMaxDepth = GetDefaultParameterU32( "originalDepthCount" ) - 1;
	m_supportDepth = GetDefaultParameterU32( "supportDepth" );
	m_formExtendedStructures = GetDefaultParameterBool( "formExtendedStructures" );
	m_useAssetSupport = GetDefaultParameterBool( "useAssetDefinedSupport" );
	m_useWorldSupport = GetDefaultParameterBool( "useWorldSupport" );
	m_initialAllowance = GetDefaultParameterU32( "initialDestructibleActorAllowanceForInstancing" );
}

void CApexDestructionResource::ConfigureNewAsset( NxParameterized::Interface* params ) const
{
	 SetParameterF32( params, "neighborPadding", m_neighborPadding );
	 SetParameterU32( params, "depthCount", m_maxDepth + 1 );
	 SetParameterU32( params, "originalDepthCount", m_originalMaxDepth + 1 );
	 SetParameterU32( params, "supportDepth", m_supportDepth );
	 SetParameterBool( params, "formExtendedStructures", m_formExtendedStructures );
	 SetParameterBool( params, "useAssetDefinedSupport", m_useAssetSupport );
	 SetParameterBool( params, "useWorldSupport", m_useWorldSupport );
	 SetParameterU32( params, "initialDestructibleActorAllowanceForInstancing", m_initialAllowance );
}

CName CApexDestructionResource::GetMaterialForChunkDepth( Uint32 depth ) const
{
	if ( depth >= m_chunkDepthMaterials.Size() )
		return CNAME( default );
	return m_chunkDepthMaterials[ depth ];
}

void CApexDestructionResource::SetMaterialForChunkDepth( Uint32 depth, const CName& materialName )
{
	// Fill in default materials until the array is big enough.
	while ( depth >= m_chunkDepthMaterials.Size() )
	{
		m_chunkDepthMaterials.PushBack( CNAME( default ) );
	}
	m_chunkDepthMaterials[ depth ] = materialName;
}

void CApexDestructionResource::OnSerialize( IFile& file )
{
	// If we're writing out this resource, truncate the chunk material list, so we aren't saving unneeded data.
	// Don't need any adjustment for reading, since we already handle material queries for out-of-range depths.
	if ( file.IsWriter() && m_chunkDepthMaterials.Size() >= m_maxDepth )
	{
		m_chunkDepthMaterials.Resize( m_maxDepth + 1 );
	}

	TBaseClass::OnSerialize( file );


#ifndef NO_EDITOR
	if ( file.IsReader() && ! VerifyChunkMaterials() )
	{
		DATA_HALT( DES_Minor, this, TXT("Physical material"), TXT("Destructible mesh does not have physical materials set for all chunk depths. Please set it to something other than 'default'") );
	}
#endif

}
#ifndef NO_EDITOR
Bool CApexDestructionResource::VerifyChunkMaterials() const
{
	// Check that the chunk depth materials are set. Warn if any are at "default". Of course, if our array is too
	// small, then that also is bad (behaves fine, but the missing entries are "default").

	TDynArray< CName > mtlNames;
	GPhysicEngine->GetPhysicalMaterialNames( mtlNames );
	// Remove the first material, since it's the default and we want to make sure nothing uses it.
	if ( mtlNames.Size() > 0 )
	{
		mtlNames.RemoveAt( 0 );
	}

	if ( m_chunkDepthMaterials.Size() < m_maxDepth + 1 )
	{
		return false;
	}

	for ( Uint32 i = 0; i < m_chunkDepthMaterials.Size(); ++i )
	{
		const CName& mtlName = m_chunkDepthMaterials[i];
		if ( !mtlNames.Exist( mtlName ) )
			return false;
	}

	return true;

}
#endif

#ifndef NO_EDITOR
void CApexDestructionResource::FillStatistics( C2dArray* array )
{
	TBaseClass::FillStatistics( array );
	if( !m_savedAsset ) return;

	physx::apex::NxDestructibleAsset* asset = ( physx::apex::NxDestructibleAsset* ) m_savedAsset;
	physx::apex::NxDestructibleAssetStats stats;
	asset->getStats( stats );

	String name = GetFriendlyName();
	Int32 row = array->GetRowIndex( TXT("Asset"), name );

	const ::NxParameterized::Interface* params = m_savedAsset->getAssetNxParameterized();

	bool succeed = true;
	Uint32 index = 0;
	Uint32 totalFacesCount = 0;
	do 
	{
		Uint32 count = 0;
		char tmpStr[128];
		sprintf_s(tmpStr, 128, "chunkConvexHulls[%d].planeCount", index++);
		succeed = NxParameterized::getParamU32(*params, tmpStr, count);
		totalFacesCount += count;

	} while ( succeed);
	

	array->SetValue( String::Printf( TXT( "%i" ), stats.chunkCount ),  String( TXT("CollisionChunkCount") ), row );
	array->SetValue( String::Printf( TXT( "%i" ), stats.chunkBytes ),  String( TXT("CollisionChunkBytes") ), row );
	array->SetValue( String::Printf( TXT( "%i" ), stats.chunkHullDataBytes ),  String( TXT("CollisionChunkHullDataBytes") ), row );
	array->SetValue( String::Printf( TXT( "%i" ), totalFacesCount ),  String( TXT("CollisionFacesCount") ), row );
	array->SetValue( String::Printf( TXT( "%i" ), stats.renderMeshAssetStats.totalBytes),  String( TXT("RenderTotalBytes") ), row ) ;
	array->SetValue( String::Printf( TXT( "%i" ), stats.renderMeshAssetStats.submeshCount),  String( TXT("RenderSubmeshCount") ), row ) ;
	array->SetValue( String::Printf( TXT( "%i" ), stats.renderMeshAssetStats.partCount),  String( TXT("RenderPartCount") ), row ) ;
	array->SetValue( String::Printf( TXT( "%i" ), stats.renderMeshAssetStats.vertexCount),  String( TXT("RenderVertexCount") ), row ) ;
	array->SetValue( String::Printf( TXT( "%i" ), stats.renderMeshAssetStats.indexCount),  String( TXT("RenderIndexCount") ), row ) ;
}
#endif

#ifndef NO_EDITOR
void CApexDestructionResource::FillLODStatistics( C2dArray* array )
{
#ifdef USE_APEX
	TBaseClass::FillStatistics( array );
	if( !m_savedAsset ) return;

	physx::apex::NxDestructibleAsset* asset = ( physx::apex::NxDestructibleAsset* ) m_savedAsset;

	String name = GetFile()->GetFileName();
	Int32 row = array->GetRowIndex( TXT("Name"), name );

	physx::apex::NxDestructibleAssetStats stats;
	asset->getStats( stats );
	array->SetValue( String::Printf( TXT( "%i" ), stats.renderMeshAssetStats.vertexCount ),  String( TXT( "LOD0" ) ), row ) ;

#endif
}
#endif