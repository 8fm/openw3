/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "entityMeshGenerator.h"
#include "editorExternalResources.h"
#include "dataError.h"
#include "../../common/redMath/random/fastRand.h"
#include "../../common/engine/mesh.h"
#include "../../common/engine/bitmapTexture.h"
#include "../../common/engine/textureArray.h"
#include "../../common/core/depot.h"
#include "../../common/core/garbageCollector.h"
#include "../../common/engine/meshComponent.h"
#include "../../common/engine/meshDataBuilder.h"
#include "../../common/engine/materialDefinition.h"
#include "../../common/engine/materialGraph.h"
#include "../../common/engine/materialInstance.h"
#include "../../common/engine/renderCommands.h"
#include "entityEditor.h"

//////////////////////////////////////////////////////////////////////////
// There are a few ugly hard-coded things happening here as far as textures
// are concerned:
//    - texture arrays
//    - diffuse color and variance color
//    - assuming everything uses same UVs
//
// If any more are needed, it might be worth just looking into doing the
// Right Thing, and actually run the material shaders. Probably can't do
// it on CPU, because of custom functions and such... but, could maybe
// make a special shader compiler that would run the material VS on the
// original vertices, and then evaluate PS for a given barycentric coord.
// ...  or something... needs more thought...
//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////
// General TODOs :
//
// Casting large textures can be quite slow. Would be nice to be able to jobify the casting, so we can make full use of the system
// cores. When I tried, there were crashes when getting values from simplygon containers, when changing refcounts on simplygon
// smart pointers, etc. It seems like simplygon is very sensitive to multiple threads, maybe using some thread-local-storage or
// something... I dunno...
//
// Prepare() does more than it really needs to, collecting full scene at default generate distance. Means that we're doing a lot of
// extra work generating the simplygon geometry when it's not going to be used. Also means we're collecting texture names for only
// the meshes and LODs present at the default generate distance.
//
// When generating a mesh for a building with an open bottom (because it's placed on the ground), we end up with the inside filled
// in. The building might only have the external walls, but because simplygon guarantees water-tight meshes, we can end up with
// twice as much geometry as we really need. There's an option to add a ground plane, which might be used to say "don't generate
// anything that's only visible from below here", but it seems to just clip the mesh (and sometimes skip out the interior triangles),
// which is less useful.
//
//////////////////////////////////////////////////////////////////////////


RED_DEFINE_STATIC_NAME( DiffuseArray );
RED_DEFINE_STATIC_NAME( NormalArray );
RED_DEFINE_STATIC_NAME( BaseDiffuse );
RED_DEFINE_STATIC_NAME( BaseNormal );
RED_DEFINE_STATIC_NAME( VarianceOffset );
RED_DEFINE_STATIC_NAME( VarianceColor );
RED_DEFINE_STATIC_NAME( TextureColor );
RED_DEFINE_STATIC_NAME( ColoredTextureBottom );
RED_DEFINE_STATIC_NAME( ColoredTextureTop );
RED_DEFINE_STATIC_NAME( DiffuseNoMips );
RED_DEFINE_STATIC_NAME( NormalsNoMips );
RED_DEFINE_STATIC_NAME( SystemNoMips );

#ifdef USE_SIMPLYGON

using namespace SimplygonSDK;


// A few settings that don't really have any reason to change...
static const Bool fixWinding			= true;
static const Bool flipNormalsGreen		= false;
static const Bool allowDX				= false;
static const Bool forcePower2			= true;
static const Bool generateTangentSpace	= true;
static const Bool flipBackfacing		= false;



static String GetMeshPath( const String& directory, const String& baseName )
{
	const Char* sep = TXT("");
	if ( !directory.EndsWith( TXT("\\") ) )
	{
		sep = TXT("\\");
	}
	return String::Printf( TXT("%s%s%s.%s"), directory.AsChar(), sep, baseName.AsChar(), CMesh::GetFileExtension() );
}

static String GetTexturePath( const String& directory, const String& baseName, const StringAnsi& textureName )
{
	String suffix;
	if ( Red::System::StringCompareNoCase( textureName.AsChar(), "Diffuse" ) == 0 )
	{
		suffix = TXT("_d");
	}
	else if ( Red::System::StringCompareNoCase( textureName.AsChar(), "Normals" ) == 0 )
	{
		suffix = TXT("_n");
	}
	else
	{
		suffix = String::Printf( TXT("_%") RED_PRIWas, textureName.AsChar() );
	}

	const Char* sep = TXT("");
	if ( !directory.EndsWith( TXT("\\") ) )
	{
		sep = TXT("\\");
	}

	return String::Printf( TXT("%s%s%s%s.%s"), directory.AsChar(), sep, baseName.AsChar(), suffix.AsChar(), CBitmapTexture::GetFileExtension() );
}

StringAnsi CEdEntityMeshGenerator::GetTextureName( const StringAnsi& channel, CBitmapTexture* texture )
{
	return texture!=nullptr ? StringAnsi::Printf( "%ls_%hs", texture->GetDepotPath().AsChar(), channel.AsChar() ) : "nullTexture";
}

spImageData CEdEntityMeshGenerator::FindMaterialTexture( const StringAnsi& channel, CBitmapTexture* texture )
{
	StringAnsi textureName = GetTextureName(channel, texture);

	// Reuse previously created image, if there is one.
	for ( Uint32 i = 0; i < m_materialTextures.Size(); ++i )
	{
		if ( m_materialTextures[i].m_texturePath == textureName )
		{
			return m_materialTextures[i].m_image;
		}
	}


	// Not found, have to add new
	spImageData img = m_simplygon->CreateImageData();

	Bool hasTextureData = ( texture != nullptr && ( texture->GetSourceData() || const_cast< CBitmapTexture::MipMap& >( texture->GetMips()[0] ).m_data.Load() ) );
	if ( !hasTextureData )
	{
		DATA_HALT( DES_Major, m_entityTemplate, TXT("Entity Mesh Generator"), TXT("Mesh in template has a material with null texture parameter. Generated textures may contain errors.") );

		// Create a dummy image
		img->Set2DSize( 1, 1 );
		img->AddColors( TYPES_ID_UCHAR, SG_IMAGEDATA_FORMAT_RGB );
		spUnsignedCharArray imgData = SimplygonSDK::SafeCast< IUnsignedCharArray >( img->GetColors() );

		Uint8 dataTuple[3] = { 255, 0, 255 };
		imgData->SetTuple( 0, dataTuple );
	}
	else
	{
		// Copy data from the texture into a new image. We assume a few channels have specific purpose, and any others are treated as regular
		// RGBA color data.
		Uint32 copySrcOffset = 0;
		const char* format = "";

		if ( channel == SG_MATERIAL_CHANNEL_NORMALS )
		{
			copySrcOffset = 0;
			format = SG_IMAGEDATA_FORMAT_RGB;
		}
		else if ( channel == SG_MATERIAL_CHANNEL_ROUGHNESS )
		{
			copySrcOffset = 3;
			format = SG_IMAGEDATA_FORMAT_L;
		}
		else if ( channel == SG_MATERIAL_CHANNEL_DIFFUSE )
		{
			copySrcOffset = 0;
			format = SG_IMAGEDATA_FORMAT_RGB;
		}
		else if ( channel == SG_MATERIAL_CHANNEL_OPACITY )
		{
			copySrcOffset = 3;
			format = SG_IMAGEDATA_FORMAT_L;
		}
		else
		{
			copySrcOffset = 0;
			format = SG_IMAGEDATA_FORMAT_RGBA;
		}

		const CBitmapTexture::MipMap& mip = texture->GetMips()[0];
		CSourceTexture* srcTex = texture->GetSourceData();
		if ( srcTex == nullptr )
		{
			//// Get the texture's mip 0, and uncompress it (in case it was a compressed format).
			//if ( !const_cast< CBitmapTexture::MipMap& >( mip ).m_data.Load() )
			//{
			//	return nullptr;
			//}

			// if we end up here then the mip data should be loaded already to check the hasTextureData

			srcTex = CreateObject< CSourceTexture >();
			srcTex->Init( mip.m_width, mip.m_height, TRF_TrueColor );
			srcTex->CreateFromMipCompressed( mip, texture->GetCompression() );
		}

		const Uint8* srcPtr = static_cast< const Uint8* >( srcTex->GetBufferAccessPointer() );

		// Create the new image and copy data from the source texture.
		img->Set2DSize( mip.m_width, mip.m_height );
		img->AddColors( TYPES_ID_UCHAR, format );
		spUnsignedCharArray imgData = SimplygonSDK::SafeCast< IUnsignedCharArray >( img->GetColors() );

		for ( Uint32 y = 0; y < mip.m_height; ++y )
		{
			for ( Uint32 x = 0; x < mip.m_width; ++x )
			{
				int idx = mip.m_width * y + x;
				imgData->SetTuple( idx, srcPtr + copySrcOffset );
				srcPtr += 4;
			}
		}

		// Re-init the source texture so it has 0 size. We Discard it, but that doesn't necessarily mean it gets deleted now, so we do this now
		// to let the bitmap buffer be freed and avoid OOM...
		srcTex->Init( 0, 0, TRF_TrueColor );
		srcTex->Discard();

		const_cast< CBitmapTexture::MipMap& >( mip ).m_data.Unload();
	}

	// Add to cache

	spTextureTable textureTable = m_scene->GetTextureTable();
	spTexture smplTexture = m_simplygon->CreateTexture();
	smplTexture->SetName( textureName.AsChar() );
	smplTexture->SetImageData( img );
	textureTable->AddTexture( smplTexture );
	//RED_LOG( Simplygon, TXT("Texture added to texture table with name: %hs"), textureName.AsChar() );

	m_materialTextures.PushBack( MaterialTexture( textureName, img ) );
	return img;
}


StringAnsi CEdEntityMeshGenerator::FindMaterialTextureName( const StringAnsi& channel, CBitmapTexture* texture )
{
	// make sure the texture is added
	FindMaterialTexture( channel, texture );
	return GetTextureName( channel, texture );
}

