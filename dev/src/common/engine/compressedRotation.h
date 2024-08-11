
#pragma once

enum ECompressedRotationType
{
	CRT_32 = 0,
	CRT_64,
	CRT_None,
};

BEGIN_ENUM_RTTI( ECompressedRotationType );
	ENUM_OPTION( CRT_32 );
	ENUM_OPTION( CRT_64 );
	ENUM_OPTION( CRT_None );
END_ENUM_RTTI();
