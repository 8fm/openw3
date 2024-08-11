/*
 * Copyright © 2011 CD Projekt Red. All Rights Reserved.
 */

#pragma once

class CFootstepDataChunk
{
	DECLARE_CLASS_MEMORY_POOL( MemoryPool_SmallObjects, MC_Engine );

	friend class CDecompressJob;

public:
	const static Float SIZE;
	const static Float CELL_SIZE;

	CFootstepDataChunk();
	~CFootstepDataChunk();

	void OnSerialize( IFile& file );

	Int8 GetValue( Float relativeX, Float relativeY );
	void InsertValue( Float relativeX, Float relativeY, Int8 soundMaterial );

	RED_INLINE Bool IsCompressed() const
	{ return m_isCompressed; }

	void DeleteUncompressedData();

private:
	class  CDecompressJob;

	const static Uint32  EDGE_SIZE;
	const static Uint32  UNCOMPRESSED_DATA_SIZE;
	
	Int32*										m_uncompressedData;
	DataBuffer*									m_compressedData;

	Bool		m_isCompressed	: 1;
	Bool		m_uncompressing : 1;

	void Compress();
	void UncompressAsync();

	void CreateUncompressedData();
	void DeleteData();
};

class CFootstepData : public CObject
{
	DECLARE_ENGINE_CLASS( CFootstepData, CObject, 0 )

public:
	CFootstepData();
	virtual ~CFootstepData();

	virtual void OnSerialize( IFile& file );

	void Reset( Uint32 chunksBySize );
	Int8 GetValue( const Vector& position );
	void InsertValue( Float relativeX, Float relativeY, Int8 soundMaterial );

private:
	CFootstepDataChunk**	m_chunks;
	Uint32					m_size;
	TPair< Int32, Int32 >		m_lastChunkXY;
	Int32						m_sweepCounter;

	CFootstepDataChunk* GetChunk( Float& relativeX, Float& relativeY, Bool createIfNotPresent );
	void DeleteChunks();
	void Sweep();
};

BEGIN_CLASS_RTTI( CFootstepData )
	PARENT_CLASS( CObject )
END_CLASS_RTTI()
