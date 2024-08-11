///////////////////////////////////////////////////////////////////////  
//  MyFullscreenQuad.h
//
//	All source files prefixed with "My" indicate an example implementation, 
//	meant to detail what an application might (and, in most cases, should)
//	do when interfacing with the SpeedTree SDK.  These files constitute 
//	the bulk of the SpeedTree Reference Application and are not part
//	of the official SDK, but merely example or "reference" implementations.
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
//  Preprocessor

#pragma once
#include "MySpeedTreeRenderer.h"


///////////////////////////////////////////////////////////////////////  
//  All SpeedTree SDK classes and variables are under the namespace "SpeedTree"

namespace SpeedTree
{

    ///////////////////////////////////////////////////////////////////////  
    //  Class CMyFullscreenQuad

    class CMyFullscreenQuad
    {
    public:
                            CMyFullscreenQuad( );
                            ~CMyFullscreenQuad( );

            st_bool         InitGfx(st_int32 nScreenWidth, st_int32 nScreenHeight, const CShaderTechnique& cTechnique);
            void            ReleaseGfxResources(void);

            st_bool         Render(const CRenderState& cRenderState) const;

    private:
            void            InitView(st_int32 nScreenWidth, st_int32 nScreenHeight);
            st_bool         InitGeometry(st_int32 nScreenWidth, st_int32 nScreenHeight, const CShaderTechnique& cTechnique);

            Mat4x4          m_mCompositeView; // modelview * projection
            CGeometryBuffer m_cQuad; // two indexed triangles
    };

} // end namespace SpeedTree
