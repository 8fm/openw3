/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "patchBuilder.h"
#include "../../common/core/sharedPtr.h"
#include "../../common/core/file.h"

//-----------------------------------------------------------------------------

// These patching classes are meant to be used for special cases in bin folder.
// Each file has CRC calculated at runtime (check CalculateSpecialCaseFileCRC).
// Files are treated atomically, so if CRC is different, whole file is patched.
// Folder structure remains the same as original, relative to given root path.
// If you want to add new special case, fill the list in cpp file.

//-----------------------------------------------------------------------------

class CPatchSpecialCasesFileToken;

class CPatchSpecialCases : public IBasePatchContentBuilder::IContentGroup
{
public:
	~CPatchSpecialCases();

	virtual void GetTokens(TDynArray< IBasePatchContentBuilder::IContentToken* >& outTokens) const;
	virtual const Uint64 GetDataSize() const;
	virtual const String GetInfo() const;

	void AddToken( CPatchSpecialCasesFileToken* token );

private:
	TDynArray<CPatchSpecialCasesFileToken*> m_tokens;

};

//-----------------------------------------------------------------------------

class CPatchSpecialCasesFileToken : public IBasePatchContentBuilder::IContentToken
{
public:
	CPatchSpecialCasesFileToken( const String& filepath,Red::TSharedPtr<IFile> file, Uint64 crc, Uint64 hash );

	virtual const Uint64 GetTokenHash() const;
	virtual const Uint64 GetDataCRC() const;
	virtual const Uint64 GetDataSize() const;
	virtual const String GetInfo() const;
	virtual void DebugDump(const String& dumpPath, const Bool isBase) const;

	RED_INLINE const String& GetFileRelativePath() const { return m_fileRelativePath; }
	RED_INLINE Red::TSharedPtr<IFile> GetFile() { return m_file; }

private:
	String m_fileRelativePath;
	Red::TSharedPtr<IFile> m_file;
	Uint64 m_hash;
	Uint64 m_crc;

};

//-----------------------------------------------------------------------------

class CPatchBuilder_SpecialCases : public IBasePatchContentBuilder
{
	DECLARE_RTTI_SIMPLE_CLASS( CPatchBuilder_SpecialCases );

public:
	virtual String GetContentType() const;
	virtual IContentGroup* LoadContent( const ECookingPlatform platform, const String& absoluteBuildPath )  override;
	virtual Bool SaveContent( const ECookingPlatform platform, IContentGroup* baseGroup, IContentGroup* patchGroup, TDynArray< IContentToken* >& patchContent, const String& absoluteBuildPath, const String& patchName ) override;

private:
	void EnumerateSpecialCasePaths(const ECookingPlatform platform, const String& absoluteBuildPath, TDynArray<String>& specialCaseRelativePaths);
	IBasePatchContentBuilder::IContentGroup* CreateContentGroup(const String& absoluteBuildPath, const TDynArray<String>& specialCaseRelativePaths);
	CPatchSpecialCasesFileToken* CreateToken(const String& absoluteBuildPath, const String& relativePath);
	bool CheckTokenExistance(const String& relativePath);
	Uint64 CalculateSpecialCaseFileHash(const String& relativePath);
	Uint64 CalculateSpecialCaseFileCRC(Red::TSharedPtr<IFile> file);

private:
	THashMap< Uint64, CPatchSpecialCasesFileToken* > m_fileTokensMap;

};

BEGIN_CLASS_RTTI( CPatchBuilder_SpecialCases );
	PARENT_CLASS( IBasePatchContentBuilder );
END_CLASS_RTTI();

//-----------------------------------------------------------------------------
