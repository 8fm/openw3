// Drawing.cpp
// Copyright (C) 2010 Audiokinetic Inc
/// \file 
// Implements the Drawing.h functions for the Windows platform


/////////////////////////
//  INCLUDES
/////////////////////////

#include "stdafx.h"
#include "Platform.h"
#include "Drawing.h"
#include <string>
#include "SimpleTextRenderer.h"
#include <AK/Tools/Common/AkPlatformFuncs.h>

using std::string;

/////////////////////////
//  GLOBAL OBJECTS
/////////////////////////

SimpleTextRenderer ^ g_pRenderer;
       
/////////////////////////
//  FUNCTIONS
/////////////////////////

static unsigned long g_iWidth;
static unsigned long g_iHeight;

unsigned long GetWindowWidth()
{
	return g_iWidth;
}

unsigned long GetWindowHeight()
{
	return g_iHeight;
}

void BeginDrawing()
{
	SimpleTextRenderer::GetInstance()->BeginDrawing();
}

void DoneDrawing()
{
	SimpleTextRenderer::GetInstance()->DoneDrawing();
}

bool InitDrawing(
	void* in_pParam,
	AkOSChar* in_szErrorBuffer,
	unsigned int in_unErrorBufferCharCount,
	AkUInt32 in_windowWidth,
	AkUInt32 in_windowHeight
)
{
	g_iWidth = in_windowWidth;
	g_iHeight = in_windowHeight;

	return true;
}


void DrawTextOnScreen( const char* in_szText, int in_iXPos, int in_iYPos, DrawStyle in_eDrawStyle )
{
	string buffer = in_szText;

	// Replace tags in the text
	ReplaceTags( buffer );

	WCHAR szMsg[ 512 ];
	AKPLATFORM::AkCharToWideChar( buffer.c_str(), 512, szMsg );

	TextType eTextType;
	TextColor eColor = TextColor_Normal;

	switch ( in_eDrawStyle )
	{
	case DrawStyle_Title:
		eTextType = TextType_Title;
		break;
	case DrawStyle_Selected:
		eTextType = TextType_Reg;
		eColor = TextColor_Selected;
		break;
	case DrawStyle_Error:
		eTextType = TextType_Reg;
		break;
	case DrawStyle_Text:
		eTextType = TextType_Text;
		break;
	case DrawStyle_Control:
		eTextType = TextType_Reg;
		break;
	}
	SimpleTextRenderer::GetInstance()->DrawText( szMsg, eTextType, eColor, in_iXPos, in_iYPos );
}


void TermDrawing()
{
}


int GetLineHeight( DrawStyle in_eDrawStyle )
{
	if ( in_eDrawStyle == DrawStyle_Title )
	{
		return 72;  // Change this if the font size is changed in InitDrawing()!!
	}
	else if ( in_eDrawStyle == DrawStyle_Text )
	{
		return 24;	// Change this if the font size is changed in InitDrawing()!!
	}
	else
	{
		return 42;	// Change this if the font size is changed in InitDrawing()!!
	}
}