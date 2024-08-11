// Copyright © 2014 CD Projekt Red. All Rights Reserved.

#pragma once

// =================================================================================================
namespace StoryScene {
// =================================================================================================

/*
Wrapper around array of floats containing values of all channels for all keys of interpolation event.
*/
template < Uint32 numChannels >
class ChannelStorage
{
public:
	typedef Float ChannelArray[ numChannels ];

	ChannelStorage( Float* channels );

	ChannelArray& GetChannels( Uint32 keyIndex );

private:
	Float* m_channelStorage;				// Contains values of all channels for all keys.
};

// =================================================================================================
// implementation
// =================================================================================================

/*
Ctor.
*/
template < Uint32 numChannels >
RED_INLINE ChannelStorage< numChannels >::ChannelStorage( Float* channels )
: m_channelStorage( channels )
{}

/*
Gets channel array containing values of all channels for specified key of interpolation event.
*/
template < Uint32 numChannels >
RED_INLINE typename ChannelStorage< numChannels >::ChannelArray& ChannelStorage< numChannels >::GetChannels( Uint32 keyIndex )
{
	return *reinterpret_cast< ChannelArray *>( m_channelStorage + keyIndex * numChannels );
}

// =================================================================================================
} // namespace StoryScene
// =================================================================================================
