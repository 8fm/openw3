///////////////////////////////////////////////////////////////////////
//
// AkSpeakerConfig.h
//
//
// Copyright 2008-2009 Audiokinetic Inc.
//
///////////////////////////////////////////////////////////////////////

#ifndef _AK_SPEAKERCONFIG_H_
#define _AK_SPEAKERCONFIG_H_

#include <AK/SoundEngine/Common/AkTypes.h>

// Speaker Positions (channel mask):
#define AK_SPEAKER_FRONT_LEFT             0x1		///< Front left speaker bit mask
#define AK_SPEAKER_FRONT_RIGHT            0x2		///< Front right speaker bit mask
#define AK_SPEAKER_FRONT_CENTER           0x4		///< Front center speaker bit mask
#define AK_SPEAKER_LOW_FREQUENCY          0x8		///< Low-frequency speaker bit mask
#define AK_SPEAKER_BACK_LEFT              0x10		///< Rear left speaker bit mask
#define AK_SPEAKER_BACK_RIGHT             0x20		///< Rear right speaker bit mask
#define AK_SPEAKER_BACK_CENTER			  0x100		///< Rear center speaker ("surround speaker") bit mask
#define AK_SPEAKER_SIDE_LEFT              0x200		///< Side left speaker bit mask
#define AK_SPEAKER_SIDE_RIGHT             0x400		///< Side right speaker bit mask

#define AK_CHANNEL_MASK_PARENT			  0x0		///< Use the channel mask of the parent. This is the value of the "Parent" setting of bus configs.
#define AK_CHANNEL_MASK_DETECT			  0x0		///< Device initialization: Auto detect the speaker setup by asking the hardware (or use platform/device default).
#define AK_CHANNEL_MASK_INVALID			  0x0		///< Invalid channel configuration.

//
// Supported speaker setups. Those are the ones that can be used in the Wwise Sound Engine audio pipeline.
//

#define AK_SPEAKER_SETUP_MONO			AK_SPEAKER_FRONT_CENTER		///< 1.0 setup channel mask
#define AK_SPEAKER_SETUP_0POINT1		AK_SPEAKER_LOW_FREQUENCY	///< 0.1 setup channel mask
#define AK_SPEAKER_SETUP_1POINT1		(AK_SPEAKER_FRONT_CENTER	| AK_SPEAKER_LOW_FREQUENCY)	///< 1.1 setup channel mask
#define AK_SPEAKER_SETUP_STEREO			(AK_SPEAKER_FRONT_LEFT		| AK_SPEAKER_FRONT_RIGHT)	///< 2.0 setup channel mask
#define AK_SPEAKER_SETUP_2POINT1		(AK_SPEAKER_SETUP_STEREO	| AK_SPEAKER_LOW_FREQUENCY)	///< 2.1 setup channel mask
#define AK_SPEAKER_SETUP_3STEREO		(AK_SPEAKER_SETUP_STEREO	| AK_SPEAKER_FRONT_CENTER)	///< 3.0 setup channel mask
#define AK_SPEAKER_SETUP_3POINT1		(AK_SPEAKER_SETUP_3STEREO	| AK_SPEAKER_LOW_FREQUENCY)	///< 3.1 setup channel mask
#define AK_SPEAKER_SETUP_4				(AK_SPEAKER_SETUP_STEREO	| AK_SPEAKER_BACK_LEFT | AK_SPEAKER_BACK_RIGHT)	///< 4.0 setup channel mask
#define AK_SPEAKER_SETUP_4POINT1		(AK_SPEAKER_SETUP_4			| AK_SPEAKER_LOW_FREQUENCY)	///< 4.1 setup channel mask
#define AK_SPEAKER_SETUP_5				(AK_SPEAKER_SETUP_4			| AK_SPEAKER_FRONT_CENTER)	///< 5.0 setup channel mask
#define AK_SPEAKER_SETUP_5POINT1		(AK_SPEAKER_SETUP_5			| AK_SPEAKER_LOW_FREQUENCY)	///< 5.1 setup channel mask
#define AK_SPEAKER_SETUP_6				(AK_SPEAKER_SETUP_4			| AK_SPEAKER_SIDE_LEFT | AK_SPEAKER_SIDE_RIGHT)	///< 6.0 setup channel mask
#define AK_SPEAKER_SETUP_6POINT1		(AK_SPEAKER_SETUP_6			| AK_SPEAKER_LOW_FREQUENCY)	///< 6.1 setup channel mask
#define AK_SPEAKER_SETUP_7				(AK_SPEAKER_SETUP_6			| AK_SPEAKER_FRONT_CENTER)	///< 7.0 setup channel mask
#define AK_SPEAKER_SETUP_7POINT1		(AK_SPEAKER_SETUP_7			| AK_SPEAKER_LOW_FREQUENCY)	///< 7.1 setup channel mask
#define AK_SPEAKER_SETUP_SURROUND		(AK_SPEAKER_SETUP_STEREO	| AK_SPEAKER_BACK_CENTER)	///< Wii surround setup channel mask

