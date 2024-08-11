///////////////////////////////////////////////////////////////////////  
//  MyRenderTargets.h
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
	//  Enumeration EMyDeferredRenderTargetType

	enum EMyDeferredRenderTargetType
	{
		MY_DEFERRED_RENDER_TARGET_0,
		MY_DEFERRED_RENDER_TARGET_1,
		MY_DEFERRED_RENDER_TARGET_DEPTH,
		MY_DEFERRED_RENDER_TARGET_COUNT_NO_A2C,
		MY_DEFERRED_RENDER_TARGET_COUNT_WITH_A2C
	};


    ///////////////////////////////////////////////////////////////////////  
    //  Class CMyDeferredRenderTargets

    class CMyDeferredRenderTargets
    {
    public:
                            CMyDeferredRenderTargets( );
                            ~CMyDeferredRenderTargets( );

            st_bool         InitGfx(st_int32 nWidth, st_int32 nHeight, st_int32 nNumSamples, st_bool bA2C);
            void            ReleaseGfxResources(void);
			void			Clear(const Vec4& vColor);

			st_bool			SetAsTarget(void);
			void			ReleaseAsTarget(void);
			st_bool			BindAsTextures(void);
			void			UnBindAsTextures(void);

    private:
			st_int32		NumRenderTargets(void) const;

			CRenderTarget	m_atTargets[MY_DEFERRED_RENDER_TARGET_COUNT_WITH_A2C];

			st_int32		m_nWidth;
			st_int32		m_nHeight;
			st_bool			m_bA2C;
    };


	///////////////////////////////////////////////////////////////////////  
	//  Enumeration EMyForwardTargetType

	enum EMyForwardTargetType
	{
		MY_FORWARD_TARGET_0,
		MY_FORWARD_TARGET_DEPTH,
		MY_FORWARD_TARGET_COUNT
	};


    ///////////////////////////////////////////////////////////////////////  
    //  Class CMyForwardTargets

    class CMyForwardTargets
    {
    public:
									CMyForwardTargets( );
									~CMyForwardTargets( );

			st_bool					InitGfx(st_int32 nWidth, st_int32 nHeight, st_int32 nNumSamples);
			void					ReleaseGfxResources(void);
			void					Clear(const Vec4& vColor);

			st_bool					SetAsTarget(st_bool bClear = true);
			void					ReleaseAsTarget(void);
			st_bool					ResolveToTexture(void);

			st_bool					BindAsTextures(void);
			void					UnBindAsTextures(void);

			const CRenderTarget&	GetNonMsaaTarget(void) const;
			const CRenderTarget&	GetTarget(EMyForwardTargetType eTarget) const;

    private:
			CRenderTarget			m_atMsaaTargets[MY_FORWARD_TARGET_COUNT];
			CRenderTarget			m_tNonMsaaResolveTarget;

			st_int32				m_nWidth;
			st_int32				m_nHeight;
			st_int32				m_nNumSamples;
    };

} // end namespace SpeedTree
