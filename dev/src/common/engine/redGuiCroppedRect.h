/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_RED_GUI 

#include "redGuiTypes.h"

namespace RedGui
{
	//////////////////////////////////////////////////////////////////////////
	// m_coord.Min.X is a left position
	// m_coord.Min.Y is a top position
	// m_coord.Max.X is a width
	// m_coord.Max.Y is a height
	class CRedGuiCroppedRect
	{
		DECLARE_CLASS_MEMORY_POOL( MemoryPool_RedGui, MC_RedGuiControls );
	public:
		CRedGuiCroppedRect(const Box2& coord)
			: m_coord(coord)
			, m_originalRect(coord)
			, m_absolutePosition(coord.Min)
			, m_croppedParent(nullptr)
			, m_padding(Box2::ZERO)
			, m_margin(Box2::ZERO)
			, m_borderVisible(true)
			, m_minMaxSize(-1.0, -1.0, -1.0, -1.0)
		{
			/* intentionally empty */
		}

		virtual ~CRedGuiCroppedRect()
		{
			/* intentionally empty */
		}


		//////////////////////////////////////////////////////////////////////////
		// CROPPED PARENT
		////////////////////////////////////////////////////////////////////////// 
		// Get parent for this cropped rectangle
		RED_INLINE CRedGuiCroppedRect* GetCroppedParent()
		{
			return m_croppedParent;
		}

		RED_INLINE void SetCroppedParent(CRedGuiCroppedRect* parent)
		{
			m_croppedParent = parent;
		}



		//////////////////////////////////////////////////////////////////////////
		// ORIGINAL SIZE
		//////////////////////////////////////////////////////////////////////////
		RED_INLINE Box2 GetOriginalRect() const
		{
			return m_originalRect;
		}

		RED_INLINE void SetOriginalRect(const Box2& box) 
		{
			m_originalRect = box;
		}



		//////////////////////////////////////////////////////////////////////////
		// MINIMUM SIZE
		//////////////////////////////////////////////////////////////////////////
		RED_INLINE Vector2 GetMinSize() const
		{
			return m_minMaxSize.Min;
		}

		RED_INLINE void SetMinSize(const Vector2& minSize) 
		{
			m_minMaxSize.Min = minSize;
		}



		//////////////////////////////////////////////////////////////////////////
		// MAXIMUM SIZE
		//////////////////////////////////////////////////////////////////////////
		RED_INLINE Vector2 GetMaxSize() const
		{
			return m_minMaxSize.Max;
		}

		RED_INLINE void SetMaxSize(const Vector2& maxSize) 
		{
			m_minMaxSize.Max = maxSize;
		}



		//////////////////////////////////////////////////////////////////////////
		// OUTSIDE COORDINATES
		////////////////////////////////////////////////////////////////////////// 
		// Set position
		RED_INLINE virtual void SetPosition(const Vector2& position)
		{
			m_coord.Min = position;
		}

		// Set size
		RED_INLINE virtual void SetSize(const Vector2& size)
		{
			m_coord.Max.X = Max< Float >( size.X, 0 );
			m_coord.Max.Y = Max< Float >( size.Y, 0 );
		}

		// Set coordinates (position and size)
		RED_INLINE virtual void SetCoord(const Box2& value)
		{			
			SetPosition(value.Min);
			SetSize(value.Max);
		}

		// Get position
		RED_INLINE Vector2 GetPosition() const
		{
			return m_coord.Min;
		}

		// Get size
		RED_INLINE Vector2 GetSize() const
		{
			return m_coord.Max;
		}

		// Get coordinates (position and size)
		RED_INLINE const Box2& GetCoord() const
		{
			return m_coord;
		}



		//////////////////////////////////////////////////////////////////////////
		// INTERNAL COORDINATES
		//////////////////////////////////////////////////////////////////////////
		// Get position - without border
		RED_INLINE Vector2 GetViewPosition() const
		{
			if(m_borderVisible == true)
			{
				// border has always 1 pixel size
				return Vector2(m_coord.Min.X + 1, m_coord.Max.Y + 1);
			}
			return m_coord.Min;
		}

		// Get size - without border
		RED_INLINE Vector2 GetViewSize() const
		{
			if(m_borderVisible == true)
			{
				// border has always 1 pixel size
				return Vector2(m_coord.Max.X - 2, m_coord.Max.Y - 2);
			}
			return m_coord.Max;
		}

		// Get coordinates (position and size) + without border
		RED_INLINE Box2 GetViewCoord() const
		{
			return Box2(GetViewPosition(), GetViewSize());
		}



