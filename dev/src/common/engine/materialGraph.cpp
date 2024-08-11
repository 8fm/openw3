/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "../engine/mbParamTexture.h"
#include "../engine/mbParamTextureArray.h"
#include "../engine/mbParamCube.h"
#include "../engine/mbParamColor.h"
#include "../engine/mbParamVector.h"
#include "../engine/mbParamScalar.h"
#include "../matcompiler/mbLighitngBase.h"
#include "../matcompiler/mbOutputColor.h"
#include "../matcompiler/mbOutputColorEyeOverlay.h"	
#include "../matcompiler/mbOutputColorDeferred.h"	
#include "../matcompiler/mbOutputColorHair.h"	
#include "../matcompiler/mbOutputColorEye.h"	
#include "../matcompiler/mbOutputColorEnhanced.h"	
#include "../matcompiler/mbOutputColorSkin.h"	
#include "../matcompiler/mbOutputVolume.h"
#include "../matcompiler/mbOutputVertexModifiers.h"
#include "../matcompiler/mbTerrainMaterialBlending.h"
#include "descriptionGraphBlock.h"
#include "materialGraph.h"
#include "materialRootDecalBlock.h"
#include "materialBlockCompiler.h"
#include "../core/events.h"
#include "renderFrame.h"
#include "renderFragment.h"

IMPLEMENT_ENGINE_CLASS( CMaterialGraph );

void CMaterialGraph::OnSerialize( IFile& file )
{
	TBaseClass::OnSerialize( file );

	// force the material data to be recompiled if it's in the old version
	if ( file.IsReader() && file.GetVersion() < VER_RECOMPILE_MATERIAL_PARAMETERS )
	{
		m_pixelParamBlockSize = 0;
		m_vertexParamBlockSize = 0;
		m_pixelParameters.Clear();
		m_vertexParameters.Clear();
		m_pixelParameterBlocks.Clear();
		m_vertexParameterBlocks.Clear();
	}

	if ( file.GetVersion() < VER_MATERIALS_CRCS )
	{
		if ( file.IsWriter() )
		{
			Uint32 size = 0;
			file << size;
		}
	}
	else
	{
		if ( !file.IsGarbageCollector() )
		{
			if ( file.IsReader() )
			{
				Uint32 crcMapSize;
				file << crcMapSize;
				Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_crcMapMutex );
				m_crcMap.Reserve( crcMapSize );
				for ( Uint32 i = 0; i < crcMapSize; ++i )
				{
					Uint32 contextId;
					Uint64 vsCRC;
					Uint64 psCRC;
					file << contextId;
					file << vsCRC;
					file << psCRC;
					m_crcMap.Insert( contextId, TPair< Uint64, Uint64 >( vsCRC, psCRC ) );
				}
			}
			else if ( file.IsWriter() )
			{
				Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_crcMapMutex );
				Uint32 size = m_crcMap.Size();
				file << size;
				for ( auto it = m_crcMap.Begin(); it != m_crcMap.End(); ++it )
				{
					Uint32 contextId = it.Key();
					file << contextId;
					file << it.Value().m_first;
					file << it.Value().m_second;
				}
			}
		}
	}
}

#ifndef NO_RESOURCE_COOKING
void CMaterialGraph::OnCook( class ICookerFramework& cooker )
{
#ifndef NO_RUNTIME_MATERIAL_COMPILATION
	UpdateParametersList();
	UpdateRenderingParams();
#endif //NO_RUNTIME_MATERIAL_COMPILATION

	TBaseClass::OnCook( cooker );

	CleanupSourceData();
#ifndef NO_EDITOR_GRAPH_SUPPORT
	// Remove connections from all blocks
	for ( auto block : m_blocks )
	{
		if ( block && block->IsA< CMaterialBlock >() )
		{
			block->RemoveAllSockets();
		}
	}
#endif

	// Remove blocks
	m_blocks.Clear();
	m_crcMap.Clear();
}
#endif


Bool CMaterialGraph::OnPropertyTypeMismatch( CName propertyName, IProperty* existingProperty, const CVariant& readValue )
{
	if ( propertyName == TXT("blocks") )
	{
		String type = readValue.GetType().AsString();
		m_blocks = *static_cast< const TDynArray< CGraphBlock* > * >( readValue.GetData() );
		return true;
	}

	return TBaseClass::OnPropertyTypeMismatch( propertyName, existingProperty, readValue );
}

