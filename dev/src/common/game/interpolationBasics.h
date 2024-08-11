// Copyright © 2014 CD Projekt Red. All Rights Reserved.

#pragma once

// =================================================================================================
namespace StoryScene {
// =================================================================================================

/*
Describes channel.
*/
enum class EInterpolationChannelType
{
	ICT_Normal,
	ICT_AngleInDegrees
};

/*
Defines range of interpolation channels.

\param channelFirst First channel.
\param channelLast Last channel (it's included in the range).
*/
template < Uint32 channelFirst, Uint32 channelLast >
class InterpolationChannelRange
{
public:
	static const Uint32 first = channelFirst;
	static const Uint32 last = channelLast;
};

/*
Defines empty range of interpolation channels.
*/
class InterpolationChannelRangeNone {};

/*
Interpolation traits.

This class template has no definition. See interpolationEventClassGenerator.h docs for
more info on how to use this class template.
*/
template < typename T >
class InterpolationTraits;

// =================================================================================================
} // namespace StoryScene
// =================================================================================================

/*

This is not in StoryScene namespace because our RTTI won't let us do this.
*/
enum EInterpolationMethod
{
	IM_Constant,				// no interpolation
	IM_Linear,					// linear interpolation
	IM_Bezier					// interpolation using cubic Bezier 2d
};

BEGIN_ENUM_RTTI( EInterpolationMethod );
	ENUM_OPTION( IM_Constant );
	ENUM_OPTION( IM_Linear );
	ENUM_OPTION( IM_Bezier );
END_ENUM_RTTI();

/*

This is not in StoryScene namespace because our RTTI won't let us do this.
*/
enum EInterpolationEasingStyle
{
	IES_Smooth,
	IES_Rapid
};

BEGIN_ENUM_RTTI( EInterpolationEasingStyle );
	ENUM_OPTION( IES_Smooth );
	ENUM_OPTION( IES_Rapid );
END_ENUM_RTTI();