spShadingNode CEdEntityMeshGenerator::SampleTextureArray( const StringAnsi& channel, CTextureArray* tex, const Vector* color, const Float colorizeBottom, const Float colorizeTop )
{
	if ( tex == nullptr )
	{
		DATA_HALT( DES_Major, m_entityTemplate, TXT("Entity Mesh Generator"), TXT("Mesh in template has a material with null texture array parameter. Generated textures may contain errors.") );
		spShadingColorNode color = m_simplygon->CreateShadingColorNode();
		color->SetColor( 1.0f, 0.0f, 1.0f, 1.0f );
		return color.GetPointer();
	}

	// Normal/Roughness map should be treated as linear data.
	Bool srgb = ( channel != SG_MATERIAL_CHANNEL_NORMALS ) && ( channel != SG_MATERIAL_CHANNEL_ROUGHNESS );

	// Build shading network to sample the texture array based on vertex color G (which has been extended in Color1)
	spShadingVertexColorNode vcol = m_simplygon->CreateShadingVertexColorNode();
	vcol->SetVertexColorSet( "COLOR1" );

	spShadingMultiplyNode scaledVcol = m_simplygon->CreateShadingMultiplyNode();
	scaledVcol->SetInput( 0, vcol );
	// Scale is from pbr_vert_blend, to select between 16 textures?
	scaledVcol->SetDefaultParameter( 1, 15.9999f, 15.9999f, 15.9999f, 15.9999f );

	

	//static Bool useDebug = false;
	//if(useDebug)
	//{//DEBUG-----------------------------------
	//	static Uint32 debugIndex = 0u;

	//	TDynArray< CBitmapTexture* > bmps;
	//	tex->GetTextures( bmps );
	//	CBitmapTexture* bmp = bmps[ Min( debugIndex, bmps.Size() - 1 ) ];
	//	spImageData texImg = FindMaterialTexture( channel, bmp );
	//	StringAnsi textureName = StringAnsi::Printf( "%ls_%d_%d", bmp->GetFile()->GetFileName().AsChar(), debugIndex, debugCounter++ );
	//	spTexture texture = m_simplygon->CreateTexture();
	//	texture->SetName( textureName.AsChar() );
	//	texture->SetImageData( texImg );
	//	textureTable->AddTexture( texture );
	//	spShadingTextureNode texNode = m_simplygon->CreateShadingTextureNode();
	//	texNode->SetTextureName( textureName.AsChar() );
	//	texNode->SetUseSRGB( srgb );
	//	texNode->SetTexCoordSet( "TEXCOORD0" );
	//	return spShadingNode( texNode );
	//}//DEBUG-----------------------------------
	//
	//static Bool useVertexColorDebug = false;
	//if(useVertexColorDebug)
	//{//DEBUG VERTEX COLORS-----------------------------------
	//	return spShadingNode( vcol );
	//}//DEBUG VERTEX COLORS-----------------------------------

	spShadingNode outputNode;

	// Basically, we're layering up each slice of the array, using shifted and clamped green from vertex color. For each layer,
	// we shift green by the layer index and clamp to 0-1. This way, we basically work our way to linear interpolation between
	// two adjacent slices.
	TDynArray< CBitmapTexture* > bmps;
	tex->GetTextures( bmps );
	for ( Uint32 i = 0; i < bmps.Size(); ++i )
	//Uint32 i = Min( 3u, bmps.Size() - 1 );
	{
		spShadingTextureNode texNode = m_simplygon->CreateShadingTextureNode();
		texNode->SetTextureName( FindMaterialTextureName( channel, bmps[i] ).AsChar() );
		texNode->SetUseSRGB( srgb );
		texNode->SetTexCoordSet( "TEXCOORD0" );

		spShadingNode sampledTexture = spShadingNode( texNode );

		if ( color != nullptr )
		{
			spShadingMultiplyNode colorizedTexture = m_simplygon->CreateShadingMultiplyNode();
			colorizedTexture->SetInput( 0, texNode );
			colorizedTexture->SetDefaultParameter( 1, color->X, color->Y, color->Z, color->W );

			spShadingColorNode bottomValue = m_simplygon->CreateShadingColorNode();
			bottomValue->SetColor( colorizeBottom, colorizeBottom, colorizeBottom, colorizeBottom );

			spShadingStepNode bottomClamped = m_simplygon->CreateShadingStepNode();
			bottomClamped->SetDefaultParameter( 0, i, i, i, i );
			bottomClamped->SetInput( 1, bottomValue );

			spShadingColorNode topValue = m_simplygon->CreateShadingColorNode();
			topValue->SetColor( colorizeTop, colorizeTop, colorizeTop, colorizeTop );

			spShadingStepNode topClamped = m_simplygon->CreateShadingStepNode();
			topClamped->SetInput( 0, topValue );
			topClamped->SetDefaultParameter( 1, i, i, i, i );

			spShadingMultiplyNode allClamped = m_simplygon->CreateShadingMultiplyNode();
			allClamped->SetInput( 0, bottomClamped );
			allClamped->SetInput( 1, topClamped );

			spShadingInterpolateNode selectivelyColorized = m_simplygon->CreateShadingInterpolateNode();
			selectivelyColorized->SetInput( 0, texNode );
			selectivelyColorized->SetInput( 1, colorizedTexture );
			selectivelyColorized->SetInput( 2, allClamped );

			sampledTexture = spShadingNode( selectivelyColorized );
		}

		if ( outputNode != nullptr )
		{
			spShadingSubtractNode select = m_simplygon->CreateShadingSubtractNode();
			select->SetInput( 0, scaledVcol );
			select->SetDefaultParameter( 1, i-1, i-1, i-1, i-1 );

			spShadingClampNode clamp = m_simplygon->CreateShadingClampNode();
			clamp->SetInput( 0, select );
			clamp->SetDefaultParameter( 1, 0, 0, 0, 0 );
			clamp->SetDefaultParameter( 2, 1, 1, 1, 1 );

			spShadingInterpolateNode interp = m_simplygon->CreateShadingInterpolateNode();
			interp->SetInput( 0, outputNode );
			interp->SetInput( 1, sampledTexture );
			interp->SetInput( 2, clamp );

			outputNode = spShadingNode( interp );
		}
		else
		{
			outputNode = spShadingNode( sampledTexture );
		}
	}

	return outputNode;
}


//spImageData CEdEntityMeshGenerator::SampleTextureArray_Debug( const StringAnsi& channel, CTextureArray* tex )
//{
//	static int debugCounter = 0;
//
//	// Basically, we're layering up each slice of the array, using shifted and clamped green from vertex color. For each layer,
//	// we shift green by the layer index and clamp to 0-1. This way, we basically work our way to linear interpolation between
//	// two adjacent slices.
//	TDynArray< CBitmapTexture* > bmps;
//	tex->GetTextures( bmps );
//	{
//		static Uint32 debugIndex = 0u;
//		spImageData texImg = FindMaterialTexture( channel, bmps[Min( debugIndex, bmps.Size() - 1 )] );
//		return texImg;
//	}
//
//	return nullptr;
//}

spMaterial CEdEntityMeshGenerator::InitSimplygonMaterial( ::IMaterial* engineMtl )
{
	spMaterial material = m_simplygon->CreateMaterial();

	IMaterialDefinition* defn = engineMtl->GetMaterialDefinition();

	Bool hasTextureColor = false;
	Vector textureColor;
	Float colorizationBottom = 0.f;
	Float colorizationTop = 17.f;

	Bool hasDiffuse = false;
	Bool hasNormal = false;

	auto params = defn->GetPixelParameters();

	//HACK we need to extract these values before sampling any array textures to make it work
	for ( Uint32 i = 0; i < params.Size(); ++i )
	{
		if ( params[i].m_type == IMaterialDefinition::PT_Color )
		{
			if ( params[i].m_name == RED_NAME( TextureColor ) )
			{
				Color color;
				engineMtl->ReadParameter( params[i].m_name, color );
				textureColor = color.ToVector();
				hasTextureColor = true;
			}
		}
		else if ( params[i].m_type == IMaterialDefinition::PT_Scalar )
		{
			if ( params[i].m_name == RED_NAME( ColoredTextureBottom ) )
			{
				engineMtl->ReadParameter( params[i].m_name, colorizationBottom );
			}
			if ( params[i].m_name == RED_NAME( ColoredTextureTop ) )
			{
				engineMtl->ReadParameter( params[i].m_name, colorizationTop );
			}
		}
		else if ( params[i].m_type == IMaterialDefinition::PT_Texture || params[i].m_type == IMaterialDefinition::PT_TextureArray )
		{
			if ( params[i].m_name == RED_NAME( Normal ) )
			{
				hasNormal = true;
			}
			else if ( params[i].m_name == RED_NAME( Diffuse ) )
			{
				hasNormal = true;
			}
		}
	}

	for ( Uint32 i = 0; i < params.Size(); ++i )
	{
		if ( params[i].m_type == IMaterialDefinition::PT_Texture )
		{
			THandle< CBitmapTexture > bmpTex = nullptr;
			engineMtl->ReadParameter( params[i].m_name, bmpTex );

			if ( params[i].m_name == RED_NAME( Normal ) )
			{
				spImageData normImg = FindMaterialTexture( SG_MATERIAL_CHANNEL_NORMALS, bmpTex.Get() );
				spImageData roughImg = FindMaterialTexture( SG_MATERIAL_CHANNEL_ROUGHNESS, bmpTex.Get() );

				material->SetUseTangentSpaceNormals( true );
				material->SetTextureImage( SG_MATERIAL_CHANNEL_NORMALS, normImg );
				material->SetTextureImage( SG_MATERIAL_CHANNEL_ROUGHNESS, roughImg );
			}
			else if ( params[i].m_name == RED_NAME( BaseNormal ) && !hasNormal )
			{
				spImageData normImg = FindMaterialTexture( SG_MATERIAL_CHANNEL_NORMALS, bmpTex.Get() );
				spImageData roughImg = FindMaterialTexture( SG_MATERIAL_CHANNEL_ROUGHNESS, bmpTex.Get() );

				material->SetUseTangentSpaceNormals( true );
				material->SetTextureImage( SG_MATERIAL_CHANNEL_NORMALS, normImg );
				material->SetTextureImage( SG_MATERIAL_CHANNEL_ROUGHNESS, roughImg );
			}
			else if ( params[i].m_name == RED_NAME( Diffuse ) )
			{
				spImageData diffImg = FindMaterialTexture( SG_MATERIAL_CHANNEL_DIFFUSE, bmpTex.Get() );
				spImageData alphaImg = FindMaterialTexture( SG_MATERIAL_CHANNEL_OPACITY, bmpTex.Get() );

				material->SetTextureImage( SG_MATERIAL_CHANNEL_DIFFUSE, diffImg );
				material->SetTextureImage( SG_MATERIAL_CHANNEL_OPACITY, alphaImg );
			}
			else if ( params[i].m_name == RED_NAME( BaseDiffuse ) && !hasDiffuse )
			{
				spImageData diffImg = FindMaterialTexture( SG_MATERIAL_CHANNEL_DIFFUSE, bmpTex.Get() );
				spImageData alphaImg = FindMaterialTexture( SG_MATERIAL_CHANNEL_OPACITY, bmpTex.Get() );

				material->SetTextureImage( SG_MATERIAL_CHANNEL_DIFFUSE, diffImg );
				material->SetTextureImage( SG_MATERIAL_CHANNEL_OPACITY, alphaImg );
			}
			else
			{
				spImageData img = FindMaterialTexture( params[i].m_name.AsAnsiChar(), bmpTex.Get() );

				material->AddUserChannel( params[i].m_name.AsAnsiChar() );
				material->SetTextureImage( params[i].m_name.AsAnsiChar(), img );
			}
		}

		// HACK : If we have a texture array, we assume it's used for something like pbr_vertblend. Basically vertex color G is used
		// to index into the array.
		else if ( params[i].m_type == IMaterialDefinition::PT_TextureArray )
		{
			THandle< CTextureArray > tex = nullptr;
			engineMtl->ReadParameter( params[i].m_name, tex );

			if ( params[i].m_name == RED_NAME( NormalArray ) )
			{
				StringAnsi normChannel = SG_MATERIAL_CHANNEL_NORMALS;
				Bool doRoughness = true;

				// If there's also a separate Normal channel, then we change it
				if ( hasNormal )
				{
					normChannel = "NormalArray";
					material->AddUserChannel( normChannel.AsChar() );
					doRoughness = false;
					break;
				}

				spShadingNode resultNode = SampleTextureArray( normChannel, tex.Get() );
				material->SetShadingNetwork( normChannel.AsChar(), resultNode );

				if ( doRoughness )
				{
					spShadingNode resultNode = SampleTextureArray( SG_MATERIAL_CHANNEL_ROUGHNESS, tex.Get() );
					material->SetShadingNetwork( SG_MATERIAL_CHANNEL_ROUGHNESS, resultNode );
				}
			}
			else 
				if ( params[i].m_name == RED_NAME( DiffuseArray ) )
			{
				StringAnsi diffChannel = SG_MATERIAL_CHANNEL_DIFFUSE;
				Bool doOpacity = true;

				// If there's also a separate Diffuse channel, then we change it
				if ( hasNormal )
				{
					diffChannel = "DiffuseArray";
					material->AddUserChannel( diffChannel.AsChar() );
					doOpacity = false;
					break;
				}

				//*
				if ( hasTextureColor )
				{
					spShadingNode resultNode = SampleTextureArray( diffChannel, tex.Get(), &textureColor, colorizationBottom, colorizationTop );
					material->SetShadingNetwork( diffChannel.AsChar(), resultNode );
				}
				else
				{
					spShadingNode resultNode = SampleTextureArray( diffChannel, tex.Get() );
					material->SetShadingNetwork( diffChannel.AsChar(), resultNode );
				}
				/*/
				material->SetTextureImage( SG_MATERIAL_CHANNEL_DIFFUSE, SampleTextureArray_Debug( diffChannel, tex.Get() ) );
				//*/

				if ( doOpacity )
				{
					spShadingNode resultNode = SampleTextureArray( SG_MATERIAL_CHANNEL_OPACITY, tex.Get() );
					material->SetShadingNetwork( SG_MATERIAL_CHANNEL_OPACITY, resultNode );
				}
			}
			else
			{
				spShadingNode resultNode = SampleTextureArray( params[i].m_name.AsAnsiChar(), tex.Get() );
				material->AddUserChannel( params[i].m_name.AsAnsiChar() );
				material->SetShadingNetwork( params[i].m_name.AsAnsiChar(), resultNode );
			}
		}

		// HACK : Also need to handle VarianceColor...
		else if ( params[i].m_type == IMaterialDefinition::PT_Color )
		{
			if ( params[i].m_name == RED_NAME( VarianceColor ) )
			{
				Color color;
				engineMtl->ReadParameter( params[i].m_name, color );
				material->AddUserChannel( "VarianceColor" );
				Vector v = color.ToVector();
				material->SetColor( "VarianceColor", v.X, v.Y, v.Z, v.W );
			}
		}
		else if ( params[i].m_type == IMaterialDefinition::PT_Scalar )
		{
			if ( params[i].m_name == RED_NAME( VarianceOffset ) )
			{
				Float offset;
				engineMtl->ReadParameter( params[i].m_name, offset );
				material->AddUserChannel( "VarianceOffset" );
				material->SetColor( "VarianceOffset", offset, offset, offset, offset );
			}
		}
	}

	return material;
}


