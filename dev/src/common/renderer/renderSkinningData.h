/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once 
#include "../engine/renderSkinningData.h"

typedef TDynArray< Float, MC_SkinningData > TSkinningDataArray;

/// Skinning data for rendering
class CRenderSkinningData : public IRenderSkinningData
{
protected:
	TSkinningDataArray		m_dataBuffer;	//!< Matrices, 3 vectors per one
	Uint32					m_numMatrices;	//!< Number of matrices
	Vector					m_bindData;		//!< offset into skinning texture

	Float*					m_writePtr;		//!< Should be advanced only on the game side
	Float*					m_readPtr;		//!< Should be advanced only on the render side

	Int16					m_customMatrixIndices[SDCM_MAX];
	Vector3					m_customHeadPos;
	Vector3					m_customHeadFrontDir;
	Vector3					m_customHeadUpDir;

	Bool					m_registerInSkinManager;	//!< If it uses skin texture or not

public:
	//! Get number of matrices
	RED_INLINE Uint32 GetNumMatrices() const { return m_numMatrices; }

	//! Get skinning data
	const void* GetReadData() const
	{ 
		return m_readPtr;
	}

	virtual void* GetWriteData() const
	{
		return m_writePtr;
	}

	virtual Uint32 GetMatrixCount() const override
	{
		return m_numMatrices;
	}

	void AdvanceRead();
	void AdvanceWrite();

	RED_INLINE void Pin(const Vector& bindData)
	{
		m_bindData = bindData;
	}

	virtual ESkinningDataMatrixType GetMatrixType() const 
	{ 
		//return SDMT_3x4Transposed;
		// i want 16Float matrices to contain some additional info
		return SDMT_4x4;
	}

public:
	virtual void SetCustomMatrixIndex( ESkinningDataCustomMatrix type, Int16 index );
	virtual Bool GetCustomMatrix( ESkinningDataCustomMatrix type, Matrix &outMatrix ) const;
	virtual void SetCustomHeadData( const Vector &pos, const Vector &frontDirection, const Vector &upDirection );
	virtual void ResetCustomHeadData();
	virtual Bool GetCustomHeadData( Vector &outPos, Vector &outFrontDirection, Vector &outUpDirection ) const;

public:
	CRenderSkinningData( Uint32 numMatrices, Bool registerInSkinManager = true );
	virtual ~CRenderSkinningData();

	//! Bind to shader constants
	void Bind();	

	const Vector& GetBindData() const {return m_bindData;}
};

/// Skinning data for rendering
class CRenderSkinningDataSimple : public IRenderSkinningData
{
protected:
	TSkinningDataArray		m_dataBuffer;	//!< Matrices, 3 vectors per one
	Uint32					m_numMatrices;	//!< Number of matrices

public:
	//! Get number of matrices
	RED_INLINE Uint32 GetNumMatrices() const { return m_numMatrices; }

	//! Get skinning data
	const void* GetReadData() const 
	{ 
		return m_dataBuffer.Data();
	}

	virtual void* GetWriteData() const
	{
		return const_cast< void* >( m_dataBuffer.Data() );
	}

	virtual Uint32 GetMatrixCount() const override
	{
		return m_numMatrices;
	}

	virtual ESkinningDataMatrixType GetMatrixType() const { return SDMT_4x4; }

public:
	CRenderSkinningDataSimple( Uint32 numMatrices );
	virtual ~CRenderSkinningDataSimple();

	//! Bind to shader constants
	void Bind();
};

/// Skinning data that isn't needed on render side. Just writes to a provided data buffer. Assumes the buffer stays alive.
class CRenderSkinningDataEngineBuffer : public IRenderSkinningData
{
protected:
	TSkinningDataArray		m_dataBuffer;	//!< 4x4 Matrices
	Uint32					m_numMatrices;

public:
	virtual Uint32 GetMatrixCount() const override
	{
		return m_numMatrices;
	}

	virtual void* GetWriteData() const 
	{ 
		return const_cast< void* >( m_dataBuffer.Data() );
	}

	virtual ESkinningDataMatrixType GetMatrixType() const { return SDMT_4x4; }

public:
	CRenderSkinningDataEngineBuffer( Uint32 numMatrices );
	virtual ~CRenderSkinningDataEngineBuffer();
};
