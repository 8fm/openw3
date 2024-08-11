/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "apexClothResource.h"
#include "../core/2darray.h"
#ifdef USE_APEX
#include "NxParamUtils.h"
#include "NxClothingAsset.h"
#include "NxRenderMeshAsset.h"
#endif

using namespace physx::apex;

IMPLEMENT_ENGINE_CLASS( CApexClothResource );

CApexClothResource::CApexClothResource()
{
}


CApexClothResource::~CApexClothResource()
{
}

void CApexClothResource::RestoreDefaults()
{
	char buffer[128];

	StringAnsi simBase = "simulation.";
	
	
	m_simDisableCCD = GetDefaultParameterBool( simBase + "disableCCD" );
	m_simThickness = GetDefaultParameterF32( simBase + "thickness" );
	m_simVirtualParticleDensity = GetDefaultParameterF32( simBase + "virtualParticleDensity" ) * 3.0f;

	m_materialIndex = GetDefaultParameterU32( "materialIndex" );
	String::ASprintf( buffer, "materialLibrary.materials[%i].", m_materialIndex );
	StringAnsi mtlBase = buffer;
	m_mtlBendingStiffness = GetDefaultParameterF32( mtlBase + "bendingStiffness" );
	m_mtlShearingStiffness = GetDefaultParameterF32( mtlBase + "shearingStiffness" );
	m_mtlTetherStiffness = GetDefaultParameterF32( mtlBase + "tetherStiffness" );
	m_mtlTetherLimit = GetDefaultParameterF32( mtlBase + "tetherLimit" );
	m_mtlDamping = GetDefaultParameterF32( mtlBase + "damping" );
	m_mtlDrag = GetDefaultParameterF32( mtlBase + "drag" );
	m_mtlComDamping = GetDefaultParameterBool( mtlBase + "comDamping" );
	m_mtlFriction = GetDefaultParameterF32( mtlBase + "friction" );
	m_mtlGravityScale = GetDefaultParameterF32( mtlBase + "gravityScale" );
	m_mtlInertiaScale = GetDefaultParameterF32( mtlBase + "inertiaScale" );
	m_mtlHardStretchLimitation = GetDefaultParameterF32( mtlBase + "hardStretchLimitation" );
	m_mtlMaxDistanceBias = GetDefaultParameterF32( mtlBase + "maxDistanceBias" );
	m_mtlSelfcollisionThickness = GetDefaultParameterF32( mtlBase + "selfcollisionThickness" );
	m_mtlSelfcollisionStiffness = GetDefaultParameterF32( mtlBase + "selfcollisionStiffness" );
	m_mtlMassScale = GetDefaultParameterF32( mtlBase + "massScale" );
}

void CApexClothResource::ConfigureNewAsset( NxParameterized::Interface* params ) const
{
//	Uint32 temp = GetDefaultParameterU32( "rootBoneIndex" );

	char buffer[128];

	StringAnsi simBase = "simulation.";

	SetParameterBool( params, simBase + "disableCCD", m_simDisableCCD );
	SetParameterF32( params, simBase + "thickness", m_simThickness );
	SetParameterF32( params, simBase + "virtualParticleDensity", m_simVirtualParticleDensity / 3.0f );

	String::ASprintf( buffer, "materialLibrary.materials[%i].", m_materialIndex );
	StringAnsi mtlBase = buffer;

	//base used physical materials params
	SetParameterF32( params, mtlBase + "gravityScale", m_mtlGravityScale );
	SetParameterF32( params, mtlBase + "friction", m_mtlFriction );
	SetParameterF32( params, mtlBase + "bendingStiffness", m_mtlBendingStiffness );
	SetParameterF32( params, mtlBase + "shearingStiffness", m_mtlShearingStiffness );
	SetParameterF32( params, mtlBase + "tetherLimit", m_mtlTetherLimit );  //used stretch limit
	SetParameterF32( params, mtlBase + "tetherStiffness", m_mtlTetherStiffness );  //used relax
	SetParameterF32( params, mtlBase + "damping", m_mtlDamping );
	SetParameterF32( params, mtlBase + "drag", m_mtlDrag );
	SetParameterF32( params, mtlBase + "inertiaScale", m_mtlInertiaScale );

	//then features params
	SetParameterF32( params, mtlBase + "maxDistanceBias", m_mtlMaxDistanceBias );
	SetParameterF32( params, mtlBase + "massScale", m_mtlMassScale );

	//for apex 1.3 self collision radius. should be greater than average max edge distance param
	SetParameterF32( params, mtlBase + "selfcollisionThickness", m_mtlSelfcollisionThickness );
	SetParameterF32( params, mtlBase + "selfcollisionStiffness", m_mtlSelfcollisionStiffness );

	//center of mass damping
	SetParameterBool( params, mtlBase + "comDamping", m_mtlComDamping );

	// to discuss to turn it off
	SetParameterF32( params, mtlBase + "hardStretchLimitation", m_mtlHardStretchLimitation );	
}


