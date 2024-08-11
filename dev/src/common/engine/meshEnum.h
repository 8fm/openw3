/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../core/enum.h"

//---------------------------------------------------------------

/// Mesh chunk shadow cascade masks
enum EMeshChunkRenderMask : Uint8
{
	MCR_Scene				= FLAG(0),
	MCR_Cascade1			= FLAG(1),
	MCR_Cascade2			= FLAG(2),
	MCR_Cascade3			= FLAG(3),
	MCR_Cascade4			= FLAG(4),
	MCR_LocalShadows		= FLAG(5),
};

BEGIN_BITFIELD_RTTI( EMeshChunkRenderMask, 1 /*bytes*/ );
	BITFIELD_OPTION( MCR_Scene );
	BITFIELD_OPTION( MCR_Cascade1 );
	BITFIELD_OPTION( MCR_Cascade2 );
	BITFIELD_OPTION( MCR_Cascade3 );
	BITFIELD_OPTION( MCR_Cascade4 );
	BITFIELD_OPTION( MCR_LocalShadows );
END_BITFIELD_RTTI();

//---------------------------------------------------------------

/// Bias on mesh shadow rendering distance
enum EMeshShadowImportanceBias : CEnum::TValueType
{
	MSIB_EvenLessImportant=-2,
	MSIB_LessImportant=-1,
	MSIB_Default=0,
	MSIB_MoreImportant=1,
	MSIB_EvenMoreImportant=2,
};

BEGIN_ENUM_RTTI( EMeshShadowImportanceBias );
	ENUM_OPTION( MSIB_EvenLessImportant );
	ENUM_OPTION( MSIB_LessImportant );
	ENUM_OPTION( MSIB_Default );
	ENUM_OPTION( MSIB_MoreImportant );
	ENUM_OPTION( MSIB_EvenMoreImportant );
END_ENUM_RTTI();

//---------------------------------------------------------------

