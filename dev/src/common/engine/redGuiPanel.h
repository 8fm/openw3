/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_RED_GUI 

#include "redGuiTypes.h"
#include "redGuiControl.h"

namespace RedGui
{
	class CRedGuiPanel : public CRedGuiControl
	{
		DECLARE_CLASS_MEMORY_POOL( MemoryPool_RedGui, MC_RedGuiControls );
	public:
		CRedGuiPanel(Uint32 left, Uint32 top, Uint32 width, Uint32 height);
		CRedGuiPanel(const CRedGuiPanel& pattern);
		virtual ~CRedGuiPanel();

		// Set control size
		virtual void SetSize(const Vector2& size);
		virtual void SetSize(Int32 width, Int32 height);
		// Set control position and size
		virtual void SetCoord(const Box2& coord);
		virtual void SetCoord(Int32 left, Int32 top, Int32 width, Int32 height);

		void Draw();

		void SetAutoSize(Bool value);
		Bool GetAutoSize() const;

		virtual void AddChild(CRedGuiControl* child);
		virtual void RemoveChild(CRedGuiControl* child);

	protected:
		void NotifyPositionChanged( RedGui::CRedGuiEventPackage& eventPackage, Vector2 oldPosition, Vector2 newPosition);
		void NotifySizeChanged( RedGui::CRedGuiEventPackage& eventPackage, Vector2 oldSize, Vector2 newSize);

	private:
		void AdjustSizeToContent();
		Bool m_autoSize;
	};

}	// namespace RedGui

#endif	// NO_RED_GUI
