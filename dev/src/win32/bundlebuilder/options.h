/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#ifndef __BUNDLE_BUILDER_OPTIONS_H__
#define __BUNDLE_BUILDER_OPTIONS_H__

#include "../../common/core/coreInternal.h"
#include "../../common/core/bundleheader.h"

namespace Red
{
	namespace System
	{
		namespace Log
		{
			class TTYWriter;
			class File;
		}
	}
}

namespace Bundler
{
	namespace Utility
	{
		void CreateDirectory( const AnsiChar* root, const AnsiChar* path );
		Bool ValidateDirectory( StringAnsi& directory );
	}

	class COptions
	{
	public:
		typedef Red::Core::Bundle::ECompressionType ECompressionType;

		COptions();
		~COptions();

		void Parse( Int32 argc, const AnsiChar* argv[] );

		void ValidatePaths();

		Bool HasErrors() const;
		void PrintErrors() const;
		void PrintCommandLine() const;

		// Accessors
		RED_INLINE const StringAnsi& GetDefinitionPath() const	{ return m_definitionFilename; }
		RED_INLINE const StringAnsi& GetDepotDirectory() const	{ return m_depotPath; }
		RED_INLINE const StringAnsi& GetCookedDirectory() const	{ return m_cookedPath; }
		RED_INLINE const StringAnsi& GetOutputDirectory() const	{ return m_outputDir; }

		RED_INLINE Uint32 GetCompressionIterations() const		{ return m_numTestIterations; }
		RED_INLINE Uint32 GetNumThreads() const					{ return m_numThreads; }
		RED_INLINE Bool IsSinglethreaded() const				{ return m_singleThreaded; }
		RED_INLINE Bool IsSilent() const						{ return m_silent; }
		RED_INLINE Bool IsProfileMode() const					{ return m_profile; }
		RED_INLINE Bool IsCompressionOverridden() const			{ return m_compressionOverride; }
		RED_INLINE ECompressionType GetCompressionType() const	{ return m_compressionOverrideType; }
		RED_INLINE Float GetMinimumCompressionRatio() const		{ return m_minimumCompressionRatio; }

		RED_INLINE Bool UseAutoCache() const					{ return m_useAutoCache; }
		RED_INLINE Bool BuildAutoCache() const					{ return m_buildAutoCache; }
		RED_INLINE const StringAnsi& GetAutoCachePath() const	{ return m_autoCachePath; }
		RED_INLINE const StringAnsi& GetBaseAutoCachePath() const	{ return m_baseAutoCachePath; }

		RED_INLINE Bool HasError() const						{ return m_hasError; }

	private:
		// Required
		StringAnsi							m_definitionFilename;
		StringAnsi							m_depotPath;
		StringAnsi							m_cookedPath;
		StringAnsi							m_outputDir;

		// Optional
		Uint32								m_numTestIterations;
		Uint32								m_numThreads;
		Bool								m_singleThreaded;
		Bool								m_silent;
		Bool								m_profile;
		Bool								m_ignoreSizeLimit;
		Bool								m_compressionOverride;
		ECompressionType					m_compressionOverrideType;

		Bool								m_useAutoCache;
		Bool								m_buildAutoCache;
		StringAnsi							m_autoCachePath;
		StringAnsi							m_baseAutoCachePath;

		Red::System::Log::TTYWriter*		m_ttyDevice;
		Red::System::Log::File*				m_fileDevice;

		// Error message
		Bool								m_hasError;
		StringAnsi							m_error;
		Float								m_minimumCompressionRatio;
	};
}

#endif // __BUNDLE_BUILDER_OPTIONS_H__
