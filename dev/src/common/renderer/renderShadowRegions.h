/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

class CRenderShadowStaticAllocator;

//-----------------------------------------------------------------------------

///dex++: Information about allocated shadow region
class CRenderShadowDynamicRegion : public IRenderObject
{
	friend class CRenderShadowDynamicAllocator;

private:
	Uint16		m_slice;
	Uint16		m_offsetX;
	Uint16		m_offsetY;
	Uint16		m_size;		// assumed square

public:
	//! Get slice index
	RED_INLINE Uint16 GetSlice() const { return m_slice; }

	//! Get offset X
	RED_INLINE Uint16 GetOffsetX() const { return m_offsetX; }

	//! Get offset Y
	RED_INLINE Uint16 GetOffsetY() const { return m_offsetY; }

	//! Get size
	RED_INLINE Uint16 GetSize() const { return m_size; }

public:
	CRenderShadowDynamicRegion();

private:
	CRenderShadowDynamicRegion( const CRenderShadowDynamicRegion& other ) {};
	CRenderShadowDynamicRegion& operator=( const CRenderShadowDynamicRegion& other ) { return *this; }
};
//dex--

//-----------------------------------------------------------------------------

//dex++: Information about allocated static cube
class CRenderShadowStaticCube : public IRenderObject
{
	friend class CRenderShadowStaticAllocator;

private:
	//! Source allocator
	CRenderShadowStaticAllocator*	m_allocator;

	//! Cube index
	Uint16						m_index;

	//! Last frame this cube was updated
	Uint32						m_lastFrame;

	//! Bounding box of the light
	Box							m_boundingBox;

public:
	//! Get the cube index (or -1 if invalid, or not set)
	RED_INLINE Uint16 GetIndex() const { return m_index; }

	//! Get the assigned bounding box
	RED_INLINE const Box& GetBoundingBox() const { return m_boundingBox; }

public:
	CRenderShadowStaticCube();

	//! Returns true if the allocated static slice is valid
	Bool IsValid() const;

	//! Update the "last frame"
	void UpdateFrameIndex( Uint32 frame );
		
private:
	CRenderShadowStaticCube( const CRenderShadowStaticCube& other ) {};
	CRenderShadowStaticCube& operator=( const CRenderShadowStaticCube& other ) { return *this; };
	virtual ~CRenderShadowStaticCube();
};

//-----------------------------------------------------------------------------
