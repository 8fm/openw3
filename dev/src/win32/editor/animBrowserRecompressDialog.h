#pragma once

#include "animbrowser.h"

class CAnimBrowserRecompressDialog : public wxDialog
{
	DECLARE_EVENT_TABLE();

public:
	CAnimBrowserRecompressDialog( wxWindow* parent, SAnimationBufferBitwiseCompressionPreset preset );
	~CAnimBrowserRecompressDialog();

	wxListBox* m_presetList;

	void OnRecompress( wxCommandEvent& event );
	void OnCancel( wxCommandEvent& event );

	SAnimationBufferBitwiseCompressionPreset GetPreset() const { return m_preset; }
	Bool WantsRecompress() const { return m_recompress; }

private:
	SAnimationBufferBitwiseCompressionPreset m_preset;
	Bool m_recompress;

	void FillList();
};

