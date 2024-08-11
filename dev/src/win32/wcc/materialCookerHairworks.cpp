/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "materialCooker.h"
#include "../../common/engine/furMeshResource.h"
#include "../../common/engine/hairworksHelpers.h"

#ifdef USE_NVIDIA_FUR

static HairWorksHelpers::DefaultLogHandler s_hairworksLogHandler;

#endif // USE_NVIDIA_FUR


Bool CMaterialCooker::InitFurCooking()
{
#ifdef USE_NVIDIA_FUR

	m_hairSDK = HairWorksHelpers::InitSDK( &s_hairworksLogHandler );
	if ( m_hairSDK == nullptr )
	{
		return false;
	}

	m_hairSDK->ClearShaderCache();

	return true;

#else
	ERR_WCC( TXT("HairWorks not enabled. Recompile with USE_NVIDIA_FUR to allow fur cooking." ) );
	return false;
#endif
}


Bool CMaterialCooker::FinishFurCooking( const MaterialCookingOptions& options )
{
#ifdef USE_NVIDIA_FUR

	RED_FATAL_ASSERT( m_hairSDK != nullptr, "HairWorks was not initialized." );

	Bool success = HairWorksHelpers::SaveShaderCache( m_hairSDK, options.m_furShaderCachePath );

	HairWorksHelpers::ShutdownSDK( m_hairSDK );

	return success;

#else
	ERR_WCC( TXT("HairWorks not enabled. Recompile with USE_NVIDIA_FUR to allow fur cooking." ) );
	return false;
#endif
}


Bool CMaterialCooker::LoadExistingFurShaders( const String& absolutePath )
{
#ifdef USE_NVIDIA_FUR

	RED_FATAL_ASSERT( m_hairSDK != nullptr, "HairWorks was not initialized." );

	return HairWorksHelpers::LoadShaderCache( m_hairSDK, absolutePath, false );

#else
	ERR_WCC( TXT("HairWorks not enabled. Recompile with USE_NVIDIA_FUR to allow fur cooking." ) );
	return false;
#endif
}