#ifndef NO_RESOURCE_IMPORT

const SMeshTypeResourceLODLevel& CApexClothResource::GetLODLevel( Uint32 level ) const
{
	if ( level < m_graphicalLodLevelInfo.Size() )
	{
		return m_graphicalLodLevelInfo[ level ];
	}
	return TBaseClass::GetLODLevel( level );
}

Bool CApexClothResource::UpdateLODSettings( Uint32 level, const SMeshTypeResourceLODLevel& lodSettings )
{
	if ( level >= m_graphicalLodLevelInfo.Size() )
	{
		return false;
	}

	m_graphicalLodLevelInfo[ level ] = lodSettings;
	return true;
}

Uint32 CApexClothResource::CountLODTriangles( Uint32 level ) const
{
#ifdef USE_APEX
	NxParameterized::Interface* renderMesh = nullptr;
	RED_ASSERT( ( m_defaultParameters ),TXT("m_defaultParameters doesnt exist") );
	if ( !m_defaultParameters ) return 0;

	if ( !NxParameterized::getParamRef( *m_defaultParameters, StringAnsi::Printf("graphicalLods[%d].renderMeshAsset", level).AsChar(), renderMesh ) )
	{
		return 0;
	}

	Int32 numSubmeshes = 0;
	if ( !NxParameterized::getParamArraySize( *renderMesh, "submeshes", numSubmeshes ) )
	{
		return 0;
	}

	// Assume geometry is triangle list
	Uint32 numTriangles = 0;
	for ( Int32 i = 0; i < numSubmeshes; ++i )
	{
		Int32 numIndices = 0;
		if ( NxParameterized::getParamArraySize( *renderMesh, StringAnsi::Printf("submeshes[%d].indexBuffer", i).AsChar(), numIndices ) )
		{
			numTriangles += numIndices / 3;
		}
	}

	return numTriangles;
#else
	return 0;
#endif
}

Uint32 CApexClothResource::CountLODVertices( Uint32 level ) const
{
#ifdef USE_APEX
	NxParameterized::Interface* renderMesh = nullptr;
	if ( !NxParameterized::getParamRef( *m_defaultParameters, StringAnsi::Printf("graphicalLods[%d].renderMeshAsset", level).AsChar(), renderMesh ) )
	{
		return 0;
	}

	Int32 numSubmeshes = 0;
	if ( !NxParameterized::getParamArraySize( *renderMesh, "submeshes", numSubmeshes ) )
	{
		return 0;
	}

	Uint32 numVertices = 0;
	for ( Int32 i = 0; i < numSubmeshes; ++i )
	{
		Uint32 submeshVertices = 0;
		if ( NxParameterized::getParamU32( *renderMesh, StringAnsi::Printf("submeshes[%d].vertexBuffer.vertexCount", i).AsChar(), submeshVertices ) )
		{
			numVertices += submeshVertices;
		}
	}

	return numVertices;
#else
	return 0;
#endif
}

Uint32 CApexClothResource::CountLODChunks( Uint32 level ) const
{
#ifdef USE_APEX
	Int32 numParts = 0;
	NxParameterized::getParamArraySize( *m_defaultParameters, StringAnsi::Printf("graphicalLods[%d].renderMeshAsset.partBounds", level).AsChar(), numParts );
	return numParts;
#else
	return 0;
#endif
}

