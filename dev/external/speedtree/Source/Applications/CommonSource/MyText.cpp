///////////////////////////////////////////////////////////////////////  
//  MyText.cpp
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

#ifdef _WIN32
	#pragma warning (disable : 4995)
#endif

#include "MyText.h"
#include "MySpeedTreeRenderer.h"
#ifdef _XBOX
	#include "AtgFont.h"
#endif
using namespace SpeedTree;


///////////////////////////////////////////////////////////////////////  
//  Statics / constants

#ifdef _XBOX
	ATG::Font* CMyText::m_pFont = NULL;
#endif


///////////////////////////////////////////////////////////////////////  
//  CMyText::CMyText

CMyText::CMyText( )
	: m_nOffset(0)
#ifdef SPEEDTREE_DIRECTX9
	, m_pHelper(NULL)
	, m_pFont9(NULL)
	, m_pSprite9(NULL)
#endif
#if defined(SPEEDTREE_OPENGL) && defined(_WIN32)
	, m_dlLists(0)
#endif
{
	#if defined(SPEEDTREE_OPENGL) && defined(_WIN32)
		memset(m_anSizes, 0, sizeof(m_anSizes));
	#endif
}


///////////////////////////////////////////////////////////////////////  
//  CMyText::Init

bool CMyText::Init(void)
{
	#if defined(SPEEDTREE_OPENGL) && defined(_WIN32)
		LOGFONT sLogFont;
		::ZeroMemory(&sLogFont, sizeof(LOGFONT));
		sLogFont.lfHeight			= -MulDiv(11, GetDeviceCaps(wglGetCurrentDC( ), LOGPIXELSY), 72);
		sLogFont.lfWidth			= 0;
		sLogFont.lfEscapement		= 0;
		sLogFont.lfOrientation		= 0;
		sLogFont.lfWeight			= 700;
		sLogFont.lfItalic			= FALSE;
		sLogFont.lfUnderline		= FALSE;
		sLogFont.lfStrikeOut		= FALSE;
		sLogFont.lfCharSet			= DEFAULT_CHARSET;
		sLogFont.lfOutPrecision		= OUT_DEFAULT_PRECIS;
		sLogFont.lfClipPrecision	= CLIP_DEFAULT_PRECIS;
		sLogFont.lfQuality			= PROOF_QUALITY;
		sLogFont.lfPitchAndFamily	= DEFAULT_PITCH | FF_DONTCARE;
		sLogFont.lfQuality		    = ANTIALIASED_QUALITY;
		strcpy(sLogFont.lfFaceName, "Arial\0");
		
		HFONT hFont = CreateFontIndirect(&sLogFont);
		SelectObject(wglGetCurrentDC( ), hFont); 

		m_dlLists = glGenLists(95);

		SIZE sSize;
		st_char szBuf[1];
		for (st_int32 i = 32; i < 127; ++i)
		{
			szBuf[0] = char(i);
			if (!GetTextExtentPoint32(wglGetCurrentDC( ), szBuf, 1, &sSize))
			{
				printf("GetTextExtentPoint32 failed in Init\n");
			}
			m_anSizes[i - 32][0] = abs(sSize.cx);
			m_anSizes[i - 32][1] = abs(sSize.cy);
		}

		if (!wglUseFontOutlines(wglGetCurrentDC( ), 32, 95, m_dlLists, 0.1f, 0.0f, WGL_FONT_POLYGONS, NULL))
		{
			if (!wglUseFontOutlines(wglGetCurrentDC( ), 32, 95, m_dlLists, 0.1f, 0.0f, WGL_FONT_POLYGONS, NULL))
			{
				st_int32 nError = GetLastError( );
				printf("wglUseFontBitmaps returned error : %d\n", nError);
			}
		}
		
		::DeleteObject(hFont);
	#endif

	return true;
}


///////////////////////////////////////////////////////////////////////  
//  CMyText::BeginText

bool CMyText::BeginText(void)
{
	#ifdef SPEEDTREE_DIRECTX9
		m_pHelper->Begin( );
		m_nOffset = 5;
	#endif

	#if defined(SPEEDTREE_OPENGL) && defined(_WIN32)
		glPushAttrib(GL_ENABLE_BIT);
		glDisable(GL_CULL_FACE);
		glDisable(GL_LIGHTING);
		glDisable(GL_BLEND);
		glDisable(GL_ALPHA_TEST);
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_DEPTH_TEST);
		if (GLEW_ARB_vertex_program)
			glDisable(GL_VERTEX_PROGRAM_ARB);
		if (GLEW_ARB_fragment_program)
			glDisable(GL_FRAGMENT_PROGRAM_ARB);
		if (glUseProgramObjectARB)
			glUseProgramObjectARB(NULL);

		glMatrixMode(GL_PROJECTION);
		glPushMatrix( );
		glLoadIdentity( );
		GLint aiViewport[4];
		glGetIntegerv(GL_VIEWPORT, aiViewport);
		glOrtho(0.0, aiViewport[2], aiViewport[3], 0.0f, 0.0, 1.0);
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix( );
		glLoadIdentity( );

		glColor4f(1.0f, 1.0f, 0.0f, 1.0f);
		m_nOffset = 5 + 10;
	#endif

	#ifdef _XBOX
		assert(m_pFont);
		DX9::Device( )->SetTexture(0, NULL);
		DX9::Device( )->SetTexture(1, NULL);
		DX9::Device( )->SetVertexShader(NULL);
		DX9::Device( )->SetPixelShader(NULL);
		m_pFont->Begin( );
		m_pFont->SetScaleFactors(0.8f, 0.8f);
		m_nOffset = 0;
	#endif

	return true;
}


