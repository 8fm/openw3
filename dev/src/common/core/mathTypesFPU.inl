/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

typedef struct
{
	Float v[4];

	const Float& operator [](Int32 i) const { return v[i]; }
	Float& operator [](Int32 i) { return v[i]; }
} VecReg;