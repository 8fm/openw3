/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_RED_GUI 

#include "redGuiControl.h"

namespace RedGui
{
	class CRedGuiTextBox : public CRedGuiControl
	{
		DECLARE_CLASS_MEMORY_POOL( MemoryPool_RedGui, MC_RedGuiControls );
		struct SRedGuiTextLine
		{
			Uint32		m_startPosition;
			Uint32		m_letterCount;
			Float		m_textWidth;

			SRedGuiTextLine( Uint32 start, Uint32 count, Float width )
				: m_startPosition( start )
				, m_letterCount( count )
				, m_textWidth( width )
			{
				/* intentionally empty */ 
			}
		};
		typedef TDynArray< SRedGuiTextLine*, MC_RedGuiControls, MemoryPool_RedGui > TextLineCollection;

	public:
		CRedGuiTextBox( Uint32 x, Uint32 y, Uint32 width, Uint32 height );
		~CRedGuiTextBox();

		Event2_PackageString	EventTextChanged;
		Event1_Package			EventTextEnter;	// send only when text box is set to single line


		String GetText() const;
		void SetText( const String& text );

		String GetSelectedText() const;

		void SetReadOnly( Bool value );
		Bool GetReadOnly() const;

		void SetMultiLine( Bool value );
		Bool GetMultiLine() const;

		void SetPasswordMode( Bool value );
		Bool GetPasswordMode() const;

		virtual void Draw() override;
		virtual void SetEnabled( Bool value );
		virtual void SetVisible( Bool value ) override;

	private:
		void OnKeySetFocus( CRedGuiControl* oldControl ) override;
		void OnKeyLostFocus( CRedGuiControl* newControl ) override;
		void OnMouseDrag( const Vector2& mousePosition, enum EMouseButton button ) override;
		void OnKeyButtonPressed( enum EInputKey key, Char text );
		void OnMouseWheel( Int32 delta ) override;
		void OnMouseButtonPressed( const Vector2& mousePosition, enum EMouseButton button ) override;
		void OnSizeChanged( const Vector2& oldSize, const Vector2& newSize ) override;

		void NotifyOnTick( RedGui::CRedGuiEventPackage& eventPackage, Float timeDelta );
		void NotifyEventButtonClickedContextMenu( RedGui::CRedGuiEventPackage& eventPackage, RedGui::CRedGuiMenuItem* menuItem );
		void NotifyScrollChangePosition( RedGui::CRedGuiEventPackage& eventPackage, Uint32 value );

		// manage view
		void UpdateView();
		void UpdateScrollPosition();
		void UpdateScrollSize();

		// 
		void AdjustAreaToCursor();

		// 
		void SplitTextToLines();
		void TokenizeText( TDynArray< String >& tokens, Char separator = TXT('\n') );

		// manage cursor position
		void CursorMoveUp( Bool shiftModificator );
		void CursorMoveDown( Bool shiftModificator );
		void CursorMoveLeft( Bool shiftModificator );
		void CursorMoveRight( Bool shiftModificator );

		void PreUpdateSelection( Bool shiftModificator );
		void UpdateSelection( Bool shiftModificator );

		void CursorMoveEndOfLine( Bool shiftModificator );
		void CursorMoveStartOfLine( Bool shiftModificator );
		Bool ManageCursorPosition(enum EInputKey key, Bool shiftModificator );
		
		void CursorGraphicToText();
		void CursorTextToGraphic();
		void RecalculateCursorPosition( const Vector2& mousePosition );	// set grapic and text position

		// add new character to text
		void InsertChar(Char text);
		void DeleteChar(Bool deleteBack);

		// manage the text
		void DeleteSelectedText();
		void CopyText();
		void PasteText();
		void CutText();
		void SelectAll();

		// misc
		void CreateContextMenu();

		//
		String GetTextFromPositions( Uint32 startPointer, Uint32 endPointer );
		void DrawLineText( const Vector2& position, Uint32 lineIndex );
		Int32 CheckPoint( const Vector2& position );
		Vector2 GetTextSize( const String& text );
		void RecalculateMaxWidth( const String& newText );
		
		//
		void DrawSelectionForLines();
		void ResetSelection();

		void OnKeyChangeRootFocus( Bool focus );
		Bool OnInternalInputEvent( enum ERedGuiInputEvent event, const Vector2& data );

		virtual void OnPendingDestruction() override final;

	private:
		// internal controls
		CRedGuiPanel*				m_croppClient;			//!< 
		CRedGuiScrollBar*			m_horizontalBar;		//!< 
		CRedGuiScrollBar*			m_verticalBar;			//!< 
		CRedGuiMenu*				m_contextMenu;			//<! context menu with basic operation on the text

		// attributes
		Bool						m_readOnly;				//<! read-only protection
		Bool						m_multiLine;			//<! way of splitting text
		Bool						m_isActive;				//<! if true - text cursor is visible and updating
		Bool						m_passwordMode;			//<! if true text is displayed as ****

		// cursor and selection		
		Vector2						m_cursorPosition;		//<! Y - it is row; X - it is column
		Vector2						m_selectionPosition;	//<! 
		Int32						m_cursorPositionInText;	//<! 
		Bool						m_cursorVisible;
		Int32						m_startSelection;
		Int32						m_startSelectionLineIndex;

		// helpers
		Uint32						m_maxLineWidth;			//!< 
		Uint32						m_maxVerticalItemCount;	//!< 
		Int32						m_activeLineIndex;
		Int32						m_firstVisibleLine;
		Int32						m_lastVisibleLine;
		Vector2						m_firstLinePosition;
		Uint32						m_horizontalRange;
		Uint32						m_verticalRange;

		// content
		String						m_fullText;				//<! full text in the contains in the text box
		String						m_passwordMask;			//<! 
		TextLineCollection			m_lines;				//<! container with all lines in the text box

	};

}	// namespace RedGui

#endif	// NO_RED_GUI
