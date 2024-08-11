/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "skeletalAnimation.h"

class IMotionExtraction : public ISerializable
{
	DECLARE_RTTI_SIMPLE_CLASS_WITH_ALLOCATOR( IMotionExtraction, MC_Animation );

public:
	//! Get movement at time
	virtual void GetMovementAtTime( Float time, AnimQsTransform& motion ) const;

	//! Get movement between time
	virtual void GetMovementBetweenTime( Float timeStart, Float timeEnd, Int32 loops, AnimQsTransform& motion ) const;

	//! Get data size
	virtual Uint32 GetDataSize() const;

	//! Get duration
	virtual Float GetDuration() const;

	//! Is tha a compressed motion extraction ?
	virtual Bool IsCompressed() const;

	//! Serialize
	virtual void OnSerialize( IFile &file ) ;
};

BEGIN_ABSTRACT_CLASS_RTTI( IMotionExtraction );
	PARENT_CLASS( ISerializable );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CUncompressedMotionExtraction : public IMotionExtraction
{
	DECLARE_RTTI_SIMPLE_CLASS( CUncompressedMotionExtraction );

	Float					m_duration;
	TDynArray< Vector >		m_frames;

public:
	//! Get movement at time
	virtual void GetMovementAtTime( Float time, AnimQsTransform& motion ) const;

	//! Get data size
	virtual Uint32 GetDataSize() const;

	//! Get duration
	virtual Float GetDuration() const;

public:
	CUncompressedMotionExtraction();

	const TDynArray< Vector >& GetUncompressedFrames() const;

	CUncompressedMotionExtraction* CreateCopy();

public:
	Bool Initialize( const CSkeletalAnimation::FactoryInfo& data );
	Bool Initialize( const CSkeletalAnimation* animation );
};

BEGIN_CLASS_RTTI( CUncompressedMotionExtraction );
	PARENT_CLASS( IMotionExtraction );
	PROPERTY( m_frames );
	PROPERTY( m_duration );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CHavokMotionExtraction : public IMotionExtraction
{
	DECLARE_RTTI_SIMPLE_CLASS( CHavokMotionExtraction );

#ifdef USE_HAVOK_ANIMATION
	THavokDataBuffer< hkaAnimatedReferenceFrame, MC_BufferAnimMotionEx, MemoryPool_Animation > m_data;
#else
	TDynArray< Vector > m_keys;
	Float m_duration;
#endif

public:
	//! Get movement at time
	virtual void GetMovementAtTime( Float time, AnimQsTransform& motion ) const;

	//! Get movement between time
	virtual void GetMovementBetweenTime( Float timeStart, Float timeEnd, Int32 loops, AnimQsTransform& motion ) const;

	//! Get data size
	virtual Uint32 GetDataSize() const;

	//! Get duration
	virtual Float GetDuration() const;

	//! Serialize object
	virtual void OnSerialize( IFile &file );

public:
	CHavokMotionExtraction();

public:
	//! Has data
	Bool HasData() const;

	//! Get number of samples
	Uint32 GetSamplesNum() const;

public:
	Bool Initialize( const CSkeletalAnimation::FactoryInfo& data );
	Bool Initialize( const CSkeletalAnimation* animation );
};

BEGIN_CLASS_RTTI( CHavokMotionExtraction );
	PARENT_CLASS( IMotionExtraction );
#ifndef USE_HAVOK_ANIMATION
	PROPERTY(m_keys);
	PROPERTY(m_duration);
#endif
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CLineMotionExtraction : public IMotionExtraction
{
	DECLARE_RTTI_SIMPLE_CLASS( CLineMotionExtraction );

	TDynArray< Vector >		m_frames;
	TDynArray< Float >		m_times;

public:
	//! Get movement at time
	virtual void GetMovementAtTime( Float time, AnimQsTransform& motion ) const;

	//! Get data size
	virtual Uint32 GetDataSize() const;

	//! Get duration
	virtual Float GetDuration() const;

public:
	CLineMotionExtraction();

	Bool Initialize( TDynArray< Vector >& frames, TDynArray< Float >& times );

	const TDynArray< Float >& GetFrameTimes() const;

	virtual Bool IsCompressed() const
	{
		return true;
	}

private:
	void FindFrameAndWeight( Float time, Int32& frame, Float& delta ) const;
};

BEGIN_CLASS_RTTI( CLineMotionExtraction );
	PARENT_CLASS( IMotionExtraction );
	PROPERTY( m_frames );
	PROPERTY( m_times );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

enum ELineMotionExFlags
{
	LMEF_X = FLAG( 0 ),
	LMEF_Y = FLAG( 1 ),
	LMEF_Z = FLAG( 2 ),
	LMEF_R = FLAG( 3 ),
};

class CLineMotionExtraction2 : public IMotionExtraction
{
	DECLARE_RTTI_SIMPLE_CLASS( CLineMotionExtraction2 );

	friend class CMotionExtractionLineCompression2;

	mutable Float			m_duration;
	TDynArray< Float >		m_frames;
	TDynArray< Uint8 >		m_deltaTimes;
	Uint8					m_flags;

public:
	static const Float		TIME_SLICE;

public:
	//! Get movement at time
	virtual void GetMovementAtTime( Float time, AnimQsTransform& motion ) const;

	//! Get data size
	virtual Uint32 GetDataSize() const;

	//! Get duration
	virtual Float GetDuration() const;

public:
	CLineMotionExtraction2();

	void GetFrameTimes( TDynArray< Float >& times ) const;

private:
	void FindFrameAndWeight( Float time, Int32& frame, Float& delta ) const;	
	AnimVector4 DecompressFrame( Int32 frame ) const;
	Uint32 GetFrameSize() const;
	Uint32 GetNumFrames() const;

	virtual Bool IsCompressed() const
	{
		return true;
	}

	RED_INLINE Float GetTimeSlice() const
	{
		Uint32 frames = 0;

		const Uint32 size = m_deltaTimes.Size();
		for ( Uint32 i=0; i<size; ++i )
		{
			frames += m_deltaTimes[ i ];
		}

		return GetDuration() / (Float)frames;
	}
};

BEGIN_CLASS_RTTI( CLineMotionExtraction2 );
	PARENT_CLASS( IMotionExtraction );
	PROPERTY( m_duration );
	PROPERTY( m_frames );
	PROPERTY( m_deltaTimes );
	PROPERTY( m_flags );
END_CLASS_RTTI();