static CBitmapTexture* CreateBitmapFromImageData( ISimplygonSDK* simplygon, spImageData imageData, spImageData optionalAlphaData, const CName& textureGroup )
{
	// Create bitmap texture for diffuse image.
	// Really? No way to just copy the colors data into our own buffer?
	spUnsignedCharArray color = SimplygonSDK::SafeCast< IUnsignedCharArray >( imageData->GetColors() );
	spUnsignedCharArray alpha = nullptr;
	if ( optionalAlphaData != nullptr )
	{
		alpha = SimplygonSDK::SafeCast< IUnsignedCharArray >( optionalAlphaData->GetColors() );
	}

	Uint32 width = imageData->GetXSize();
	Uint32 height = imageData->GetYSize();
	Uint32 pitch = width * 4;

	Uint8* outData = static_cast< Uint8* >( RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_Temporary, pitch * height ) );

	spUnsignedCharData col = simplygon->CreateUnsignedCharData();
	spUnsignedCharData alp = simplygon->CreateUnsignedCharData();
	for ( Uint32 y = 0; y < height; ++y )
	{
		for ( Uint32 x = 0; x < width; ++x )
		{
			color->GetTuple( x + y*width, col );
			outData[x*4 + y*pitch + 0] = col[0];
			outData[x*4 + y*pitch + 1] = col[1];
			outData[x*4 + y*pitch + 2] = col[2];
			outData[x*4 + y*pitch + 3] = 0xff;

			if ( alpha != nullptr )
			{
				alpha->GetTuple( x + y*width, alp );
				outData[x*4 + y*pitch + 3] = alp[0];
			}
		}
	}

	CSourceTexture* sourceTexture = CreateObject< CSourceTexture >();
	sourceTexture->Init( width, height, TRF_TrueColor );
	sourceTexture->CreateFromRawData( outData, width, height, pitch );

	RED_MEMORY_FREE( MemoryPool_Default, MC_Temporary, outData );

	CBitmapTexture* texture = CreateObject< CBitmapTexture >();
	texture->InitFromSourceData( sourceTexture, textureGroup );

	return texture;
}


Bool CEdEntityMeshGenerator::CollectScene()
{
	GFeedback->UpdateTaskInfo( TXT("Collecting scene") );

	// create a scene-object
	m_scene					= m_simplygon->CreateScene();
	m_sourceMaterialTable	= m_scene->GetMaterialTable();

	Uint32 numMtls = 0;

	// create geometry for all meshes, using the LOD that would be used at the reference distance.
	const Uint32 numMeshes = m_sourceMeshComponents.Size();
	for ( Uint32 mesh_i = 0; mesh_i < numMeshes; ++mesh_i )
	{
		GFeedback->UpdateTaskProgress( mesh_i, numMeshes - 1 );

		CMeshComponent* component = m_sourceMeshComponents[ mesh_i ];

		// If the mesh is not visible or not marked for proxy mesh generation, don't include it.
		if ( !component->IsVisible() && !component->UseWithSimplygonOnly() )
		{
			continue;
		}

		CMesh* mesh = component->GetMeshNow();

		// Pick LOD based on reference distance
		Int32 lodIndex = 0;
		for ( Int32 lod_i = mesh->GetNumLODLevels() - 1; lod_i >= 0; --lod_i )
		{
			if ( mesh->GetLODLevel( lod_i ).m_distance <= m_settings.m_generateDistance )
			{
				lodIndex = lod_i;
				break;
			}
		}
		const CMesh::LODLevel& lod = mesh->GetMeshLODLevels()[ lodIndex ];

		const CMeshData meshData( mesh );
		const auto& chunksConst = meshData.GetChunks();

		// Group the mesh's chunks under a single node, for easier transforms.
		spSceneNode meshNode = m_simplygon->CreateSceneNode();
		meshNode->SetName( UNICODE_TO_ANSI( component->GetName().AsChar() ) );
		m_scene->GetRootNode()->AddChild( meshNode );

		// Set transform for the mesh chunks.
		const Matrix& meshMtx = component->GetLocalToWorld();
		spMatrix4x4 nodeTx = meshNode->GetRelativeTransform();
		for ( Uint32 i = 0; i < 4; ++i )
		{
			for ( Uint32 j = 0; j < 4; ++j )
			{
				nodeTx->SetElement( i, j, meshMtx.V[i].A[j] );
			}
		}


		for ( Uint32 chunk_i = 0; chunk_i < lod.m_chunks.Size(); ++chunk_i )
		{
			Uint16 chunkID = lod.m_chunks[ chunk_i ];
			const SMeshChunk& chunk = chunksConst[ chunkID ];

			Int32 mid = chunk.m_materialID;
			THandle< ::IMaterial > mtl = mesh->GetMaterials()[mid];
			ERenderingSortGroup sg = RSG_DebugUnlit;
			if( mtl )
			{
				if( IMaterialDefinition* definition = mtl->GetMaterialDefinition() )
				{
					sg = definition->GetRenderingSortGroup();
				}
			}

			if( sg != RSG_Volumes && sg != RSG_WaterBlend )
			{
				spSceneMesh sgMesh = m_simplygon->CreateSceneMesh();

				StringAnsi sgMeshName = StringAnsi::Printf( "%" RED_PRIus "_%" RED_PRIu16, component->GetName().AsChar(), chunkID );
				sgMesh->SetName( sgMeshName.AsChar() );

				spPackedGeometryData packedGeom = SimplygonHelpers::CreateGeometryFromMeshChunk( m_simplygon, chunk, numMtls, fixWinding );

				spGeometryData unpackedGeom = packedGeom->NewUnpackedCopy();

				sgMesh->SetGeometry( unpackedGeom );

				// Keep an extra copy of the geometry, because the remeshing process seems to destroy the geomety in the scene, and we need the
				// original to manually recast textures.
				m_originalGeometry.PushBack( unpackedGeom->NewCopy( true ) );
				m_geometryMtlMap.PushBack( mtl.Get() );

				// add mesh to the rootnode
				meshNode->AddChild( sgMesh );
			}
		}

		for ( Uint32 mtl_i = 0; mtl_i < mesh->GetMaterials().Size(); ++mtl_i )
		{
			THandle< ::IMaterial > mtl = mesh->GetMaterials()[mtl_i];
			if ( !mtl.IsValid() )
			{
				m_errorMessage = String::Printf( TXT("Error while collecting scene - component '%ls' uses mesh '%ls' that has broken material '%ls'"),
					component->GetName().AsChar(), mesh->GetDepotPath().AsChar(), mtl_i < mesh->GetMaterialNames().Size() ? mesh->GetMaterialNames()[mtl_i].AsChar() : TXT("unknown") );
				return false;
			}

			spMaterial sourceMaterial = InitSimplygonMaterial( mtl.Get() );
			m_sourceMaterialTable->AddMaterial( sourceMaterial );
		}

		numMtls += mesh->GetMaterials().Size();
	}

	m_sourceTextureTable = m_scene->GetTextureTable();

	return true;
}

spMappingImage CEdEntityMeshGenerator::Remesh()
{
	SimplygonHelpers::SimplygonProgressObserver progressObserver;

	spRemeshingProcessor remesher = m_simplygon->CreateRemeshingProcessor();
	spRemeshingSettings remeshSettings = remesher->GetRemeshingSettings();
	remeshSettings->SetOnScreenSize( m_settings.m_screenSize );
	remeshSettings->SetMergeDistance( m_settings.m_mergeDistance );
	remeshSettings->SetHardEdgeAngleInRadians( DEG2RAD(m_settings.m_hardEdgeAngle) );
	remeshSettings->SetSurfaceTransferMode( SG_SURFACETRANSFER_ACCURATE );

	if( m_settings.m_useCullingPlane )
	{
		remeshSettings->SetUseCuttingPlanes( true );

		//Define the cutting plane, add it to the scene, and create a selection set containing it
		real cuttingPlanePos[] = { 0.f, 0.f, m_sourceMeshBounds.Min.Z + m_settings.m_cullingPlaneOffset };
		real cuttingPlaneNormal[] = { 0.0f, 0.0f, 1.0f }; //This doesn't need to be normalized
		spScenePlane cutPlane = m_scene->GetRootNode()->CreateChildPlane( cuttingPlanePos, cuttingPlaneNormal ); //Could also create new spScenePlane and AddChild it to scene
		spSelectionSet selectionSet = m_simplygon->CreateSelectionSet();
		selectionSet->AddItem(cutPlane->GetNodeGUID());
		int selectionSetId = m_scene->GetSelectionSetTable()->AddSelectionSet(selectionSet);
		remeshSettings->SetCuttingPlaneSelectionSetID(selectionSetId);
	}

	spMappingImageSettings mappingImageSettings = remesher->GetMappingImageSettings();
	if ( m_settings.m_textureSize == 0 )
	{
		mappingImageSettings->SetUseAutomaticTextureSize( true );
	}
	else
	{
		mappingImageSettings->SetWidth( m_settings.m_textureSize );
		mappingImageSettings->SetHeight( m_settings.m_textureSize );
	}
	mappingImageSettings->SetForcePower2Texture( forcePower2 );

	//HACK division by 0 is not fun...
	Uint32 gutter = Max( m_settings.m_gutterSpace, 1u );

	mappingImageSettings->SetGutterSpace( gutter );
	mappingImageSettings->SetMultisamplingLevel( m_settings.m_multisampling );
	mappingImageSettings->SetGenerateMappingImage( true );
	mappingImageSettings->SetGenerateTexCoords( true );
	mappingImageSettings->SetGenerateTangents( true );

	remesher->SetScene( m_scene );

	//static Bool exportObj = false;
	//if( exportObj )
	//{
	//	RED_LOG( Simplygon, TXT("Exporting mesh with %d materials"), m_scene->GetMaterialTable()->GetMaterialsCount() );
	//	SimplygonHelpers::ExportToObj( m_simplygon, m_scene, TXT("simplygonDebug.obj") );
	//}

	//static Bool saveScene = false;
	//if( saveScene )
	//{
	//	m_scene->SaveToFile( "simplygonDebug.scene" );
	//}

	remesher->AddObserver( &progressObserver, SimplygonSDK::SG_EVENT_PROGRESS );
	remesher->AddObserver( &progressObserver, SimplygonSDK::SG_EVENT_PROCESS_STARTED );

	remesher->RemeshGeometry();

	return remesher->GetMappingImage();
}

