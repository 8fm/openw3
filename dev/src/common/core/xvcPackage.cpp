/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "xvcPackage.h"
#include "../redSystem/crt.h"

//////////////////////////////////////////////////////////////////////////
// SXvcLocale
//////////////////////////////////////////////////////////////////////////
SXvcLocale::SXvcLocale()
{
	m_buf[0] = '\0';
}

SXvcLocale::SXvcLocale( const AnsiChar* name )
{
	Red::System::StringCopy( m_buf, name, ARRAY_COUNT(m_buf) );
	for ( AnsiChar* pch = m_buf; *pch; ++pch )
	{
		if ( *pch >= 'A' && *pch <= 'Z' )
		{
			*pch += static_cast< AnsiChar >('a' - 'A');
		}
	}
}

void SXvcLocale::RemoveRegionCode()
{
	AnsiChar* pch = Red::System::StringSearch( m_buf, '-' );
	if ( pch )
	{
		*pch = '\0';
	}
}

//////////////////////////////////////////////////////////////////////////
// Serialization
//////////////////////////////////////////////////////////////////////////
IFile& operator<<( IFile& ar, SXvcLocale& localeName )
{
	ar.Serialize( localeName.m_buf, ARRAY_COUNT(localeName.m_buf) );
	return ar;
}

IFile& operator<<( IFile& ar, SXvcPackage& package )
{
	ar << package.m_header;
	ar << package.m_languageLocaleMap;
	ar << package.m_chunkNameMap;
	ar << package.m_installOrderContent;
	ar << package.m_binChunkID;
	ar << package.m_supportedSpeechLanguages;
	ar << package.m_supportedTextLanguages;
	ar << package.m_defaultSpeechLanguage;
	ar << package.m_launchContentName;
	ar << package.m_patchLevel;
	return ar;
}

IFile& operator<<( IFile& ar, SXvcPackageHeader& header )
{
	ar << header.m_magic;
	ar << header.m_version;

	return ar;
}

IFile& operator<<( IFile& ar, SXvcContent& content )
{
	ar << content.m_contentName;
	ar << content.m_chunkID;
	ar << content.m_languageChunks;

	return ar;
}

IFile& operator<<( IFile& ar, SXvcLanguageChunk& chunk )
{
	ar << chunk.m_locale;
	ar << chunk.m_chunkID;

	return ar;
}

Bool SXvcPackage::Verify( const SXvcPackage& xvcPackage )
{
	const SXvcPackageHeader& packageHeader = xvcPackage.m_header;
	if ( packageHeader.m_magic != SXvcPackageHeader::MAGIC )
	{
		ERR_CORE(TXT("XVC package magic %u does not match expected %u"), packageHeader.m_magic, SXvcPackageHeader::MAGIC );
		return false;
	}

	if ( packageHeader.m_version != SXvcPackageHeader::VERSION )
	{
		ERR_CORE(TXT("XVC package version %u does not match expected %u"), packageHeader.m_version, SXvcPackageHeader::VERSION );
		return false;
	}

	if ( xvcPackage.m_binChunkID == INVALID_XVC_CHUNK_ID )
	{
		ERR_CORE(TXT("XVC binChunkID %u is invalid"), xvcPackage.m_binChunkID );
		return false;
	}

	return true;
}

#ifdef RED_LOGGING_ENABLED
void DumpXvcPackage( const SXvcPackage& package )
{
	LOG_CORE(TXT("---------------------"));
	LOG_CORE(TXT("]XVC package dump["));
	LOG_CORE(TXT("---------------------"));

	LOG_CORE(TXT("package.m_header.m_magic: 0x%08X (expect 0x%08X: %ls)"), package.m_header.m_magic, package.m_header.MAGIC, 
		(package.m_header.m_magic == package.m_header.MAGIC) ? TXT("match") : TXT("MISMATCH!"));

	LOG_CORE(TXT("package.m_header.m_version: %u (expect %u: %ls)"), package.m_header.m_version, package.m_header.VERSION, 
		(package.m_header.m_version == package.m_header.VERSION) ? TXT("match") : TXT("MISMATCH!"));

	LOG_CORE(TXT("package.m_languageLocaleMap:"));
	for ( auto it = package.m_languageLocaleMap.Begin(); it != package.m_languageLocaleMap.End(); ++it )
	{
		const StringAnsi& gameLang = it->m_first;
		const SXvcLocale& locale = it->m_second;
		LOG_CORE(TXT("\tgameLang {%hs} -> locale {%hs}"), gameLang.AsChar(), locale.AsChar() );
	}

	LOG_CORE(TXT("launch0: {%ls}"), package.m_launchContentName.AsChar() );
	LOG_CORE(TXT("package.m_installOrderContent (%u entries):"), package.m_installOrderContent.Size() );
	for ( const SXvcContent& content : package.m_installOrderContent )
	{
		LOG_CORE(TXT("\tcontent {%ls}, chunkID {%u}, numLangChunks {%u}"), content.m_contentName.AsChar(), content.m_chunkID, content.m_languageChunks.Size() );
		for ( const SXvcLanguageChunk& langChunk : content.m_languageChunks )
		{
			LOG_CORE(TXT("\t\tlanguage chunkID {%u}, locale {%hs}"), langChunk.m_chunkID, langChunk.m_locale.AsChar() );
		}
	}

	LOG_CORE(TXT("package.m_chunkNameMap (%u entries):"), package.m_chunkNameMap.Size() );
	for ( auto it = package.m_chunkNameMap.Begin(); it != package.m_chunkNameMap.End(); ++it )
	{
		const Uint32 chunkID = it->m_first;
		CName chunkName = it->m_second;
		LOG_CORE(TXT("\tchunkID {%u} -> chunkName {%ls}"), chunkID, chunkName.AsChar() );
	}

	LOG_CORE(TXT("package.m_supportedSpeechLanguages (%u entries):"), package.m_supportedSpeechLanguages.Size());
	for ( const StringAnsi& lang : package.m_supportedSpeechLanguages )
	{
		LOG_CORE(TXT("\tlang {%hs}"), lang.AsChar());
	}


	LOG_CORE(TXT("package.m_defaultSpeechLanguage: {%hs}"), package.m_defaultSpeechLanguage.AsChar() );
	LOG_CORE(TXT(" package.m_patchLevel: {%u}"), package.m_patchLevel );

	LOG_CORE(TXT("package.m_supportedTextLanguages (%u entries):"), package.m_supportedTextLanguages.Size());
	for ( const StringAnsi& lang : package.m_supportedTextLanguages )
	{
		LOG_CORE(TXT("\tlang {%hs}"), lang.AsChar());
	}
}
#endif