#if !defined( NO_RUNTIME_MATERIAL_COMPILATION )
void CMaterialGraph::OnPostLoad()
{
	// offsets are now 32/64 bit compatible, recompute only if needed
	const Bool shouldRebuildParameters = (m_pixelParamBlockSize == 0);
	if ( shouldRebuildParameters )
	{
		UpdateParametersList();
	}
}

void CMaterialGraph::UpdateParametersList()
{
	// Clear list of parameters
	m_pixelParamBlockSize = 0;
	m_vertexParamBlockSize = 0;
	m_pixelParameters.Clear();
	m_vertexParameters.Clear();
	m_pixelParameterBlocks.Clear();
	m_vertexParameterBlocks.Clear();

	// Scan for artist variables
	for ( auto block : m_blocks )
	{
		// Get artist variable block
		CMaterialParameter* param = Cast< CMaterialParameter >( block );
		if ( param && param->GetParameterName() )
		{
			CMaterialGraph::EParameterType paramType = PT_None;
			Uint32 paramDataSize = 0;

			// Add to parameter list
			if ( param->GetShaderTarget() & MSH_PixelShader )
			{
				m_pixelParameterBlocks.PushBack( param );
			}
			if ( param->GetShaderTarget() & MSH_VertexShader )
			{
				m_vertexParameterBlocks.PushBack( param );
			}

			// Allocate texture parameter
			if ( param->IsA< CMaterialParameterTexture >() )
			{
				paramDataSize = sizeof( TUniPointer< void > ); // this is 32/64 bit compatible
				paramType = PT_Texture;
			}

			// Allocate texture parameter
			if ( param->IsA< CMaterialParameterTextureArray >() )
			{
				paramDataSize = sizeof( TUniPointer< void > ); // this is 32/64 bit compatible
				paramType = PT_TextureArray;
			}

			// Allocate cube parameter
			if ( param->IsA< CMaterialParameterCube >() )
			{
				paramDataSize = sizeof( TUniPointer< void > ); // this is 32/64 bit compatible
				paramType = PT_Cube;
			}

			// Allocate color parameter
			if ( param->IsA< CMaterialParameterColor >() )
			{
				paramDataSize = sizeof( Color );
				paramType = PT_Color;
			}

			// Allocate vector parameter
			if ( param->IsA< CMaterialParameterVector >() )
			{
				paramDataSize = sizeof( Vector );
				paramType = PT_Vector;
			}

			// Allocate scalar parameter
			if ( param->IsA< CMaterialParameterScalar >() )
			{
				paramDataSize = sizeof( Float );
				paramType = PT_Scalar;				
			}

			// Map parameter
			if ( param->GetShaderTarget() & MSH_PixelShader )
			{
				const Uint32 paramOffset = m_pixelParamBlockSize;
				m_pixelParamBlockSize += paramDataSize;

				ASSERT( paramOffset < 255 );

				m_pixelParameters.PushBack( Parameter( paramType, param->GetParameterName(), (Uint8)paramOffset ) );
			}

			if ( param->GetShaderTarget() & MSH_VertexShader )
			{
				const Uint32 paramOffset = m_vertexParamBlockSize;
				m_vertexParamBlockSize += paramDataSize;

				ASSERT( paramOffset < 255 );

				m_vertexParameters.PushBack( Parameter( paramType, param->GetParameterName(), (Uint8)paramOffset ) );
			}
		}
	}
}

