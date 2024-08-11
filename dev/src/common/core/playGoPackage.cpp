/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

// Here instead of in streamingInstallerOrbis so I can extern it in without the unit tests failing to link because
// they don't link in platform
// Overrides any potential misbehaving PriorityIO(), useful for when we're blocking waiting to install from bluray during the loading screen
Bool GPlayGoFullInstallSpeedOverride;

#include "playGoPackage.h"

//////////////////////////////////////////////////////////////////////////
// Serialization
//////////////////////////////////////////////////////////////////////////
IFile& operator<<( IFile& ar, SPlayGoPackage& package )
{
	ar << package.m_header;
	ar << package.m_languageFlagMap;			
	ar << package.m_chunkNameMap;
	ar << package.m_installOrderContent;
	ar << package.m_supportedSpeechLanguages;	
	ar << package.m_supportedTextLanguages;	
	ar << package.m_defaultSpeechLanguage;
	ar << package.m_launchContentName;
	ar << package.m_patchLevel;

	return ar;
}

IFile& operator<<( IFile& ar, SPlayGoPackageHeader& header )
{
	ar << header.m_magic;
	ar << header.m_version;

	return ar;
}

IFile& operator<<( IFile& ar, SPlayGoContent& content )
{
	ar << content.m_contentName;
	ar << content.m_installOrderChunks;
	
	return ar;
}

IFile& operator<<( IFile& ar, SPlayGoChunk& chunk )
{
	ar << (Uint64&)chunk.m_languageFlag;
	ar << chunk.m_chunkID;

	return ar;
}

#ifdef RED_LOGGING_ENABLED
void DumpPlayGoPackage( const SPlayGoPackage& package )
{
	LOG_CORE(TXT("---------------------"));
	LOG_CORE(TXT("]PlayGo package dump["));
	LOG_CORE(TXT("---------------------"));

	LOG_CORE(TXT("package.m_header.m_magic: 0x%08X (expect 0x%08X: %ls)"), package.m_header.m_magic, package.m_header.MAGIC, 
		(package.m_header.m_magic == package.m_header.MAGIC) ? TXT("match") : TXT("MISMATCH!"));

	LOG_CORE(TXT("package.m_header.m_version: %u (expect %u: %ls)"), package.m_header.m_version, package.m_header.VERSION, 
		(package.m_header.m_version == package.m_header.VERSION) ? TXT("match") : TXT("MISMATCH!"));

	LOG_CORE(TXT("package.m_languageFlagMap:"));
	for ( auto it = package.m_languageFlagMap.Begin(); it != package.m_languageFlagMap.End(); ++it )
	{
		const StringAnsi& gameLang = it->m_first;
		Uint64 flag = it->m_second;
		LOG_CORE(TXT("\tgameLang {%hs} -> languagFlag {0x%016llX}"), gameLang.AsChar(), flag );
	}

	LOG_CORE(TXT("launch0: {%ls}"), package.m_launchContentName.AsChar() );
	LOG_CORE(TXT("package.m_installOrderContent (%u entries):"), package.m_installOrderContent.Size() );
	for ( const SPlayGoContent& content : package.m_installOrderContent )
	{
		LOG_CORE(TXT("\tcontent {%ls}, numChunks {%u}"), content.m_contentName.AsChar(), content.m_installOrderChunks.Size() );
		for ( const SPlayGoChunk& chunk : content.m_installOrderChunks )
		{
			LOG_CORE(TXT("\t\tchunkID {%u}, language mask {0x%016llX}"), chunk.m_chunkID, chunk.m_languageFlag );
		}
	}

	LOG_CORE(TXT("package.m_chunkNameMap (%u entries):"), package.m_chunkNameMap.Size() );
	for ( auto it = package.m_chunkNameMap.Begin(); it != package.m_chunkNameMap.End(); ++it )
	{
		const Uint32 chunkID = it->m_first;
		const CName chunkName = it->m_second;
		LOG_CORE(TXT("\tchunkID {%u} -> chunkName {%ls}"), chunkID, chunkName.AsChar());
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
#endif // RED_LOGGING_ENABLED