// Note. DPL2 does not really have 4 channels, but it is used by plugins to differentiate from stereo setup.
#define AK_SPEAKER_SETUP_DPL2			(AK_SPEAKER_SETUP_4)		///< Wii DPL2 setup channel mask


//
// Extra speaker setups. This is a more exhaustive list of speaker setups, which might not all be supported
// by the Wwise Sound Engine audio pipeline.
//

#define AK_SPEAKER_SETUP_0_1		( AK_SPEAKER_LOW_FREQUENCY )							//0.1
#define AK_SPEAKER_SETUP_1_0		( AK_SPEAKER_FRONT_LEFT )								//1.0 (L)
#define AK_SPEAKER_SETUP_1_1		( AK_SPEAKER_FRONT_LEFT	| AK_SPEAKER_LOW_FREQUENCY )	//1.1 (L)

#define AK_SPEAKER_SETUP_1_0_CENTER	( AK_SPEAKER_FRONT_CENTER )							//1.0 (C)
#define AK_SPEAKER_SETUP_1_1_CENTER ( AK_SPEAKER_FRONT_CENTER	| AK_SPEAKER_LOW_FREQUENCY )	//1.1 (C)

#define AK_SPEAKER_SETUP_2_0		( AK_SPEAKER_FRONT_LEFT	| AK_SPEAKER_FRONT_RIGHT )							//2.0
#define AK_SPEAKER_SETUP_2_1		( AK_SPEAKER_FRONT_LEFT	| AK_SPEAKER_FRONT_RIGHT | AK_SPEAKER_LOW_FREQUENCY )	//2.1

#define AK_SPEAKER_SETUP_3_0		( AK_SPEAKER_FRONT_LEFT	| AK_SPEAKER_FRONT_RIGHT | AK_SPEAKER_FRONT_CENTER )	//3.0
#define AK_SPEAKER_SETUP_3_1		( AK_SPEAKER_SETUP_3_0	| AK_SPEAKER_LOW_FREQUENCY )	//3.1

#define AK_SPEAKER_SETUP_FRONT		( AK_SPEAKER_SETUP_3_0 )
#define AK_SPEAKER_SETUP_REAR		( AK_SPEAKER_BACK_LEFT	| AK_SPEAKER_BACK_RIGHT )			//Rear
#define AK_SPEAKER_SETUP_SIDE		( AK_SPEAKER_SIDE_LEFT	| AK_SPEAKER_SIDE_RIGHT )			//Side

#define AK_SPEAKER_SETUP_4_0		( AK_SPEAKER_SETUP_2_0	| AK_SPEAKER_SETUP_REAR )	//4.0 (rear)
#define AK_SPEAKER_SETUP_4_1		( AK_SPEAKER_SETUP_4_0	| AK_SPEAKER_LOW_FREQUENCY )	//4.1 (rear)
#define AK_SPEAKER_SETUP_5_0		( AK_SPEAKER_SETUP_4_0	| AK_SPEAKER_FRONT_CENTER )	//5.0 (rear)
#define AK_SPEAKER_SETUP_5_1		( AK_SPEAKER_SETUP_5_0	| AK_SPEAKER_LOW_FREQUENCY )	//5.1 (rear)