Uint32 CApexClothResource::CountLODMaterials( Uint32 level ) const
{
#ifdef USE_APEX
	Int32 numMaterials = 0;
	NxParameterized::getParamArraySize( *m_defaultParameters, StringAnsi::Printf("graphicalLods[%d].renderMeshAsset.materialNames", level).AsChar(), numMaterials );
	return numMaterials;
#else
	return 0;
#endif
}


#endif

void CApexClothResource::AddRef()
{
	TBaseClass::AddRef();
	Prepare();
}

void CApexClothResource::Prepare()
{

#ifdef USE_APEX
	if( !NxGetApexSDK() )
	{
		return;
	}

	const NxParameterized::Interface* params = m_defaultParameters;

	// If we don't have cached default params, but have an asset created, grab params from there. This is the case with NO_EDITOR,
	// since in that case we null out default params instead of cloning it.
	if ( params == nullptr && m_savedAsset != nullptr )
	{
		params = m_savedAsset->getAssetNxParameterized();
	}

	// Read in bone info
	if ( !params ) return;

	// If we're reading in a resource with no graphical LOD info, we'll get it from the asset parameters. Probably an old resource.
	if ( m_graphicalLodLevelInfo.Size() == 0 )
	{
		Int32 numLods = 0;
		NxParameterized::getParamArraySize( *params, "graphicalLods", numLods );
		m_graphicalLodLevelInfo.Resize( numLods );

		for ( Int32 i = 0; i < numLods; ++i )
		{
			m_graphicalLodLevelInfo[ i ].m_distance = GetDefaultLODDistance( i );
		}
	}

	NxClothingAsset* asset = static_cast< NxClothingAsset* >( GetAsset() );
	Uint32 numBones = asset->getNumUsedBones();

	////////////////////////////////
	/////////
	/////////	TODO: uncomment RED_FATAL_ASSERT after reimporting cloth asset just as marker
	/////////
	////////////////////////////////
	//RED_FATAL_ASSERT( numBones == m_boneCount, "Different number of bones in cloth asset: '%ls'. There are [%d] bones, should be [%d]. Please reimport assets.", GetFile()->GetFileName().AsChar(), m_boneNames.Size(), numBones );

	NxParameterized::getParamU32( *params, "materialIndex", m_materialIndex );
#endif
}


Float CApexClothResource::GetAverageEdgeLength() const
{
	return GetDefaultParameterF32( "physicalMeshes[0].physicalMesh.averageEdgeLength", 0.0f );
}

void CApexClothResource::OnPreSave()
{
	TBaseClass::OnPreSave();

	// right now do presave only from editor. cuz addref has some conections to NO_EDITOR macro.
	// because this could be called from other places do nothing and modify this only from editor
	// onced removed remove also addref and ifndef
#ifndef NO_EDITOR

	// early out for cooker.
	if( GIsCooker )	return;

	/*
	// refresh bone info
	AddRef();
	NxClothingAsset* asset = static_cast< NxClothingAsset* >( GetAsset() );
	if ( asset != nullptr )
	{
		m_boneCount = asset->getNumUsedBones();
		m_boneNames.Resize( m_boneCount );
		m_boneMatrices.Resize( m_boneCount );

		for ( Uint32 i = 0; i < m_boneCount; ++i )
		{
			m_boneNames[i].Set( ANSI_TO_UNICODE( asset->getBoneName( i ) ) );

			// cloth simulations all happen in local coordinates (local to the attached bone)
			m_boneMatrices[i].SetIdentity();
		}
	}
	ReleaseRef();
	*/

	// remove self collision support
	if( m_mtlSelfcollisionStiffness > 0.f )
	{
		//RED_LOG( RED_LOG_CHANNEL(ApexClothing), TXT("Clothing with self collision! Removing self collision from asset: %ls"), GetFriendlyName().AsChar() );
		m_mtlSelfcollisionStiffness = 0.f;
		m_mtlSelfcollisionThickness = 0.f;
	}

	/*
	// create links for changing extension
	String savePath = GetFile()->GetAbsolutePath();
	String link = TXT(".link");
	savePath += link;

	String depoPath = GetFile()->GetDepotPath();
	CFilePath p( depoPath );
	String newExtension = TXT("redcloth");
	p.SetExtension( newExtension );

	GFileManager->SaveStringToFile( savePath, p.ToString() );
	*/
	/*
	CFilePath fileName( GetFile()->GetFileName() );
	CFilePath::PathString p = fileName.GetFileName();
	GetFile()->Rename( p, TXT("redcloth") );
	*/
#endif // NO_EDITOR
}

