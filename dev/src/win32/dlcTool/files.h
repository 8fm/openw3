/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once

namespace DLCTool
{

	/// File stream
	class DLCFileStream
	{
	public:
		virtual ~DLCFileStream() {};
		virtual void Serialize( void* data, UINT size )=0;
		virtual DWORD GetSize() const=0;
		virtual UINT GetOffset() const=0;
		virtual void Seek( UINT offset )=0;
	};

	///////////////////////////////////////////////////////////////////////////

	/// Raw file reader
	class DLCResourceReader : public DLCFileStream
	{
	protected:
		HMODULE	m_module;
		HGLOBAL m_resPtr;
		DWORD	m_size;
		UINT	m_offset;
		void*	m_resBeginPtr;

	public:
		DLCResourceReader( HMODULE module, HRSRC resource, HGLOBAL resPtr );
		virtual ~DLCResourceReader();

		virtual void Serialize( void* data, UINT size );
		virtual DWORD GetSize() const;
		virtual UINT GetOffset() const;
		virtual void Seek( UINT offset );

		static DLCResourceReader* CreateReader( int resourceName );
	};

	///////////////////////////////////////////////////////////////////////////

	/// Raw file reader
	class DLCFileReader : public DLCFileStream
	{
	protected:
		HANDLE	m_fileHandle;
		UINT	m_baseOffset;
		UINT	m_size;

	public:
		DLCFileReader( HANDLE hFileHandle, UINT baseOffset );
		virtual ~DLCFileReader();

		virtual void Serialize( void* data, UINT size );
		virtual DWORD GetSize() const;
		virtual UINT GetOffset() const;
		virtual void Seek( UINT offset );

		static DLCFileReader* CreateReader( const WCHAR* filePath );
	};

	/// Raw file writer
	class DLCFileWriter : public DLCFileStream
	{
	protected:
		HANDLE	m_fileHandle;

	public:
		DLCFileWriter( HANDLE hFileHandle );
		virtual ~DLCFileWriter();

		virtual void Serialize( void* data, UINT size );
		virtual DWORD GetSize() const;
		virtual UINT GetOffset() const;
		virtual void Seek( UINT offset );

		static DLCFileWriter* CreateWriter( const WCHAR* filePath );
	};

	/// Raw file reader
	class RawFileReader : public DLCFileStream
	{
	protected:
		HANDLE	m_fileHandle;
		UINT	m_baseOffset;
		DWORD	m_size;

	public:
		RawFileReader( const WCHAR* fileName );
		RawFileReader( HMODULE hFileHandle, UINT baseOffset );
		virtual ~RawFileReader();

		virtual void Serialize( void* data, UINT size );
		virtual DWORD GetSize() const;
		virtual UINT GetOffset() const;
		virtual void Seek( UINT offset );
	};

	/// Raw file writer
	class RawFileWriter : public DLCFileStream
	{
	protected:
		HANDLE	m_fileHandle;

	public:
		RawFileWriter( const WCHAR* fileName );
		virtual ~RawFileWriter();

		virtual void Serialize( void* data, UINT size );
		virtual DWORD GetSize() const;
		virtual UINT GetOffset() const;
		virtual void Seek( UINT offset );
	};

	/// Read compressed UINT
	INT ReadCompressedNumber( DLCFileStream* stream );
}