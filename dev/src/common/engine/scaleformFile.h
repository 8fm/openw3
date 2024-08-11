/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#ifdef USE_SCALEFORM

//////////////////////////////////////////////////////////////////////////
// CScaleformFileReaderWrapper
//////////////////////////////////////////////////////////////////////////
class CScaleformFileReaderWrapper : public SF::File
{
private:
	IFile*	m_file;
	StringAnsi m_filePath;

public:
	CScaleformFileReaderWrapper( const String& filePath, Bool buffered = true );
	CScaleformFileReaderWrapper( IFile* file );

	virtual ~CScaleformFileReaderWrapper();

public:
	virtual const SFChar* GetFilePath() override;

	virtual SFBool IsValid() override;
	virtual SFBool IsWritable() override;

	virtual SFInt Tell() override;
	virtual SF::SInt64 LTell () override;
	virtual SFInt GetLength() override;
	virtual SF::SInt64 LGetLength() override;
	virtual SFInt GetErrorCode() override;

	virtual SFInt Write(const SF::UByte *pbufer, SFInt numBytes) override;
	virtual SFInt Read(SF::UByte *pbufer, SFInt numBytes) override;

	virtual SFInt SkipBytes(SFInt numBytes) override;
	virtual SFInt BytesAvailable() override;
	virtual SFBool Flush() override;

	virtual SFInt Seek(SFInt offset, SFInt origin=Seek_Set) override;
	virtual SF::SInt64 LSeek(SF::SInt64 offset, SFInt origin=Seek_Set) override;

	virtual SFBool ChangeSize(int newSize) override;
	virtual SFInt CopyFromStream(SF::File *pstream, SFInt byteSize) override;

	virtual SFBool Close() override;
};

//////////////////////////////////////////////////////////////////////////
// CScaleformSwfResourceFileWrapper
//////////////////////////////////////////////////////////////////////////
class CScaleformSwfResourceFileWrapper : public SF::DelegatedFile
{
private:
	typedef Red::Threads::CMutex CMutex;
	typedef Red::Threads::CScopedLock< CMutex > CScopedLock;

	static CMutex								st_loadedSwfLock;
	static THashMap< String, CSwfResource* >	st_loadedSwfMap;

private:
	CSwfResource*		m_swfResource;

public:
	CScaleformSwfResourceFileWrapper( CSwfResource* swfResource );
	virtual ~CScaleformSwfResourceFileWrapper();

public:
	static const THandle< CSwfTexture >& GetExportedTexture( const String& textureLinkageName );

private:
	CScaleformSwfResourceFileWrapper( const CScaleformSwfResourceFileWrapper& other );
	void operator=( const CScaleformSwfResourceFileWrapper& rhs );
};

class CScaleformUrlBuilder : public GFx::URLBuilder
{
public:
	virtual void BuildURL( SF::String* ppath, const LocationInfo& loc );
};

//////////////////////////////////////////////////////////////////////////
// CScaleformFileOpener
//////////////////////////////////////////////////////////////////////////
class CScaleformFileOpener : public GFx::FileOpener
{
public:
	virtual SF::File*	OpenFile( const SFChar* purl, SFInt flags = SF::FileConstants::Open_Read | SF::FileConstants::Open_Buffered, 
		SFInt mode = SF::FileConstants::Mode_ReadWrite );

#ifndef RED_FINAL_BUILD
	virtual SF::SInt64 GetFileModifyTime(const SFChar* purl);
#endif

private:
	CSwfResource*		OpenSwf( const String& url );
};

//////////////////////////////////////////////////////////////////////////
// CScaleformImageCreator
//////////////////////////////////////////////////////////////////////////
class CScaleformImageCreator : public GFx::ImageCreator
{
private:
	typedef GFx::ImageCreator TBaseClass;

public:
	CScaleformImageCreator( SF::Ptr<SF::Render::TextureManager> textureManager );

	virtual SF::Render::Image* LoadProtocolImage( const GFx::ImageCreateInfo& info, const SF::String& url );
	virtual SF::Render::Image* LoadExportedImage(const GFx::ImageCreateExportInfo& info, const SF::String& url);

	void AddAdditionalContentDirectory( const String& contentDir );
	void RemoveAdditionalContentDirectory(  const String& contentDir );

private:
	SF::Ptr<SF::Render::Image> LoadImage( const GFx::ImageCreateInfo& info, const Char* filePath );

private:
	String m_depotPathPrefix;
	TDynArray<String> m_additionalContentPathPrefix;
	Uint32 m_lastUsedPrefixPathIndex;

};

#endif // USE_SCALEFORM
