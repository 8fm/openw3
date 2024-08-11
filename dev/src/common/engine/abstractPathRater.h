#pragma once

#include "../core/refCountPointer.h"

#include "pathlibConst.h"

namespace PathLib
{
	class CPathLink;
};

class IPathRater
{
protected:
	typedef Red::Threads::CAtomic< Int32 > AtomicInt;

	AtomicInt					m_refCount;

	virtual ~IPathRater()																	{}
public:
	typedef TRefCountPointer< IPathRater > Ptr;

	IPathRater()
		: m_refCount( 0 )																	{}

	virtual void				PathFindingStarted();
	virtual void				PathFindingEnded();

	virtual PathLib::PathCost	CountRealRate( const Vector3& from, const PathLib::CPathLink* pathLink );
	virtual PathLib::PathCost	CountHeurusticRate( const Vector3& from, const Vector3& to );

	void						AddRef()													{ m_refCount.Increment(); }
	void						Release()													{ if( m_refCount.Decrement() == 0 ) delete this; }
};