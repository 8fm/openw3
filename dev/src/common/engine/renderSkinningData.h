#pragma once

#include "renderObject.h"

enum ESkinningDataMatrixType : CEnum::TValueType
{
	SDMT_3x4Transposed,		//!< 3x4 transposed matrix, as used in "extended" skinning data.
	SDMT_4x4				//!< 4x4 matrix, as used in "simple" skinning data.
};

enum ESkinningDataCustomMatrix
{
	SDCM_EyeOrientationLeft,
	SDCM_EyeOrientationRight,
	SDCM_HeadTransformation,

	SDCM_MAX
};

/// Skinning data
class IRenderSkinningData : public IRenderObject
{
public:
	virtual Uint32 GetMatrixCount() const = 0;
	virtual void AdvanceWrite() {};
	virtual void* GetWriteData() const=0;
	virtual ESkinningDataMatrixType GetMatrixType() const=0;
	virtual void SetCustomMatrixIndex( ESkinningDataCustomMatrix type, Int16 index ) {}
	virtual void SetCustomHeadData( const Vector &pos, const Vector &frontDirection, const Vector &upDirection ) {}
	virtual void ResetCustomHeadData() {}
	virtual Bool GetCustomMatrix( ESkinningDataCustomMatrix type, Matrix &outMatrix ) const { return false; }
	virtual Bool GetCustomHeadData( Vector &outPos, Vector &outFrontDirection, Vector &outUpDirection ) const { return false; }
};