void CMaterialGraph::UpdateRenderingParams()
{
	Bool isDecal = false;
	ERenderingSortGroup decalSortGroup = RSG_DecalModulativeColor;
	Bool isDeferredLighten = false;	
	Bool isEmissive = false;
	Bool isNoDepthWrite = false;
	Bool isHair = false;
	Bool isForward = false;
	Bool isEyeOverlay = false;
	Bool isSkin = false;
	ERenderingBlendMode blendMode = RBM_None;
	Bool isAccumulativelyRefracted = false;
	Float lightingFallbackThreshold = -1.f;
	Float lightingFallbackBlendRange = 0.f;
	m_isForward = false;
	m_isTwoSided = false;
	m_isEmissive = false;
	m_isMasked = false;
	m_canOverrideMasked = true;
	m_isAccumulativelyRefracted = false;
	m_isReflectiveMasked = false;
	m_isEye = false;
	m_paramMask = 0;
	m_isVolumeRendering = false;	
	m_isWaterBlended = false;

	for ( auto block : m_blocks )
	{
		// Lighting model block
		CMaterialBlock* param = Cast< CMaterialBlock >( block );
		if ( !param )
			continue;
		
		// Decal block
		if ( param->IsA< CMaterialRootDecalBlock >() )
		{			
			CMaterialRootDecalBlock* block = SafeCast< CMaterialRootDecalBlock >( param );			
			decalSortGroup = block->GetSortGroup();
			isDecal = true;
		}

		// Deferred lighting block
		if ( param->IsA< CMaterialBlockOutputColorDeferred >() )
		{			
			isDeferredLighten = true;			
		}

		if ( param->IsA< CMaterialBlockOutputColorSkin >() )
		{
			isSkin = true;
		}

		if ( param->IsA< CMaterialBlockOutputColorHair >() )
		{
			isHair = true;
		}

		if ( param->IsA< CMaterialBlockOutputColorEye >() )
		{
			isForward = true;
			m_isEye = true;
		}

		if ( param->IsA< CMaterialBlockOutputColorEnhanced >() )
		{
			isForward = true;
		}

		if ( param->IsA< CMaterialBlockOutputColorEyeOverlay >() )
		{
			isEyeOverlay = true;
		}

		// Output color
		if ( param->IsA< CMaterialBlockOutputColor >() )
		{
			CMaterialBlockOutputColor* block = SafeCast< CMaterialBlockOutputColor >( param );			
			isNoDepthWrite = block->m_noDepthWrite;
			m_isReflectiveMasked = block->IsReflectiveMasked();			
		}

		// Volume output block
		if ( param->IsA< CMaterialBlockOutputVolume >() )
		{			
			m_isVolumeRendering = true;

			CMaterialBlockOutputVolume* block = SafeCast< CMaterialBlockOutputVolume >( param );			
			m_isWaterBlended = block->IsWaterBlended();
		}

		// Root block
		if ( param->IsA< CMaterialRootBlock >() && param->GetShaderTarget() == MSH_PixelShader )
		{
			CMaterialRootBlock* block = SafeCast< CMaterialRootBlock >( param );
			isEmissive					= block->IsEmissive();
			m_isEmissive				= block->IsEmissive();
			m_isMasked					= block->IsMasked();
			m_canOverrideMasked			= block->CanOverrideMasked();
			blendMode					= block->GetBlendMode();
			isAccumulativelyRefracted	= block->IsAccumulativelyRefracted();
			m_isTwoSided				= block->IsTwoSided();
			m_isMimicMaterial			= block->IsMimicMaterial();
		}

		// Engine value
		m_paramMask |= param->CalcRenderingFragmentParamMask();
	}
	
	// Assume unlit for now
	m_sortGroup = RSG_Unlit;
	m_blendMode = RBM_None;
	
	// rendering as a volume proxy only
	if( m_isVolumeRendering )
	{
		if( m_isWaterBlended ) m_sortGroup = RSG_WaterBlend;
		else
			m_sortGroup = RSG_Volumes;
	}
	else
	{
		if ( isHair )
		{
			m_sortGroup = RSG_Hair;
		}

		if ( isForward )
		{
			m_sortGroup = RSG_Forward;
		}

		if ( isSkin )
		{
			m_sortGroup = RSG_Skin;
		}

		if ( isEyeOverlay )
		{
			m_sortGroup = RSG_EyeOverlay;
		}

		if ( isDecal )
		{
			// Use decal sort group
			m_sortGroup = decalSortGroup;
		}
		else if ( isDeferredLighten )
		{
			// Use deferred rendering

			if ( isEmissive )
			{
				m_sortGroup = RSG_LitOpaqueWithEmissive;
			}
			else
			{
				m_sortGroup = RSG_LitOpaque;
			}
		}
		else if ( RBM_None != blendMode )
		{
			if ( RBM_Refractive == blendMode )
			{
				// Use refractive background
				if ( isNoDepthWrite )
				{
					m_sortGroup = RSG_RefractiveBackground;
				}
				else
				{
					m_sortGroup = RSG_RefractiveBackgroundDepthWrite;
				}

				m_blendMode = RBM_None;
			}
			else
			{
				// Use transparency rendering
				m_sortGroup					= m_isReflectiveMasked ? RSG_TransparentBackground : RSG_Transparent;
				m_blendMode					= blendMode;
				m_isAccumulativelyRefracted = isAccumulativelyRefracted;
			}
		}
	}
}

