// This code contains NVIDIA Confidential Information and is disclosed to you
// under a form of NVIDIA software license agreement provided separately to you.
//
// Notice
// NVIDIA Corporation and its licensors retain all intellectual property and
// proprietary rights in and to this software and related documentation and
// any modifications thereto. Any use, reproduction, disclosure, or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA Corporation is strictly prohibited.
//
// ALL NVIDIA DESIGN SPECIFICATIONS, CODE ARE PROVIDED "AS IS.". NVIDIA MAKES
// NO WARRANTIES, EXPRESSED, IMPLIED, STATUTORY, OR OTHERWISE WITH RESPECT TO
// THE MATERIALS, AND EXPRESSLY DISCLAIMS ALL IMPLIED WARRANTIES OF NONINFRINGEMENT,
// MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE.
//
// Information and code furnished is believed to be accurate and reliable.
// However, NVIDIA Corporation assumes no responsibility for the consequences of use of such
// information or for any infringement of patents or other rights of third parties that may
// result from its use. No license is granted by implication or otherwise under any patent
// or patent rights of NVIDIA Corporation. Details are subject to change without notice.
// This code supersedes and replaces all information previously supplied.
// NVIDIA Corporation products are not authorized for use as critical
// components in life support devices or systems without express written approval of
// NVIDIA Corporation.
//
// Copyright (c) 2008-2013 NVIDIA Corporation. All rights reserved.

#ifndef NOISE_H
#define NOISE_H

#include "authoring/ApexCSGMath.h"

#include "NoiseUtils.h"

#ifndef WITHOUT_APEX_AUTHORING
 
namespace ApexCSG 
{

class UserRandom;

/**
	Provides Perlin noise sampling across multiple dimensions and for different data types
*/
template<typename T, int SampleSize = 1024, int D = 3, class VecType = Vec<T, D> >
class PerlinNoise
{
public:
	PerlinNoise(UserRandom& rnd, int octaves = 1, T frequency = 1., T amplitude = 1.)
	  : mRnd(rnd),
		mOctaves(octaves),
		mFrequency(frequency),
		mAmplitude(amplitude),
		mbInit(false)
	{

	}

	void reset(int octaves = 1, T frequency = (T)1., T amplitude = (T)1.)
	{
		mOctaves   = octaves;
		mFrequency = frequency;
		mAmplitude = amplitude;
		init();
	}

	T sample(const VecType& point) 
	{
		return perlinNoise(point);
	}

private:
	PerlinNoise& operator=(const PerlinNoise&);

	T perlinNoise(VecType point)
	{
		if (!mbInit)
			init();

		const int octaves  = mOctaves;
		const T frequency  = mFrequency;
		T amplitude        = mAmplitude;
		T result           = (T)0;

		point *= frequency;

		for (int i = 0; i < octaves; ++i)
		{
			result    += noiseSample<T, SampleSize>(point, p, g) * amplitude;
			point     *= (T)2.0;
			amplitude *= (T)0.5;
		}

		return result;
	}

	void init(void)
	{
		mbInit = true;

		int i, j, k;

		for (i = 0 ; i < SampleSize; i++)
		{
			p[i]  = i;
			for (j = 0; j < D; ++j)
				g[i][j] = (T)((mRnd.getInt() % (SampleSize + SampleSize)) - SampleSize) / SampleSize;
			g[i].normalize();
		}

		while (--i)
		{
			k    = p[i];
			p[i] = p[j = mRnd.getInt() % SampleSize];
			p[j] = k;
		}

		for (i = 0 ; i < SampleSize + 2; ++i)
		{
			p [SampleSize + i] =  p[i];
			for (j = 0; j < D; ++j)
				g[SampleSize + i][j] = g[i][j];
		}

	}

	UserRandom& mRnd;
	int   mOctaves;
	T     mFrequency;
	T     mAmplitude;

	// Permutation vector
	int p[SampleSize + SampleSize + 2];
	// Gradient vector
	VecType g[SampleSize + SampleSize + 2];

	bool  mbInit;
};

}

#endif /* #ifndef WITHOUT_APEX_AUTHORING */

#endif /* #ifndef NOISE_H */

