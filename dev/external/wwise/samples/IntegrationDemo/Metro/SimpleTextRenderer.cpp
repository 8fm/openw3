#include "stdafx.h"
#include "SimpleTextRenderer.h"

using namespace D2D1;
using namespace DirectX;
using namespace Microsoft::WRL;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::UI::Core;

SimpleTextRenderer^ SimpleTextRenderer::s_instance;

SimpleTextRenderer::SimpleTextRenderer() 
	: m_renderNeeded(true)
{
	s_instance = this;
}

void SimpleTextRenderer::CreateDeviceIndependentResources()
{
	DirectXBase::CreateDeviceIndependentResources();

	DX::ThrowIfFailed(
		m_dwriteFactory->CreateTextFormat(
			L"Segoe UI",
			nullptr,
			DWRITE_FONT_WEIGHT_NORMAL,
			DWRITE_FONT_STYLE_NORMAL,
			DWRITE_FONT_STRETCH_NORMAL,
			42.0f,
			L"en-US",
			&m_textFormat[ TextType_Reg ]
			)
		);

	DX::ThrowIfFailed(
		m_textFormat[ TextType_Reg ]->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING)
		);

	DX::ThrowIfFailed(
		m_dwriteFactory->CreateTextFormat(
			L"Segoe UI",
			nullptr,
			DWRITE_FONT_WEIGHT_NORMAL,
			DWRITE_FONT_STYLE_NORMAL,
			DWRITE_FONT_STRETCH_NORMAL,
			72.0f,
			L"en-US",
			&m_textFormat[ TextType_Title ]
			)
		);

	DX::ThrowIfFailed(
		m_textFormat[ TextType_Title ]->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING)
		);

	DX::ThrowIfFailed(
		m_dwriteFactory->CreateTextFormat(
			L"Segoe UI",
			nullptr,
			DWRITE_FONT_WEIGHT_NORMAL,
			DWRITE_FONT_STYLE_NORMAL,
			DWRITE_FONT_STRETCH_NORMAL,
			24.0f,
			L"en-US",
			&m_textFormat[ TextType_Text ]
			)
		);

	DX::ThrowIfFailed(
		m_textFormat[ TextType_Text ]->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING)
		);
}

void SimpleTextRenderer::CreateDeviceResources()
{
	DirectXBase::CreateDeviceResources();

	DX::ThrowIfFailed(
		m_d2dContext->CreateSolidColorBrush(
			ColorF(ColorF::Black),
			&m_textBrush[ TextColor_Normal ]
			)
		);

	DX::ThrowIfFailed(
		m_d2dContext->CreateSolidColorBrush(
			ColorF(ColorF::DeepPink),
			&m_textBrush[ TextColor_Selected ]
			)
		);
}

void SimpleTextRenderer::CreateWindowSizeDependentResources()
{
	DirectXBase::CreateWindowSizeDependentResources();

	// Add code to create window size dependent objects here.
}

void SimpleTextRenderer::Update(float timeTotal, float timeDelta)
{
	(void) timeTotal; // Unused parameter.
	(void) timeDelta; // Unused parameter.

	// Add code to update time dependent objects here.
}

void SimpleTextRenderer::BeginDrawing()
{
	m_d2dContext->BeginDraw();

	m_d2dContext->Clear( ColorF( ColorF::White ) );

}
void SimpleTextRenderer::DrawText( WCHAR * in_szText, int in_eTextType, int in_eColor, int in_X, int in_Y )
{
	auto rect = D2D1_RECT_F();
	rect.bottom = 768.0f;
	rect.top = (FLOAT) in_Y;
	rect.left = (FLOAT) in_X;
	rect.right = 1366.0f;

	m_d2dContext->DrawText( in_szText, wcslen( in_szText ), m_textFormat[ in_eTextType ].Get(), rect, m_textBrush[ in_eColor ].Get() );
}

void SimpleTextRenderer::DoneDrawing()
{
	// Ignore D2DERR_RECREATE_TARGET. This error indicates that the device
	// is lost. It will be handled during the next call to Present.
	HRESULT hr = m_d2dContext->EndDraw();
	if (hr != D2DERR_RECREATE_TARGET)
	{
		DX::ThrowIfFailed(hr);
	}

	m_renderNeeded = false;
}

void SimpleTextRenderer::SaveInternalState(IPropertySet^ state)
{
}

void SimpleTextRenderer::LoadInternalState(IPropertySet^ state)
{
}
