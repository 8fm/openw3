/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "foliageInstance.h"

RED_INLINE Vector3 SFoliageInstance::GetUpVector() const
{
	return Vector3( 0.0f, 0.0f, 1.0f );
}

RED_INLINE Vector3 SFoliageInstance::GetRightVector() const
{
	const Float x = 2.0f * ( - m_w * m_z );
	const Float y = 1.0f - 2.0f * ( m_z * m_z );
	const Float z = 0.0f;
	return Vector3( x, y, z );
}

RED_INLINE Vector SFoliageInstance::GetQuaterion() const
{
	return Vector( 0.0f, 0.0f, m_z, m_w );
}

RED_INLINE const Vector3 & SFoliageInstance::GetPosition() const
{
	return m_position;
}

RED_INLINE Float SFoliageInstance::GetScale() const
{
	return m_scale;
}