void CMaterialGraph::Compile( IMaterialCompiler* compiler ) const
{
	// Compile
	CMaterialBlockCompiler blockCompiler( (CMaterialGraph*)this, compiler );
	blockCompiler.CompileRoots();
}

void CMaterialGraph::CacheCRCs( IMaterialCompiler* compiler )
{
	Uint64 vsCRC = compiler->GetVSCodeCRC();
	Uint64 psCRC = compiler->GetPSCodeCRC();

	Uint32 contextId = compiler->GetContext().CalcID();

	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_crcMapMutex );
	if ( m_crcMap.KeyExist( contextId ) )
	{
		m_crcMap[ contextId	].m_first = vsCRC;
		m_crcMap[ contextId	].m_second = psCRC;
	}
	else
	{
		TPair< Uint64, Uint64 > crcs( vsCRC, psCRC );
		m_crcMap.Insert( contextId, crcs );
	}
}

Bool CMaterialGraph::SupportsContext( const MaterialRenderingContext& context ) const
{
	if ( IsCooked() )
	{
		WARN_ENGINE( TXT("Material '%ls' is COOKED - we don't have the source blocks to compile it from!"), GetDepotPath().AsChar() );
		return false;
	}

	if ( !TBaseClass::SupportsContext( context ) )
	{
		return false;
	}

	if ( CompileAllTechniques() )
	{
		return true;
	}

	CMaterialRootBlock* rootBlock = GetRootBaseBlock();
	RED_FATAL_ASSERT( rootBlock, "Material '%ls' has no RootBlock.", GetDepotPath().AsChar() );

	TDynArray< ERenderingPass > validPasses;
	rootBlock->GetValidPasses( validPasses );
	if ( !validPasses.FindPtr( context.m_renderingContext->m_pass ) )
	{
		WARN_ENGINE( TXT("Material '%ls', OutputBlock does not support proper RenderPass: %u"), GetDepotPath().AsChar(), context.m_renderingContext->m_pass );
		return false;
	}

	// check vertex factory
	switch ( context.m_vertexFactory )
	{
	case MVF_MeshStatic:
	case MVF_MeshSkinned:
		if ( !CanUseOnMeshes() && !CanUseOnCollapsableObjects() )
		{
			WARN_ENGINE( TXT("Material '%ls' cannot be used on meshes OR collapsable objects"), GetDepotPath().AsChar() );
			return false;
		}
		break;
	case MVF_MeshDestruction:
		if ( !CanUseOnDestructionMeshes() )
		{
			WARN_ENGINE( TXT("Material '%ls' cannot be used on destruction meshes"), GetDepotPath().AsChar() );
			return false;
		}
		break;
	case MVF_ApexWithBones:
	case MVF_ApexWithoutBones:
		if ( !CanUseOnApexMeshes() )
		{
			WARN_ENGINE( TXT("Material '%ls' cannot be used on APEX objects"), GetDepotPath().AsChar() );
			return false;
		}
		break;

	case MVF_ParticleBilboard:
	case MVF_ParticleBilboardRain:
	case MVF_ParticleParallel:
	case MVF_ParticleMotionBlur:
	case MVF_ParticleSphereAligned:
	case MVF_ParticleVerticalFixed:
	case MVF_ParticleTrail:
	case MVF_ParticleFacingTrail:
	case MVF_ParticleBeam:
	case MVF_ParticleScreen:
		if ( rootBlock->IsDeferred() || !CanUseOnParticles() )
		{
			WARN_ENGINE( TXT("Material '%ls' used in Deferred pass OR cannot be used on particles"), GetDepotPath().AsChar() );
			return false;
		}
		break;
	}

	// check skinning
	switch ( context.m_vertexFactory )
	{
	case MVF_MeshSkinned:
	case MVF_ApexWithBones:
	case MVF_MeshDestruction:
		if ( !CanUseSkinning() )
		{
			WARN_ENGINE( TXT("Material '%ls' does not support skinning"), GetDepotPath().AsChar() );
			return false;
		}
		if ( context.m_useInstancing && !CanUseSkinnedInstancing() )
		{
			WARN_ENGINE( TXT("Material '%ls' does not support skinned instancing"), GetDepotPath().AsChar() );
			return false;
		}
	}

	// check other properties
	if ( ( context.m_discardingPass && context.m_uvDissolveSeparateUV ) && !CanUseOnMorphMeshes() )
	{
		WARN_ENGINE( TXT("Material '%ls' cannot be used on Morphed meshes"), GetDepotPath().AsChar() );
		return false;
	}
	if ( context.m_hasVertexCollapse && !CanUseOnCollapsableObjects() )
	{
		WARN_ENGINE( TXT("Material '%ls' cannot be used on Collapsable meshes"), GetDepotPath().AsChar() );
		return false;
	}
	if ( context.m_pregeneratedMaps && !IsTerrainMaterial() )
	{
		WARN_ENGINE( TXT("Material '%ls' is not a terrain material (pregenerated maps)"), GetDepotPath().AsChar() );
		return false;
	}
	if ( context.m_lowQuality && !IsTerrainMaterial() )
	{
		WARN_ENGINE( TXT("Material '%ls' is not a terrain material (low quality)"), GetDepotPath().AsChar() );
		return false;
	}

	return true;
}

