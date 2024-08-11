/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
* Kamil Nowakowski
*/
#pragma once

class CRenderDistantLightBatcher : public IDynamicRenderResource
{

private:

	TDynArray< GpuApi::SystemVertex_PosColorFUV >	m_vertexArray;	//!< Array of instanced lights
	GpuApi::SystemVertex_PosColorFUV*				m_ptr;			//!< Pointer to fast go throught all array
	GpuApi::SystemVertex_PosColorFUV*				m_ptrEnd;		//!< 

public:

	CRenderDistantLightBatcher();
	~CRenderDistantLightBatcher();

	//! Is batcher empty ?
	RED_INLINE Bool	Empty() const { return m_vertexArray.Empty(); }
	
	//! Reserve some memory and set up filling pointer
	void PredictedNumberOfLights( Uint32 count );

	//! Render all lights at once
	void RenderLights( Float intensityScale );

	//! Add new light to the bachter
	void AddLight( const Vector& position, Float radius , const Vector& color );

protected:
	// Device Reset/Lost handling
	virtual void OnDeviceLost() {}
	virtual void OnDeviceReset() {}
	virtual CName GetCategory() const;
	virtual Uint32 GetUsedVideoMemory() const { return 0; }

};