#define AK_SPEAKER_SETUP_4_0_SIDE	( AK_SPEAKER_SETUP_2_0	| AK_SPEAKER_SETUP_SIDE )		//4.0 (side)
#define AK_SPEAKER_SETUP_4_1_SIDE	( AK_SPEAKER_SETUP_4_0_SIDE	| AK_SPEAKER_LOW_FREQUENCY )	//4.1 (side)
#define AK_SPEAKER_SETUP_5_0_SIDE	( AK_SPEAKER_SETUP_4_0_SIDE	| AK_SPEAKER_FRONT_CENTER )	//5.0 (side)
#define AK_SPEAKER_SETUP_5_1_SIDE	( AK_SPEAKER_SETUP_5_0_SIDE	| AK_SPEAKER_LOW_FREQUENCY )	//5.1 (side)

#define AK_SPEAKER_SETUP_6_0		( AK_SPEAKER_SETUP_4_0	| AK_SPEAKER_SETUP_SIDE )		//6.0
#define AK_SPEAKER_SETUP_6_1		( AK_SPEAKER_SETUP_6_0	| AK_SPEAKER_LOW_FREQUENCY )	//6.1
#define AK_SPEAKER_SETUP_7_0		( AK_SPEAKER_SETUP_5_0	| AK_SPEAKER_SETUP_SIDE )		//7.0
#define AK_SPEAKER_SETUP_7_1		( AK_SPEAKER_SETUP_7_0	| AK_SPEAKER_LOW_FREQUENCY )	//7.1

inline void AK_SPEAKER_SETUP_FIX_LEFT_TO_CENTER( AkUInt32 &io_uChannelMask )
{
	if( !(io_uChannelMask & AK_SPEAKER_FRONT_CENTER) 
		&& !(io_uChannelMask & AK_SPEAKER_FRONT_RIGHT)
		&& (io_uChannelMask & AK_SPEAKER_FRONT_LEFT) )
	{
		io_uChannelMask &= ~AK_SPEAKER_FRONT_LEFT;		// remove left
		io_uChannelMask |= AK_SPEAKER_FRONT_CENTER;	// add center
	}
}

inline void AK_SPEAKER_SETUP_FIX_SIDE_TO_REAR( AkUInt32 &io_uChannelMask )
{
	if( io_uChannelMask & AK_SPEAKER_SETUP_SIDE && !( io_uChannelMask & AK_SPEAKER_SETUP_REAR ) )
	{
		io_uChannelMask &= ~AK_SPEAKER_SETUP_SIDE;	// remove sides
		io_uChannelMask |= AK_SPEAKER_SETUP_REAR;	// add rears
	}
}

inline void AK_SPEAKER_SETUP_CONVERT_TO_SUPPORTED( AkUInt32 &io_uChannelMask )
{
	AK_SPEAKER_SETUP_FIX_LEFT_TO_CENTER( io_uChannelMask );
	AK_SPEAKER_SETUP_FIX_SIDE_TO_REAR( io_uChannelMask );
}

/// Returns the number of channels of a given channel configuration.
static inline unsigned int ChannelMaskToNumChannels( AkChannelMask in_uChannelMask )
{
	unsigned int num = 0;
	while( in_uChannelMask ){ ++num; in_uChannelMask &= in_uChannelMask-1; } // iterate max once per channel.
	return num;
}

/// Returns a 'best guess' channel configuration from a given number of channels.
/// Will return 0 if no guess can be made.
static inline AkChannelMask ChannelMaskFromNumChannels( unsigned int in_uNumChannels )
{
	AkChannelMask uChannelMask = 0;

	switch ( in_uNumChannels )
	{
	case 1:
		uChannelMask = AK_SPEAKER_SETUP_1_0_CENTER;
		break;
	case 2:
		uChannelMask = AK_SPEAKER_SETUP_2_0;
		break;
	case 3:
		uChannelMask = AK_SPEAKER_SETUP_2_1;
		break;
	case 4:
		uChannelMask = AK_SPEAKER_SETUP_4_0;
		break;
	case 5:
		uChannelMask = AK_SPEAKER_SETUP_5_0;
		break;
	case 6:
		uChannelMask = AK_SPEAKER_SETUP_5_1;
		break;
	case 7:
		uChannelMask = AK_SPEAKER_SETUP_7;
		break;
	case 8:
		uChannelMask = AK_SPEAKER_SETUP_7POINT1;
		break;
	}

	return uChannelMask;
}

