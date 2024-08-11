/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "../../common/core/exporter.h"
#include "../../common/renderer/build.h"
#include "../../common/engine/bitmapTexture.h"

#ifdef USE_NVIDIA_FUR

#include "../../../external/NvidiaHair/include/GFSDK_HairWorks.h"

#endif // USE_NVIDIA_FUR

#pragma warning( push )
#pragma warning( disable:4530 ) // C++ exception handler used, but unwind semantics are not enabled. Specify /EHsc
#include <vector>
#include "../../common/engine/furMeshResource.h"
#pragma warning( pop )

class CHairWorksExporter : public IExporter
{
	DECLARE_ENGINE_CLASS( CHairWorksExporter, IExporter, 0 );

public:
	CHairWorksExporter();
	virtual Bool DoExport( const ExportOptions& options );

	static GpuApi::eTextureSaveFormat FileExtensionToSaveFormat( const String& extension );
};

DEFINE_SIMPLE_RTTI_CLASS(CHairWorksExporter,IExporter);
IMPLEMENT_ENGINE_CLASS(CHairWorksExporter);

namespace {
#ifdef USE_NVIDIA_FUR
	void CopyHairTextureName( char** textureNames, int idx, THandle< CBitmapTexture >& texture )
	{
		if ( texture )
		{
			const CBitmapTexture* tex = texture.Get();
			if ( tex && tex->GetFile() )
			{
				if ( textureNames[ idx ] )
				{
					strcpy( textureNames[ idx ], UNICODE_TO_ANSI( tex->GetFile()->GetDepotPath().AsChar() ) );
				}
			}
		}
		else
		{
			*textureNames[ idx ] = '\0';
		}
	}
#endif
}

CHairWorksExporter::CHairWorksExporter()
{
	// Supported class
	m_resourceClass = ClassID< CFurMeshResource >();

	// Supported formats
	m_formats.PushBack( CFileFormat( TXT("apx"), TXT("NVIDIA HairWorks APX") ) );
}

