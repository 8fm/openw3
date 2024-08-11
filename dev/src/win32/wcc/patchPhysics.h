/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "patchBuilder.h"


//-----------------------------------------------------------------------------

class CPatchPhysicsFileToken;
class CPatchPhysicsFile;
class CCollisionCacheData;

//-----------------------------------------------------------------------------

/// Group of collision cache (the build)
class CPatchPhysics : public IBasePatchContentBuilder::IContentGroup
{
public:
	/// Interface
	virtual void GetTokens( TDynArray< IBasePatchContentBuilder::IContentToken* >& outTokens ) const override;
	virtual const Uint64 GetDataSize() const override;
	virtual const String GetInfo() const override;

	/// Load all of the collision caches from given directory
	static CPatchPhysics* LoadCollisionCaches( const String& baseDirectory );

private:
	String										m_basePath;
	TDynArray< CPatchPhysicsFileToken* >		m_tokens;
	TDynArray< CPatchPhysicsFile* >				m_collisionCaches;
};

//-----------------------------------------------------------------------------

/// Information about single patchable entry in the collision cache
class CPatchPhysicsFileToken : public IBasePatchContentBuilder::IContentToken
{
public:
	CPatchPhysicsFileToken( const CPatchPhysicsFile* data, Uint32 entryIndex, StringAnsi filePath, Uint64 hash );
	~CPatchPhysicsFileToken();

	/// Access
	const StringAnsi& GetFilePath() const;

	/// IContentToken interface
	virtual const Uint64 GetTokenHash() const override;
	virtual const Uint64 GetDataCRC() const override;
	virtual const Uint64 GetDataSize() const override;
	virtual const String GetInfo() const override;
	virtual void DebugDump( const String& dumpPath, const Bool isBase ) const override;

public:
	Uint32							m_entryIndex;
	const CCollisionCacheData*		m_data;
	const CPatchPhysicsFile*		m_ownerCacheFile;

	StringAnsi			m_filePath;
	Uint64				m_tokenHash;
	Uint64				m_dataCRC;			// For most tokens same in collision cache, for apexes without version bits
};

//-----------------------------------------------------------------------------

/// Patch builder for physics (collision cache)
class CPatchBuilder_Physics : public IBasePatchContentBuilder
{
	DECLARE_RTTI_SIMPLE_CLASS( CPatchBuilder_Physics );

public:
	CPatchBuilder_Physics();
	~CPatchBuilder_Physics();

	/// Interface
	virtual String GetContentType() const override;
	virtual IBasePatchContentBuilder::IContentGroup* LoadContent( const ECookingPlatform platform, const String& absoluteBuildPath )  override;
	virtual Bool SaveContent( const ECookingPlatform platform, IContentGroup* baseGroup, IContentGroup* patchGroup, TDynArray< IBasePatchContentBuilder::IContentToken* >& patchContent, const String& absoluteBuildPath, const String& patchName ) override;
};

BEGIN_CLASS_RTTI( CPatchBuilder_Physics );
PARENT_CLASS( IBasePatchContentBuilder );
END_CLASS_RTTI();

//-----------------------------------------------------------------------------