#ifndef NO_EDITOR
void CApexClothResource::FillStatistics( C2dArray* array )
{
#ifdef USE_APEX
	TBaseClass::FillStatistics( array );
	if( !m_savedAsset ) return;

	physx::apex::NxClothingAsset* asset = ( physx::apex::NxClothingAsset* ) m_savedAsset;
	Uint32 lodCount = asset->getNumGraphicalLodLevels();
	physx::apex::NxRenderMeshAssetStats statsSum;
	statsSum.totalBytes = 0;
	statsSum.submeshCount = 0;
	statsSum.partCount = 0;
	statsSum.vertexCount = 0;
	statsSum.indexCount = 0;
	statsSum.vertexBufferBytes = 0;
	statsSum.indexBufferBytes = 0;
	for( Uint32 i = 0; i != lodCount; ++i )
	{
		const NxRenderMeshAsset* renderMeshAsset = asset->getRenderMeshAsset( i );
		if( !renderMeshAsset ) continue;
		physx::apex::NxRenderMeshAssetStats stats;
		renderMeshAsset->getStats( stats );
		statsSum.totalBytes += stats.totalBytes;
		statsSum.submeshCount += stats.submeshCount;
		statsSum.partCount += stats.partCount;
		statsSum.vertexCount += stats.vertexCount;
		statsSum.indexCount += stats.indexCount;
		statsSum.vertexBufferBytes += stats.vertexBufferBytes;
		statsSum.indexBufferBytes += stats.indexBufferBytes;
	}

	String name = GetFriendlyName();
	Int32 row = array->GetRowIndex( TXT("Asset"), name );

	array->SetValue( String::Printf( TXT( "%i" ), statsSum.totalBytes),  String( TXT("RenderTotalBytes") ), row ) ;
	array->SetValue( String::Printf( TXT( "%i" ), statsSum.submeshCount),  String( TXT("RenderSubmeshCount") ), row ) ;
	array->SetValue( String::Printf( TXT( "%i" ), statsSum.partCount),  String( TXT("RenderPartCount") ), row ) ;
	array->SetValue( String::Printf( TXT( "%i" ), statsSum.vertexCount),  String( TXT("RenderVertexCount") ), row ) ;
	array->SetValue( String::Printf( TXT( "%i" ), statsSum.indexCount),  String( TXT("RenderIndexCount") ), row ) ;
#endif //USE_APEX
}

void CApexClothResource::FillLODStatistics( C2dArray* array )
{
#ifdef USE_APEX
	TBaseClass::FillStatistics( array );
	if( !m_savedAsset ) return;

	physx::apex::NxClothingAsset* asset = ( physx::apex::NxClothingAsset* ) m_savedAsset;
	Uint32 lodCount = asset->getNumGraphicalLodLevels();

	String name = GetFile()->GetFileName();
	Int32 row = array->GetRowIndex( TXT("Name"), name );

	for( Uint32 i = 0; i <= lodCount; ++i )
	{
		const NxRenderMeshAsset* renderMeshAsset = asset->getRenderMeshAsset( i );
		if( !renderMeshAsset ) continue;
		physx::apex::NxRenderMeshAssetStats stats;
		renderMeshAsset->getStats( stats );
		array->SetValue( String::Printf( TXT( "%i" ), stats.vertexCount ),  String::Printf( TXT( "LOD%i" ), i ), row ) ;
	}
#endif //USE_APEX
}

#endif //NO_EDITOR