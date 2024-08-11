/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_RED_GUI 

#include "redGuiControl.h"

namespace RedGui
{
	class CRedGuiImage : public CRedGuiControl
	{
		DECLARE_CLASS_MEMORY_POOL( MemoryPool_RedGui, MC_RedGuiControls );
	public:
		CRedGuiImage(Uint32 x, Uint32 y, Uint32 width, Uint32 height);
		virtual ~CRedGuiImage();

		THandle< CBitmapTexture > GetImage();
		void SetImage(const String& pathToFile);
		void SetImage( THandle< CBitmapTexture > texture);
		void SetImage( CGatheredResource& resource );

		const String& GetImagePath() const;

		void Draw();

	protected:
		void LoadImage(const String& path);
		void UpdateLayout();

	private:
		THandle< CBitmapTexture >	m_imageInstance;	//!<
		String						m_pathToFile;		//!<
	};

}	// namespace RedGui

#endif	// NO_RED_GUI
