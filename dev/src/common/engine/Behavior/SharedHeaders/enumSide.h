#pragma once
#include "..\..\..\core\enumBuilder.h"


enum ESide
{
	S_Unknown = -1,
	S_Left,
	S_Right
};

BEGIN_ENUM_RTTI( ESide );
	ENUM_OPTION( S_Unknown );
	ENUM_OPTION( S_Left );
	ENUM_OPTION( S_Right );
END_ENUM_RTTI();