CBitmapTexture* CEdEntityMeshGenerator::CastDiffuse( spMappingImage mappingImage )
{
	// spMappingImageMeshData contains information needed to convert the mapping data to the individual
	// geometries from the collection of geometries
	spMappingImageMeshData mapping_image_mesh_data = mappingImage->GetMappingMeshData();
	const Uint32 geometryCount = mapping_image_mesh_data->GetMappedGeometriesCount();

	// The spChunkedImageData contains per-texel mapping information back to the original geometries
	// as two fields:
	//
	//  - Original triangle id, from a geometry consisted of all the original geometry combined.
	//    This means that the local triangle IDs from each original geometries will not valid here.
	//    Instead, the triangle ID is a global ID from the combined geometry.
	//    To map back to local triangle IDs, the spMappingImageMeshData is used.
	//
	//  - The equivalent Barycentric coordinates
	//
	spChunkedImageData chunked_image_data = mappingImage->GetImageData();

	// This is HACK solving an internal Simplygon bug (if texture size is set to 64x64, chunked_image_data->GetTotal(X/Y)Size() returns size that is twice as big as actual image results, lol)
	const Uint32 width = chunked_image_data->GetTotalXSize() / ( m_settings.m_multisampling * ( m_settings.m_textureSize == 64 ? 2 : 1 ) );
	const Uint32 height = chunked_image_data->GetTotalYSize() / ( m_settings.m_multisampling * ( m_settings.m_textureSize == 64 ? 2 : 1 ) );

	GRender->Flush();

	GpuApi::TextureDesc rtDesc;
	rtDesc.format = GpuApi::TEXFMT_Float_R32G32B32A32;
	rtDesc.width = width;
	rtDesc.height = height;
	rtDesc.initLevels = 1;
	rtDesc.msaaLevel = 0;
	rtDesc.sliceNum = 1;
	rtDesc.type = GpuApi::TEXTYPE_2D;
	rtDesc.usage = GpuApi::TEXUSAGE_RenderTarget;
	GpuApi::TextureRef texRef = GpuApi::CreateTexture( rtDesc, GpuApi::TEXG_System );
	if( !texRef )
	{
		RED_HALT( "Couldn't create render target for cast material" );
		return false;
	}

	GpuApi::RenderTargetSetup rtSetup_GBuffers;
	rtSetup_GBuffers.SetColorTarget( 0, texRef );
	rtSetup_GBuffers.SetViewportFromTarget( texRef );
	GpuApi::SetupRenderTargets( rtSetup_GBuffers );
	GpuApi::ViewportDesc desc;
	desc.x = 0; desc.y = 0; desc.width = width; desc.height = height;
	GpuApi::SetViewport(desc);

	const Vector clearColor(0,0,0,0);
	GpuApi::ClearColorTarget( texRef, &(clearColor.X) );

	Bool asyncCompilationMode = GRender->GetAsyncCompilationMode();
	GRender->SetAsyncCompilationMode( false );

	THashMap< Uint32, TDynArray< IntermediateVStream > > geomVstreamMap;

	// Get the number of image data chunks
	const Uint32 x_chunks = chunked_image_data->GetXSize();
	const Uint32 y_chunks = chunked_image_data->GetYSize();

	Uint8* samples_used = static_cast< Uint8* >( RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_Temporary, width * height ) );

	// Loop the image data
	// First by chunk
	// Then by pixel
	// Then by sub-pixel for multisampling
	for ( Uint32 x_chunk = 0; x_chunk < x_chunks; ++x_chunk )
	{
		for ( Uint32 y_chunk = 0; y_chunk < y_chunks; ++y_chunk )
		{
			GFeedback->UpdateTaskProgress( y_chunk + x_chunk*y_chunks, x_chunks*y_chunks );

			// Lock and fetch a chunk of mapping image pixels
			spImageData current_chunk = chunked_image_data->LockChunk2D(x_chunk, y_chunk);

			// Get the mapping image fields "TriangleIds" & "BarycentricCoords"
			spRidArray triangle_ids = SafeCast<IRidArray>(current_chunk->GetField( "TriangleIds" ));
			spUnsignedShortArray barycentric_coords = SafeCast<IUnsignedShortArray>(current_chunk->GetField( "BarycentricCoords" ));

			// Continue with next iteration if any of the fields don't exist
			if ( !triangle_ids || !barycentric_coords )
				continue;

			// Get the number of pixels in the chunk, excluding sub-pixels
			const Uint32 px_size = current_chunk->GetXSize() / m_settings.m_multisampling;
			const Uint32 py_size = current_chunk->GetYSize() / m_settings.m_multisampling;

			//Loop the pixels
			for ( Uint32 py = 0; py < py_size; ++py )
			{
				for ( Uint32 px = 0; px < px_size; ++px )
				{
					// Texel coordinates in the target texture.
					const Uint32 texture_px = x_chunk * px_size + px;
					const Uint32 texture_py = y_chunk * py_size + py;

					// Loop the sub-pixels
					for ( Uint32 py_multisample = 0; py_multisample < m_settings.m_multisampling; ++py_multisample )
					{
						for ( Uint32 px_multisample = 0; px_multisample < m_settings.m_multisampling; ++px_multisample )
						{
							// The one-dimensional index of the current sub-pixel
							// index = column + row * num_columns
							rid sub_pixel_index = ( px * m_settings.m_multisampling + px_multisample ) + ( py * m_settings.m_multisampling + py_multisample ) * ( px_size * m_settings.m_multisampling );

							// The global triangle ID in the combined geometry
							rid global_triangle_id = triangle_ids->GetItem( sub_pixel_index );

							// Continue with the next sub-pixel, should the current sub-pixel not map to a triangle
							if ( global_triangle_id < 0 )
							{
								continue;
							}

							// Get the corresponding barycentric coordinates in the original geometry
							Uint16 barycentric_x = barycentric_coords->GetItem( sub_pixel_index * 2 + 0 );
							Uint16 barycentric_y = barycentric_coords->GetItem( sub_pixel_index * 2 + 1 );
							Uint16 barycentric_z = 65535 - barycentric_x - barycentric_y;

							// Get the normalized barycentric coordinates
							Float barycentric_x_normalized = barycentric_x / 65535.0f;
							Float barycentric_y_normalized = barycentric_y / 65535.0f;
							Float barycentric_z_normalized = barycentric_z / 65535.0f;

							// Declare the local triangle id variable
							Uint32 local_triangle_id = -1;
							Int32 mapped_geometry_id = -1;

							// To find which original geometry is being mapped to using the global triangle id
							// the mapping image mesh data is used.
							for ( Uint32 g_id = 0; g_id < geometryCount; ++g_id )
							{
								Int32 next_geometry_starting_triangle_id = -1;

								// If g_id is the last geometry, then keep next_geometry_starting_triangle_id as -1
								if ( g_id < geometryCount - 1 )
								{
									next_geometry_starting_triangle_id = mapping_image_mesh_data->GetStartTriangleIdOfGeometry( g_id + 1 );
								}

								// If the global triangle id is below the next geometry's starting triangle id,
								// we know that the current geometry (g_id) will contain the current triangle
								//
								// Also if next_geometry_starting_triangle_id is -1, since then we're at the last geometry
								if ( global_triangle_id < next_geometry_starting_triangle_id || next_geometry_starting_triangle_id == -1 )
								{
									mapped_geometry_id = g_id;
									local_triangle_id = global_triangle_id - mapping_image_mesh_data->GetStartTriangleIdOfGeometry( g_id );
									break;
								}
							}

							// Make sure the mapped-to geometry is valid
							if ( mapped_geometry_id < 0 || mapped_geometry_id >= (Int32)geometryCount )
							{
								RED_HALT( "Invalid geometry ID" );
								continue;
							}

							Uint32 pixel_index = texture_px + texture_py * width;
							samples_used[pixel_index]++;

							spGeometryData& geometry = m_originalGeometry[ mapped_geometry_id ];

							Int32 corner0 = local_triangle_id * 3 + 0;
							Int32 corner1 = local_triangle_id * 3 + 1;
							Int32 corner2 = local_triangle_id * 3 + 2;

							IntermediateVStream vertex;

							{
								vertex.m_position[0] = texture_px;
								vertex.m_position[1] = texture_py;
							}

							{
								// Use texcoord0 to sample the texture
								spRealArray texcoords = geometry->GetTexCoords( 0 );

								Float u_corner0 = texcoords->GetItem(corner0 * 2 + 0);
								Float v_corner0 = texcoords->GetItem(corner0 * 2 + 1);

								Float u_corner1 = texcoords->GetItem(corner1 * 2 + 0);
								Float v_corner1 = texcoords->GetItem(corner1 * 2 + 1);

								Float u_corner2 = texcoords->GetItem(corner2 * 2 + 0);
								Float v_corner2 = texcoords->GetItem(corner2 * 2 + 1);

								// With the barycentric coordinates we can interpolate to a precise texture coordinate value in the triangle
								vertex.m_uv0[0] = barycentric_x_normalized * u_corner0 + barycentric_y_normalized * u_corner1 + barycentric_z_normalized * u_corner2;
								vertex.m_uv0[1] = barycentric_x_normalized * v_corner0 + barycentric_y_normalized * v_corner1 + barycentric_z_normalized * v_corner2;
							}

							{
								// Use texcoord1 to sample the texture
								spRealArray texcoords = geometry->GetTexCoords( 1 );

								Float u_corner0 = texcoords->GetItem(corner0 * 2 + 0);
								Float v_corner0 = texcoords->GetItem(corner0 * 2 + 1);

								Float u_corner1 = texcoords->GetItem(corner1 * 2 + 0);
								Float v_corner1 = texcoords->GetItem(corner1 * 2 + 1);

								Float u_corner2 = texcoords->GetItem(corner2 * 2 + 0);
								Float v_corner2 = texcoords->GetItem(corner2 * 2 + 1);

								// With the barycentric coordinates we can interpolate to a precise texture coordinate value in the triangle
								vertex.m_uv1[0] = barycentric_x_normalized * u_corner0 + barycentric_y_normalized * u_corner1 + barycentric_z_normalized * u_corner2;
								vertex.m_uv1[1] = barycentric_x_normalized * v_corner0 + barycentric_y_normalized * v_corner1 + barycentric_z_normalized * v_corner2;
							}

							{
								// Use normals to sample the texture
								spRealArray normals = geometry->GetNormals();

								Float nx_corner0 = normals->GetItem(corner0 * 3 + 0);
								Float ny_corner0 = normals->GetItem(corner0 * 3 + 1);
								Float nz_corner0 = normals->GetItem(corner0 * 3 + 2);

								Float nx_corner1 = normals->GetItem(corner1 * 3 + 0);
								Float ny_corner1 = normals->GetItem(corner1 * 3 + 1);
								Float nz_corner1 = normals->GetItem(corner1 * 3 + 2);

								Float nx_corner2 = normals->GetItem(corner2 * 3 + 0);
								Float ny_corner2 = normals->GetItem(corner2 * 3 + 1);
								Float nz_corner2 = normals->GetItem(corner2 * 3 + 2);

								// With the barycentric coordinates we can interpolate to a precise texture normal value in the triangle
								vertex.m_normal[0] = barycentric_x_normalized * nx_corner0 + barycentric_y_normalized * nx_corner1 + barycentric_z_normalized * nx_corner2;
								vertex.m_normal[1] = barycentric_x_normalized * ny_corner0 + barycentric_y_normalized * ny_corner1 + barycentric_z_normalized * ny_corner2;
								vertex.m_normal[2] = barycentric_x_normalized * nz_corner0 + barycentric_y_normalized * nz_corner1 + barycentric_z_normalized * nz_corner2;
								vertex.m_normal[3] = 0.0f;
							}

							{
								// Use normals to sample the texture
								spRealArray tangents = geometry->GetTangents( 0 );

								Float tx_corner0 = tangents->GetItem(corner0 * 3 + 0);
								Float ty_corner0 = tangents->GetItem(corner0 * 3 + 1);
								Float tz_corner0 = tangents->GetItem(corner0 * 3 + 2);

								Float tx_corner1 = tangents->GetItem(corner1 * 3 + 0);
								Float ty_corner1 = tangents->GetItem(corner1 * 3 + 1);
								Float tz_corner1 = tangents->GetItem(corner1 * 3 + 2);

								Float tx_corner2 = tangents->GetItem(corner2 * 3 + 0);
								Float ty_corner2 = tangents->GetItem(corner2 * 3 + 1);
								Float tz_corner2 = tangents->GetItem(corner2 * 3 + 2);

								// With the barycentric coordinates we can interpolate to a precise texture tangent value in the triangle
								vertex.m_tangent[0] = barycentric_x_normalized * tx_corner0 + barycentric_y_normalized * tx_corner1 + barycentric_z_normalized * tx_corner2;
								vertex.m_tangent[1] = barycentric_x_normalized * ty_corner0 + barycentric_y_normalized * ty_corner1 + barycentric_z_normalized * ty_corner2;
								vertex.m_tangent[2] = barycentric_x_normalized * tz_corner0 + barycentric_y_normalized * tz_corner1 + barycentric_z_normalized * tz_corner2;
							}

							{
								spRealArray colors = geometry->GetColors( 0 );

								spRealData col0 = m_simplygon->CreateRealData();
								colors->GetTuple( (rid)corner0, col0 );

								spRealData col1 = m_simplygon->CreateRealData();
								colors->GetTuple( (rid)corner1, col1 );

								spRealData col2 = m_simplygon->CreateRealData();
								colors->GetTuple( (rid)corner2, col2 );

								// With the barycentric coordinates we can interpolate to a precise vertex color value in the triangle
								vertex.m_color[0] = barycentric_x_normalized * col0[0] + barycentric_y_normalized * col1[0] + barycentric_z_normalized * col2[0];
								vertex.m_color[1] = barycentric_x_normalized * col0[1] + barycentric_y_normalized * col1[1] + barycentric_z_normalized * col2[1];
								vertex.m_color[2] = barycentric_x_normalized * col0[2] + barycentric_y_normalized * col1[2] + barycentric_z_normalized * col2[2];
								vertex.m_color[3] = barycentric_x_normalized * col0[3] + barycentric_y_normalized * col1[3] + barycentric_z_normalized * col2[3];
							}

							geomVstreamMap[mapped_geometry_id].PushBack( vertex );
						}
					}
				}
			}

			// Unlock the chunk of image data
			chunked_image_data->UnlockChunk2D(x_chunk, y_chunk);
		}

		for( Uint32 i = 0; i < geometryCount; ++i )
		{
			TDynArray< IntermediateVStream >& vstream = geomVstreamMap[i];
			if( vstream.Size() == 0 )
				continue;

			// Fake windows HACK
			IMaterialDefinition* materialDef = m_geometryMtlMap[i]->GetMaterialDefinition();
			String name = materialDef->GetFriendlyName();
			if( name.ContainsSubstring( TXT("pbr_fake_interior") ) )
			{
				for( auto v : vstream )
				{
					Uint32 pixel_index =  v.m_position[0] + v.m_position[1] * width;
					samples_used[pixel_index] = 128;
				}
				vstream.ClearFast();
				continue;
			}

			GpuApi::BufferInitData data;	
			data.m_buffer = vstream.Data();
			data.m_elementCount = vstream.Size();
			GpuApi::BufferRef vb = GpuApi::CreateBuffer( vstream.Size() * sizeof( IntermediateVStream ), GpuApi::BCC_Structured, GpuApi::BUT_Immutable, 0, &data );
			if( !vb )
			{
				vstream.ClearFast();
				RED_HALT( "Couldn't create vertex buffer for cast material" );
				continue;
			}
			GpuApi::BindBufferSRV( vb, 0, GpuApi::VertexShader );

			GRender->CastMaterialToTexture( vstream.Size(), width, height, m_geometryMtlMap[i], m_settings.m_showDistance );

			GpuApi::SafeRelease(vb);
			vstream.ClearFast();
		}
	}
	GRender->SetAsyncCompilationMode( asyncCompilationMode );

	Uint32 pitch32 = width * 16;
	Float* outData = static_cast< Float* >( RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_Temporary, pitch32 * height ) );
	GpuApi::GrabTexturePixels( texRef, 0, 0, width, height, outData );

	GpuApi::SafeRelease( texRef );
	
	// Resolve
	for( Uint32 y = 0; y < height; ++y )
	{
		for( Uint32 x = 0; x < width; ++x )
		{
			Uint32 pixel_index =  x + y * width;
			Vector* pixelPtr = ((Vector*)( outData + 4 * pixel_index ));
			if( samples_used[pixel_index] > 0 && samples_used[pixel_index] < 100 )
			{
				Vector pixel = *pixelPtr;

				Float s = (Float)samples_used[pixel_index];
				*pixelPtr = Vector(pixel.X/s, pixel.Y/s, pixel.Z/s, pixel.W );
			}
			else
			{
				*pixelPtr = Vector(0,0,0,1);
			}
		}
	}