CMaterialRootBlock* CMaterialGraph::GetRootBaseBlock() const
{
	for ( auto block : GraphGetBlocks() )
	{
		CMaterialRootBlock* rootBlock = Cast< CMaterialRootBlock >( block );
		if ( rootBlock )
		{
			if ( rootBlock->IsA< CMaterialBlockOutputVertexModifiers >() )
			{
				continue;
			}
			return rootBlock;
		}
	}

	// No root block ?
	return nullptr;
}

Bool CMaterialGraph::IsTerrainMaterial() const
{
	for ( auto graphBlock : GraphGetBlocks() )
	{
		CMaterialBlock* block = Cast< CMaterialBlock >( graphBlock );
		if ( block && block->IsA< CMaterialTerrainMaterialBlending >() )
		{
			return true;
		}
	}

	// No terrain block, assume no terrain material
	return false;
}

#endif	//NO_RUNTIME_MATERIAL_COMPILATION


void CMaterialGraph::GetAllParameterNames( TDynArray< CName >& outNames ) const
{
	// PushBackUnique is maybe not ideal, but we're probably not dealing with large numbers of parameters.
	for ( auto param : m_pixelParameterBlocks )
	{
		outNames.PushBackUnique( param->GetParameterName() );
	}
	for ( auto param : m_vertexParameterBlocks )
	{
		outNames.PushBackUnique( param->GetParameterName() );
	}
}


Bool CMaterialGraph::WriteParameterRaw( const CName& name, const void* data, Bool /*recreateResource*/ /*= true*/ )
{
	Bool result = false;

	// Find parameter
	for ( Uint32 i=0; i<m_pixelParameterBlocks.Size(); i++ )
	{
		CMaterialParameter* param = SafeCast< CMaterialParameter >( m_pixelParameterBlocks[i] );
		if ( name == param->GetParameterName() )
		{
			// Copy value
			IProperty* prop = param->GetParameterProperty();
			void* propData = prop->GetOffsetPtr( param );
			prop->GetType()->Copy( propData, data );
			result = true;
		}
	}

	for ( Uint32 i=0; i<m_vertexParameterBlocks.Size(); i++ )
	{
		CMaterialParameter* param = SafeCast< CMaterialParameter >( m_vertexParameterBlocks[i] );
		if ( name == param->GetParameterName() )
		{
			// Copy value
			IProperty* prop = param->GetParameterProperty();
			void* propData = prop->GetOffsetPtr( param );
			prop->GetType()->Copy( propData, data );
			result = true;
		}
	}

	// Not found
	return result;
}

