/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "pathlibSimpleBuffers.h"
#include "pathlibConst.h"

class CDirectory;


namespace PathLib
{

class CAreaRes;
class CAreaDescription;
class CAreaNavgraphsRes;
class CObstaclesMap;
class CTerrainMap;
class CNavmeshRes;

class CResPtr
{
	// class has virtual methods. We could split it into one class with core functions and
	// one with virtuals.
public:
	CAreaRes*						m_res;

	CResPtr()
		: m_res( nullptr )																{}
	~CResPtr();

	Bool IsConstructed() const															{ return m_res != nullptr; }
	CAreaRes* GetRes() const															{ return m_res; }

	void Release();
	void Feed( CAreaRes* res );

	Bool Save( CAreaDescription* area, CDirectory* dir );
	Bool SyncLoad( CAreaDescription* area );

	virtual CAreaRes* VConstruct()														= 0;
	virtual ENavResType VGetResType() const												= 0;
	virtual void VGetResName( CAreaDescription* area, String& outName )					= 0;
};

template < class ResType >
class TResPtr : public CResPtr
{
private:
	typedef CResPtr Super;

public:
	static void GetLocalResName( CAreaDescription* area, String& outName );
	RED_INLINE static const ENavResType GetResType()									{ return ResType::GetResType(); }

	RED_INLINE ResType* Get() const													{ return static_cast< ResType* const >( m_res ); }

	RED_INLINE ResType& operator*() const												{ return *static_cast< ResType* >( m_res ); }
	RED_INLINE ResType* operator->() const											{ return static_cast< ResType* >( m_res ); }

	ResType* Construct()																{ ASSERT( !m_res ); m_res = new ResType(); return static_cast< ResType* >( m_res ); }
};

class CObstaclesMapResPtr : public TResPtr< CObstaclesMap >
{
public:
	CAreaRes* VConstruct() override;
	ENavResType VGetResType() const override;
	void VGetResName( CAreaDescription* area, String& outName ) override;
};

class CAreaNavgraphsResPtr : public TResPtr< CAreaNavgraphsRes >
{
public:
	CAreaRes* VConstruct() override;
	ENavResType VGetResType() const override;
	void VGetResName( CAreaDescription* area, String& outName ) override;
};

class CTerrainMapResPtr : public TResPtr< CTerrainMap >
{
public:
	CAreaRes* VConstruct() override;
	ENavResType VGetResType() const override;
	void VGetResName( CAreaDescription* area, String& outName ) override;
};

class CNavmeshResPtr : public TResPtr< CNavmeshRes >
{
public:
	CAreaRes* VConstruct() override;
	ENavResType VGetResType() const override;
	void VGetResName( CAreaDescription* area, String& outName ) override;
};



};			// namespace PathLib


template<> 
RED_INLINE void CSimpleBufferWriter::SmartPut< PathLib::CResPtr >( const PathLib::CResPtr& val )
{
}
template<>
RED_INLINE Bool CSimpleBufferReader::SmartGet< PathLib::CResPtr >( PathLib::CResPtr& val )
{
	return true;
}

#define DECLARE_SMARTPUTGET( _c )														\
	template<>																			\
	RED_INLINE void CSimpleBufferWriter::SmartPut< _c >( const _c& val )				\
	{																					\
		SmartPut< PathLib::CResPtr >( val );											\
	}																					\
	template<>																			\
	RED_INLINE Bool CSimpleBufferReader::SmartGet< _c >( _c& val )					\
	{																					\
		return SmartGet< PathLib::CResPtr >( val );										\
	}

DECLARE_SMARTPUTGET( PathLib::CObstaclesMapResPtr )
DECLARE_SMARTPUTGET( PathLib::CAreaNavgraphsResPtr )
DECLARE_SMARTPUTGET( PathLib::CTerrainMapResPtr )
DECLARE_SMARTPUTGET( PathLib::CNavmeshResPtr )


#undef DECLARE_SMARTPUTGET