#define MAX_DILATION 10
	Uint32 maxDilation = 0;
	Uint32 tmpSize = width;
	while( tmpSize > 1 )
	{
		maxDilation++;
		tmpSize = tmpSize >> 1;
	}
	Uint32 dilation = Min( m_settings.m_dilation, maxDilation );

	// UV Dilation
	Float* outDataDilate[MAX_DILATION];
	Uint8* samplesUsedDilate[MAX_DILATION];
	outDataDilate[0] = outData;
	samplesUsedDilate[0] = samples_used;

	Uint32 prevWidth = width;
	Uint32 prevHeight = height;
	Uint32 prevPitch32 = prevWidth * 16;
	for( Uint32 m = 1; m < dilation; ++m )
	{
		Uint32 newWidth = prevWidth / 2;
		Uint32 newHeight = prevHeight / 2;
		Uint32 newPitch32 = newWidth * 16;
		outDataDilate[m] = static_cast< Float* >( RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_Temporary, newPitch32 * newHeight ) );
		samplesUsedDilate[m] = static_cast< Uint8* >( RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_Temporary, newWidth * newHeight ) );
		for( Uint32 y = 0; y < prevHeight; y+=2 )
		{
			for( Uint32 x = 0; x < prevWidth; x+=2 )
			{
				Vector pixel(0,0,0,0);
				Uint32 samplesUsed = 0;
				for( Uint32 j = 0; j < 2; ++j )
				{
					for( Uint32 i = 0; i < 2; ++i )
					{
						Uint32 pixel_index = (x+i) + (y+j) * prevWidth;
						if( samplesUsedDilate[m-1][pixel_index] > 0 )
						{
							samplesUsed++;
							pixel += *((Vector*)( outDataDilate[m-1] + 4 * pixel_index ));
						}
					}
				}
			
				Uint32 pixel_index_half = ( x + y * newWidth ) / 2;
				Vector* pixelPtr = ((Vector*)( outDataDilate[m] + 4 * pixel_index_half ));
				if( samplesUsed > 0 )
				{
					*pixelPtr = pixel / (Float)samplesUsed;
					samplesUsedDilate[m][pixel_index_half] = samplesUsed;
				}
				else
				{
					*pixelPtr = Vector(0,0,0,1);
					samplesUsedDilate[m][pixel_index_half] = 0;
				}
			}
		}

		prevWidth = newWidth;
		prevHeight = newHeight;
	}

	for( Int32 m = dilation-2; m >= 0 ; m-- )
	{
		Uint32 newWidth = prevWidth * 2;
		Uint32 newHeight = prevHeight * 2;

		for( Uint32 y = 0; y < newHeight; y+=2 )
		{
			for( Uint32 x = 0; x < newWidth; x+=2 )
			{
				for( Uint32 j = 0; j < 2; ++j )
				{
					for( Uint32 i = 0; i < 2; ++i )
					{
						Uint32 pixel_index = (x+i) + (y+j) * newWidth;
						if( samplesUsedDilate[m][pixel_index] == 0 )
						{
							Vector* pixelPtr = ((Vector*)( outDataDilate[m] + 4 * pixel_index ));

							Uint32 pixel_index_half = ( x + y * prevWidth ) / 2;
							*pixelPtr = *((Vector*)( outDataDilate[m+1] + 4 * pixel_index_half ));
						}
					}
				}
			}
		}

		RED_MEMORY_FREE( MemoryPool_Default, MC_Temporary, outDataDilate[m+1] );
		RED_MEMORY_FREE( MemoryPool_Default, MC_Temporary, samplesUsedDilate[m+1] );

		prevWidth = newWidth;
		prevHeight = newHeight;
	}

	// Pack to 8-bit channels
	Uint32 pitch8 = width * 4;
	Uint8* resolvedData = static_cast< Uint8* >( RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_Temporary, pitch8 * height ) );
	for( Uint32 y = 0; y < height; ++y )
	{
		for( Uint32 x = 0; x < width; ++x )
		{
			Uint32 pixel_index =  x + y * width;
			Vector pixel = *((Vector*)( outData + 4 * pixel_index ));

			Uint32* rgba = (Uint32*)( resolvedData + 4 * pixel_index );
			*rgba = Color( pixel ).ToUint32();
		}
	}

	RED_MEMORY_FREE( MemoryPool_Default, MC_Temporary, outData );
	RED_MEMORY_FREE( MemoryPool_Default, MC_Temporary, samples_used );

	CSourceTexture* sourceTexture = CreateObject< CSourceTexture >();
	sourceTexture->Init( width, height, TRF_TrueColor );
	sourceTexture->CreateFromRawData( resolvedData, width, height, pitch8 );

	RED_MEMORY_FREE( MemoryPool_Default, MC_Temporary, resolvedData );

	CBitmapTexture* texture = CreateObject< CBitmapTexture >();
	// debug for no compression
	//texture->InitFromSourceData( sourceTexture, RED_NAME( SystemNoMips ) );
	texture->InitFromSourceData( sourceTexture, RED_NAME( DiffuseNoMips ) );

	return texture;
}

Bool CEdEntityMeshGenerator::CastTextures( spMappingImage mappingImage )
{
	GFeedback->UpdateTaskInfo( TXT("Casting textures") );

	//// Save out a texture for each channel.
	const Uint32 numChannels = m_textureNames.Size();
	for ( Uint32 i = 0; i < numChannels; ++i )
	{
		const StringAnsi& name = m_textureNames[ i ];

		if ( name == SG_MATERIAL_CHANNEL_DIFFUSE )
		{
			// HACK : For diffuse, we need to handle a few special cases.
			CBitmapTexture* tex = CastDiffuse( mappingImage );
			tex->AddToRootSet();
			m_generatedTextures.Insert( name, tex );
		}
		else if ( name == SG_MATERIAL_CHANNEL_NORMALS )
		{
			spImageData normImage = m_simplygon->CreateImageData();
			spImageData roughImage = m_simplygon->CreateImageData();

			{
				spNormalCaster cast = m_simplygon->CreateNormalCaster();
				cast->SetSourceMaterials( m_sourceMaterialTable );
				cast->SetSourceTextures( m_sourceTextureTable );
				cast->SetMappingImage( mappingImage );
				cast->SetOutputChannels( 3 );
				cast->SetOutputChannelBitDepth( 8 );
				cast->SetDilation( m_settings.m_dilation );
				cast->SetFlipGreen( flipNormalsGreen );
				cast->SetGenerateTangentSpaceNormals( generateTangentSpace );
				cast->SetOutputImage( normImage );
				cast->SetFlipBackfacingNormals( flipBackfacing );
				cast->CastMaterials();
			}

			{
				spColorCaster cast = m_simplygon->CreateColorCaster();
				cast->SetColorType( SG_MATERIAL_CHANNEL_ROUGHNESS );
				cast->SetSourceMaterials( m_sourceMaterialTable );
				cast->SetSourceTextures( m_sourceTextureTable );
				cast->SetMappingImage( mappingImage );
				cast->SetOutputChannels( 1 );
				cast->SetOutputChannelBitDepth( 8 );
				cast->SetDilation( m_settings.m_dilation );
				cast->SetOutputImage( roughImage );
				cast->SetIsSRGB( false );
				cast->CastMaterials();
			}

			CBitmapTexture* tex = CreateBitmapFromImageData( m_simplygon, normImage, roughImage, RED_NAME( NormalsNoMips ) );
			tex->AddToRootSet();
			m_generatedTextures.Insert( name, tex );
		}
		else
		{
			spImageData image = m_simplygon->CreateImageData();

			spColorCaster cast = m_simplygon->CreateColorCaster();
			cast->SetColorType( name.AsChar() );
			cast->SetSourceMaterials( m_sourceMaterialTable );
			cast->SetSourceTextures( m_sourceTextureTable );
			cast->SetMappingImage( mappingImage );
			cast->SetOutputChannels( 4 );
			cast->SetOutputChannelBitDepth( 8 );
			cast->SetDilation( m_settings.m_dilation );
			cast->SetOutputImage( image );
			cast->CastMaterials();

			CBitmapTexture* tex = CreateBitmapFromImageData( m_simplygon, image, nullptr, RED_NAME( DiffuseNoMips ) );
			tex->AddToRootSet();
			m_generatedTextures.Insert( name, tex );
		}
	}

	return true;
}