Bool CMaterialGraph::ReadParameterRaw( const CName& name, void* data ) const
{
	Bool result = false;

	// Find parameter
	for ( Uint32 i=0; i<m_pixelParameterBlocks.Size(); i++ )
	{
		CMaterialParameter* param = SafeCast< CMaterialParameter >( m_pixelParameterBlocks[i] );
		if ( name == param->GetParameterName() )
		{
			IProperty* prop = param->GetParameterProperty();
			void* propData = prop->GetOffsetPtr( param );
			prop->GetType()->Copy( data, propData );
			result = true;
		}
	}

	for ( Uint32 i=0; i<m_vertexParameterBlocks.Size(); i++ )
	{
		CMaterialParameter* param = SafeCast< CMaterialParameter >( m_vertexParameterBlocks[i] );
		if ( name == param->GetParameterName() )
		{
			IProperty* prop = param->GetParameterProperty();
			void* propData = prop->GetOffsetPtr( param );
			prop->GetType()->Copy( data, propData );
			result = true;
		}
	}

	// Not found
	return result;
}

CMaterialParameter* CMaterialGraph::FindParameter( const CName& name, Bool caseSensitive /* = true */ )
{
    // Find parameter
    if ( caseSensitive )
	{
	    for ( Uint32 i=0; i<m_pixelParameterBlocks.Size(); i++ )
	    {
		    CMaterialParameter* param = SafeCast< CMaterialParameter >( m_pixelParameterBlocks[i] );
            if ( name == param->GetParameterName() )
		    {
			    return param;
		    }
	    }

		for ( Uint32 i=0; i<m_vertexParameterBlocks.Size(); i++ )
		{
			CMaterialParameter* param = SafeCast< CMaterialParameter >( m_vertexParameterBlocks[i] );
			if ( name == param->GetParameterName() )
			{
				return param;
			}
		}
	}
    else
    {
RED_MESSAGE(  "CMaterialGraph::FindParameter -> Can we remove the need for a case insensitive search?" )
        String sName = name.AsString();

        for ( Uint32 i=0; i<m_pixelParameterBlocks.Size(); i++ )
	    {
		    CMaterialParameter* param = SafeCast< CMaterialParameter >( m_pixelParameterBlocks[i] );
            if ( param->GetParameterName().AsString().EqualsNC( sName ) )
		    {
			    return param;
		    }
	    }

		for ( Uint32 i=0; i<m_vertexParameterBlocks.Size(); i++ )
		{
			CMaterialParameter* param = SafeCast< CMaterialParameter >( m_vertexParameterBlocks[i] );
			if ( param->GetParameterName().AsString().EqualsNC( sName ) )
			{
				return param;
			}
		}
    }

	// Not found
	return NULL;
}

CObject *CMaterialGraph::GraphGetOwner()
{
	// Material blocks are instanced here
	return this;
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CMaterialGraph::GraphStructureModified()
{
#ifndef NO_RUNTIME_MATERIAL_COMPILATION
	UpdateParametersList();
	UpdateRenderingParams();
#else
	GPUAPI_LOG_WARNING( TXT( "Material compilation requested without compilation code built in" ) );
#endif
}

Bool CMaterialGraph::GraphSupportsBlockClass( CClass *blockClass ) const
{
	ASSERT( blockClass );
	return !blockClass->IsAbstract() 
		&& ( blockClass->IsBasedOn( ClassID< CMaterialBlock >() ) || blockClass->IsBasedOn( ClassID< CGraphHelperBlock >() ) );
}

Vector CMaterialGraph::GraphGetBackgroundOffset() const
{
	return m_offset;
}

void CMaterialGraph::GraphSetBackgroundOffset( const Vector& offset )
{
	m_offset = offset;
}

#endif // !NO_EDITOR_GRAPH_SUPPORT

#ifndef NO_FILE_SOURCE_CONTROL_SUPPORT

Bool CMaterialGraph::Reload(Bool confirm)
{
	if ( confirm )
	{
		EDITOR_QUEUE_EVENT( CNAME( FileReloadConfirm ), CreateEventData( this ) );
	}
	else
	{
		CResource* res = this;
		TBaseClass::Reload( confirm );
	}
	return true;
}

#endif

Bool CMaterialGraph::TryGetCRC( Uint32 contextId, Uint64& vsCRC, Uint64& psCRC ) const
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_crcMapMutex );
	TPair< Uint64, Uint64 > crcs;
	if ( m_crcMap.Find( contextId, crcs ) )
	{
		vsCRC = crcs.m_first;
		psCRC = crcs.m_second;
		return true;
	}
	return false;
}