#ifdef USE_NVIDIA_FUR
static GFSDK_HAIR_RETURNCODES AddSettingsToShaderCache( GFSDK_HairSDK* hairSDK, const GFSDK_HairInstanceDescriptor& instanceDesc, const CFurMeshResource* fur )
{
	GFSDK_HAIR_RETURNCODES returnValue = GFSDK_RETURN_OK;

	GFSDK_HairShaderCacheSettings settings;
	settings.SetFromInstanceDescriptor( instanceDesc );

	if ( instanceDesc.m_useTextures )
	{
		settings.isTextureUsed[ GFSDK_HAIR_TEXTURE_DENSITY			] = fur->m_physicalMaterials.m_volume.m_densityTex.IsValid();
		settings.isTextureUsed[ GFSDK_HAIR_TEXTURE_ROOT_COLOR		] = fur->m_graphicalMaterials.m_color.m_rootColorTex.IsValid();
		settings.isTextureUsed[ GFSDK_HAIR_TEXTURE_TIP_COLOR		] = fur->m_graphicalMaterials.m_color.m_tipColorTex.IsValid();
		settings.isTextureUsed[ GFSDK_HAIR_TEXTURE_STRAND			] = fur->m_graphicalMaterials.m_color.m_strandTex.IsValid();
		settings.isTextureUsed[ GFSDK_HAIR_TEXTURE_ROOT_WIDTH		] = fur->m_physicalMaterials.m_strandWidth.m_rootWidthTex.IsValid();
		settings.isTextureUsed[ GFSDK_HAIR_TEXTURE_TIP_WIDTH		] = fur->m_physicalMaterials.m_strandWidth.m_tipWidthTex.IsValid();
		settings.isTextureUsed[ GFSDK_HAIR_TEXTURE_STIFFNESS		] = fur->m_physicalMaterials.m_stiffness.m_stiffnessTex.IsValid();
		settings.isTextureUsed[ GFSDK_HAIR_TEXTURE_ROOT_STIFFNESS	] = fur->m_physicalMaterials.m_stiffness.m_rootStiffnessTex.IsValid();
		settings.isTextureUsed[ GFSDK_HAIR_TEXTURE_CLUMP_SCALE		] = fur->m_physicalMaterials.m_clumping.m_clumpScaleTex.IsValid();
		settings.isTextureUsed[ GFSDK_HAIR_TEXTURE_CLUMP_ROUNDNESS	] = fur->m_physicalMaterials.m_clumping.m_clumpRoundnessTex.IsValid();
		settings.isTextureUsed[ GFSDK_HAIR_TEXTURE_CLUMP_NOISE		] = fur->m_physicalMaterials.m_clumping.m_clumpNoiseTex.IsValid();
		settings.isTextureUsed[ GFSDK_HAIR_TEXTURE_WAVE_SCALE		] = fur->m_physicalMaterials.m_waveness.m_waveScaleTex.IsValid();
		settings.isTextureUsed[ GFSDK_HAIR_TEXTURE_WAVE_FREQ		] = fur->m_physicalMaterials.m_waveness.m_waveFreqTex.IsValid();
		settings.isTextureUsed[ GFSDK_HAIR_TEXTURE_LENGTH			] = fur->m_physicalMaterials.m_volume.m_lengthTex.IsValid();

		for ( Uint32 i = 0; i < GFSDK_HAIR_NUM_TEXTURES; ++i )
		{
			settings.textureChannel[ i ] = instanceDesc.m_textureChannels[ i ];
		}
	}

	GFSDK_HAIR_RETURNCODES hairworksRet;

	settings.useCullSphere = false;
	hairworksRet = hairSDK->AddToShaderCache( settings );
	if ( hairworksRet != GFSDK_RETURN_OK )
	{
		ERR_WCC( TXT(" Failed to compile non-culled material") );
		returnValue = hairworksRet;
	}

	settings.useCullSphere = true;
	hairworksRet = hairSDK->AddToShaderCache( settings );
	if ( hairworksRet != GFSDK_RETURN_OK )
	{
		ERR_WCC( TXT(" Failed to compile culled material") );
		returnValue = hairworksRet;
	}

	return returnValue;
}
#endif


Bool CMaterialCooker::CompileFurShader( CFurMeshResource* fur )
{
#ifdef USE_NVIDIA_FUR

	RED_FATAL_ASSERT( m_hairSDK != nullptr, "HairWorks was not initialized." );
	RED_FATAL_ASSERT( fur != nullptr, "Can't compile shader for a null fur resource" );

	LOG_WCC( TXT("Compiling fur shaders for %ls"), fur->GetDepotPath().AsChar() );

	{
		GFSDK_HairInstanceDescriptor instDesc;
		fur->CreateDefaultHairInstanceDesc( &instDesc );

		GFSDK_HAIR_RETURNCODES hairworksRet = AddSettingsToShaderCache( m_hairSDK, instDesc, fur );
		if ( hairworksRet != GFSDK_RETURN_OK )
		{
			ERR_WCC( TXT(" Error while compiling default material") );
		}
	}

	if ( fur->IsUsingWetness() )
	{
		GFSDK_HairInstanceDescriptor instDesc;
		fur->CreateTargetHairInstanceDesc( &instDesc, 0 );

		GFSDK_HAIR_RETURNCODES hairworksRet = AddSettingsToShaderCache( m_hairSDK, instDesc, fur );
		if ( hairworksRet != GFSDK_RETURN_OK )
		{
			ERR_WCC( TXT(" Error while compiling wetness material") );
		}
	}

	return true;

#else
	ERR_WCC( TXT("HairWorks not enabled. Recompile with USE_NVIDIA_FUR to allow fur cooking." ) );
	return false;
#endif
}
