/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once

class CEdScreenplayControl : public wxRichTextCtrl
{
private:
	CEdInteractiveDialogEditor*		m_editor;

	CName							m_currentBlockName;
	Int32							m_currentTokenIndex;
	Int32							m_caretOffset;

	Bool							m_caretAtEmptyLine	: 1;
	Bool							m_scrollToCaret		: 1;

public: 
									CEdScreenplayControl( wxWindow* parent, CEdInteractiveDialogEditor* editor );

	void							Init( wxPanel* screenplayPanel );
	void							UpdateRelativeCaretPosition( Int32 lineStartPosition, Int32 lineEndPosition, Int32 elementIndex, CName blockName );
	void							SetRelativeCaretPosition( CName blockName, Int32 tokenIndex, Int32 caretOffset, Bool emptyLine );

	RED_INLINE CName				GetCurrentBlockName() const		{ return m_currentBlockName; }
	RED_INLINE Int32				GetCurrentBlockToken() const	{ return m_currentTokenIndex; }
	RED_INLINE Int32				GetCaretOffset() const			{ return m_caretOffset; }
	RED_INLINE Bool				IsCaretAtEmptyLine() const		{ return m_caretAtEmptyLine; }
	RED_INLINE Bool				ShouldScrollToCaret() const		{ return m_scrollToCaret; }
	RED_INLINE void				SetScrollToCaret( Bool scroll ) { m_scrollToCaret = scroll; }

	virtual void					PositionCaret(	wxRichTextParagraphLayoutBox* container = NULL );	
	virtual void					Paste();

private:
	void							OnScreenplayContentChange( wxCommandEvent& event );
	void							OnScreenplayKillFocus( wxCommandEvent& event );
};