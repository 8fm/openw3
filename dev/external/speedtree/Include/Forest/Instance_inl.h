///////////////////////////////////////////////////////////////////////  
//  Instance.inl
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
//  CInstance::CInstance

ST_INLINE CInstance::CInstance( ) :
	m_pInstanceOf(NULL),
	m_fScalar(1.0f)
{
	SetOrientation(CCoordSys::UpAxis( ), CCoordSys::RightAxis( ));
}


///////////////////////////////////////////////////////////////////////
//  CInstance::~CInstance

ST_INLINE CInstance::~CInstance( )
{
}


///////////////////////////////////////////////////////////////////////  
//  CInstance::SetPos

ST_INLINE void CInstance::SetPos(const Vec3& vPos)
{
	m_vPos = vPos;
}


///////////////////////////////////////////////////////////////////////  
//  CInstance::SetScalar

ST_INLINE void CInstance::SetScalar(st_float32 fScalar)
{
	m_fScalar = fScalar;
}


///////////////////////////////////////////////////////////////////////  
//  CInstance::SetOrientation

ST_INLINE void CInstance::SetOrientation(const Vec3& , const Vec3& vRight)
{
	Vec3 vRightNorm(vRight);
	vRightNorm.Normalize( );

	#ifdef SPEEDTREE_COMPRESS_INSTANCE_VECTORS
		CCore::CompressVec3(m_auiRight, vRightNorm);
	#else
		m_vRight = vRightNorm;
	#endif
}


///////////////////////////////////////////////////////////////////////  
//  CInstance::SetInstanceOf

ST_INLINE void CInstance::SetInstanceOf(const CTree* pBaseTree)
{
	m_pInstanceOf = pBaseTree;
}


///////////////////////////////////////////////////////////////////////  
//  CInstance::InstanceOf

ST_INLINE const CTree* CInstance::InstanceOf(void) const
{
	return m_pInstanceOf;
}


///////////////////////////////////////////////////////////////////////  
//  CInstance::GetPos

ST_INLINE const Vec3& CInstance::GetPos(void) const
{
	return m_vPos;
}


///////////////////////////////////////////////////////////////////////  
//  CInstance::GetScalar

ST_INLINE st_float32 CInstance::GetScalar(void) const
{
	return m_fScalar;
}


///////////////////////////////////////////////////////////////////////  
//  CInstance::GetUpVector

ST_INLINE Vec3 CInstance::GetUpVector(void) const
{
	#ifdef SPEEDTREE_COMPRESS_INSTANCE_VECTORS
		return Vec3( 0.0f, 0.0f, 1.0f ); // ctremblay +- up vector are always the same.
	#else
		return Vec3( 0.0f, 0.0f, 1.0f ); // ctremblay +- up vector are always the same.
	#endif
}


///////////////////////////////////////////////////////////////////////  
//  CInstance::GetRightVector

ST_INLINE Vec3 CInstance::GetRightVector(void) const
{
	#ifdef SPEEDTREE_COMPRESS_INSTANCE_VECTORS
		return CCore::UncompressVec3(m_auiRight);
	#else
		return m_vRight;
	#endif
}


///////////////////////////////////////////////////////////////////////
//  CTreeInstance::CTreeInstance

ST_INLINE CTreeInstance::CTreeInstance( ) :
	m_fCullingRadius(-1.0f),
	m_pUserData(NULL)
{
}


///////////////////////////////////////////////////////////////////////
//  CTreeInstance::~CTreeInstance

ST_INLINE CTreeInstance::~CTreeInstance( )
{
}


///////////////////////////////////////////////////////////////////////
//  CTreeInstance::ComputeCullParameters

ST_INLINE void CTreeInstance::ComputeCullParameters(void)
{
	st_assert(m_pInstanceOf, "CTreeInstance must know its base tree before most operations can occur");

	// the geometric center is computed by rotating and scaling the base tree's
	// extents based on this instance's parameters and then computing the center
	// of the new extents
	CExtents cBaseExtents = InstanceOf( )->GetExtents( );
	cBaseExtents.Scale(GetScalar( ));
	cBaseExtents.Orient(GetUpVector( ), GetRightVector( ));

	m_vGeometricCenter = cBaseExtents.GetCenter( ) + GetPos( );

	// the culling radius is measured as the longest distance from the center of
	// new box to one of the corners (all corners are equidistant from the center)
	m_fCullingRadius = cBaseExtents.ComputeRadiusFromCenter3D( );
}


///////////////////////////////////////////////////////////////////////  
//  CTreeInstance::GetGeometricCenter

ST_INLINE const Vec3& CTreeInstance::GetGeometricCenter(void) const
{
	return m_vGeometricCenter;
}


///////////////////////////////////////////////////////////////////////  
//  CTreeInstance::GetCullingRadius

ST_INLINE st_float32 CTreeInstance::GetCullingRadius(void) const
{
	return m_fCullingRadius;
}


///////////////////////////////////////////////////////////////////////
//  CTreeInstance::GetUserData

ST_INLINE void* CTreeInstance::GetUserData(void) const	
{
	return m_pUserData;
}


///////////////////////////////////////////////////////////////////////
//  CTreeInstance::SetUserData

ST_INLINE void CTreeInstance::SetUserData(void* pUserData)
{
	m_pUserData = pUserData;
}
