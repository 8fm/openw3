
#pragma once

#include "../core/bitFieldBuilder.h"

enum EActorLodFlag
{
	AL_Visible				= FLAG( 0 ),
	AL_Visible_Time1		= FLAG( 1 ),
	AL_Visible_Time2		= FLAG( 2 ),
	AL_Visible_Time3		= FLAG( 3 ),
	AL_NotVisible			= FLAG( 4 ),
	AL_NotVisible_Time1		= FLAG( 5 ),
	AL_NotVisible_Time2		= FLAG( 6 ),
	AL_NotVisible_Time3		= FLAG( 7 ),
	AL_Camera_Range1		= FLAG( 8 ),
	AL_Camera_Range1_Time1	= FLAG( 9 ),
	AL_Camera_Range1_Time2	= FLAG( 10 ),
	AL_Camera_Range2		= FLAG( 11 ),
	AL_Camera_Range2_Time1	= FLAG( 12 ),
	AL_Camera_Range2_Time2	= FLAG( 13 ),
	AL_Camera_Range3		= FLAG( 14 ),
	AL_Camera_Range3_Time1	= FLAG( 15 ),
	AL_Camera_Range3_Time2	= FLAG( 16 ),
};

BEGIN_BITFIELD_RTTI( EActorLodFlag, 4 );
	BITFIELD_OPTION( AL_Visible );
	BITFIELD_OPTION( AL_Visible_Time1 );
	BITFIELD_OPTION( AL_Visible_Time2 );
	BITFIELD_OPTION( AL_Visible_Time3 );
	BITFIELD_OPTION( AL_NotVisible );
	BITFIELD_OPTION( AL_NotVisible_Time1 );
	BITFIELD_OPTION( AL_NotVisible_Time2 );
	BITFIELD_OPTION( AL_NotVisible_Time3 );
	BITFIELD_OPTION( AL_Camera_Range1 );
	BITFIELD_OPTION( AL_Camera_Range1_Time1 );
	BITFIELD_OPTION( AL_Camera_Range1_Time2 );
	BITFIELD_OPTION( AL_Camera_Range2 );
	BITFIELD_OPTION( AL_Camera_Range2_Time1 );
	BITFIELD_OPTION( AL_Camera_Range2_Time2 );
	BITFIELD_OPTION( AL_Camera_Range3 );
	BITFIELD_OPTION( AL_Camera_Range3_Time1 );
	BITFIELD_OPTION( AL_Camera_Range3_Time2 );
END_BITFIELD_RTTI();

class IActorLodInterface
{
public:
	virtual EActorLodFlag GetLevel() const = 0;
	virtual Bool TestOr( Uint32 flagOr ) const = 0;
	virtual Bool TestAnd( Uint32 flagAnd ) const = 0;
	virtual Bool TestAndOr( Uint32 flagAnd, Uint32 flagOr ) const = 0;
};
