/***********************************************************************
  The content of this file includes source code for the sound engine
  portion of the AUDIOKINETIC Wwise Technology and constitutes "Level
  Two Source Code" as defined in the Source Code Addendum attached
  with this file.  Any use of the Level Two Source Code shall be
  subject to the terms and conditions outlined in the Source Code
  Addendum and the End User License Agreement for Wwise(R).

  Version: v2013.2.9  Build: 4872
  Copyright (c) 2006-2014 Audiokinetic Inc.
 ***********************************************************************/

// Some math approximation that may be useful to speed things up (particularly on PowerPC based architectures)
// WARNING: You should always ensure that the error that these routines yield for the input range you are using is acceptable

#include "math.h"
#include "float.h"
static const float shift23 = (1<<23);
static const float OOshift23 = 1.f / (1<<23);

#if defined(AK_XBOX360) || defined(AK_WII) || defined(AK_WIIU)
AkForceInline AkReal32 Log2Approx(AkReal32 fVal)
{
	AKASSERT( fVal > 0.f );
	AkReal32 LogBodge=0.346607f;
	AkReal32 x;
	AkReal32 y;
	x=(AkReal32)(*(int *)&fVal);
	x*= OOshift23; //1/pow(2,23);
	x=x-127.f;

	y=x-floorf(x);
	y=(y-y*y)*LogBodge;
	return x+y;
}

AkForceInline AkReal32 Pow2Approx(AkReal32 fVal)
{
	static const AkReal32 PowBodge=0.33971f;
	AkReal32 x;
	AkReal32 y=fVal-floorf(fVal);
	y=(y-y*y)*PowBodge;

	x=fVal+127.f-y;
	x*= shift23; //pow(2,23);
	*(int*)&x=(int)x;
	return x;
}

AkForceInline AkReal32 PowApprox(AkReal32 fBase, AkReal32 fExp)
{
	AKASSERT( fBase >= 0.f ); // Does not handle -a^b cases
	AkReal32 fResult;
	if ( fBase > 0.f )
		fResult = Pow2Approx(fExp*Log2Approx(fBase));
	else
	{
		AKASSERT( fExp != 0.f ); // Does not handle 0.f^0.f case
		fResult = 0.f;
	}
	return fResult;
}

AkForceInline AkReal32 ExpApprox(AkReal32 fVal)
{
	static const AkReal32 E = 2.71828182845904523536f;
	return PowApprox(E,fVal);
}

AkForceInline AkReal32 SqrtApprox(AkReal32 fVal)
{
	// Avoid log of zero problems
	AkReal32 fResult;
	if ( fVal > 0.f )
		fResult = Pow2Approx(0.5f * Log2Approx(fVal));
	else
		fResult = 0.f;
	return fResult;
}
#else

// Do not use approximation on the SPU (above code does not work)

AkForceInline AkReal32 Log2Approx(AkReal32 fVal)
{
	static const AkReal32 fOneOverLog2 = 1.442695040888963f;
	return log(fVal) * fOneOverLog2; 
}

AkForceInline AkReal32 Pow2Approx(AkReal32 fVal)
{
	return pow(2.f,fVal);
}

AkForceInline AkReal32 PowApprox(AkReal32 fBase, AkReal32 fExp)
{
	return pow(fBase,fExp);
}

AkForceInline AkReal32 ExpApprox(AkReal32 fVal)
{
	return exp(fVal);
}

AkForceInline AkReal32 SqrtApprox(AkReal32 fVal)
{
	return sqrt(fVal);
}

#endif