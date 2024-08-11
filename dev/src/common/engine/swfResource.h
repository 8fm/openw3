	/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#pragma once
#include "../core/resource.h"
#include "../core/cooker.h"
#include "../core/hashmap.h"
#include "../core/importer.h"

#include "swfTexture.h"

class CDependencyLoaderSwf;

//////////////////////////////////////////////////////////////////////////
// SSwfFontDesc
//////////////////////////////////////////////////////////////////////////
struct SSwfFontDesc
{
	DECLARE_RTTI_STRUCT( SSwfFontDesc );

	String		m_fontName;
	Uint32		m_numGlyphs;
	Bool		m_italic;
	Bool		m_bold;

	SSwfFontDesc( const String& fontName, Uint32 numGlyphs, Bool italic, Bool bold )
		: m_fontName( fontName )
		, m_numGlyphs( numGlyphs )
		, m_italic( italic )
		, m_bold( bold )
	{}

	SSwfFontDesc()
		: m_numGlyphs( 0 )
		, m_italic( false )
		, m_bold( false )
	{}
};

//////////////////////////////////////////////////////////////////////////
// SSwfHeaderInfo
//////////////////////////////////////////////////////////////////////////
struct SSwfHeaderInfo
{
	DECLARE_RTTI_STRUCT( SSwfHeaderInfo );

	Float	m_frameRate;
	Float	m_frameHeight;
	Float	m_frameWidth;
	Uint32	m_frameCount;
	Float	m_height;
	Float	m_width;
	Uint32	m_version;
	Bool	m_compressed;

	SSwfHeaderInfo()
		: m_frameRate( 0.f )
		, m_frameHeight( 0.f )
		, m_frameWidth( 0.f )
		, m_frameCount( 0 )
		, m_height( 0.f )
		, m_width( 0.f )
		, m_version( 0 )
		, m_compressed( false )
	{}
};

//////////////////////////////////////////////////////////////////////////
// CSwfResource
//////////////////////////////////////////////////////////////////////////
class CSwfResource : public CResource
{
	DECLARE_ENGINE_RESOURCE_CLASS( CSwfResource, CResource, "redswf", "Flash SWF" );

	friend class CDependencyLoaderSwf;

public:
	static const THandle< CSwfTexture >					NULL_SWF_TEXTURE_HANDLE;
	static const Char* RAW_SWF_LINKAGE_NAME;

private:
	String												m_linkageName;
	TDynArray< SSwfFontDesc >							m_fonts;
	TDynArray< THandle< CSwfTexture > >					m_textures;
	DataBuffer											m_dataBuffer;
	SSwfHeaderInfo										m_header;
	String												m_imageImportOptions;
	LatentDataBuffer									m_sourceSwf;
	
private:
	// Initialized OnPostLoad() and never resized afterwards, so can use for safely returning const references
	THashMap< String, THandle< CSwfTexture > >			m_textureMap;

public:
	CSwfResource();
	virtual ~CSwfResource();

public:
	virtual void							OnSerialize( IFile& file ) override;
	virtual void							OnPostLoad() override;
	virtual void							CleanupSourceData() override;

#ifndef NO_DATA_VALIDATION
	virtual void							OnCheckDataErrors() const override;
#endif

#ifndef NO_RESOURCE_COOKING
	virtual void							OnCook( ICookerFramework& cooker ) override;
#endif

public:
	virtual void							GetAdditionalInfo( TDynArray< String >& info ) const override;

public:
	const String&							GetLinkageName() const { return m_linkageName; }

public:
	const DataBuffer&						GetDataBuffer() const { return m_dataBuffer; }
	const TDynArray< SSwfFontDesc >&		GetFontDescs() const { return m_fonts; }
	const THandle< CSwfTexture >&			GetTextureExport( const String& textureLinkageName ) const;

public:
#ifndef NO_EDITOR
	const TDynArray< THandle< CSwfTexture > >&	GetTextures() const { return m_textures; }
#endif

public:
	LatentDataBuffer&						GetSourceSwf() { return m_sourceSwf; }

public:
	static Bool								VerifySwf( const DataBuffer& dataBuffer );
	static Bool								VerifySwf( const LatentDataBuffer& dataBuffer ); // LEGACY!!

public:
#ifndef NO_RESOURCE_IMPORT
	
	struct TextureInfo
	{
		String										m_linkageName;
		CSwfTexture::MipMap							m_mipMap;
		CName										m_textureGroupName;
		ETextureRawFormat							m_textureRawFormat;

		TextureInfo()
			: m_textureRawFormat( TRF_TrueColor )
		{}
	};

	struct FactoryInfo : public CResource::FactoryInfo< CSwfResource >
	{
		mutable DataBuffer								m_dataBuffer;
		mutable LatentDataBuffer						m_sourceSwf;
		String											m_linkageName;
		TDynArray< TextureInfo >						m_textureInfos;
		TDynArray< SSwfFontDesc >						m_fonts;
		SSwfHeaderInfo									m_header;
		String											m_imageImportOptions;

		FactoryInfo()
			: m_dataBuffer( TDataBufferAllocator< MC_BufferFlash >::GetInstance() )
			, m_sourceSwf( MC_BufferFlash )
		{}
	};

	struct ImportParams : public IImporter::ImportOptions::ImportParams
	{
		Bool m_noFeedback;

		ImportParams()
			: m_noFeedback( false )
		{}
	};

	struct CookInfo
	{
		mutable DataBuffer								m_dataBuffer;
		mutable TDynArray< TextureInfo >				m_textureInfos;

		CookInfo()
			: m_dataBuffer( TDataBufferAllocator< MC_BufferFlash >::GetInstance() )
		{}
	};

	static CSwfResource* Create( const FactoryInfo& data );
	static Bool			 SetCookedData( CSwfResource* swfResource, const CookInfo& data, ECookingPlatform cookingPlatform );

	CSwfTexture* GetThumbnailTextureSource() const;
#endif
};

//////////////////////////////////////////////////////////////////////////
// RTTI Boilerplate
//////////////////////////////////////////////////////////////////////////
BEGIN_CLASS_RTTI( CSwfResource );
	PARENT_CLASS( CResource );
	PROPERTY_RO( m_linkageName, TXT("Linkage name") );
	PROPERTY_RO( m_fonts, TXT("Fonts") );
	PROPERTY( m_textures );
	PROPERTY_RO_NOT_COOKED( m_header, TXT("Header") );
	PROPERTY_RO_NOT_COOKED( m_imageImportOptions, TXT("Image import options") );
END_CLASS_RTTI();

BEGIN_CLASS_RTTI( SSwfFontDesc );
	PROPERTY_RO( m_fontName, TXT("Font name") );
	PROPERTY_RO( m_numGlyphs, TXT("Number of glyph shapes") );
	PROPERTY_RO( m_italic, TXT("Italic") );
	PROPERTY_RO( m_bold, TXT("Bold") );
END_CLASS_RTTI();

BEGIN_CLASS_RTTI( SSwfHeaderInfo );
	PROPERTY_RO( m_frameRate, TXT("Frame rate") );
	PROPERTY_RO( m_frameHeight, TXT("Frame height") );
	PROPERTY_RO( m_frameWidth, TXT("Frame width") );
	PROPERTY_RO( m_frameCount, TXT("Frame count") );
	PROPERTY_RO( m_height, TXT("Height") );
	PROPERTY_RO( m_width, TXT("Width") )
	PROPERTY_RO( m_version, TXT("Version") );
	PROPERTY_RO( m_compressed, TXT("Compressed") );
END_CLASS_RTTI();
