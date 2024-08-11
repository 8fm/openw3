/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "patchBuilder.h"
#include "../../common/core/sharedPtr.h"
#include "../../common/core/file.h"
#include "../../common/engine/soundCacheDataFormat.h"

//-----------------------------------------------------------------------------

class CPatchSoundFileToken;

//-----------------------------------------------------------------------------

class CPatchSounds : public IBasePatchContentBuilder::IContentGroup
{
public:
	~CPatchSounds();

	virtual void GetTokens( TDynArray< IBasePatchContentBuilder::IContentToken* >& outTokens ) const override;
	virtual const Uint64 GetDataSize() const override;
	virtual const String GetInfo() const override;

	void AddToken( CPatchSoundFileToken* token );

private:
	TDynArray< CPatchSoundFileToken* > m_tokens;

};

//-----------------------------------------------------------------------------

class CSoundCacheSimpleReader
{
public:
	CSoundCacheSimpleReader( const String& filepath );

	Bool Load();

	TDynArray<CSoundCacheData::CacheToken>& GetTokens();
	Uint32 GetTokenCount();
	TDynArray<AnsiChar>& GetStrings();

	Red::TSharedPtr<IFile> GetFile();
	Red::TSharedPtr<CSoundCacheData> GetCacheData();

private:
	Bool LoadCacheData();
	Bool ValidateSoundCacheHeader();
	Bool CreateFileReader();

private:
	String m_filepath;
	Red::TSharedPtr<IFile> m_cacheFile;
	Red::TSharedPtr<CSoundCacheData> m_cacheData;
	Uint32 m_version;
};

//-----------------------------------------------------------------------------

class CSoundCachePatchTokenExctactor
{
public:
	CSoundCachePatchTokenExctactor();

	void SetSource( CSoundCacheSimpleReader* source );
	void ResetSource();
	void Extract( CPatchSounds* outTokens );

private:
	void GetTokenNameAndNameHash( const Uint32 tokenIndex, String& outName, Uint64& outNameHash );
	Bool CheckTokenAlreadyExtracted( const Uint64 tokenHash );
	CPatchSoundFileToken* CreatePatchToken(const Uint32 tokenIdx, const String& tokenName, const Uint64 soundCacheTokenHash);

	Uint64 GetTokenCrc(const Uint32 tokenIdx, const String &tokenName);
	CSoundCacheData::CacheToken& GetTokenData( const Uint32 tokenIndex );
	void ReadTokenContentToBuffer(const CSoundCacheData::CacheToken &cacheTokenData, TDynArray< Uint8 >& readBuffer);
	Uint64 CalculateTokenCrc(const String &tokenName, const TDynArray< Uint8 >& cacheTokenContent);

private:
	THashMap< Uint64, CPatchSoundFileToken* > m_fileTokensMap;
	CSoundCacheSimpleReader* m_source;
	TDynArray< Uint8 > m_readBuffer;

};

//-----------------------------------------------------------------------------

class CPatchSoundFileToken : public IBasePatchContentBuilder::IContentToken
{
public:
	CPatchSoundFileToken( const String& tokenName, Red::TSharedPtr<IFile> cacheFile, Red::TSharedPtr<CSoundCacheData> cacheData, Int32 cacheTokenIndex, Uint64 crc );
	~CPatchSoundFileToken();

	virtual const Uint64 GetTokenHash() const override;
	virtual const Uint64 GetDataCRC() const override;
	virtual const Uint64 GetDataSize() const override;
	virtual const String GetInfo() const override;
	virtual void DebugDump( const String& dumpPath, const Bool isBase ) const override;

public:
	String m_tokenName;
	Uint64 m_tokenHash;
	Uint64 m_cacheTokenCrc;
	Uint32 m_cacheTokenIndex;
	Red::TSharedPtr<IFile> m_cacheFile;
	Red::TSharedPtr<CSoundCacheData> m_cacheDataFormat;

};

//-----------------------------------------------------------------------------

class CPatchBuilder_Sounds : public IBasePatchContentBuilder
{
	DECLARE_RTTI_SIMPLE_CLASS( CPatchBuilder_Sounds );

public:
	virtual String GetContentType() const override;
	virtual IBasePatchContentBuilder::IContentGroup* LoadContent( const ECookingPlatform platform, const String& absoluteBuildPath )  override;
	virtual Bool SaveContent( const ECookingPlatform platform, IContentGroup* baseGroup, IContentGroup* patchGroup, TDynArray< IBasePatchContentBuilder::IContentToken* >& patchContent, const String& absoluteBuildPath, const String& patchName ) override;

private:
	Bool FillPatchSounds(CPatchSounds* patchSounds, const TDynArray<String>& soundCachePaths);

};

BEGIN_CLASS_RTTI( CPatchBuilder_Sounds );
	PARENT_CLASS( IBasePatchContentBuilder );
END_CLASS_RTTI();

//-----------------------------------------------------------------------------