///////////////////////////////////////////////////////////////////////  
//  CMyText::DrawLine

bool CMyText::DrawLine(const char* pText)
{
	assert(pText);

	#ifdef SPEEDTREE_DIRECTX9
		// render in black first
		{
			m_pHelper->SetForegroundColor(D3DXCOLOR(0.0f, 0.0f, 0.0f, 1.0f));
			m_pHelper->SetInsertionPos(5 + 1, m_nOffset + 1);
			if (FAILED(m_pHelper->DrawTextLine(pText)))
				return false;
		}

		// render in color
		{
			m_pHelper->SetForegroundColor(D3DXCOLOR(1.0f, 1.0f, 0.0f, 1.0f));
			m_pHelper->SetInsertionPos(5, m_nOffset);
			if (FAILED(m_pHelper->DrawTextLine(pText)))
				return false;
		}
	#endif

	#if defined(SPEEDTREE_OPENGL) && defined(_WIN32)
		if (pText != NULL && m_dlLists != 0)
		{
			st_int32 nLength = st_int32(strlen(pText));
			if (nLength > 0)
			{
				glListBase(m_dlLists - 32); 

				// render in black first
				{
					glColor3f(0.0f, 0.0f, 0.0f);
					glPushMatrix( );
					glTranslatef(5.0f + 2.0f, st_float32(m_nOffset + 1.0f), 0.0f);				
					glScalef(17.0f, -17.0f, 17.0f);
					glCallLists(nLength, GL_UNSIGNED_BYTE, (const GLvoid*) pText); 
					glPopMatrix( );
				}

				// render in  color
				{
					glColor3f(1.0f, 1.0f, 0.0f);
					glPushMatrix( );
					glTranslatef(5.0, st_float32(m_nOffset), 0.0f);
					glScalef(17.0f, -17.0f, 17.0f);
					glCallLists(nLength, GL_UNSIGNED_BYTE, (const GLvoid*) pText); 
					glPopMatrix( );
				}
			}
		}
	#endif

	#ifdef _XBOX
		assert(m_pFont);
		WCHAR wcStatsString[512];
		MultiByteToWideChar(CP_ACP, 0, pText, strlen(pText) + 1, wcStatsString, ARRAYSIZE(wcStatsString));
		m_pFont->DrawText(0, FLOAT(m_nOffset), 0xffffff00, wcStatsString, ATGFONT_LEFT);
	#endif

	// advance offset for next line
	m_nOffset += 17;

	return true;
}


///////////////////////////////////////////////////////////////////////  
//  CMyText::EndText

void CMyText::EndText(void)
{
	#ifdef SPEEDTREE_DIRECTX9
		m_pHelper->End( );
	#endif

	#if defined(SPEEDTREE_OPENGL) && defined(_WIN32)
		glListBase(0);
		glMatrixMode(GL_PROJECTION);
		glPopMatrix( );
		glMatrixMode(GL_MODELVIEW);
		glPopMatrix( );
		glPopAttrib( );
	#endif

	#ifdef _XBOX
		assert(m_pFont);
		m_pFont->End( );
	#endif
}


///////////////////////////////////////////////////////////////////////  
//  CMyText::ReleaseGfxResources

void CMyText::ReleaseGfxResources(void)
{
	#ifdef SPEEDTREE_DIRECTX9
		ST_SAFE_RELEASE(m_pFont9);
		ST_SAFE_RELEASE(m_pSprite9);
		SAFE_DELETE(m_pHelper);
	#endif

	#if defined(SPEEDTREE_OPENGL) && defined(_WIN32)
		if (m_dlLists != 0)
			glDeleteLists(m_dlLists, 95);
		m_dlLists = 0;
	#endif
}


///////////////////////////////////////////////////////////////////////  
//  CMyText::OnLostDevice

#ifdef SPEEDTREE_DIRECTX9
	void CMyText::OnLostDevice(void)
	{
		if (m_pFont9 != NULL) 
			m_pFont9->OnLostDevice( );
		SAFE_RELEASE(m_pSprite9);
		SAFE_DELETE(m_pHelper);
	}
#endif


///////////////////////////////////////////////////////////////////////  
//  CMyText::OnResetDevice

#ifdef SPEEDTREE_DIRECTX9
	void CMyText::OnResetDevice(void)
	{
		if (m_pFont9 == NULL)
		{
			D3DXCreateFont(DX9::Device( ), 18, 0, FW_BOLD, 1, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, 
							PROOF_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Arial", &m_pFont9);
		}
		else
		{
			m_pFont9->OnResetDevice( );
		}

		D3DXCreateSprite(DX9::Device( ), &m_pSprite9);
		m_pHelper = new CDXUTTextHelper(m_pFont9, m_pSprite9, NULL, NULL, 15);
	}
#endif


///////////////////////////////////////////////////////////////////////  
//  CMyText::SetFont

#ifdef _XBOX
	void CMyText::SetFont(::ATG::Font* pFont)
	{
		m_pFont = pFont;
	}
#endif

