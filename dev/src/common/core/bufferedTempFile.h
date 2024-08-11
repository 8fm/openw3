/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

/// Temporary file writer that overwrites the specified file only after successful save
class CBufferedTempFile : public IFile
{
public:
	CBufferedTempFile( const String& targetFile );
	virtual ~CBufferedTempFile();

	// IFile interface
	virtual void Serialize( void* buffer, size_t size ) override;
	virtual Uint64 GetOffset() const override;
	virtual Uint64 GetSize() const override;
	virtual void Seek( Int64 offset ) override;

protected:
	IFile*			m_tempWriter;
	String			m_targetFile;
	String			m_tempFileName;
};