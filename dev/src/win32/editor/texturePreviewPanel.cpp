/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "texturePreviewPanel.h"
#include "editorExternalResources.h"

#include "../../common/core/depot.h"
#include "../../common/engine/materialDefinition.h"
#include "../../common/engine/materialGraph.h"
#include "../../common/engine/materialInstance.h"
#include "../../common/engine/bitmapTexture.h"
#include "../../common/engine/cubeTexture.h"

CEdTexturePreviewPanel::CEdTexturePreviewPanel( wxWindow* parent, CResource* texture, Float lodBias )
	: CEdMaterialPreviewPanel( parent )
	, m_mipLevel( 0 )
{
	m_diffuseMaterial		= LoadResource< IMaterial >( DIFFUSEMAP_MATERIAL );
	m_diffuseCubeMaterial	= LoadResource< IMaterial >( DIFFUSECUBEMAP_MATERIAL );
	m_normalMaterial		= LoadResource< IMaterial >( NORMALMAP_MATERIAL );

	if ( texture->IsA<CBitmapTexture>() )
	{
		SetShape( CEdMaterialPreviewPanel::SHAPE_Sphere );
	}
	else if ( texture->IsA<CCubeTexture>() )
	{
		SetShape( CEdMaterialPreviewPanel::SHAPE_Box );
	}

	SetTexture( texture, lodBias );
}

void CEdTexturePreviewPanel::SetTexture( CResource* texture, Float lodBias )
{
	if ( !texture )
	{
		return;
	}

	IMaterial* baseMaterial = nullptr;
	CBitmapTexture* tex2D = Cast< CBitmapTexture >( texture );
	CCubeTexture* texCube = Cast< CCubeTexture >( texture );

	if ( tex2D )
	{
		const CName& textureGroup = tex2D->GetTextureGroup().m_groupName;
		if   ( textureGroup == CName( TXT("CharacterNormal") ) || textureGroup == CName( TXT("CharacterNormalHQ") )
			|| textureGroup == CName( TXT("DetailNormalMap") ) || textureGroup == CName( TXT("MimicDecalsNormal") )
			|| textureGroup == CName( TXT("NormalmapGloss") )  || textureGroup == CName( TXT("SpecialQuestNormal") )
			|| textureGroup == CName( TXT("SpeedTreeNormal") ) || textureGroup == CName( TXT("SpeedTreeNormalDetail") )
			|| textureGroup == CName( TXT("TerrainNormal") )   || textureGroup == CName( TXT("WorldNormal") )
			|| textureGroup == CName( TXT("WorldNormalHQ") ) )
		{
			baseMaterial = m_normalMaterial;
		}
		else
		{
			baseMaterial = m_diffuseMaterial;
		}
	}
	else if ( texCube )
	{
		baseMaterial = m_diffuseCubeMaterial;
	}
	else
	{
		RED_HALT( "texture type not supported, only cubemaps and 2D texture are" );
		baseMaterial = m_diffuseMaterial;
	}


	IMaterial* currentMaterial = GetMaterial();

	CMaterialInstance* instance = nullptr;
	// If we're using the same base material, we can just update parameters on the existing material instance
	if ( currentMaterial && currentMaterial->IsA< CMaterialInstance >() && currentMaterial->GetBaseMaterial() == baseMaterial )
	{
		instance = Cast< CMaterialInstance >( currentMaterial );
	}
	// Otherwise need to create a new instance
	else
	{
		instance = new CMaterialInstance( GetPreviewWorld(), baseMaterial );
	}

	if ( !instance )
	{
		return;
	}

	const IMaterialDefinition* definition = instance->GetMaterialDefinition();
	const IMaterialDefinition::TParameterArray& params = definition->GetPixelParameters();
	for ( Uint32 j = 0; j < params.Size(); ++j )
	{
		if ( params[j].m_name == RED_NAME( Diffuse ) && ( params[j].m_type == CMaterialGraph::PT_Texture || params[j].m_type == CMaterialGraph::PT_Atlas ) )
		{
			THandle< ITexture > texHandle( Cast< ITexture >( texture ) );
			instance->WriteParameter( params[j].m_name, texHandle );
		}

		if ( params[j].m_name == RED_NAME( Diffuse ) && params[j].m_type == CMaterialGraph::PT_Cube )
		{
			THandle< CCubeTexture > texHandle( Cast< CCubeTexture >( texture ) );
			instance->WriteParameter( params[j].m_name, texHandle );
		}

		if ( params[j].m_name == RED_NAME( LodBias ) && params[j].m_type == CMaterialGraph::PT_Scalar  )
		{
			instance->WriteParameter( params[j].m_name, lodBias );
		}
	}

	SetMaterial( instance );

	m_mipLevel = ( Int32 )lodBias;
}

void CEdTexturePreviewPanel::SetMaterial( IMaterial* material )
{
	CEdMaterialPreviewPanel::SetMaterial( material );
	RefreshPreviewVisibility( material != nullptr );
}

void CEdTexturePreviewPanel::OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame )
{
	CEdMaterialPreviewPanel::OnViewportGenerateFragments( view, frame );

	frame->AddDebugScreenFormatedText( 20, 20, Color::YELLOW, TXT("MIP level: %i"), m_mipLevel );
}