Bool CEdEntityMeshGenerator::BuildMeshInfo( CMesh::FactoryInfo& outFactoryInfo )
{
	outFactoryInfo.m_buildConvexCollision = false;
	outFactoryInfo.m_importCollision = false;
	outFactoryInfo.m_entityProxy = true;

	// New mesh has an empty LOD level 0. This makes it so the mesh will not be visible up close (the entity's normal meshes are used).
	if ( m_settings.m_showDistance > 0 )
	{
		CMesh::LODLevel* nullLodLevel = new (outFactoryInfo.m_lodLevels) CMesh::LODLevel();
	}
	// Add a second LOD with the show distance. This LOD will be visible from this distance and further away.
	CMesh::LODLevel* lodLevel = new (outFactoryInfo.m_lodLevels) CMesh::LODLevel();
	lodLevel->m_meshTypeLOD.m_distance = m_settings.m_showDistance;

	outFactoryInfo.m_materialNames.PushBack( TXT("ProxyMaterial") );


	GFeedback->UpdateTaskInfo( TXT("Creating mesh") );
	GFeedback->UpdateTaskProgress( 0, 1 );

	// create chunks for the new LOD level
	TDynArray< spSceneNode > nodesToProcess;
	nodesToProcess.PushBack( m_scene->GetRootNode() );

	while ( !nodesToProcess.Empty() )
	{
		spSceneNode thisNode = nodesToProcess.PopBackFast();

		// Dig down into children.
		const Uint32 numChildren = thisNode->GetChildCount();
		for ( Uint32 i = 0; i < numChildren; ++i )
		{
			nodesToProcess.PushBack( thisNode->GetChild( i ) );
		}


		spSceneMesh asMesh = SimplygonSDK::SafeCast< ISceneMesh >( thisNode );
		if ( asMesh == nullptr )
		{
			continue;
		}


		lodLevel->m_chunks.PushBack( static_cast< Uint16 >( outFactoryInfo.m_chunks.Size() ) );

		spGeometryData geom = asMesh->GetGeometry();
		spPackedGeometryData packed_geom = geom->NewPackedCopy();

		// We use 16-bit index buffer, so make sure we don't have too many vertices!
		if ( packed_geom->GetVertexCount() > 65536 )
		{
			m_errorMessage = String::Printf( TXT("Generated mesh has too many vertices (%u). Try decreasing \"Pixel size\" or increasing \"Merge distance\"."), packed_geom->GetVertexCount() );
			return false;
		}

		const Uint32 numVertices = packed_geom->GetVertexCount();
		const Uint32 numIndices = packed_geom->GetTriangleCount() * 3;

		if ( numVertices > 0 && numIndices > 0 )
		{
			outFactoryInfo.m_chunks.Grow();
			SMeshChunk& newChunk = outFactoryInfo.m_chunks.Back();

			newChunk.m_numVertices = numVertices;
			newChunk.m_numIndices = numIndices;

			// We assume we only need a single material for the remeshed geometry. This seems to be the case!
			newChunk.m_materialID = 0;

			// TODO : For now, we're only dealing with static meshes. In the future, it might be useful to be able
			// to create simplified proxies for characters too.
			newChunk.m_vertexType = MVT_StaticMesh;

			// we have to fix the winding on the packed geometry
			if (fixWinding)
			{
				spRidArray vertex_ids = packed_geom->GetVertexIds();
				for( Uint32 t = 0; t < packed_geom->GetTriangleCount(); t++ )
				{
					rid corner1 = vertex_ids->GetItem(t*3+1);
					rid corner2 = vertex_ids->GetItem(t*3+2);

					vertex_ids->SetItem(t*3+1, corner2);
					vertex_ids->SetItem(t*3+2, corner1);
				}
			}

			spRidArray packedvertex_ids		= packed_geom->GetVertexIds();
			spRealArray packedcoords		= packed_geom->GetCoords();
			spRealArray packedtexcoords0	= packed_geom->GetTexCoords( 0 );
			spRealArray packedtexcoords1	= packed_geom->GetTexCoords( 1 );
			spRealArray packednormals		= packed_geom->GetNormals();
			spRealArray packedtangents		= packed_geom->GetTangents( 0 );
			spRealArray packedbinormals		= packed_geom->GetBitangents( 0 );
			spRealArray packedcolors		= packed_geom->GetColors( 0 );
			spRealArray packedboneweights	= packed_geom->GetBoneWeights();
			spRidArray packedboneindices	= packed_geom->GetBoneIds();

			newChunk.m_vertices.Reserve(newChunk.m_numVertices);
			for (Uint32 vi = 0; vi < newChunk.m_numVertices; ++vi)
			{
				newChunk.m_vertices.Grow();
				SMeshVertex& vertex = newChunk.m_vertices.Back();

				spRealData pos = m_simplygon->CreateRealData();
				spRealData uv0 = m_simplygon->CreateRealData();
				spRealData uv1 = m_simplygon->CreateRealData();
				spRealData nrm = m_simplygon->CreateRealData();
				spRealData tan = m_simplygon->CreateRealData();
				spRealData bnr = m_simplygon->CreateRealData();
				spRealData col = m_simplygon->CreateRealData();
				spRealData whs = m_simplygon->CreateRealData();
				spRidData ids = m_simplygon->CreateRidData();

				// Most vertex attributes can be gotten directly.
				if ( packedcoords )
				{
					packedcoords->GetTuple( (rid)vi, pos );
					vertex.m_position[0] = pos[0];
					vertex.m_position[1] = pos[1];
					vertex.m_position[2] = pos[2];
				}
				
				if ( packedtexcoords0 )
				{
					packedtexcoords0->GetTuple( (rid)vi, uv0 );
					vertex.m_uv0[0] = uv0[0];
					vertex.m_uv0[1] = uv0[1];
				}

				if ( packedtexcoords1 )
				{
					packedtexcoords0->GetTuple( (rid)vi, uv1 );
					vertex.m_uv1[0] = uv1[0];
					vertex.m_uv1[1] = uv1[1];
				}

				if ( packednormals )
				{
					packednormals->GetTuple( (rid)vi, nrm );
					vertex.m_normal[0] = nrm[0];
					vertex.m_normal[1] = nrm[1];
					vertex.m_normal[2] = nrm[2];
				}

				if ( packedtangents )
				{
					packednormals->GetTuple( (rid)vi, tan );
					vertex.m_tangent[0] = tan[0];
					vertex.m_tangent[1] = tan[1];
					vertex.m_tangent[2] = tan[2];
				}

				if ( packedbinormals )
				{
					packednormals->GetTuple( (rid)vi, bnr );
					vertex.m_binormal[0] = bnr[0];
					vertex.m_binormal[1] = bnr[1];
					vertex.m_binormal[2] = bnr[2];
				}

				if ( packedboneweights )
				{
					packednormals->GetTuple( (rid)vi, whs );
					Uint32 itemcount = whs->GetItemCount();
					for ( Uint32 item =0; item < itemcount; ++item )
					{
						vertex.m_weights[item] = whs[item];
					}
				}

				// But there are a couple that simplygon uses a different data type for. These need to be converted.
				if ( packedboneindices )
				{
					packedboneindices->GetTuple( (rid)vi, ids );

					vertex.m_indices[0] = static_cast<Uint8>( ids[0] );
					vertex.m_indices[1] = static_cast<Uint8>( ids[1] );
					vertex.m_indices[2] = static_cast<Uint8>( ids[2] );
					vertex.m_indices[3] = static_cast<Uint8>( ids[3] );
				}

				if ( packedcolors )
				{
					packedcolors->GetTuple( (rid)vi, col );
					vertex.m_color = ((Uint32)(255.f*col[0]))<<24 | ((Uint32)(255.f*col[1]))<<16 | ((Uint32)(255.f*col[2]))<<8;
				}
			}

			newChunk.m_indices.Reserve(newChunk.m_numIndices);
			for (Uint32 ii = 0; ii < newChunk.m_numIndices; ++ii )
			{
				newChunk.m_indices.PushBack( (Uint16)packedvertex_ids->GetItem(ii) );
			}
		}
	}

	return true;
}


void CEdEntityMeshGenerator::CollectTextureNames( SimplygonSDK::spMaterialTable materialTable, TDynArray< StringAnsi >& outTextureNames )
{
	// Scan the materials we just created, and pull out all texture names.
	const Uint32 numMtls = materialTable->GetMaterialsCount();
	for ( Uint32 mtl_i = 0; mtl_i < numMtls; ++mtl_i )
	{
		spMaterial mtl = materialTable->GetMaterial( mtl_i );

		Bool didNormalRoughness = false;

		const Uint32 numChannels = mtl->GetChannelCount();
		for ( Uint32 channel = 0; channel < numChannels; ++channel )
		{
			StringAnsi channelName = mtl->GetChannelFromIndex( channel );

			// Don't do roughness, since it's combined with normals. Same with opacity, since it's in diffuse.
			if ( channelName == SG_MATERIAL_CHANNEL_ROUGHNESS || channelName == SG_MATERIAL_CHANNEL_OPACITY )
			{
				continue;
			}

			spImageData textureImage = mtl->GetTextureImage( channelName.AsChar() );
			spShadingNode shadingNode = mtl->GetShadingNetwork( channelName.AsChar() );
			if ( textureImage != nullptr || shadingNode != nullptr )
			{
				outTextureNames.PushBackUnique( channelName );
			}
		}
	}
}



CEdEntityMeshGenerator::CEdEntityMeshGenerator( CEntityTemplate* templ )
	: m_simplygon( nullptr )
	, m_scene( nullptr )
	, m_sourceMaterialTable( nullptr )
	, m_generatedMesh( nullptr )
	, m_entityTemplate( templ )
{
	Int32 res = SimplygonHelpers::InitSDK( m_simplygon );
	if ( res != SG_ERROR_NOERROR || m_simplygon == nullptr )
	{
		m_errorMessage = String::Printf( TXT("Unable to initialize Simplygon SDK; Code: %i; Msg: %s"), res, SimplygonHelpers::GetErrorText( res ) );
	}
}

CEdEntityMeshGenerator::~CEdEntityMeshGenerator()
{
	Reset();
	SimplygonHelpers::ShutdownSDK();
}


void CEdEntityMeshGenerator::Reset()
{
	m_errorMessage = String::EMPTY;

	if ( m_generatedMesh != nullptr )
	{
		m_generatedMesh->RemoveFromRootSet();
		m_generatedMesh = nullptr;
	}

	for ( auto iter = m_generatedTextures.Begin(); iter != m_generatedTextures.End(); ++iter )
	{
		iter->m_second->RemoveFromRootSet();
	}
	m_generatedTextures.ClearFast();

	for ( Uint32 i = 0; i < m_sourceMeshComponents.Size(); ++i )
	{
		m_sourceMeshComponents[ i ]->RemoveFromRootSet();
	}
	m_sourceMeshComponents.ClearFast();
	m_sourceMeshBounds.Clear();

	m_textureNames.ClearFast();

	if ( m_scene != nullptr )
	{
		m_scene->Clear();
		m_scene = nullptr;
	}
	m_sourceTextureTable = nullptr;
	m_sourceMaterialTable = nullptr;

	m_meshFactoryInfo = CMesh::FactoryInfo();

	m_materialTextures.ClearFast();

	m_originalGeometry.ClearFast();

	m_geometryMtlMap.ClearFast();
}


Float CEdEntityMeshGenerator::EstimateShowDistance()
{
	// Get the total bounding box of all meshes. This should be pretty close to what the proxy will have.
	Box entityBox( Box::RESET_STATE );
	for ( Uint32 i = 0; i < m_sourceMeshComponents.Size(); ++i )
	{
		CMeshComponent* meshComp = m_sourceMeshComponents[ i ];
		entityBox.AddBox( meshComp->GetBoundingBox() );
	}

	// Now find an appropriate show distance. The basic idea is that we pick a distance such that when the first of the gathered meshes
	// disappears, the proxy will then be shown.
	//
	// The simple (and not quite correct) way to do this would be to just take the minimum auto-hide distance. This isn't right, since
	// the mesh bounding boxes are not centered at the same point as the proxy's.
	//
	// So instead, we find how far away each mesh's "auto-hide reference" is from the proxy bounds, and subtract that from the mesh's
	// auto-hide distance before finding the minimum. This pulls the distance in a bit, so that we don't have spots where a mesh has
	// hidden, but the proxy isn't quite close enough.
	//
	// "auto-hide reference" is either the mesh's bounding box limits, or pivot point (if pivot is used for LOD calculations). We find
	// the maximum distance from the reference to the proxy bounds along each axis.


	// 2000 seems to be the distance used when AH is "infinite"
	Float showDistance = 2000.0f;

	for ( Uint32 i = 0; i < m_sourceMeshComponents.Size(); ++i )
	{
		CMeshComponent* meshComp = m_sourceMeshComponents[ i ];
		CMesh* mesh = meshComp->GetMeshNow();

		// Don't factor non-hiding meshes into this... We could just skip them entirely when building the proxy mesh, but really we
		// don't want meshes with infinite AH.
		if ( mesh->GetAutoHideDistance() <= 0.0f )
		{
			continue;
		}

		Float distFromBox = 0;
		
		const Box& compBox = meshComp->GetBoundingBox();
		Vector minDiff = compBox.Min - entityBox.Min;
		Vector maxDiff = entityBox.Max - compBox.Max;
		Vector maxDist = Vector::Max4( minDiff, maxDiff );
		distFromBox = Max( maxDist.X, maxDist.Y, maxDist.Z );		

		showDistance = Min( showDistance, mesh->GetAutoHideDistance() - distFromBox );
	}

	return showDistance;
}


Uint32 CEdEntityMeshGenerator::EstimateScreenSize( Float showDistance )
{
	// Screen resolution to project at
	Float testWidth = 1920.0f, testHeight = 1080.0f;

	Float fov = 75.0f;
	Float aspect = testWidth / testHeight;
	// Pick near/far. Doesn't really matter, just as long as showDistance is within that range.
	Float zNear = showDistance * 0.5f;
	Float zFar = showDistance * 2.0f;
	CRenderCamera cam( Vector( 0, 0, 0, 1 ), EulerAngles( 0, 0, 0 ), fov, aspect, zNear, zFar, 1.0f );

	// Figure out how big the scene is at the requested showDistance. This is just a rough approximation, but should give a decent value for
	// the screen size parameter.

	// Largely taken from meshBillboards.cpp

	// First, find out how big one pixel is, in world space, at the requested distance.
	Float pixelSize = 0.01f;
	{
		// Figure out appropriate projected depth value
		const Float zScale = ( zFar / ( zFar - zNear ) );
		const Float ppZ = zScale - ( ( 1.0f / showDistance ) * ( zNear * zScale ) );
		ASSERT( ppZ >= 0.0f && ppZ <= 1.0f );

		// Calculate coordinates for two pixels in screen space
		const Float dx = 1.0f / testWidth;
		Vector posA( 0,		0.0f, ppZ, 1.0f ); 
		Vector posB( dx,	0.0f, ppZ, 1.0f ); 

		// Project from screen space to world space
		Vector worldA = cam.GetScreenToView().TransformVectorWithW( posA );
		Vector worldB = cam.GetScreenToView().TransformVectorWithW( posB );
		worldA /= worldA.W;
		worldB /= worldB.W;

		// Check distance
		ASSERT( Abs< Float >( showDistance - worldA.Z ) < 0.1f );
		ASSERT( Abs< Float >( showDistance - worldB.Z ) < 0.1f );

		// Calculate pixel "width"
		pixelSize = worldA.DistanceTo( worldB );
	}

	Float screenSize = m_scene->GetRadius() / pixelSize;
	return ( Uint32 )Clamp( screenSize, 40.0f, 1200.0f );
}