bool CHairWorksExporter::DoExport( const ExportOptions& options )
{
#ifdef USE_NVIDIA_FUR
	ASSERT( options.m_resource->IsA( ClassID<CFurMeshResource>() ));

	CFurMeshResource* furMeshResource = Cast< CFurMeshResource >( options.m_resource );

	GFSDK_HairSDK* hairSDK = GetRenderer()->GetHairSDK();
	if ( !hairSDK )
	{
		RED_LOG_ERROR( CNAME( CRenderProxy_Fur ), TXT( "HairWorks libray not initialized" ) );
		return false;
	}

	// Create hair asset descriptor
	GFSDK_HairAssetDescriptor m_hairDesc;
	furMeshResource->CreateHairAssetDesc( &m_hairDesc );

	// Create hair asset from descriptor
	GFSDK_HairAssetID assetID;
	GFSDK_HAIR_RETURNCODES loadResult = hairSDK->CreateHairAsset( m_hairDesc, &assetID );
	RED_ASSERT( loadResult == GFSDK_RETURN_OK, TXT("Fur creation failed") );
			
	// Create hair instance from asset
	ID3D11DeviceContext* ctxD3D = GpuApi::Hacks::GetDeviceContext();
	GFSDK_HairInstanceID instID;
	GFSDK_HAIR_RETURNCODES createResult = hairSDK->CreateHairInstance( (GFSDK_HairAssetID)assetID, &instID);
	RED_ASSERT( createResult == GFSDK_RETURN_OK, TXT("Fur instance creation failed") );

	// Create hair instance descriptor
	GFSDK_HairInstanceDescriptor defaultInstanceDesc, targetInstanceDesc;

	furMeshResource->CreateDefaultHairInstanceDesc( &defaultInstanceDesc );
	furMeshResource->CreateTargetHairInstanceDesc( &targetInstanceDesc, 0);

	// Update instance using instance descriptor
	GFSDK_HAIR_RETURNCODES updateResult = GFSDK_RETURN_OK;
	updateResult |= hairSDK->UpdateInstanceDescriptor( instID, defaultInstanceDesc, 0 );
	updateResult |= hairSDK->UpdateInstanceDescriptor( instID, targetInstanceDesc, 1 );

	RED_ASSERT( updateResult == GFSDK_RETURN_OK, TXT("Fur instance update failed") );

	GFSDK_HairWorksInfo info;
	gfsdk_char* nameBuffer = new gfsdk_char[ GFSDK_HAIR_MAX_STRING * GFSDK_HAIR_NUM_TEXTURES ];
	char* textureNames[GFSDK_HAIR_NUM_TEXTURES];
	for (int i = 0; i < GFSDK_HAIR_NUM_TEXTURES; i++)
	{
		textureNames[i] = nameBuffer + i * GFSDK_HAIR_MAX_STRING;
	}
	
	CopyHairTextureName( textureNames, GFSDK_HAIR_TEXTURE_DENSITY, furMeshResource->m_physicalMaterials.m_volume.m_densityTex );
	CopyHairTextureName( textureNames, GFSDK_HAIR_TEXTURE_ROOT_COLOR, furMeshResource->m_graphicalMaterials.m_color.m_rootColorTex );
	CopyHairTextureName( textureNames, GFSDK_HAIR_TEXTURE_TIP_COLOR, furMeshResource->m_graphicalMaterials.m_color.m_tipColorTex );
	CopyHairTextureName( textureNames, GFSDK_HAIR_TEXTURE_ROOT_WIDTH, furMeshResource->m_physicalMaterials.m_strandWidth.m_rootWidthTex );
	CopyHairTextureName( textureNames, GFSDK_HAIR_TEXTURE_TIP_WIDTH, furMeshResource->m_physicalMaterials.m_strandWidth.m_tipWidthTex );
	CopyHairTextureName( textureNames, GFSDK_HAIR_TEXTURE_STIFFNESS, furMeshResource->m_physicalMaterials.m_stiffness.m_stiffnessTex );
	CopyHairTextureName( textureNames, GFSDK_HAIR_TEXTURE_ROOT_STIFFNESS, furMeshResource->m_physicalMaterials.m_stiffness.m_rootStiffnessTex );
	CopyHairTextureName( textureNames, GFSDK_HAIR_TEXTURE_CLUMP_SCALE, furMeshResource->m_physicalMaterials.m_clumping.m_clumpScaleTex );
	CopyHairTextureName( textureNames, GFSDK_HAIR_TEXTURE_CLUMP_ROUNDNESS, furMeshResource->m_physicalMaterials.m_clumping.m_clumpRoundnessTex );
	CopyHairTextureName( textureNames, GFSDK_HAIR_TEXTURE_CLUMP_NOISE, furMeshResource->m_physicalMaterials.m_clumping.m_clumpNoiseTex );
	CopyHairTextureName( textureNames, GFSDK_HAIR_TEXTURE_WAVE_SCALE, furMeshResource->m_physicalMaterials.m_waveness.m_waveScaleTex );
	CopyHairTextureName( textureNames, GFSDK_HAIR_TEXTURE_WAVE_FREQ, furMeshResource->m_physicalMaterials.m_waveness.m_waveFreqTex );
	CopyHairTextureName( textureNames, GFSDK_HAIR_TEXTURE_STRAND, furMeshResource->m_graphicalMaterials.m_color.m_strandTex );
	CopyHairTextureName( textureNames, GFSDK_HAIR_TEXTURE_LENGTH, furMeshResource->m_physicalMaterials.m_volume.m_lengthTex );
	CopyHairTextureName( textureNames, GFSDK_HAIR_TEXTURE_SPECULAR, furMeshResource->m_graphicalMaterials.m_specular.m_specularTex );

	GFSDK_HAIR_RETURNCODES saveResult = hairSDK->SaveHairInstanceToFile( UNICODE_TO_ANSI( options.m_saveFilePath.AsChar() ), 
		instID, &info, textureNames );
	RED_ASSERT( saveResult == GFSDK_RETURN_OK, TXT("Fur save failed") );

	delete nameBuffer;

	return saveResult == GFSDK_RETURN_OK;
#else
	return false;
#endif
}