/// Channel ordering type. 
enum AkChannelOrdering
{
	ChannelOrdering_Standard,	// L-R-C-Lfe-RL-RR-SL-SR
	ChannelOrdering_RunTime		// L-R-C-RL-RR-SL-SR-Lfe
};

/// Convert channel indices as they are ordered in standard (wav) or Wwise sound engine (wem) wave files 
/// (which follow channel mask bit values, except that the Lfe is at the end in the case of wems) 
/// into display indices. Desired display order is L-R-C-SL-SR-RL-RR-Lfe. Note that 4-5.x configurations 
/// may define back or side channels. Either way they are "Surround" channels and are assigned to "SL, SR" names.
static inline unsigned int ChannelIndexToDisplayIndex( AkChannelOrdering in_eOrdering, unsigned int in_uChannelMask, unsigned int in_uChannelIdx )
{
	if ( in_eOrdering == ChannelOrdering_Standard )
	{
		unsigned int uNumChannelsFront = ChannelMaskToNumChannels( in_uChannelMask & AK_SPEAKER_SETUP_FRONT );
		if ( ( in_uChannelMask & AK_SPEAKER_LOW_FREQUENCY )
			&& ( in_uChannelIdx == uNumChannelsFront ) )
		{
			// Lfe. Return penultimate channel.
			in_uChannelIdx = ChannelMaskToNumChannels( in_uChannelMask ) - 1;
		}
		else if ( in_uChannelIdx >= uNumChannelsFront )	// strictly greater than uNumChannelsFront (lfe index) if lfe is present, greater or equal otherwise.
		{
			// Back channel. Return index or index-1 if there is an LFE (uLfeOffset==1).
			unsigned int uLfeOffset = ( in_uChannelMask & AK_SPEAKER_LOW_FREQUENCY ) ? 1 : 0;

			// 6-7.x: Need to swap back and sides.
			if ( ( in_uChannelMask & AK_SPEAKER_SETUP_REAR ) && ( in_uChannelMask & AK_SPEAKER_SETUP_SIDE ) )
			{
				unsigned int uRearIdx = uNumChannelsFront + uLfeOffset;
				unsigned int uSideIdx = uRearIdx + 2;
				if ( in_uChannelIdx >= uSideIdx )
					in_uChannelIdx -= 2;	// input is side, swap it with back.
				else
					in_uChannelIdx += 2;	// input is back, swap it with side.
			}
			in_uChannelIdx -= uLfeOffset;	// compensate for LFE if it was skipped above.
		}
	}
	else
	{
		// 6-7.x: Need to swap back and sides.
		if ( ( in_uChannelMask & AK_SPEAKER_SETUP_REAR ) && ( in_uChannelMask & AK_SPEAKER_SETUP_SIDE ) )
		{
			unsigned int uRearIdx = ChannelMaskToNumChannels( in_uChannelMask & AK_SPEAKER_SETUP_FRONT );
			unsigned int uMaxIdx = ChannelMaskToNumChannels( in_uChannelMask & ~AK_SPEAKER_LOW_FREQUENCY );

			if ( in_uChannelIdx >= uRearIdx  
				&& in_uChannelIdx < uMaxIdx )
			{
				// Surround channel (not LFE).
				unsigned int uSideIdx = uRearIdx + 2;
				if ( in_uChannelIdx >= uSideIdx )
					in_uChannelIdx -= 2;	// input is side, swap it with back.
				else
					in_uChannelIdx += 2;	// input is back, swap it with side.
			}
		}
	}

	return in_uChannelIdx;
}

#endif //_AK_SPEAKERCONFIG_H_
