/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../../common/core/importer.h"

#include "../../common/engine/swfResource.h"
#include "../../common/engine/swfTexture.h"

class CGFxExporter
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Engine );

private:
	String		m_tempPath;

public:
	enum EImageQuality
	{
		ImageQuality_Fast,
		ImageQuality_Normal,
		ImageQuality_Production,
		ImageQuality_Highest,
	};

	struct SExportOptions
	{
		String			m_gfxExportExePath;
		String			m_dumpDirectoryPath;
		String			m_sourceFilePath;
		String			m_linkageFilePath;
		EImageQuality	m_imageQuality;
		Bool			m_doFullExports;
		Bool			m_doSwfCompression;

		SExportOptions()
			: m_imageQuality( ImageQuality_Fast )
			, m_doFullExports( true )
			, m_doSwfCompression( true )
		{}
	};

	struct SExportInfo
	{
		DataBuffer								m_strippedSwf;
		SSwfHeaderInfo							m_header;
		TDynArray< SSwfFontDesc >				m_fonts;
		TDynArray< CSwfResource::TextureInfo >	m_textureInfos;
		String									m_linkageName;
		String									m_imageExportString;
		
		SExportInfo()
			: m_strippedSwf( TDataBufferAllocator< MC_BufferFlash >::GetInstance() )
		{}
	};

public:
	CGFxExporter();

	virtual Bool DoExport( const SExportOptions& exportOptions, SExportInfo& outExportInfo, Bool showFeedback = true );

private:
	Bool ImportSwfTexture( const String& absolutePath, CSwfResource::TextureInfo& outTextureInfo );

private:
#ifdef USE_SCALEFORM
	Bool GetSwfExportInfo( const String& absolutePath, TDynArray< SSwfFontDesc >& outSwfFontDescs, SSwfHeaderInfo& outSwfHeaderInfo );

	// Fails because of stuff like gfxfontlib not being present
	//Bool GetSwfImportInfos( const String& absolutePath, TDynArray< SSwfImportInfo >& outSwfImports );
#endif
};