Bool CEdEntityMeshGenerator::Prepare( CEntity* entity )
{
	// TODO : We shouldn't really need to do a full collect here. We just need to get a list of the textures that could
	// be generated, and an estimate of default screen size and show distance.

	// TODO : Also with this as-is, we're only checking the LOD at the default generate distance, so the set of textures
	// could potentially change later when we actually process things. Might be better to scan all materials of all meshes
	// here, to make sure we don't miss anything.

	if ( m_simplygon == nullptr )
	{
		m_errorMessage = TXT("Simplygon SDK not initialized. Do you have a proper license?");
		return false;
	}

	Reset();

	// Collect all meshes used by entity.
	for ( ComponentIterator< CMeshComponent > iter( entity ); iter; ++iter )
	{
		CMeshComponent* meshComp = *iter;
		if ( meshComp == nullptr )
		{
			continue;
		}

		CMesh* mesh = meshComp->GetMeshNow();
		if ( mesh == nullptr )
		{
			continue;
		}

		// Don't include entity proxy meshes! This prevents us from including an old version of the proxy in the new version.
		if ( mesh->IsEntityProxy() )
		{
			continue;
		}

		meshComp->AddToRootSet();
		m_sourceMeshComponents.PushBack( meshComp );
		m_sourceMeshBounds.AddBox( meshComp->GetBoundingBox() );
	}

	if ( !CollectScene() )
	{
		return false;
	}

	// Scan the materials we just created, and pull out all texture names.
	CollectTextureNames( m_sourceMaterialTable, m_textureNames );

	return true;
}



CEdEntityMeshGenerator::Settings CEdEntityMeshGenerator::ProvideSettings( const Settings& settings )
{
	m_settings = settings;

	if ( forcePower2 && m_settings.m_textureSize != 0 && !IsPow2( m_settings.m_textureSize ) )
	{
		m_settings.m_textureSize = RoundDownToPow2( m_settings.m_textureSize );
	}
	m_settings.m_multisampling = Clamp( m_settings.m_multisampling, 1u, 8u );
	m_settings.m_showDistance = Max( m_settings.m_showDistance, 0.0f );
	m_settings.m_generateDistance = Max( m_settings.m_generateDistance, 0.0f );
	m_settings.m_autohideDistance = Max( m_settings.m_autohideDistance, 0.0f );
	m_settings.m_screenSize = Clamp( m_settings.m_screenSize, 40u, 1200u );

	return m_settings;
}



Bool CEdEntityMeshGenerator::ReCollect( CEntity* entity )
{
	if ( m_simplygon == nullptr )
	{
		m_errorMessage = TXT("Simplygon SDK not initialized. Do you have a proper license?");
		return false;
	}

	Reset();

	// Collect all meshes used by entity.
	for ( ComponentIterator< CMeshComponent > iter( entity ); iter; ++iter )
	{
		CMeshComponent* meshComp = *iter;
		if ( meshComp == nullptr )
		{
			continue;
		}

		CMesh* mesh = meshComp->GetMeshNow();
		if ( mesh == nullptr )
		{
			continue;
		}

		// Don't include entity proxy meshes! This prevents us from including an old version of the proxy in the new version.
		if ( mesh->IsEntityProxy() )
		{
			continue;
		}

		// If the mesh is visible at the generate distance, include it.
		if ( mesh->GetAutoHideDistance() <= 0 || mesh->GetAutoHideDistance() > m_settings.m_generateDistance )
		{
			meshComp->AddToRootSet();
			m_sourceMeshComponents.PushBack( meshComp );
			m_sourceMeshBounds.AddBox( meshComp->GetBoundingBox() );
		}
	}

	if ( m_sourceMeshComponents.Empty() )
	{
		m_errorMessage = TXT("No meshes collected. Try decreasing the 'Generate distance' so it is within some meshes' auto-hide distances");
		return false;
	}

	if ( !CollectScene() )
	{
		return false;
	}

	// Scan the materials we just created, and pull out all texture names.
	CollectTextureNames( m_sourceMaterialTable, m_textureNames );

	return true;
}



Bool CEdEntityMeshGenerator::Process()
{
	if ( m_simplygon == nullptr )
	{
		m_errorMessage = TXT("Simplygon SDK not initialized. Do you have a proper license?");
		return false;
	}

	spMappingImage mappingImage = Remesh();
	if ( mappingImage == nullptr )
	{
		return false;
	}

	if ( !CastTextures( mappingImage ) )
	{
		return false;
	}

	if ( !BuildMeshInfo( m_meshFactoryInfo ) )
	{
		return false;
	}

	return true;
}


void CEdEntityMeshGenerator::CheckFilesExist( const String& directory, const String& baseName, const TDynArray< StringAnsi >& texturesToUse, TDynArray< String >& outFiles )
{
	// Check if the mesh already exists.
	String meshDepotPath = GetMeshPath( directory, baseName );
	if ( GDepot->FileExist( meshDepotPath ) )
	{
		outFiles.PushBack( meshDepotPath );
	}

	// Also check if textures exist.
	for ( Uint32 i = 0; i < texturesToUse.Size(); ++i )
	{
		String texDepotPath = GetTexturePath( directory, baseName, texturesToUse[i] );
		if ( GDepot->FileExist( texDepotPath ) )
		{
			outFiles.PushBack( texDepotPath );
		}
	}
}


Bool CEdEntityMeshGenerator::SaveFiles( const String& directory, const String& baseName, const TDynArray< StringAnsi >& texturesToUse )
{
	GFeedback->UpdateTaskInfo( TXT("Saving textures") );

	// First, save out the textures. This way, they'll have their correct paths for when we create and save the mesh.
	for ( Uint32 i = 0; i < texturesToUse.Size(); ++i )
	{
		GFeedback->UpdateTaskProgress( i, texturesToUse.Size() - 1 );

		const StringAnsi& texName = texturesToUse[ i ];

		CBitmapTexture* tex = nullptr;
		if ( !m_generatedTextures.Find( texName, tex ) )
		{
			RED_HALT( "Couldn't find '%" RED_PRIWas TXT("' in generated textures"), texName.AsChar() );
			return false;
		}
		if ( tex == nullptr )
		{
			RED_HALT( "No generated texture for %" RED_PRIWas, texName.AsChar() );
			return false;
		}

		String texDepotPath = GetTexturePath( directory, baseName, texName );
		CDiskFile* texFile = GDepot->FindFile( texDepotPath );
		if ( texFile != nullptr )
		{
			if ( !texFile->MarkModified() )
			{
				m_errorMessage = String::Printf( TXT("Cannot overwrite texture '%s'"), texDepotPath.AsChar() );
				return false;
			}

			// If it's already loaded, saving over it with a new texture will fail, so we need to replace the existing data.
			if ( texFile->GetResource() != nullptr )
			{
				CBitmapTexture* existingTex = SafeCast< CBitmapTexture >( texFile->GetResource() );

				// Copy tex to existingTex
				if ( !existingTex->InitFromCompressedMip( tex->GetMips()[0], tex->GetTextureGroupName(), tex->GetFormat() ) )
				{
					RED_HALT( "Couldn't initialize texture with generated mip" );
					return false;
				}

				// Switch tex to the existing texture. Make sure to update root set stuff!
				tex->RemoveFromRootSet();
				tex = existingTex;
				tex->AddToRootSet();

				// Replace in m_generatedTextures.
				m_generatedTextures.Set( texName, tex );
			}
		}

		CDirectory* texDir = GDepot->CreatePath( texDepotPath );
		if ( !tex->SaveAs( texDir, CFilePath( texDepotPath ).GetFileNameWithExt() ) )
		{
			m_errorMessage = String::Printf( TXT("Failed to save texture '%s'"), texDepotPath.AsChar() );
			return false;
		}
	}


	GFeedback->UpdateTaskInfo( TXT("Saving mesh") );

	// Create mesh. If file already exists, replace the existing resource.
	String meshDepotPath = GetMeshPath( directory, baseName );
	CDiskFile* meshFile = GDepot->FindFile( meshDepotPath );
	if ( meshFile != nullptr && meshFile->Load() )
	{
		if ( !meshFile->MarkModified() )
		{
			m_errorMessage = String::Printf( TXT("Cannot overwrite mesh '%s'"), meshDepotPath.AsChar() );
			return false;
		}
		m_meshFactoryInfo.m_reuse = SafeCast< CMesh >( meshFile->GetResource() );
	}
	else
	{
		m_meshFactoryInfo.m_reuse = nullptr;
	}

	m_generatedMesh = CMesh::Create( m_meshFactoryInfo );
	if ( m_generatedMesh == nullptr )
	{
		RED_HALT( "Couldn't create mesh" );
		return false;
	}

	// Make sure it doesn't get GC'd
	m_generatedMesh->AddToRootSet();

	m_generatedMesh->SetAutoHideDistance( m_settings.m_autohideDistance );


	// Create default material for the generated mesh. By default, we'll use pbr_std, taking Diffuse and Normals textures
	// from the generated textures.
	{
		const CMeshTypeResource::TMaterials& materials = m_generatedMesh->GetMaterials();
		RED_ASSERT( materials.Size() == 1, TXT("Unexpected number of materials %u. Expected 1."), materials.Size() );
		if ( materials.Size() == 0 )
		{
			return false;
		}

		THandle< ::IMaterial > mtl = materials[0];
		CMaterialInstance* instance = Cast< CMaterialInstance >( mtl.Get() );
		if ( instance == nullptr )
		{
			// Create instance
			instance = CreateObject< CMaterialInstance >( m_generatedMesh );
			// Set instance on mesh material (terribly hacky)
			const_cast< CMeshTypeResource::TMaterials& >( materials )[ 0 ] = instance;
		}

		// Set pbr_std base material
		CMaterialGraph* pbr = LoadResource< CMaterialGraph >( PROXYMESH_DEFAULT_MATERIAL );
		RED_ASSERT( pbr != nullptr, TXT("Can't load material graph '%s'"), PROXYMESH_DEFAULT_MATERIAL );
		instance->SetBaseMaterial( pbr );

		// Assign textures
		CBitmapTexture* diffuseTexture = nullptr;
		if ( m_generatedTextures.Find( "Diffuse", diffuseTexture ) && diffuseTexture != nullptr && texturesToUse.Exist( "Diffuse" ) )
		{
			instance->WriteParameter( RED_NAME( Diffuse ), THandle< CBitmapTexture >( diffuseTexture ) );
		}
		CBitmapTexture* normalsTexture = nullptr;
		if ( m_generatedTextures.Find( "Normals", normalsTexture ) && normalsTexture != nullptr && texturesToUse.Exist( "Normals" ) )
		{
			instance->WriteParameter( RED_NAME( Normal ),  THandle< CBitmapTexture >( normalsTexture ) );
		}
	}


	// Now that the textures are saved, we can save the mesh. Should do it in this order, because the mesh probably references
	// some textures in its material.
	// We can safely save the new mesh, even if the file already exists, because it hasn't been loaded.
	CDirectory* meshDir = GDepot->CreatePath( meshDepotPath );
	if ( !m_generatedMesh->SaveAs( meshDir, CFilePath( meshDepotPath ).GetFileNameWithExt() ) )
	{
		m_errorMessage = String::Printf( TXT("Failed to save mesh '%s'"), meshDepotPath.AsChar() );
		return false;
	}


	return true;
}

#endif


//////////////////////////////////////////////////////////////////////////


BEGIN_EVENT_TABLE( CEdEntityMeshGeneratorSettingsDlg, wxDialog )
	EVT_BUTTON( XRCID("generate"), CEdEntityMeshGeneratorSettingsDlg::OnGenerate )
	EVT_BUTTON( XRCID("cancel"), CEdEntityMeshGeneratorSettingsDlg::OnCancel )
	EVT_CHOICE( XRCID("textureSize"), CEdEntityMeshGeneratorSettingsDlg::OnTextureSizeChanged )
END_EVENT_TABLE();

CEdEntityMeshGeneratorSettingsDlg::CEdEntityMeshGeneratorSettingsDlg( wxWindow *parent )
{
	wxXmlResource::Get()->LoadDialog( this, parent, wxT("EntityMeshGenSettings") );
	XRCCTRL( *this, "sizeWarning", wxStaticText )->Show( false );
}

void CEdEntityMeshGeneratorSettingsDlg::OnCancel( wxCommandEvent& event )
{
	EndDialog( wxID_CANCEL ); 
}

void CEdEntityMeshGeneratorSettingsDlg::OnGenerate( wxCommandEvent& event )
{
	EndDialog( wxID_OK ); 
}


void CEdEntityMeshGeneratorSettingsDlg::OnTextureSizeChanged( wxCommandEvent& event )
{
	const Bool isLarge = GetSelectedTextureSize() > 1024;
	wxStaticText* warningText = XRCCTRL( *this, "sizeWarning", wxStaticText );
	warningText->Show( isLarge );
	warningText->GetParent()->GetSizer()->Layout();
}



String CEdEntityMeshGeneratorSettingsDlg::GetOutputFolder() const
{
	return String( XRCCTRL( *this, "outputFolder", wxTextCtrl )->GetValue().c_str() );
}

void CEdEntityMeshGeneratorSettingsDlg::SetOutputFolder( const String& path )
{
	XRCCTRL( *this, "outputFolder", wxTextCtrl )->SetValue( path.AsChar() );
}


String CEdEntityMeshGeneratorSettingsDlg::GetBaseName() const
{
	return String( XRCCTRL( *this, "baseName", wxTextCtrl )->GetValue().c_str() );
}

void CEdEntityMeshGeneratorSettingsDlg::SetBaseName( const String& baseName )
{
	XRCCTRL( *this, "baseName", wxTextCtrl )->SetValue( baseName.AsChar() );
}

