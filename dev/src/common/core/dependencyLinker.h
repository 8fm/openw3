/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

/// Base class for dependency file related magic (resource system)
class IDependencyLinker : public IFile, protected Red::System::NonCopyable
{
	friend class CReslinkerCommandlet;

protected:
	IFile*		m_file;					// Internal data serialization

public:
	IDependencyLinker( IFile& file );
	virtual ~IDependencyLinker();

public:
	// IFile interface
	virtual const Char *GetFileNameForDebug() const { return m_file->GetFileNameForDebug(); }
	virtual void Serialize( void* buffer, size_t size );
	virtual Uint64 GetOffset() const;
	virtual Uint64 GetSize() const;
	virtual void Seek( Int64 offset );
};
