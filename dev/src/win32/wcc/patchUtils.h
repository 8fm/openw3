/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once

namespace PatchUtils
{
	// Allocates global big 512MB of memory for the "temp stuff"
	// It's hot where I'm going for this...
	extern void* AllocateTempMemory();

	/// Copy file content
	extern Bool CopyFileContent( IFile& srcFile, IFile& destFile );

	/// Copy file content and compute its crc
	extern Bool CopyFileContent( IFile& srcFile, IFile& destFile, Uint32& outCrc );

	/// Copy file content to absolute path
	extern Bool CopyFileContent( IFile& srcFile, const String& destPath );

	/// Copy file content from one absolute place to other to absolute place
	extern Bool CopyFileContent( const String& srcPath, const String& destPath );

	/// Copy data from one file and compress it
	extern Bool CompressFileContentZLIB( IFile& srcFile, IFile& destFile );

	/// Copy data from one file and compress it
	extern Bool CompressFileContentDoboz( IFile& srcFile, IFile& destFile );

	/// Copy data from one file and compress it
	extern Bool CompressFileContentLZ4HC( IFile& srcFile, IFile& destFile );

	/// Calculate CRC of data block in file
	extern Uint64 CalcFileBlockCRC( IFile* file, const Uint64 size, Uint64 crc );

	/// Calculate CRC of resource file
	extern Uint64 CalcDependencyFileCRC( IFile* file, Uint64 crc );

	/// Generate safe type names
	extern CName GetSafeTypeName( CName name );

	/// Is given path a base content or dlc path (not a patch/mod) ?
	extern Bool IsContentOrDlcPath( const String& path );

	/// Offset file reader
	class COffsetFileReader : public IFile
	{
	public:
		COffsetFileReader( IFile* base, Uint64 offset, Uint32 size );
		virtual void Serialize( void* buffer, size_t size ) override;
		virtual Uint64 GetOffset() const override;
		virtual Uint64 GetSize() const override;
		virtual void Seek( Int64 offset ) override;

	private:
		IFile*			m_base;
		Uint64			m_offset;
		Uint32			m_size;
	};

	/// Simple memory file reader
	class CSimpleMemoryFileReader : public IFile
	{
	public:
		CSimpleMemoryFileReader( void* memory, Uint32 size );

		virtual void Serialize( void* buffer, size_t size ) override;
		virtual Uint64 GetOffset() const override;
		virtual Uint64 GetSize() const override;
		virtual void Seek( Int64 offset ) override;

	private:
		void*			m_data;
		Uint64			m_offset;
		Uint32			m_size;
	};

} // PatchUtils