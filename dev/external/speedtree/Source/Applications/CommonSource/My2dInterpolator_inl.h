///////////////////////////////////////////////////////////////////////  
//  My2dInterpolator.h
//
//	*** INTERACTIVE DATA VISUALIZATION (IDV) CONFIDENTIAL AND PROPRIETARY INFORMATION ***
//
//	This software is supplied under the terms of a license agreement or
//	nondisclosure agreement with Interactive Data Visualization, Inc. and
//  may not be copied, disclosed, or exploited except in accordance with 
//  the terms of that agreement.
//
//      Copyright (c) 2003-2014 IDV, Inc.
//      All rights reserved in all media.
//
//      IDV, Inc.
//      http://www.idvinc.com


///////////////////////////////////////////////////////////////////////  
//  CMy2dInterpolator::Clear

template <class T>
inline void CMy2dInterpolator<T>::Clear(void)
{
	if (m_pData != NULL)
	{
		st_delete_array<T>(m_pData);
		m_pData = NULL;
	}

	m_nWidth = 0;
	m_nHeight = 0;
}


///////////////////////////////////////////////////////////////////////  
//  CMy2dInterpolator::NearestNeighbor

template <class T>
inline T CMy2dInterpolator<T>::NearestNeighbor(st_float32 fU, st_float32 fV) const
{
	st_assert(m_nWidth > 0 && m_nHeight > 0 && m_pData, "CMy2dInterpolator::NearestNeighbor() called before CMy2dInterpolator initialized");

	fU -= st_int32(fU);
	if (fU < 0.0f)
		fU += 1.0f;
	if (fU >= 1.0f)
		fU = 0.0f;

	fV -= st_int32(fV);
	if (fV < 0.0f)
		fV += 1.0f;
	if (fV >= 1.0f)
		fV = 0.0f;

	const st_int32 nU = st_int32(fU * m_nWidth + 0.5f);
	const st_int32 nV = st_int32(fV * m_nHeight + 0.5f);

	return m_pData[nU + nV * m_nWidth];
}


///////////////////////////////////////////////////////////////////////  
//  CMy2dInterpolator::InterpolateValue

template <class T>
inline T CMy2dInterpolator<T>::InterpolateValue(st_float32 fU, st_float32 fV) const
{
	st_assert(m_nWidth > 0 && m_nHeight > 0 && m_pData, "CMy2dInterpolator::InterpolateValue() called before CMy2dInterpolator initialized");

	fU -= st_int32(fU);
	if (fU < 0.0f)
		fU += 1.0f;
	if (fU >= 1.0f)
		fU = 0.0f;

	fV -= st_int32(fV);
	if (fV < 0.0f)
		fV += 1.0f;
	if (fV >= 1.0f)
		fV = 0.0f;

	const st_int32 nLowerX = st_uint32(fU * st_float32(m_nWidth));
	const st_int32 nLowerY = st_uint32(fV * st_float32(m_nHeight));
	const st_int32 nHigherX = (nLowerX + 1 != m_nWidth) ? nLowerX + 1 : 0;
	const st_int32 nHigherY = (nLowerY + 1 != m_nHeight) ? nLowerY + 1 : 0;

	// determine how far into quad we are
	fU = Frac(fU * st_float32(m_nWidth));
	fV = Frac(fV * st_float32(m_nHeight));

	// because quad is triangulated, we have to know if it's the top or bottom triangle
	st_bool bLowerTriangle = (fU < 1.0f - fV);

	const T& tLowerLeft = m_pData[nLowerX + nLowerY * m_nWidth];
	const T& tLowerRight = m_pData[nHigherX + nLowerY * m_nWidth];
	const T& tUpperLeft = m_pData[nLowerX + nHigherY * m_nWidth];
	const T& tUpperRight = m_pData[nHigherX + nHigherY * m_nWidth];

	if (bLowerTriangle)
	{
		st_float32 fWeight1 = fV;
		st_float32 fWeight2 = 1.0f - fU - fV;
		st_float32 fWeight3 = 1.0f - fWeight1 - fWeight2;

		return tUpperLeft * fWeight1 + tLowerLeft * fWeight2 + tLowerRight * fWeight3;
	}
	else
	{
		st_float32 fWeight1 = 1.0f - fU;
		st_float32 fWeight2 = fU + fV - 1.0f;
		st_float32 fWeight3 = 1.0f - fWeight1 - fWeight2;

		return tUpperLeft * fWeight1 + tUpperRight * fWeight2 + tLowerRight * fWeight3;
	}
}


///////////////////////////////////////////////////////////////////////  
//  CMy2dInterpolator::InterpolateValueClamped

