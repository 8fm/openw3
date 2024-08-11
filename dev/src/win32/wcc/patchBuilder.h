/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once

/// Patching helper - source of patch data
class IBasePatchContentBuilder : public ISerializable
{
	DECLARE_RTTI_SIMPLE_CLASS( IBasePatchContentBuilder );

public:
	IBasePatchContentBuilder();
	virtual ~IBasePatchContentBuilder();

	/// Abstract "token" that represents content data
	/// We don't own those
	class IContentToken
	{
	public:
		/// Get the unique token ID (usually hash of the filename)
		virtual const Uint64 GetTokenHash() const = 0;

		/// Get the CRC of the data
		virtual const Uint64 GetDataCRC() const = 0;

		/// Get the estimated data size
		virtual const Uint64 GetDataSize() const = 0;

		/// Describe the data
		virtual const String GetInfo() const = 0;

		/// Get additional data for token
		virtual const Uint64 GetAdditionalData() const { return 0; }

		/// Get additional debug information for token
		virtual const String GetAdditionalInfo() const { return String::EMPTY; }


		/// Dump content
		virtual void DebugDump( const String& dumpPath, const Bool isBase ) const = 0;

	protected:
		IContentToken();
		virtual ~IContentToken();
	};

	/// Content source (a build in general)
	class IContentGroup
	{
	public:
		/// Get content tokens (the content of the build)
		virtual void GetTokens( TDynArray< IContentToken* >& outTokens ) const = 0;

		/// Get the total estimated data size
		virtual const Uint64 GetDataSize() const = 0;

		/// Describe the data
		virtual const String GetInfo() const = 0;

	protected:
		IContentGroup();
		virtual ~IContentGroup();
	};

	/// Get type of supported content
	virtual String GetContentType() const = 0;

	/// Can we use this patcher with mods ?
	virtual Bool CanUseWithMods() const { return false; }

	/// Load content sources
	virtual IContentGroup* LoadContent( const ECookingPlatform platform, const String& absoluteBuildPath ) = 0;

	/// Store content group
	virtual Bool SaveContent( const ECookingPlatform platform, IContentGroup* baseGroup, IContentGroup* patchGroup, TDynArray< IContentToken* >& patchContent, const String& absoluteBuildPath, const String& patchName ) = 0;
};

BEGIN_ABSTRACT_CLASS_RTTI( IBasePatchContentBuilder );
	PARENT_CLASS( ISerializable );
END_CLASS_RTTI();