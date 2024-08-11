/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "inputKeys.h"

// Input action
enum EInputAction : Int32
{
	IACT_None=0,		//!< No action
	IACT_Press=1,		//!< Button was pressed 
	IACT_Release=2,		//!< Button was released
	IACT_Axis=3,		//!< Axis controller was moved
};

BEGIN_ENUM_RTTI( EInputAction );
ENUM_OPTION( IACT_None );
ENUM_OPTION( IACT_Press );
ENUM_OPTION( IACT_Release );
ENUM_OPTION( IACT_Axis );
END_ENUM_RTTI();

struct SBufferedInputEvent
{
	EInputKey		key;
	EInputAction	action;
	Float			data;
	Int32			x;
	Int32			y;

	explicit SBufferedInputEvent( const EInputKey& _key = IK_None, const EInputAction& _action = IACT_None, Float _data = 0.f )
		: key( _key )
		, action( _action )
		, data( _data )
	{};
};

typedef TDynArray< SBufferedInputEvent > BufferedInput;
