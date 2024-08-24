#pragma once

#include "DirectXBase.h"

enum TextType
{
	TextType_Reg,
	TextType_Title,
	TextType_Text
};

enum TextColor
{
	TextColor_Normal,
	TextColor_Selected
};

// This class renders simple text with a colored background.
ref class SimpleTextRenderer sealed : public DirectXBase
{
public:
	SimpleTextRenderer();

	// DirectXBase methods.
	virtual void CreateDeviceIndependentResources() override;
	virtual void CreateDeviceResources() override;
	virtual void CreateWindowSizeDependentResources() override;

	void BeginDrawing();
	void DrawText( WCHAR * in_szText, int in_eType, int in_eColor, int in_X, int in_Y );
	void DoneDrawing();

	// Method for updating time-dependent objects.
	void Update(float timeTotal, float timeDelta);

	// Methods to save and load state in response to suspend.
	void SaveInternalState(Windows::Foundation::Collections::IPropertySet^ state);
	void LoadInternalState(Windows::Foundation::Collections::IPropertySet^ state);

	static SimpleTextRenderer^ GetInstance() { return s_instance; }

private:
	static SimpleTextRenderer^ s_instance;

	Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> m_textBrush[ 2 ];
	Microsoft::WRL::ComPtr<IDWriteTextFormat> m_textFormat[ 3 ];
	//Microsoft::WRL::ComPtr<IDWriteTextLayout> m_textLayout;
	//DWRITE_TEXT_METRICS m_textMetrics;
	//Windows::Foundation::Point m_textPosition;
	bool m_renderNeeded;
};
