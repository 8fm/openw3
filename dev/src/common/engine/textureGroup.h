/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

//----------------------------------------

/// Type of texture compression
enum ETextureCompression : CEnum::TValueType
{
	TCM_None,
	TCM_DXTNoAlpha,
	TCM_DXTAlpha,
	TCM_RGBE,
	TCM_Normals,
	TCM_NormalsHigh,
	TCM_NormalsGloss,
	TCM_TileMap,
	TCM_DXTAlphaLinear,
	TCM_QualityR,
	TCM_QualityRG,
	TCM_QualityColor,
};

BEGIN_ENUM_RTTI( ETextureCompression );
	ENUM_OPTION( TCM_None );
	ENUM_OPTION( TCM_DXTNoAlpha );
	ENUM_OPTION( TCM_DXTAlpha );
	ENUM_OPTION( TCM_RGBE );
	ENUM_OPTION( TCM_Normals );
	ENUM_OPTION( TCM_NormalsHigh );
	ENUM_OPTION( TCM_NormalsGloss );
	ENUM_OPTION( TCM_DXTAlphaLinear );
	ENUM_OPTION( TCM_QualityR );
	ENUM_OPTION( TCM_QualityRG );
	ENUM_OPTION( TCM_QualityColor );
END_ENUM_RTTI();

/// Get image compression hint
GpuApi::EImageCompressionHint	GetImageCompressionHint( ETextureCompression compression );

//----------------------------------------

/// Texture content category
enum ETextureCategory : CEnum::TValueType
{
	// Generic (unclassified texture)
	eTextureCategory_Generic,

	// World texture
	eTextureCategory_World,

	// Scene (but not character related texture)
	eTextureCategory_Scene,

	// Character texture
	eTextureCategory_Characters,

	// Head texture
	eTextureCategory_Heads,


	eTextureCategory_MAX,
};

BEGIN_ENUM_RTTI( ETextureCategory );
	ENUM_OPTION_DESC( TXT("Generic"), eTextureCategory_Generic );
	ENUM_OPTION_DESC( TXT("World"), eTextureCategory_World );
	ENUM_OPTION_DESC( TXT("Scene"), eTextureCategory_Scene );
	ENUM_OPTION_DESC( TXT("Characters"), eTextureCategory_Characters );
	ENUM_OPTION_DESC( TXT("Heads"), eTextureCategory_Heads );
END_ENUM_RTTI();

//----------------------------------------

/// Texture group information
class TextureGroup
{
	DECLARE_RTTI_STRUCT( TextureGroup );

public:
	CName					m_groupName;		//!< Name of the group
	ETextureCompression		m_compression;		//!< Texture compression to use
	ETextureCategory		m_category;			//!< Texture content category
	Int32					m_maxSize;			//!< Maximum texture size to use
	Bool					m_isUser:1;			//!< User can select this texture group setting in the editor
	Bool					m_isStreamable:1;	//!< Can this type of textures be streamed
	Bool					m_isResizable:1;	//!< Can this type of textures be resized by skipping top mipmaps
	Bool					m_isDetailMap:1;	//!< Is this texture a detail map
	Bool					m_isAtlas:1;		//!< Is atlas texture
	Bool					m_hasMipchain:1;	//!< Whether we should generate a full mipchain for the texture
	Bool					m_highPriority:1;	//!< Texture is always a high priority for streaming

public:
	TextureGroup();
	~TextureGroup();
};

BEGIN_CLASS_RTTI( TextureGroup );
END_CLASS_RTTI();

//----------------------------------------
