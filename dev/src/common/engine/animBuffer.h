
#pragma once

#if defined(USE_HAVOK_ANIMATION) || defined(USE_HAVOK_DATA_IMPORT)

#include "havokDataBuffer.h"

typedef THavokDataBuffer< hkaAnimation, MC_BufferAnimation, MemoryPool_Animation > tHavokAnimationBuffer;

struct AnimBuffer
{
	DECLARE_STRUCT_MEMORY_POOL( MemoryPool_Animation, MC_Animation );
	tHavokAnimationBuffer*		m_animations;
	Uint32						m_animNum;

	AnimBuffer()
		: m_animations( NULL )
		, m_animNum( 0 )
	{

	}

	~AnimBuffer()
	{
		Clear();
	}

	const Uint32 GetPartsNum() const
	{
		return m_animNum;
	}

	friend IFile& operator<<( IFile& file, AnimBuffer& buff )
	{
		// Serialize anim num
		file << buff.m_animNum;

		if ( file.IsWriter() )
		{
			// Serialize all animation buffers
			for ( Uint32 i=0; i<buff.m_animNum; ++i )
			{
				// Remove all annotation tracks
				hkaAnimation* animationObject = buff.m_animations[ i ].GetHavokObject();
				if( animationObject != NULL )
				{
					animationObject->m_numAnnotationTracks = 0;
				}

				// Buffers
				buff.m_animations[ i ].Serialize( file );
				buff.m_animations[ i ].Unload();
			}
		}
		else
		{
			const Uint32 SOME_SANE_NUMBER( 10000 );
			ASSERT( buff.m_animNum < SOME_SANE_NUMBER );
			if ( buff.m_animNum > 0 && buff.m_animNum < SOME_SANE_NUMBER )
			{
				buff.m_animations = new tHavokAnimationBuffer[ buff.m_animNum ];

				// Load animation
				for ( Uint32 i=0; i<buff.m_animNum; ++i )
				{
					buff.m_animations[ i ].Serialize( file );
				}
			}
		}
		return file;
	}

	void Clear()
	{
		m_animNum = 0;

		delete[] m_animations;
		m_animations = NULL;
	}

	const hkaAnimation* GetHavokAnimation( Uint32 index ) const
	{
		ASSERT( m_animations );
		ASSERT( m_animNum > 0 && index < m_animNum );

		if ( index < m_animNum )
		{
			tHavokAnimationBuffer* data = const_cast< tHavokAnimationBuffer* >( &(m_animations[ index ]) );
			hkaAnimation* animation = data->GetHavokObject();

			ASSERT( animation );

			return animation;
		}

		// No animation
		return NULL;
	}
};

#endif