void CEdEntityMeshGeneratorSettingsDlg::AddTexture( const StringAnsi& name, Bool saveByDefault )
{
	wxCheckListBox* texList = XRCCTRL( *this, "textures", wxCheckListBox );
	Int32 idx = texList->Append( name.AsChar() );
	texList->Check( idx, saveByDefault );
}

Bool CEdEntityMeshGeneratorSettingsDlg::GetTextureSave( const StringAnsi& name ) const
{
	wxCheckListBox* texList = XRCCTRL( *this, "textures", wxCheckListBox );
	Int32 idx = texList->FindString( name.AsChar() );
	if ( idx == -1 )
	{
		return false;
	}

	return texList->IsChecked( idx );
}

void CEdEntityMeshGeneratorSettingsDlg::GetTexturesToSave( TDynArray< StringAnsi >& outTextures ) const
{
	wxCheckListBox* texList = XRCCTRL( *this, "textures", wxCheckListBox );

	for ( Uint32 i = 0; i < texList->GetCount(); ++i )
	{
		if ( texList->IsChecked( i ) )
		{
			outTextures.PushBackUnique( StringAnsi( texList->GetString( i ) ) );
		}
	}
}


Uint32 CEdEntityMeshGeneratorSettingsDlg::GetSelectedTextureSize() const
{
	const Uint32 sizes[] = { 0, 64, 128, 256, 512, 1024, 2048 };
	return sizes[ XRCCTRL( *this, "textureSize", wxChoice )->GetSelection() ];
}



CEdEntityMeshGenerator::Settings CEdEntityMeshGeneratorSettingsDlg::GetSettings() const
{
	CEdEntityMeshGenerator::Settings settings;

	settings.m_mergeDistance		= XRCCTRL( *this, "mergeDistance", wxSpinCtrl )->GetValue();
	settings.m_gutterSpace			= XRCCTRL( *this, "gutterSpace", wxSpinCtrl )->GetValue();
	settings.m_dilation				= XRCCTRL( *this, "dilation", wxSpinCtrl )->GetValue();
	settings.m_multisampling		= XRCCTRL( *this, "multisampling", wxSpinCtrl )->GetValue();
	settings.m_screenSize			= XRCCTRL( *this, "pixelSize", wxSpinCtrl )->GetValue();

	settings.m_textureSize			= GetSelectedTextureSize();

	String showDistanceStr			= XRCCTRL( *this, "showDistance", wxTextCtrl )->GetValue();
	FromString( showDistanceStr, settings.m_showDistance );

	String genDistanceStr			= XRCCTRL( *this, "genDistance", wxTextCtrl )->GetValue();
	FromString( genDistanceStr, settings.m_generateDistance );

	String autohideStr				= XRCCTRL( *this, "autohide", wxTextCtrl )->GetValue();
	FromString( autohideStr, settings.m_autohideDistance );

	settings.m_hardEdgeAngle		= XRCCTRL( *this, "hardEdgeAngle", wxSpinCtrl )->GetValue();
	settings.m_useCullingPlane		= XRCCTRL( *this, "useCullingPlane", wxCheckBox )->GetValue();
	settings.m_cullingPlaneOffset	= XRCCTRL( *this, "cullingPlaneOffset", wxSpinCtrl )->GetValue() / 100.f; // there is no way to use float spinctrl so the offset is given in cm

	return settings;
}

void CEdEntityMeshGeneratorSettingsDlg::SetSettings( const CEdEntityMeshGenerator::Settings& settings )
{
	XRCCTRL( *this, "mergeDistance", wxSpinCtrl )	->SetValue( settings.m_mergeDistance		);
	XRCCTRL( *this, "gutterSpace", wxSpinCtrl )		->SetValue( settings.m_gutterSpace			);
	XRCCTRL( *this, "dilation", wxSpinCtrl )		->SetValue( settings.m_dilation				);
	XRCCTRL( *this, "multisampling", wxSpinCtrl )	->SetValue( settings.m_multisampling		);
	XRCCTRL( *this, "pixelSize", wxSpinCtrl )		->SetValue( settings.m_screenSize			);

	const Uint32 sizes[] = { 0, 64, 128, 256, 512, 1024, 2048 };
	Int32 idx = ARRAY_COUNT_U32( sizes ) - 1;
	while ( idx > 0 )
	{
		if ( sizes[idx] <= settings.m_textureSize )
		{
			break;
		}
		--idx;
	}
	XRCCTRL( *this, "textureSize", wxChoice )		->SetSelection( idx );

	XRCCTRL( *this, "showDistance", wxTextCtrl )	->SetValue( ToString( settings.m_showDistance ).AsChar() );
	XRCCTRL( *this, "genDistance", wxTextCtrl )		->SetValue( ToString( settings.m_generateDistance ).AsChar() );
	XRCCTRL( *this, "autohide", wxTextCtrl )		->SetValue( ToString( settings.m_autohideDistance ).AsChar() );
}

////////////////////

BEGIN_EVENT_TABLE( CEdEntityProxySetupDlg, wxDialog )
	EVT_BUTTON( XRCID("process"), CEdEntityProxySetupDlg::OnProcess )
	EVT_BUTTON( XRCID("cancel"), CEdEntityProxySetupDlg::OnCancel )
	EVT_LISTBOX_DCLICK( XRCID("meshes"), CEdEntityProxySetupDlg::OnMouseDblClick )
	END_EVENT_TABLE();

CEdEntityProxySetupDlg::CEdEntityProxySetupDlg( CEdEntityEditor *parent )
	: m_parent( parent )
{
	wxXmlResource::Get()->LoadDialog( this, parent, wxT("EntityProxySetup") );	
	
	m_meshes = XRCCTRL( *this, "meshes", wxCheckListBox );
	m_proxy = XRCCTRL( *this, "proxy", wxCheckListBox );

	m_meshesStatus = XRCCTRL( *this, "meshesStatus", wxCheckListBox );
	m_proxyStatus = XRCCTRL( *this, "proxyStatus", wxCheckListBox );

	m_meshes->Clear();
	m_proxy->Clear();
}

void CEdEntityProxySetupDlg::OnCancel( wxCommandEvent& event )
{
	EndDialog( wxID_CANCEL ); 
}

void CEdEntityProxySetupDlg::OnMouseDblClick( wxCommandEvent& event)
{
	RED_ASSERT(m_meshes);
	Uint32 itemIndex = event.GetInt();

	if (m_meshes->IsSelected( itemIndex ) == true)
	{
		TDynArray< CComponent* > components;
		if (m_meshComponents[ itemIndex ] != nullptr )
		{
			CComponent* c = m_meshComponents[ itemIndex ]->AsComponent();
			components.PushBack(c);
			m_parent->SelectComponents( components );
		}
		EndDialog( wxID_CANCEL );				
	}
}

void CEdEntityProxySetupDlg::OnProcess( wxCommandEvent& event )
{	
	Uint32 numberOfProxiesSet = 0;
	Uint32 numberOfMeshesSet = 0;
	TDynArray<String> meshesProcessed;
	TDynArray<String> proxiesProcessed;

	Uint32 total = 1;

	GFeedback->BeginTask( TXT("Processing proxy..."), false );
	
	Float forceProxyDistance = 70.0f;
	String proxyShowDistanceStr = XRCCTRL( *this, "proxyShowDistance", wxTextCtrl )->GetValue();
	if( FromString<Float>( proxyShowDistanceStr, forceProxyDistance ) )
	{
		total = m_proxyMeshComponents.Size();

		for(Uint32 i=0; i<m_proxyMeshComponents.Size(); ++i)
		{		
			if( m_proxy->IsChecked( i ) )
			{
				CMesh* m = m_proxyMeshComponents[i]->GetMeshNow();

				if( m != nullptr )
				{
					CDiskFile* mFile = m->GetFile();
					if( mFile ) 
					{
						mFile->SilentCheckOut();

						const CMesh::TLODLevelArray& lods = m->GetMeshLODLevels();
						CMesh::LODLevel& proxyLod = (CMesh::LODLevel&) lods[1];
						proxyLod.m_meshTypeLOD.m_distance = forceProxyDistance;	

						numberOfProxiesSet++;
					}

					if( !m->Save() ) 
					{
						String insStr = TXT( "Failed to checkout and save: " );
						insStr += m_proxy->GetString( i );
						proxiesProcessed.PushBack( insStr );			
					}
				}
			}

			GFeedback->UpdateTaskProgress( i, total );
		}
	}

	GFeedback->EndTask();
	
	Float forceMeshDistance = 75.0f;
	String meshHideDistanceStr = XRCCTRL( *this, "meshHideDistance", wxTextCtrl )->GetValue();
	if( FromString<Float>( meshHideDistanceStr, forceMeshDistance ) )
	{
		total = m_meshComponents.Size();

		for(Uint32 i=0; i<m_meshComponents.Size(); ++i)
		{		
			if( m_meshes->IsChecked( i ) )
			{
				CMesh* m = m_meshComponents[i]->GetMeshNow();
				if( m != nullptr )
				{

					CDiskFile* mFile = m->GetFile();
					if( mFile ) 
					{
						mFile->SilentCheckOut();
												
						m->SetAutoHideDistance( forceMeshDistance );

						// need to check and re-scale the lods also
						const CMesh::TLODLevelArray& lods = m->GetMeshLODLevels();
						for(Uint32 j=0; j<lods.Size(); ++j)
						{
							CMesh::LODLevel& meshLod = (CMesh::LODLevel&) lods[j];
							if( meshLod.GetDistance() > forceMeshDistance )
							{
								meshLod.m_meshTypeLOD.m_distance = forceMeshDistance;	
							}							
						}						

						numberOfMeshesSet++;						

						if( !m->Save() ) 
						{
							String insStr = TXT( "Failed to checkout and save: " );
							insStr += (String)m_meshes->GetString( i );
							meshesProcessed.PushBack( insStr );			
						}
					}					
				}
			}
			GFeedback->UpdateTaskProgress( i, total );
		}
	}

	GFeedback->EndTask();

	CDrawableComponent::RecreateProxiesOfRenderableComponents();

	// Fill some summary
	wxNotebook* mainTab = XRCCTRL( *this, "mainTab", wxNotebook );	
	mainTab->SetSelection( 1 );
	
	if( proxiesProcessed.Size() == 0 )
	{		
		m_proxyStatus->Insert( TXT("All selected proxies saved!"), 0 );
	}
	else
	{
		for( Uint32 i=0; i<proxiesProcessed.Size(); ++i ) 
		{
			m_proxyStatus->Insert( proxiesProcessed[i].AsChar(), m_proxyStatus->GetCount() );
		}
	}
	

	if( meshesProcessed.Size() == 0 )
	{
		m_meshesStatus->Insert( TXT("All selected meshes saved!"), 0 );
	}
	else
	{
		for( Uint32 i=0; i<meshesProcessed.Size(); ++i ) 
		{
			m_meshesStatus->Insert( meshesProcessed[i].AsChar(), m_meshesStatus->GetCount() );
		}
	}	

	wxStaticText* statusProxySizer = XRCCTRL( *this, "statusProxySizer", wxStaticText );
	wxStaticText* statusMeshSizer = XRCCTRL( *this, "statusMeshSizer", wxStaticText );	

	statusProxySizer->SetLabel( ToString( numberOfProxiesSet ).AsChar() );
	statusMeshSizer->SetLabel( ToString( numberOfMeshesSet ).AsChar() );

	meshesProcessed.Clear();
	proxiesProcessed.Clear();

	//EndDialog( wxID_OK ); 
}

Bool CEdEntityProxySetupDlg::Insert( CMeshComponent* mc ) 
{
	CMesh* m = mc->GetMeshNow();

	if( m != nullptr )
	{
		String meshPath;
		Float meshAutohide = 0.0f;
		Bool isProxy = false;

		if( m->IsEntityProxy() )
		{				
			const CMesh::TLODLevelArray& lods = m->GetMeshLODLevels();				

			if( lods.Size() != 2 ) 
			{
				GFeedback->ShowError( TXT("Proxy mesh: %s, have invalid numner of lods, please fix it!"), mc->GetName().AsChar() );
				return false;
			}

			m_proxyMeshComponents.PushBack( mc );

			meshPath = mc->GetName();
			const CMesh::LODLevel& proxyLod = lods[1];
			meshAutohide = proxyLod.GetDistance();
			isProxy = true;
		}
		else
		{
			m_meshComponents.PushBack( mc );
			
			meshPath = mc->GetMeshResourcePath();
			meshAutohide = mc->GetAutoHideDistance();
			isProxy = false;
		}

		wxCheckListBox* current = isProxy ? m_proxy : m_meshes;	
		Uint32 count = current->GetCount();

		String listString = ToString( meshAutohide );
		listString += TXT("m : ") + meshPath;

		current->Insert( listString.AsChar(), count );
		current->Check( count );

		return true;
	}

	return false;
}