template <class T>
inline T CMy2dInterpolator<T>::InterpolateValueClamped(st_float32 fU, st_float32 fV) const
{
	st_assert(m_nWidth > 0 && m_nHeight > 0 && m_pData, "CMy2dInterpolator::InterpolateValueClamped() called before CMy2dInterpolator initialized");

	if (fU < 0.0f)
		fU = 0.0f;
	if (fU > 1.0f)
		fU = 1.0f;

	if (fV < 0.0f)
		fV = 0.0f;
	if (fV > 1.0f)
		fV = 1.0f;

	st_float32 fWidthScale = st_float32(m_nWidth - 1);
	st_float32 fHeightScale = st_float32(m_nHeight - 1);

	const st_int32 nLowerX = st_int32(fU * fWidthScale);
	const st_int32 nLowerY = st_int32(fV * fHeightScale);
	const st_int32 nHigherX = (nLowerX + 1 != m_nWidth) ? nLowerX + 1 : 0;
	const st_int32 nHigherY = (nLowerY + 1 != m_nHeight) ? nLowerY + 1 : 0;

	// determine how far into quad we are
	fU = Frac(fU * fWidthScale);
	fV = Frac(fV * fHeightScale);

	// because quad is triangulated, we have to know if it's the top or bottom triangle
	st_bool bLowerTriangle = (fU < 1.0f - fV);

	const T& tLowerLeft = m_pData[nLowerX + nLowerY * m_nWidth];
	const T& tLowerRight = m_pData[nHigherX + nLowerY * m_nWidth];
	const T& tUpperLeft = m_pData[nLowerX + nHigherY * m_nWidth];
	const T& tUpperRight = m_pData[nHigherX + nHigherY * m_nWidth];

	if (bLowerTriangle)
	{
		st_float32 fWeight1 = fV;
		st_float32 fWeight2 = 1.0f - fU - fV;
		st_float32 fWeight3 = 1.0f - fWeight1 - fWeight2;

		return tUpperLeft * fWeight1 + tLowerLeft * fWeight2 + tLowerRight * fWeight3;
	}
	else
	{
		st_float32 fWeight1 = 1.0f - fU;
		st_float32 fWeight2 = fU + fV - 1.0f;
		st_float32 fWeight3 = 1.0f - fWeight1 - fWeight2;

		return tUpperLeft * fWeight1 + tUpperRight * fWeight2 + tLowerRight * fWeight3;
	}
}


///////////////////////////////////////////////////////////////////////  
//  CMy2dInterpolator::Smooth

template <class T>
inline T CMy2dInterpolator<T>::Smooth(st_float32 fU, st_float32 fV, st_float32 fDistance, st_float32 fSlope, st_bool bClamped) const
{
	T tReturn;

	if (bClamped)
		tReturn = InterpolateValueClamped(fU, fV);
	else
		tReturn = InterpolateValue(fU, fV);

	if (fDistance > 0.0f)
	{
		const st_float32 afTestPoints[8][2] = 
		{
			{  1.0f * fDistance,   0.0f * fDistance },
			{ -1.0f * fDistance,   0.0f * fDistance },
			{  0.0f * fDistance,   1.0f * fDistance },
			{  0.0f * fDistance,  -1.0f * fDistance },
			{  0.333f * fDistance,  0.333f * fDistance },
			{  0.333f * fDistance, -0.333f * fDistance },
			{ -0.333f * fDistance,  0.333f * fDistance },
			{ -0.333f * fDistance, -0.333f * fDistance }
		};

		st_float32 fTotalWeight = 1.0f;
		T tMax = tReturn;
		for (st_uint32 i = 0; i < 8; ++i)
		{
			const st_float32* pTestPoint = afTestPoints[i];
			
			T tThis;
			if (bClamped)
				tThis = InterpolateValueClamped(fU + pTestPoint[0], fV + pTestPoint[1]);
			else
				tThis = InterpolateValue(fU + pTestPoint[0], fV + pTestPoint[1]);
			if (tThis > tMax)
				tMax = tThis;

			st_float32 fWeight = (1.0f - sqrt((pTestPoint[0] * pTestPoint[0]) + (pTestPoint[1] * pTestPoint[1])));
			fTotalWeight += fWeight;
			tReturn += fWeight * tThis;
		}	
		tReturn /= fTotalWeight;

		tReturn = Interpolate(tReturn, tMax, fSlope);
	}

	return tReturn;
}


///////////////////////////////////////////////////////////////////////  
//  CMy2dInterpolator::IsPresent

template <class T>
inline st_bool CMy2dInterpolator<T>::IsPresent(void) const
{
	return (m_pData != NULL);
}