		//////////////////////////////////////////////////////////////////////////
		// ABSOLUTE OUTSIDE COORDINATES
		//////////////////////////////////////////////////////////////////////////
		// Get position in screen coordinates
		RED_INLINE Vector2 GetAbsolutePosition() const
		{
			return m_absolutePosition;
		}

		// Get position in screen coordinates
		RED_INLINE Vector2 GetAbsolutePositionViewArea() const
		{
			if(m_borderVisible == true)
			{
				// border has always 1 pixel size
				return Vector2(m_absolutePosition.X + 1, m_absolutePosition.Y + 1);
			}
			return m_absolutePosition;
		}

		// Get rectangle in screen coordinates (position and size)
		RED_INLINE Box2 GetAbsoluteCoord() const
		{
			return Box2(m_absolutePosition.X, m_absolutePosition.Y, m_coord.Max.X, m_coord.Max.Y);
		}

		// Get rectangle in screen coordinates (position and size)
		RED_INLINE Box2 GetAbsoluteCoordViewArea() const
		{
			return Box2(GetAbsolutePositionViewArea(), GetViewSize());
		}

		// Get x in screen coordinates
		RED_INLINE Int32 GetAbsoluteLeft() const
		{
			return static_cast<Int32>(m_absolutePosition.X);
		}

		// Get y in screen coordinates
		RED_INLINE Int32 GetAbsoluteTop() const
		{
			return static_cast<Int32>(m_absolutePosition.Y);
		}



		//////////////////////////////////////////////////////////////////////////
		// HIERARCHY COORDINATES
		////////////////////////////////////////////////////////////////////////// 
		// Get left coordinate in hierarchy
		RED_INLINE Int32 GetLeft() const
		{
			if(m_borderVisible == true)
			{
				return static_cast<Int32>(m_coord.Min.X) + 1;
			}
			return static_cast<Int32>(m_coord.Min.X);
		}

		// Get top coordinate in hierarchy
		RED_INLINE Int32 GetTop() const
		{
			if(m_borderVisible == true)
			{
				return static_cast<Int32>(m_coord.Min.Y) + 1;
			}
			return static_cast<Int32>(m_coord.Min.Y);
		}

		// Get width
		RED_INLINE Int32 GetWidth() const
		{
			if(m_borderVisible == true)
			{
				return static_cast<Int32>(m_coord.Max.X) - 2;
			}
			return static_cast<Int32>(m_coord.Max.X);
		}

		// Get height
		RED_INLINE Int32 GetHeight() const
		{
			if(m_borderVisible == true)
			{
				return static_cast<Int32>(m_coord.Max.Y) - 2;
			}
			return static_cast<Int32>(m_coord.Max.Y);
		}



		//////////////////////////////////////////////////////////////////////////
		// PADDING
		////////////////////////////////////////////////////////////////////////// 
		// Get padding size
		RED_INLINE Box2 GetPadding() const
		{
			return m_padding;
		}
		
		// Set padding size
		RED_INLINE virtual void SetPadding(const Box2& padding)
		{
			m_padding = padding;
		}




		//////////////////////////////////////////////////////////////////////////
		// MARGIN
		////////////////////////////////////////////////////////////////////////// 
		// Get margin size
		RED_INLINE Box2 GetMargin() const
		{
			return m_margin;
		}

		// Set margin size
		RED_INLINE virtual void SetMargin(const Box2& margin)
		{
			m_margin = margin;
		}



		//////////////////////////////////////////////////////////////////////////
		// BORDER
		// One border line has always 1 pixel size
		////////////////////////////////////////////////////////////////////////// 
		RED_INLINE Bool GetBorderVisible() const
		{
			return m_borderVisible;
		}

		RED_INLINE void SetBorderVisible(Bool value)
		{
			m_borderVisible = value;
		}

protected:
		Bool				m_borderVisible;			//!< Visible of border

		Box2				m_coord;					//!< Min is a dependent position, Max is a size
		Box2				m_padding;					//!< Empty area between parent edges and children edges
		Box2				m_originalRect;				//!< Reference coordinates, usualy set during creating the control 
		Box2				m_margin;					//!< Empty area between control edges and sibilings edges
		Box2				m_minMaxSize;				//!< 

		Vector2				m_absolutePosition;			//!< Position in screen/viewport coordinates
		CRedGuiCroppedRect* m_croppedParent;			//!< Rectangle which clip this control
	};

}	// namespace RedGui

#endif	// NO_RED_GUI
