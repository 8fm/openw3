/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "speedTreeRenderInterface.h"

#ifdef USE_SPEED_TREE

class CRenderSpeedTreeResource : public IRenderObject
{
	DECLARE_RENDER_OBJECT_MEMORYCLASS( MC_RenderObjectSpeedtree )
protected:
	SpeedTree::CTreeRender*		m_tree;			//!< Actual speed tree object.
	String						m_path;			//!< TODO: introduce more intuitive way of organising speed tree textures, than having them relative to SRT path
	String						m_fileName;		//!< SRT file name
	Bool 						m_initialized;	//!< Was it initialized on the renderer side?

public:
	CRenderSpeedTreeResource();
	~CRenderSpeedTreeResource();

	// Compile from particle emitter resource
	static IRenderObject* Create( const CSRTBaseTree* baseTreeRes );

	static void GetCollision( IRenderObject* renderSpeedTreeResource, TDynArray< Sphere >& collision );
	static void GetSpeedTreeStatistic( IRenderObject* renderSpeedTreeResource, TDynArray< SSpeedTreeResourceMetrics::SGeneralSpeedTreeStats >& stats );

	void ReleaseTextures();

	// Get or speed tree resource
	SpeedTree::CTreeRender* GetRenderBaseTree() { return m_tree; }
	void SetRenderBaseTree( SpeedTree::CTreeRender* tree ) { ASSERT( m_tree == NULL ); m_tree = tree; }

	const String& GetSRTPath() const { return m_path; }
	const String& GetSRTFileName() const { return m_fileName; } 
	
	void SetSRTPath( const String& path ) { m_path = path; }

	bool IsInitialized() { return m_initialized; }
	void SetHasInitialized() { m_initialized = true; }

	void SetSRTFileName( const String& fileName ) { m_fileName = fileName; }
};

#endif

