/**
 * Copyright © 2009 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "emitterGraphEditor.h"
#include "wxThumbnailImageLoader.h"
#include "particleEditor.h"
#include "../../common/engine/particleInitializerLifeTime.h"
#include "../../common/core/feedback.h"
#include "../../common/core/depot.h"
#include "../../common/engine/particleModule.h"
#include "../../common/engine/particleEmitter.h"
#include "../../common/engine/particleSystem.h"
#include "../../common/engine/materialDefinition.h"
#include "../../common/engine/texture.h"

#define ID_EDIT_ADD						0
#define ID_EDIT_CLONE					1
#define ID_EDIT_REMOVE					2
#define ID_EDIT_DEBUG					3
#define ID_EDIT_RESET					4
#define ID_EDIT_FIT						5
#define ID_EDIT_COPY_TO_CLIPBOARD		6
#define ID_EDIT_PASTE_FROM_CLIPBOARD	7
#define ID_EDIT_REMOVE_MODIFICATOR		9
#define ID_EDIT_COPY_MODIFICATOR		10
#define ID_EDIT_GLOBAL_DISABLE			11

// This will use some range of values.
#define ID_EDIT_ADD_MODIFICATOR			100

#define ID_EDIT_CURVE					500

TDynArray< THandle< CParticleEmitter > > CEdEmitterGraphEditor::m_emittersInClipboard = TDynArray< THandle< CParticleEmitter > >();

BEGIN_EVENT_TABLE(CEdEmitterGraphEditor, CEdCanvas)
	EVT_MOUSEWHEEL(CEdEmitterGraphEditor::OnMouseWheel)
	EVT_KEY_DOWN(CEdEmitterGraphEditor::OnKeyDown)
END_EVENT_TABLE()

CEdEmitterGraphEditor::CEdEmitterGraphEditor(CEdParticleEditor* editor, wxWindow *parent, CParticleSystem *particleSystem)
: CEdCanvas(parent)
, m_editor(editor)
, m_selectRectColor(255, 255, 255)
, m_selectedColor(255, 255, 0)
, m_editedColor(255, 0, 0)
, m_particleSystem(particleSystem)
, m_action(MA_None)
, m_moveTotal(0)
, m_wheelTotal(0.0f)
, m_blocksMoved(false)
, m_chessboard(NULL)
, m_activeModule(NULL)
, m_selectedModificator(NULL)
, m_activeHitArea(HA_Outside)
{
	ASSERT(m_editor);

	// Prepare chessboard
	m_chessboard = new Gdiplus::Bitmap(EmitterLayoutInfo::ThumbnailSize, EmitterLayoutInfo::ThumbnailSize);
	if (Gdiplus::Graphics *graphics = Gdiplus::Graphics::FromImage(m_chessboard))
	{
		const Int32 cellSize = 8;
		Gdiplus::SolidBrush whiteBrush(Gdiplus::Color::White);
		Gdiplus::SolidBrush grayBrush(Gdiplus::Color::Gray);
		graphics->FillRectangle(&whiteBrush, 0, 0, EmitterLayoutInfo::ThumbnailSize, EmitterLayoutInfo::ThumbnailSize);
		for (int y = 0; y < EmitterLayoutInfo::ThumbnailSize / cellSize; ++y)
		{
			for (int x = 0; x < EmitterLayoutInfo::ThumbnailSize / cellSize; ++x)
			{
				if ((x + y) % 2 == 0)
					graphics->FillRectangle(&grayBrush, x * cellSize, y * cellSize, cellSize, cellSize);
			}
		}
		delete graphics;
	}
	
	UpdateLayout();
}

CEdEmitterGraphEditor::~CEdEmitterGraphEditor()
{
	if (m_chessboard)
	{
		delete m_chessboard;
		m_chessboard = NULL;
	}
}

IParticleModule *CEdEmitterGraphEditor::GetEditedModule()
{
	if (m_selectedModificator)
	{
		ASSERT(m_selected.Size() == 0);
		return m_selectedModificator;
	}
	if (m_selected.Size() == 1)
	{
		ASSERT(m_selectedModificator == NULL);
		return *m_selected.Begin();
	}

	return NULL;
}

void CEdEmitterGraphEditor::UpdateLayout()
{
	// Prepare layout map for existing emitters
	const TDynArray< CParticleEmitter* >& emitters = m_particleSystem->GetEmitters();

	// Check if emitters need reordering (if all have position (0,0) )
	Bool reorder = false;
	wxPoint position(0, 0);
	if (emitters.Size() > 0)
	{
		reorder = true;
		for (Int32 i = 0; i < (Int32) emitters.Size(); ++i)
		{
			emitters[i]->GetPosition(position.x, position.y);
			if (position.x != 0 || position.y != 0)
			{
				reorder = false;
				break;
			}
		}
	}

	position = wxPoint(10, 10);
	for (Int32 i = 0; i < (Int32) emitters.Size(); ++i)
	{
		UpdateEmitterLayout(emitters[i]);
		if (reorder)
		{
			emitters[i]->SetPosition(position.x, position.y);
			EmitterLayoutInfo *layout = m_layout.FindPtr(emitters[i]);
			position.x += layout->m_size.x + 5;
		}
	}

	Repaint();
}

void CEdEmitterGraphEditor::ScaleToClientView()
{
	// Prepare layout map for existing emitters
	const TDynArray< CParticleEmitter* >& emitters = m_particleSystem->GetEmitters();
	if (emitters.Size() > 0)
	{
		// Get client rectangle
		wxRect clientRect = GetClientRect();
		if (clientRect.width == 0 || clientRect.height == 0)
		{
			return;
		}

		// Find bounding rect for all emitters
		wxRect boundingRect = GetBoundingRect(emitters[0]);
		for (Uint32 i = 1; i < emitters.Size(); ++i)
		{
			const wxRect rect = GetBoundingRect(emitters[i]);
			boundingRect = rect.Union(boundingRect);
		}

		// Add margin
		boundingRect.Inflate(50, 50);

		// Calculate scale
		const wxRect canvasView = ClientToCanvas(clientRect);
		Float scale;
		if (boundingRect.width > boundingRect.height)
			scale = canvasView.width / (Float)boundingRect.width;
		else
			scale = canvasView.height / (Float)boundingRect.height;

		const wxPoint clientCenter(clientRect.x + clientRect.width / 2, clientRect.y + clientRect.height / 2);
		const wxPoint boundingCenter(boundingRect.x + boundingRect.width / 2, boundingRect.y + boundingRect.height / 2);

		// Set new offset and scale
		SetOffset(GetOffset() + ClientToCanvas(clientCenter) - boundingCenter);
		ScaleGraph(GetScale() * scale, clientCenter);
	}
}

void CEdEmitterGraphEditor::Select( CParticleEmitter *emitter, Bool state )
{
	SelectEmitter( emitter, state, true );
	Repaint();
}

wxRect CEdEmitterGraphEditor::GetBoundingRect(CParticleEmitter *emitter) const
{
	const EmitterLayoutInfo *layout = m_layout.FindPtr(emitter);
	ASSERT(layout);
	Int32 x, y;
	emitter->GetPosition(x, y);
	return wxRect(x, y, layout->m_size.x, layout->m_size.y);
}

void CEdEmitterGraphEditor::UpdateEmitterLayout(CParticleEmitter *emitter)
{
	ASSERT(emitter);
	EmitterLayoutInfo *layout = m_layout.FindPtr(emitter);
	if (!layout)
	{
		m_layout.Insert(emitter, EmitterLayoutInfo());
		layout = m_layout.FindPtr(emitter);
	}
	
	// Find max modificator
	const Int32 numButtons = 1 + 3; // first button is checkbox indicating if modificator is enabled or disabled
	Int32 modificatorWidth = 0;
	const TDynArray<IParticleModule *> &modules = emitter->GetModules();	
	for (Int32 i = 0; i < (Int32)modules.Size(); ++i)
	{
		IParticleModule *module = modules[i];
		wxPoint size = TextExtents(GetGdiBoldFont(), module->GetEditorName());
		modificatorWidth = max(modificatorWidth, size.x + 2 + numButtons * (EmitterLayoutInfo::ButtonSize + 2));
	}

	// Add left and right margins
	modificatorWidth = 3 + modificatorWidth + 3;

	// Calculate text size
	wxPoint size = TextExtents(GetGdiBoldFont(), emitter->GetEditorName());
	Int32 modificatorY = max(50, 5 + size.y + EmitterLayoutInfo::ThumbnailSize + 10);

	// Calculate block rectangle
	layout->m_size.x = max(size.x, EmitterLayoutInfo::ThumbnailSize + 2 * (2 + EmitterLayoutInfo::ButtonSize + 2 + EmitterLayoutInfo::ButtonSize + 7));
	layout->m_size.y = modificatorY;
	if (!layout->m_collapsed)
	{
		layout->m_size.y += modules.Size() * EmitterLayoutInfo::ModificatorHeight;
	}
	layout->m_size.x = max(layout->m_size.x, modificatorWidth);

	// Calculate trigger rectangle
	layout->m_triggerPoint = wxPoint(layout->m_size.x - 2 * EmitterLayoutInfo::ButtonSize - 2 - 7, modificatorY - EmitterLayoutInfo::ButtonSize - 5);

	// Calculate BirthRate modificiator rectangle
	layout->m_birthRatePoint = wxPoint(layout->m_triggerPoint.x + EmitterLayoutInfo::ButtonSize + 2, layout->m_triggerPoint.y);

	// Calculate name offset
	layout->m_nameOffset.x = (layout->m_size.x - size.x) / 2;
	layout->m_nameOffset.y = 5;

	// Calculate thumbnail offset
	layout->m_thumbnailOffset.x = (layout->m_size.x - EmitterLayoutInfo::ThumbnailSize) / 2;
	layout->m_thumbnailOffset.y = layout->m_nameOffset.y + size.y + 5;

	// Calculate enable rectangle
	layout->m_enablePoint = wxPoint( layout->m_size.x - 2 * EmitterLayoutInfo::ButtonSize - 2 - 7, layout->m_thumbnailOffset.y );

	// Calculate show rectangle
	layout->m_showOnlyPoint = wxPoint( layout->m_size.x - 2 * EmitterLayoutInfo::ButtonSize - 2 - 7, ( layout->m_thumbnailOffset.y+20 ) );

	// Calculate layout for modules
	const Int32 modificatorHeight = layout->m_collapsed ? 0 : EmitterLayoutInfo::ModificatorHeight;
	for (Int32 i = 0; i < (Int32)modules.Size(); ++i)
	{
		ModificatorLayoutInfo *modificatorLayout = layout->m_modificators.FindPtr(modules[i]);
		if (!modificatorLayout)
		{
			layout->m_modificators.Insert(modules[i], ModificatorLayoutInfo());
			modificatorLayout = layout->m_modificators.FindPtr(modules[i]);
		}

		modificatorLayout->m_localRect.x = 0;
		modificatorLayout->m_localRect.y = modificatorY + i * EmitterLayoutInfo::ModificatorHeight;
		modificatorLayout->m_localRect.width = modificatorWidth;
		modificatorLayout->m_localRect.height = modificatorHeight;

		modificatorLayout->m_enablePoint.x = 2;
		modificatorLayout->m_enablePoint.y = 2;

		modificatorLayout->m_moveUpPoint.x = modificatorWidth - 3 * (EmitterLayoutInfo::ButtonSize + 2) - 3;
		modificatorLayout->m_moveUpPoint.y = (modificatorHeight - EmitterLayoutInfo::ButtonSize) / 2;

		modificatorLayout->m_moveDownPoint.x = modificatorWidth - 2 * (EmitterLayoutInfo::ButtonSize + 2) - 3;
		modificatorLayout->m_moveDownPoint.y = (modificatorHeight - EmitterLayoutInfo::ButtonSize) / 2;

		modificatorLayout->m_curvePoint.x = modificatorWidth - (EmitterLayoutInfo::ButtonSize + 2) - 3;
		modificatorLayout->m_curvePoint.y = (modificatorHeight - EmitterLayoutInfo::ButtonSize) / 2;
	}
}

void CEdEmitterGraphEditor::ShowOnlySelectedEmitter()
{
	// Draw unselected emitters
	const TDynArray< CParticleEmitter* >& emitters = m_particleSystem->GetEmitters();
	for (Uint32 i = 0; i < emitters.Size(); ++i)
	{
		if ( IsEmitterSelected(emitters[i]) )
		{
			if ( emitters[i]->IsShowing() )
			{
				emitters[i]->SetShowing( false );
				// Check if this is the only one left...
				if ( !IsAnyEmitterShowing() )
				{
					// Reset all the emitters to their original state
					ResetEmitterEnabledState();
				}
				else
				{
					emitters[i]->SetEnable( false );
				}
			}
			else
			{
				// Check if this is the only one that will show...
				if ( !IsAnyEmitterShowing() )
				{
					 // Remember the state of the emitters
					GetEmittersEnabledState();
				}
				emitters[i]->SetShowing( true );
				emitters[i]->SetEnable( true );
			}		
		}
		m_editor->OnEmitterChanged( emitters[i] );
	}
	wxCommandEvent tempEvent = wxCommandEvent();
	m_editor->OnTimeRestart(tempEvent);
}

Bool CEdEmitterGraphEditor::IsAnyEmitterShowing()
{
	const TDynArray< CParticleEmitter* >& emitters = m_particleSystem->GetEmitters();
	for (Uint32 i = 0; i < emitters.Size(); ++i)
	{
		if ( emitters[i]->IsShowing() )
		{
			return true;
		}		
	}
	return false;
}

void CEdEmitterGraphEditor::GetEmittersEnabledState()
{
	const TDynArray< CParticleEmitter* >& emitters = m_particleSystem->GetEmitters();
	for (Uint32 i = 0; i < emitters.Size(); ++i)
	{
		if ( emitters[i]->IsEnabled() )
		{
			emitters[i]->SetEnabledState( true );
		}	
		else
		{
			emitters[i]->SetEnabledState( false );
		}
		if ( !IsEmitterSelected( emitters[i] ) )
		{
			emitters[i]->SetEnable( false );
		}
	}
}

void CEdEmitterGraphEditor::ResetEmitterEnabledState()
{
	const TDynArray< CParticleEmitter* >& emitters = m_particleSystem->GetEmitters();
	for (Uint32 i = 0; i < emitters.Size(); ++i)
	{
		if ( emitters[i]->EnabledState() )
		{
			emitters[i]->SetEnable( true );
		}	
		else
		{
			emitters[i]->SetEnable( false );
		}
	}
}

void CEdEmitterGraphEditor::PaintCanvas(Int32 width, Int32 height)
{
	wxColour back = GetCanvasColor();
	Clear(back);

	// Draw unselected emitters
	const TDynArray< CParticleEmitter* >& emitters = m_particleSystem->GetEmitters();
	for (Uint32 i = 0; i < emitters.Size(); ++i)
	{
		if (!IsEmitterSelected(emitters[i]))
		{
			DrawEmitterBlock(emitters[i]);
		}
	}

	// Draw selected emitters
	for (THashSet<CParticleEmitter *>::iterator it = m_selected.Begin(); it != m_selected.End(); ++it)
	{
		DrawEmitterBlock(*it);
	}

	// Draw selection rectangle
	if (m_action == MA_SelectingBlocks && m_selectRectStart != m_selectRectEnd)
	{
		wxRect selectRect(m_selectRectStart, m_selectRectEnd);
		wxColor fillColor(m_selectRectColor.Red(), m_selectRectColor.Green(), m_selectRectColor.Blue(), 50);
		FillRect(selectRect, fillColor);
		DrawRect(selectRect, m_selectRectColor);
	}
}

void CEdEmitterGraphEditor::DrawEmitterBlock(CParticleEmitter *emitter)
{
	ASSERT(emitter);
	EmitterLayoutInfo *layout = m_layout.FindPtr(emitter);
	if (!layout)
	{
		return;
	}

	Uint32 currentLod = m_editor->GetEditingLOD();
	const Bool enabledForLod = emitter->GetLOD( currentLod ).m_isEnabled;

	const wxColor editorColor = wxColor(emitter->GetEditorColor().R, emitter->GetEditorColor().G, emitter->GetEditorColor().B, emitter->GetEditorColor().A);
	Float hue, sat, val;
	RGBToHSV(editorColor, hue, sat, val);
	const wxColor editorBorderColor = HSVToRGB(hue, sat, 0.4f * val);
	const wxColor editorTextColor = HSVToRGB(hue, sat, 0.5f * val);

	const wxColor textColor = enabledForLod ? editorTextColor : ConvertToGrayscale(editorTextColor);
	wxColor backColor = enabledForLod ? editorColor : ConvertToGrayscale(editorColor);
	wxColor borderColor = enabledForLod ? editorBorderColor : ConvertToGrayscale(editorBorderColor);
	const wxColor selectedColor = enabledForLod ? m_selectedColor : m_selectedColor;
	const wxColor editedColor = enabledForLod ? m_editedColor : m_editedColor;
	const wxColor activeColorWithAlpha(selectedColor.Red(), selectedColor.Green(), selectedColor.Blue(), 40);
	const wxColor selectedColorWithAlpha(selectedColor.Red(), selectedColor.Green(), selectedColor.Blue(), 100);
	const Float borderWidth = 2.0f;

	if ( !emitter->IsEnabled() )
	{
		backColor = wxColor( 0, 0, 0, 0 );
		borderColor = wxColor( 0, 0, 0, 63 );
	}

	wxPoint pos;
	emitter->GetPosition(pos.x, pos.y);
	const wxRect emitterRect(pos, layout->m_size);

	// Draw emitter's block
	FillRoundedRect(emitterRect, backColor, 5);
	
	if (m_activeModule == emitter)
		FillRoundedRect(emitterRect, activeColorWithAlpha, 5);

	// Draw border
	DrawRoundedRect(emitterRect, IsEmitterSelected(emitter) ? selectedColor : borderColor, 5, borderWidth);

	// Draw emitter's name
	wxPoint textPosition(emitterRect.x + layout->m_nameOffset.x, emitterRect.y + layout->m_nameOffset.y);
	DrawText(textPosition, GetGdiBoldFont(), emitter->GetEditorName(), textColor);

	// Draw enable button
	wxRect enableRect( layout->m_enablePoint, wxSize( EmitterLayoutInfo::ButtonSize, EmitterLayoutInfo::ButtonSize ) );
	enableRect.Offset( emitterRect.x, emitterRect.y );
	if ( m_activeModule == emitter && m_activeHitArea == HA_Enable )
	{
		DrawRoundedRect( enableRect, borderColor, 3,  1.0f );
	}
	enableRect.Deflate( 3.0f, 3.0f );
	if ( enabledForLod )
	{
		DrawCheckInRect( enableRect, borderColor, 2.0f );
	}
	else
	{
		DrawCross( enableRect, borderColor, 2.0f );
	}

	// Draw Show button
	wxRect showRect( layout->m_showOnlyPoint, wxSize( EmitterLayoutInfo::ButtonSize, EmitterLayoutInfo::ButtonSize ) );
	showRect.Offset( emitterRect.x, emitterRect.y );
	if ( m_activeModule == emitter && m_activeHitArea == HA_ShowOnly )
	{
		DrawRoundedRect( showRect, borderColor, 3,  1.0f );
	}
	showRect.Deflate( 3.0f, 3.0f );
	if ( emitter->IsShowing() )
	{
		DrawEyeInRect( showRect, borderColor, 2.0f );
	}
	else
	{
		DrawClosedEyeInRect( showRect, borderColor, 2.0f );
	}

	// Draw thumbnail
	wxPoint thumbnailPoint(emitterRect.x + layout->m_thumbnailOffset.x, emitterRect.y + layout->m_thumbnailOffset.y);
	// Try to get diffuse texture from material parameters
	Gdiplus::Bitmap *bitmap = GetThumbnailForMaterial( emitter->GetMaterial() );

	if ( bitmap )
	{
		DrawImage(m_chessboard, thumbnailPoint.x, thumbnailPoint.y, EmitterLayoutInfo::ThumbnailSize, EmitterLayoutInfo::ThumbnailSize);
		DrawImage(bitmap, thumbnailPoint.x, thumbnailPoint.y, EmitterLayoutInfo::ThumbnailSize, EmitterLayoutInfo::ThumbnailSize);
	}
	else
	{
		static wxColor foreColor(240, 240, 240);
		static wxColor backgroundColor(128, 128, 128);
		static String noImageText = TXT("No Image!");
		wxPoint textSize = TextExtents(GetGdiBoldFont(), noImageText);
		wxPoint pos(thumbnailPoint.x + (EmitterLayoutInfo::ThumbnailSize - textSize.x) / 2, thumbnailPoint.y + (EmitterLayoutInfo::ThumbnailSize - textSize.y) / 2);
		wxRect thumbnailRect(thumbnailPoint.x, thumbnailPoint.y, EmitterLayoutInfo::ThumbnailSize, EmitterLayoutInfo::ThumbnailSize);
		FillRect(thumbnailRect, backgroundColor);
		DrawText(pos, GetGdiBoldFont(), noImageText, foreColor);
	}

	// Draw birth rate curve
	TDynArray<CurveParameter *> emitterCurves;
	emitter->GetCurves(emitterCurves);
	if (emitterCurves.Size() > 0)
	{
		wxRect birthRateRect(layout->m_birthRatePoint, wxSize(EmitterLayoutInfo::ButtonSize, EmitterLayoutInfo::ButtonSize));
		birthRateRect.Offset(emitterRect.x, emitterRect.y);
		DrawCurveRect(birthRateRect, editorColor, emitterCurves.Size() == 1, (m_activeModule == emitter && m_activeHitArea == HA_BirthRateCurve));
	}

	// Draw modificator's trigger
	if (layout->m_modificators.Size() > 0)
	{
		// Draw trigger border
		wxRect triggerRect(layout->m_triggerPoint, wxSize(EmitterLayoutInfo::ButtonSize, EmitterLayoutInfo::ButtonSize));
		triggerRect.Offset(emitterRect.x, emitterRect.y);
		if (m_activeModule == emitter && m_activeHitArea == HA_Trigger)
			DrawRoundedRect(triggerRect, borderColor, 3, 1.0f);

		// Draw arrow
		wxPoint p1, p2, p3;
		CalculateTriangleInsideRect(triggerRect, p1, p2, p3, !layout->m_collapsed, 5);
		FillTriangle(p1.x, p1.y, p2.x, p2.y, p3.x, p3.y, borderColor);

		// Draw modificators
		if (!layout->m_collapsed)
		{
			const Int32 left = emitterRect.x + 6;
			const Int32 right = emitterRect.x + emitterRect.width - 6;
			const TDynArray<IParticleModule *> &modules = emitter->GetModules();
			for (Int32 i = 0; i < (Int32)modules.Size(); ++i)
			{
				if (ModificatorLayoutInfo *modificatorLayout = layout->m_modificators.FindPtr(modules[i]))
				{
					const wxRect modificatorRect = wxRect(modificatorLayout->m_localRect.x + emitterRect.x, modificatorLayout->m_localRect.y + emitterRect.y, modificatorLayout->m_localRect.GetWidth(), modificatorLayout->m_localRect.GetHeight());
					wxPoint textPosition(modificatorRect.x + 5 + EmitterLayoutInfo::ButtonSize + 2, modificatorRect.y + 3);

					if (m_activeModule == modules[i])
					{
						FillRoundedRect(modificatorRect.Deflate(5, 1), activeColorWithAlpha, 3.0f);
					}

					if (m_selectedModificator == modules[i])
					{
						FillRoundedRect(modificatorRect.Deflate(5, 1), selectedColorWithAlpha, 3.0f);
					}
					
					// Draw separator
					DrawLine(left, modificatorRect.y, right, modificatorRect.y, borderColor);

					// Draw disable button frame, if modificator is hovered
					if ( m_activeModule == modules[i] )
					{
						wxRect disableRect( modificatorLayout->m_enablePoint, wxSize( modificatorLayout->ButtonSize, modificatorLayout->ButtonSize ) );
						disableRect.Offset( modificatorRect.x, modificatorRect.y );

						DrawRect( disableRect, borderColor, ( m_activeHitArea == HA_ModificatorEnable ) ? 2.0f : 1.0f );

						disableRect.Deflate( 2.0f, 2.0f );
						// If modifier is hovered, we want the check icon to be visible
						if ( modules[i]->IsEnabled() )
						{
							DrawCheckInRect( disableRect, borderColor, 2.0f );
						}
					}

					// Draw cross if modificator is disabled
					if ( !modules[i]->IsEnabled() )
					{
						wxRect disableRect( modificatorLayout->m_enablePoint, wxSize( modificatorLayout->ButtonSize, modificatorLayout->ButtonSize ) );
						disableRect.Offset( modificatorRect.x, modificatorRect.y );
						disableRect.Deflate(2, 2);
						DrawCross( disableRect, borderColor, 2.0f );
					}
					
					// Draw buttons for only selected modificator
					if (m_activeModule == modules[i])
					{
						// Draw MoveUp button
						if (i > 0)
						{
							wxRect upRect(modificatorLayout->m_moveUpPoint, wxSize(EmitterLayoutInfo::ButtonSize, EmitterLayoutInfo::ButtonSize));
							upRect.Offset(modificatorRect.x, modificatorRect.y);
							
							CalculateTriangleInsideRect(upRect, p1, p2, p3, true, 5);
							FillTriangle(p1.x, p1.y, p2.x, p2.y, p3.x, p3.y, borderColor);

							if (m_activeHitArea == HA_MoveUp)
								DrawRect(upRect, borderColor);
						}

						// Draw MoveDown button
						if (i < (Int32)modules.Size() - 1)
						{
							wxRect downRect(modificatorLayout->m_moveDownPoint, wxSize(EmitterLayoutInfo::ButtonSize, EmitterLayoutInfo::ButtonSize));
							downRect.Offset(modificatorRect.x, modificatorRect.y);

							CalculateTriangleInsideRect(downRect, p1, p2, p3, false, 5);
							FillTriangle(p1.x, p1.y, p2.x, p2.y, p3.x, p3.y, borderColor);

							if (m_activeHitArea == HA_MoveDown)
								DrawRect(downRect, borderColor);
						}

						// This is the rectangle around the curve icon, see if we can fill it
						TDynArray< CurveParameter* > curves;
						modules[i]->GetCurves(curves);
						if (curves.Size())
						{
							wxRect curveRect(modificatorLayout->m_curvePoint, wxSize(EmitterLayoutInfo::ButtonSize, EmitterLayoutInfo::ButtonSize));
							curveRect.Offset(modificatorRect.x, modificatorRect.y);
	
							wxColor selectColor = wxColor( 0,0,0 );
							// If it is active we draw the rectangle around the curve
							if((m_activeHitArea == HA_ModificatorCurve))
							{
								DrawRect(curveRect, selectColor);
							}
						}
					}
					
					// Draw the Curve always since you want to know which modifiers have a curve or not
					TDynArray< CurveParameter* > curves;
					modules[i]->GetCurves(curves);
					if (curves.Size())
					{
						wxRect curveRect(modificatorLayout->m_curvePoint, wxSize(EmitterLayoutInfo::ButtonSize, EmitterLayoutInfo::ButtonSize));
						curveRect.Offset(modificatorRect.x, modificatorRect.y);

						// if this has been added we change the color. mthorzen
						wxColor fillColor = wxColor( 255,255,255, 75 );
						if( modules[i]->IsSelected() )
						{
							fillColor = wxColor( 255,255,255 );
						}

						FillRect( curveRect, fillColor );

						wxColor curvColor = wxColor(modules[i]->GetEditorColor().R, modules[i]->GetEditorColor().G, modules[i]->GetEditorColor().B, modules[i]->GetEditorColor().A );

						static wxPoint points[4];

						Float dx = curveRect.width / 3.0f;
						Float dy = curveRect.height / 3.0f;

						points[0] = wxPoint(curveRect.x + 0 * dx, curveRect.y + 2 * dy);
						points[1] = wxPoint(curveRect.x + 1 * dx, curveRect.y + 0 * dy);
						points[2] = wxPoint(curveRect.x + 2 * dx, curveRect.y + 5 * dy);
						points[3] = wxPoint(curveRect.x + 3 * dx, curveRect.y + 0 * dy);
						DrawCurve(points, curvColor, 1.9f);
					}

					// Draw normal text
					DrawText(textPosition, GetGdiBoldFont(), modules[i]->GetEditorName(), textColor);
				}
			}
		}
	}
}

void CEdEmitterGraphEditor::DrawCurveRect(wxRect rect, wxColor color, Bool singleCurve, Bool active)
{
	static wxPoint points[4];

	Float dx = rect.width / 3.0f;
	Float dy = rect.height / 3.0f;

	// if active module contains more than one curve then draw button with two curves
	if (!singleCurve)
	{
		points[0] = wxPoint(rect.x + 0 * dx, rect.y + 1 * dy);
		points[1] = wxPoint(rect.x + 2 * dx, rect.y + 0 * dy);
		points[2] = wxPoint(rect.x + 2 * dx, rect.y + 1 * dy);
		points[3] = wxPoint(rect.x + 3 * dx, rect.y + 3 * dy);
		
		wxColor curveColor(color.Red(), color.Green(), color.Blue(), 190);
		DrawCurve(points, curveColor, 1.9f);
	}

	// draw single curve
	{
		points[0] = wxPoint(rect.x + 0 * dx, rect.y + 2 * dy);
		points[1] = wxPoint(rect.x + 1 * dx, rect.y + 0 * dy);
		points[2] = wxPoint(rect.x + 2 * dx, rect.y + 5 * dy);
		points[3] = wxPoint(rect.x + 3 * dx, rect.y + 0 * dy);
		DrawCurve(points, color, 1.9f);
	}

	if (active)
		DrawRect(rect, color);
}

void CEdEmitterGraphEditor::CalculateTriangleInsideRect(const wxRect &rect, wxPoint &p1, wxPoint &p2, wxPoint &p3, Bool up, Int32 factor)
{
	if (factor <= 0)
	{
		factor = 5;
	}

	if (up)
	{
		p1 = wxPoint(rect.x + rect.width / 2, rect.y + rect.height / factor);
		p2 = wxPoint(rect.x + rect.width / factor, rect.y + rect.height - rect.height / factor);
		p3 = wxPoint(rect.x + rect.width - rect.width / factor, rect.y + rect.height - rect.height / factor);
	}
	else
	{
		p1 = wxPoint(rect.x + rect.width / 2, rect.y + rect.height - rect.height / factor);
		p2 = wxPoint(rect.x + rect.width / factor, rect.y + rect.height / factor);
		p3 = wxPoint(rect.x + rect.width - rect.width / factor, rect.y + rect.height / factor);
	}
}

CEdEmitterGraphEditor::EHitArea CEdEmitterGraphEditor::GetEmitterHitArea(CParticleEmitter *emitter, const wxPoint &point, IParticleModule *&activeModule)
{
	if (emitter)
	{
		if (EmitterLayoutInfo *layout = m_layout.FindPtr(emitter))
		{
			wxPoint pos;
			emitter->GetPosition(pos.x, pos.y);
			const wxRect emitterRect(pos, layout->m_size);

			// Set current emitter as active
			activeModule = emitter;

			// Check if point is inside the emitter
			if (emitterRect.Contains(point))
			{
				wxRect triggerRect(layout->m_triggerPoint, wxSize(EmitterLayoutInfo::ButtonSize, EmitterLayoutInfo::ButtonSize));
				triggerRect.Offset(emitterRect.x, emitterRect.y);
				if (triggerRect.Contains(point))
				{
					return HA_Trigger;
				}

				wxRect birthRateRect(layout->m_birthRatePoint, wxSize(EmitterLayoutInfo::ButtonSize, EmitterLayoutInfo::ButtonSize));
				birthRateRect.Offset(emitterRect.x, emitterRect.y);
				if (birthRateRect.Contains(point))
				{
					return HA_BirthRateCurve;
				}

				wxRect enableRect( layout->m_enablePoint, wxSize( EmitterLayoutInfo::ButtonSize, EmitterLayoutInfo::ButtonSize ) );
				enableRect.Offset( emitterRect.x, emitterRect.y );
				if ( enableRect.Contains( point ) )
				{
					return HA_Enable;
				}

				wxRect showRect( layout->m_showOnlyPoint, wxSize( EmitterLayoutInfo::ButtonSize, EmitterLayoutInfo::ButtonSize ) );
				showRect.Offset( emitterRect.x, emitterRect.y );
				if ( showRect.Contains( point ) )
				{
					return HA_ShowOnly;
				}

				if (!layout->m_collapsed)
				{
					const TDynArray<IParticleModule *> &modules = emitter->GetModules();
					if (modules.Size() > 0)
					{
						ModificatorLayoutInfo *modificatorLayout = layout->m_modificators.FindPtr(modules[0]);

						wxRect modificatorsRect = modificatorLayout->m_localRect;
						modificatorsRect.Offset(emitterRect.x, emitterRect.y);
						modificatorsRect.height = layout->m_size.y - modificatorLayout->m_localRect.y;

						// Check if point is inside modificators area
						if (modificatorsRect.Contains(point))
						{
							Int32 index = (point.y - modificatorsRect.y) / EmitterLayoutInfo::ModificatorHeight;
							ASSERT(index >= 0 && index < (Int32) modules.Size());

							// Set current modificator as active module
							activeModule = modules[index];
							
							modificatorLayout = layout->m_modificators.FindPtr(modules[index]);
							wxRect modificatorRect = modificatorLayout->m_localRect;
							modificatorRect.Offset(emitterRect.x, emitterRect.y);

							// Check if point is inside enable button
							wxRect modifierEnableRect( modificatorLayout->m_enablePoint, wxSize( ModificatorLayoutInfo::ButtonSize, EmitterLayoutInfo::ButtonSize ) );
							modifierEnableRect.Offset( modificatorRect.x, modificatorRect.y );
							if ( modifierEnableRect.Contains( point ) )
							{
								return HA_ModificatorEnable;
							}

							// Check if point is inside any of the modificator's buttons
							if (index > 0)
							{
								wxRect upRect(modificatorLayout->m_moveUpPoint, wxSize(EmitterLayoutInfo::ButtonSize, EmitterLayoutInfo::ButtonSize));
								upRect.Offset(modificatorRect.x, modificatorRect.y);
								if (upRect.Contains(point))
								{
									return HA_MoveUp;
								}
							}

							if (index < (Int32)modules.Size() - 1)
							{
								wxRect downRect(modificatorLayout->m_moveDownPoint, wxSize(EmitterLayoutInfo::ButtonSize, EmitterLayoutInfo::ButtonSize));
								downRect.Offset(modificatorRect.x, modificatorRect.y);
								if (downRect.Contains(point))
								{
									return HA_MoveDown;
								}
							}

							wxRect curveRect(modificatorLayout->m_curvePoint, wxSize(EmitterLayoutInfo::ButtonSize, EmitterLayoutInfo::ButtonSize));
							curveRect.Offset(modificatorRect.x, modificatorRect.y);
							if (curveRect.Contains(point))
							{
								return HA_ModificatorCurve;
							}

							return HA_Modificator;
						}
					}
				}
				
				return HA_Emitter;
			}
		}		
	}

	activeModule = NULL;
	return HA_Outside;
}

void CEdEmitterGraphEditor::ScaleGraph(Float scale, wxPoint point)
{
	scale = Clamp<Float>(scale, 0.2f, 6.0f);

	wxRect clientRect = GetClientRect();
	if (!clientRect.Contains(point))
	{
		// Calculate center of client area
		point.x = clientRect.x + clientRect.width / 2;
		point.y = clientRect.y + clientRect.height / 2;
	}

	wxPoint wp1 = ClientToCanvas(point);
	SetScale(scale);
    wxPoint wp2 = ClientToCanvas(point);
	wxPoint center = GetOffset() - (wp1 - wp2);
	SetOffset(center);
}

Gdiplus::Bitmap* CEdEmitterGraphEditor::GetThumbnailForMaterial( IMaterial* material )
{
	if ( !material || !material->GetMaterialDefinition() )
	{
		return NULL;
	}

	// 1. Find candidate textures (any texture or atlas)
	TDynArray< CName > candidateTextures;
	const IMaterialDefinition::TParameterArray& parameters = material->GetMaterialDefinition()->GetPixelParameters();
	for ( Uint32 i=0; i<parameters.Size(); ++i )
	{
		const IMaterialDefinition::Parameter& parameter = parameters[i];
		if ( parameter.m_type == IMaterialDefinition::PT_Texture || parameter.m_type == IMaterialDefinition::PT_Atlas )
		{
			candidateTextures.PushBack( parameter.m_name );
		}
	}

	// 2. Try to find the best candidate, looking at parameter name
	for ( Uint32 i=0; i<candidateTextures.Size(); ++i )
	{
		// Ok, texture parameter. Check how suitable it is, to be used as a emitter thumbnail (best representation of how particles will look)
		if ( candidateTextures[i].AsString().ContainsSubstring( TXT("color") ) || candidateTextures[i].AsString().ContainsSubstring( TXT("diffuse") ) )
		{
			// Parameter is described as color, or diffuse. That simplifies things, let's finish.
			THandle< ITexture > texture;
			material->ReadParameter( candidateTextures[i], texture );
			return GetThumbnail( texture.Get() );
		}
	}
	
	// 3. If parameter names told us nothing, just pick the biggest texture. It will likely be the most representative.
	ITexture* biggestTexture = NULL;
	Uint32 biggestSize = 0;
	for ( Uint32 i=0; i<candidateTextures.Size(); ++i )
	{
		THandle< ITexture > texture;
		material->ReadParameter( candidateTextures[i], texture );
		if ( texture && texture->GetRenderResource() )
		{
			Uint32 size = texture->GetRenderResource()->GetUsedVideoMemory();
			if ( biggestSize < size )
			{
				biggestTexture = texture.Get();
				biggestSize = size;
			}
		}
	}

	return GetThumbnail( biggestTexture );
}

Gdiplus::Bitmap *CEdEmitterGraphEditor::GetThumbnail(CResource *resource)
{
	CThumbnail *thumbnail = NULL;

	// Get thumbnail from resource
	if (resource && resource->GetFile())
	{
		CDiskFile *diskFile = resource->GetFile();

		if (diskFile->GetThumbnails().Size() == 0)
		{
			diskFile->UpdateThumbnail();
		}

		if (diskFile->GetThumbnails().Size() > 0)
		{
			thumbnail = diskFile->GetThumbnails()[0];
		}
	}

	// First check the cache
	if (Gdiplus::Bitmap **bitmap = m_thumbnails.FindPtr(thumbnail))
	{
		return *bitmap;
	}

	// If bitmap does not exist in the cache, create it
	if (thumbnail)
	{
		if (CWXThumbnailImage *thumbnailImage = (CWXThumbnailImage *)thumbnail->GetImage())
		{
			Gdiplus::Bitmap *bitmap = thumbnailImage->GetBitmap();
			m_thumbnails.Insert(thumbnail, bitmap);
			return bitmap;
		}
	}

	return NULL;
}

void CEdEmitterGraphEditor::DrawCheckInRect( wxRect rect, wxColor color, Float width )
{
	wxPoint v1( rect.x, rect.y + rect.height / 2 );
	wxPoint v2( rect.x + rect.width/2, rect.y + rect.height );
	wxPoint v3( rect.x + rect.width, rect.y );
	DrawLine( v1, v2, color, width );
	DrawLine( v2, v3, color, width );
}

void CEdEmitterGraphEditor::DrawEyeInRect( wxRect rect, wxColor color, Float width )
{
	Float circleWidth = 4.5f;
	DrawArc( rect, 20.0f, 140.0f, color, width );
	DrawArc( rect, -20.0f, -140.0f, color, width );
	FillCircle( (rect.x + ( (rect.width/2) - (circleWidth/4) ) ), (rect.y + ( (rect.height/2) ) - (circleWidth/4) ), circleWidth, color );
}

void CEdEmitterGraphEditor::DrawClosedEyeInRect( wxRect rect, wxColor color, Float width )
{
	wxRect halfRect; 
	halfRect.SetHeight( rect.height/2 );
	halfRect.SetWidth( rect.width );
	halfRect.SetX( rect.x );
	halfRect.SetY( rect.y );
	halfRect.Offset( 0,halfRect.GetHeight()/2 );
	DrawArc( halfRect, 20.0f, 140.0f, color, width );
	DrawArc( halfRect, -20.0f, -140.0f, color, width );
}

class ParticleEmittersArrayWrapper : public wxObject
{
public:
	TDynArray<CParticleEmitter *> m_emitters;

public:
	ParticleEmittersArrayWrapper( TDynArray<CParticleEmitter *> &emittersArray )
	{
		for (Int32 i = 0; i < (Int32) emittersArray.Size(); ++i)
			m_emitters.PushBack(emittersArray[i]);
	}
};

class ParticleEmitterWrapper : public wxObject
{
public:
	CParticleEmitter *m_emitter;

public:
	ParticleEmitterWrapper(CParticleEmitter* emitter)
		: m_emitter(emitter) {}
};

class ModificatorWrapper : public wxObject
{
public:
	CParticleEmitter *m_emitter;
	IParticleModule *m_module;

public:
	ModificatorWrapper(CParticleEmitter *emitter, IParticleModule *module)
		: m_emitter(emitter)
		, m_module(module) {}
};

void CEdEmitterGraphEditor::MouseClick( wxMouseEvent& event )
{
	// Pass to the base class
	CEdCanvas::MouseClick( event );

	if ( event.ButtonDown() )
	{
		MouseDown( event );
	}
	else
	{
		MouseUp( event );
	}

	m_lastMousePosition = event.GetPosition();
}

void CEdEmitterGraphEditor::MouseDown( wxMouseEvent& event )
{
	// Zooming
	if ( event.ShiftDown() )
	{
		// Only when in no mode
		if (m_action == MA_None )
		{
			if ( event.LeftDown() )
			{
				// Full zoom in
				ScaleGraph(2.0f, event.GetPosition());
				// Don't do any more processing
				return;
			}
			else if ( event.RightDown() )
			{
				// Full zoom out
				ScaleGraph(1.0f, event.GetPosition());
				// Don't do any more processing
				return;
			}
		}
	}

	// Background drag
	if ( m_action == MA_None && event.RightDown() )
	{
		m_moveTotal	= 0;
		m_action = MA_BackgroundScroll;
		CaptureMouse(true, false);
	}

	// Click
	if ( m_action == MA_None && event.LeftDown() )
	{
		IParticleModule *activeModule = NULL;
		EHitArea hitArea = GetEmitterHitArea(GetEmitter(m_activeModule), ClientToCanvas(event.GetPosition()), activeModule);

		if (m_activeModule != activeModule)
		{
			m_activeModule = activeModule;
			Repaint();
		}

		if (m_activeHitArea != hitArea)
		{
			m_activeHitArea = hitArea;
			Repaint();
		}

		CParticleEmitter *emitter = GetEmitter(m_activeModule);
		IParticleModule *modificator = GetModificator(m_activeModule);

		if (hitArea == HA_Emitter)
		{
			ASSERT(IsEmitter(m_activeModule));
		}
		else if (hitArea == HA_Trigger)
		{
			ASSERT(IsEmitter(m_activeModule));
			EmitterLayoutInfo *layout = m_layout.FindPtr(emitter);
			layout->m_collapsed = !layout->m_collapsed;
			UpdateEmitterLayout(emitter);
			Repaint();
			return;
		}
		else if ( hitArea == HA_ShowOnly )
		{
   			ASSERT(IsEmitter(m_activeModule));
			ShowOnlySelectedEmitter();
			
			UpdateEmitterLayout(emitter);
			Repaint();
			return;
		}
		else if ( hitArea == HA_Enable )
		{
			ASSERT(IsEmitter(m_activeModule));
			RED_ASSERT( m_editor->GetEditingLOD() < emitter->GetLODCount() );

			SParticleEmitterLODLevel& lod = emitter->GetLOD( m_editor->GetEditingLOD() );
			lod.m_isEnabled = !lod.m_isEnabled;
			m_editor->OnEmitterChanged( emitter );
			UpdateEmitterLayout(emitter);
			Repaint();
			return;
		}
		else if ( hitArea == HA_Modificator )
		{
			ASSERT(IsModificator(m_activeModule));
		}
		else if ( hitArea == HA_ModificatorEnable )
		{
			ASSERT( IsModificator( m_activeModule ) );
			activeModule->SetEnable( !activeModule->IsEnabled() );
			m_editor->OnEmitterChanged( emitter );
			emitter->ResetInstances();
			// update
			UpdateEmitterLayout(emitter);
			Repaint();
		}
		else if (hitArea == HA_MoveUp)
		{
			ASSERT(IsModificator(m_activeModule));

			// Move selected modificator up
			if (emitter)
			{
				emitter->MoveModule(modificator, -1);
					
				// update
				UpdateEmitterLayout(emitter);
				Repaint();

				// Move mouse cursor
				const wxPoint p1 = CanvasToClient(wxPoint(0, 0));
				const wxPoint p2 = CanvasToClient(wxPoint(0, EmitterLayoutInfo::ModificatorHeight));
				const Int32 modificatorHeight = p2.y - p1.y;
				wxPoint cursorPos(event.GetX(), event.GetY() - modificatorHeight);
				cursorPos = ClientToScreen(cursorPos);
				SetCursorPos(cursorPos.x, cursorPos.y);
			}

			return;
		}
		else if (hitArea == HA_MoveDown)
		{
			ASSERT(IsModificator(m_activeModule));

			// Move selected modificator down
			if (emitter)
			{
				emitter->MoveModule(modificator, 1);

				// update
				UpdateEmitterLayout(emitter);
				Repaint();

				// Move mouse cursor
				const wxPoint p1 = CanvasToClient(wxPoint(0, 0));
				const wxPoint p2 = CanvasToClient(wxPoint(0, EmitterLayoutInfo::ModificatorHeight));
				const Int32 modificatorHeight = p2.y - p1.y;
				wxPoint cursorPos(event.GetX(), event.GetY() + modificatorHeight);
				cursorPos = ClientToScreen(cursorPos);
				SetCursorPos(cursorPos.x, cursorPos.y);

				return;
			}
		}
		else if (hitArea == HA_ModificatorCurve || hitArea == HA_BirthRateCurve)
		{
			// When clicking here we should lock the particle in the curve editor canvas, then we should unhook it if it is pinned
			if (emitter)
			{
				// Get curves
				TDynArray< CurveParameter* > curves;
				m_activeModule->GetCurves(curves);
				if (hitArea == HA_ModificatorCurve)
				{
					ASSERT(IsModificator(m_activeModule));
				}

				// Add curves to menu
				if (curves.Size() > 1)
				{
					wxMenu menu;
					menu.Append( wxID_ANY, TXT("Curves"), wxEmptyString );
					menu.AppendSeparator();
					for (Uint32 i=0; i < curves.Size(); i++)
					{
						if (CurveParameter* param = curves[i])
						{
							menu.Append( ID_EDIT_CURVE + i, param->GetName().AsString().AsChar(), wxEmptyString );
							if (hitArea == HA_ModificatorCurve)
								menu.Connect( ID_EDIT_CURVE + i, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdEmitterGraphEditor::OnEditCurve ), new ModificatorWrapper(emitter, modificator), this );
							else
								menu.Connect( ID_EDIT_CURVE + i, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdEmitterGraphEditor::OnEditCurve ), new ParticleEmitterWrapper(emitter), this );
						}
					}
					PopupMenu(&menu);
				}
				else if (curves.Size() == 1)
				{
					if( !m_activeModule->IsSelected() )
					{
						// Get the color from the module and put that into the curve color
						// if the color is black lets change it here.
						const Color tempColor = m_activeModule->GetEditorColor();

						if( tempColor.R == 0 && tempColor.G == 0 && tempColor.B == 0 )
						{
							m_activeModule->SetEditorColor( RandomPastelColor() );
						}
						curves[0]->SetColor( m_activeModule->GetEditorColor() );

						// Only delete none pinned groups
						m_editor->DeleteAllCurveGroups();

						m_activeModule->SetSelected( true );

						CParticleEmitter *emitter = GetEmitter(m_activeModule);
						String emitterName  = emitter->GetEditorName();
						String modName = m_activeModule->GetEditorName();
						const String moduleName = emitterName+TXT(":")+modName;
						Bool pinned = true;
						m_editor->EditCurve(curves[0], moduleName, pinned);
					}
					else
					{
						m_activeModule->SetSelected( false );
						// Only delete none pinned groups
						m_editor->DeleteAllCurveGroups();

						CParticleEmitter *emitter = GetEmitter(m_activeModule);
						String emitterName  = emitter->GetEditorName();
						String modName = m_activeModule->GetEditorName();
						const String moduleName = emitterName+TXT(":")+modName;

						m_editor->DeleteCurveGroup( moduleName );
					}
				}
			}

			return;
		}

		// Click selection
		if (IsEmitter(m_activeModule))
		{
			CParticleEmitter *emitter = GetEmitter(m_activeModule);
			if ( event.ControlDown() )
			{
				SelectEmitter(emitter, !IsEmitterSelected(emitter));
			}
			else
			{
				if (!IsEmitterSelected(emitter))
				{
					m_selected.Clear();
					SelectEmitter(emitter, true);
				}

				// If there are selected emitters begin dragging
				if (m_selected.Size())
				{
					m_blocksMoved = false;
					m_action = MA_MovingBlocks;
					CaptureMouse( true, false );
				}
			}

			Repaint();
		}
		else if (IsModificator(m_activeModule))
		{
			SelectModificator(m_activeModule);
			// Make this be the selection instead of the small rectangle on the right
			if (emitter)
			{
				// Get curves
				TDynArray< CurveParameter* > curves;
				m_activeModule->GetCurves(curves);
				if (hitArea == HA_ModificatorCurve)
				{
					ASSERT(IsModificator(m_activeModule));
				}

				if (curves.Size() == 1)
				{
					// Get the color from the module and put that into the curve color
					// if the color is black lets change it here.
					const Color tempColor = m_activeModule->GetEditorColor();

					if( tempColor.R == 0 && tempColor.G == 0 && tempColor.B == 0 )
					{
						m_activeModule->SetEditorColor( RandomPastelColor() );
					}
					curves[0]->SetColor( m_activeModule->GetEditorColor() );

					// need to check if they are pinned or not
					m_editor->DeleteAllCurveGroups();
					// Module Name and the emitter name
					CParticleEmitter *emitter = GetEmitter(m_activeModule);
					String emitterName  = emitter->GetEditorName();
					String modName = m_activeModule->GetEditorName();
					const String moduleName = emitterName+TXT(":")+modName;
					m_editor->EditCurve(curves[0], moduleName);
				}
				else
				{
					m_editor->DeleteAllCurveGroups();
				}
			}
			Repaint();
		}

		// Selection rectangle
		if (m_activeModule == NULL)
		{
			// Initialize selection rect
			m_selectRectStart = ClientToCanvas( event.GetPosition() );
			m_selectRectEnd = ClientToCanvas( event.GetPosition() );

			// Clears everything that was selected.
			m_editor->DeleteAllCurveGroups();
			m_selected.Clear();
			m_selectedModificator = NULL;
			
			// Start mode
			m_action = MA_SelectingBlocks;
			CaptureMouse(true, false);
		}
	}
}

void CEdEmitterGraphEditor::MouseUp( wxMouseEvent& event )
{
	// Left button
	if ( m_action == MA_BackgroundScroll && event.RightUp() )
	{
		// Uncapture
		m_action = MA_None;
		CaptureMouse(false, false);

		// Minimal movement, show menu
		if (m_moveTotal < 5)
		{
			OpenContextMenu();
		}
	}

	// Finished movement
	if ( m_action == MA_MovingBlocks && event.LeftUp() )
	{
		if (!m_blocksMoved)
		{
			// deselect blocks if we just clicked on block
			SelectAll(false);

			// select active block
			if (CParticleEmitter *emitter = GetEmitter(m_activeModule))
			{
				SelectEmitter(emitter, true);
			}

			Repaint();
		}

		m_action = MA_None;
		CaptureMouse( false, false );
	}

	// Finished window selection
	if ( m_action == MA_SelectingBlocks && event.LeftUp() )
	{
		// End drag
		m_action = MA_None;
		CaptureMouse( false, false );

		// Assemble rect
		wxRect selectionRect;
		selectionRect.x = Min( m_selectRectStart.x, m_selectRectEnd.x );
		selectionRect.y = Min( m_selectRectStart.y, m_selectRectEnd.y );
		selectionRect.width = Abs( m_selectRectEnd.x - m_selectRectStart.x );
		selectionRect.height = Abs( m_selectRectEnd.y - m_selectRectStart.y );

		// No control, deselect all blocks first
		if ( ! event.ControlDown() )
		{
			SelectAll(false);
		}

		// Select blocks from area
		SelectEmittersFromArea(selectionRect);

		// Repaint
		Repaint();
	}
}

void CEdEmitterGraphEditor::MouseMove( wxMouseEvent& event, wxPoint delta )
{
	wxPoint point = event.GetPosition();

	// Accumulate move
	m_moveTotal += Abs( delta.x ) + Abs( delta.y );

	if (m_action == MA_MovingBlocks)
	{
		// windows were moved
		m_blocksMoved = true;

		//Float scale = GetScale();
		//MoveSelectedBlocks( wxPoint( delta.x / scale, delta.y / scale ) );

		wxPoint deltaMove = ClientToCanvas(point) - ClientToCanvas(m_lastMousePosition);
		wxPoint pos;
		for (THashSet<CParticleEmitter *>::iterator it = m_selected.Begin(); it != m_selected.End(); ++it)
		{
			(*it)->GetPosition(pos.x, pos.y);
			(*it)->SetPosition(pos.x + deltaMove.x, pos.y + deltaMove.y);
		}
		Repaint(true);
	}
	else if (m_action == MA_BackgroundScroll)
	{
		//Float scale = GetScale();
		//ScrollBackgroundOffset( wxPoint( delta.x / scale, delta.y / scale ) );
		wxPoint deltaMove = ClientToCanvas(point) - ClientToCanvas(m_lastMousePosition);
		SetOffset(GetOffset() + deltaMove);
		Repaint(true);
	}
	else if (m_action == MA_SelectingBlocks)
	{
		// Set selection rect
		m_selectRectEnd = ClientToCanvas( point );

		// Repaint
		Repaint();
	}
	else if (m_action == MA_None)
	{
		// Check if mouse is over any emitter
		wxPoint canvasPoint = ClientToCanvas(point);
		IParticleModule *activeModule = NULL;
		EHitArea hitArea = HA_Outside;
		const TDynArray< CParticleEmitter* >& emitters = m_particleSystem->GetEmitters();
		for (Int32 i = emitters.Size() - 1; i >= 0; --i)
		{
			hitArea = GetEmitterHitArea(emitters[i], canvasPoint, activeModule);
			if (activeModule)
			{
				break;
			}
		}

		if (m_activeModule != activeModule)
		{
			m_activeModule = activeModule;
			Repaint();
		}

		if (m_activeHitArea != hitArea)
		{
			m_activeHitArea = hitArea;
			Repaint();
		}
	}

	m_lastMousePosition = point;
}

void CEdEmitterGraphEditor::OnMouseWheel( wxMouseEvent& event )
{
	const Float canvasWheelZoomRatio = 1.2f;
	const Float zoomSnapFactor = 4.0f;
	Float delta = (event.GetWheelDelta() / (Float)event.GetWheelRotation());
	Float scale = GetScale();
	Float newScale = scale / (Float)powf(canvasWheelZoomRatio, -delta);

	const Int32 minValue = Int32(min(scale, newScale) - 0.001f);
	const Int32 maxValue = Int32(max(scale, newScale) + 0.001f);
	if (minValue == 0 && maxValue == 1)
	{
		if (m_wheelTotal < zoomSnapFactor)
		{
			newScale = 1.0f;
			m_wheelTotal += fabs(delta);
		}
		else
		{
			m_wheelTotal = 0.0f;
		}
	}

	ScaleGraph(newScale, event.GetPosition());
	Repaint();
}

void CEdEmitterGraphEditor::OnKeyDown(wxKeyEvent &event)
{
	if (event.GetKeyCode() == WXK_DELETE)
	{
		IParticleModule *editedModule = GetEditedModule();
		if (IsModificator(editedModule))
		{
			// Remove modifier
			RemoveModule( editedModule );
		}
		else if (IsEmitter(editedModule))
		{
			for (THashSet<CParticleEmitter *>::iterator it = m_selected.Begin(); it != m_selected.End(); ++it)
			{
				CParticleEmitter* emitter = *it;
				ASSERT(emitter);

				// Notify editor, that this emitter is going to be removed
				m_editor->OnEmitterRemove( emitter );

				// Remove emitter
				m_particleSystem->RemoveEmitter( emitter );

				// Update layout
				m_layout.Erase( emitter );
			}

			// Clear selection
			SelectAll(false);
		}
		
		Repaint();
	}
	else if (event.GetKeyCode() == (Int32)'A')
	{
		if (event.ControlDown())
		{
			SelectAll(true);
		}
	}
}

void CEdEmitterGraphEditor::OpenContextMenu()
{
	wxMenu menu;

	// Create popup menu only for selected modules
	if (CParticleEmitter *emitter = GetEmitter(m_activeModule))
	{
		if (IsEmitterSelected(emitter))
		{
			if (IsEmitter(m_activeModule))
			{
				TDynArray<CParticleEmitter *> emittersArray;
				for (THashSet<CParticleEmitter *>::iterator it = m_selected.Begin(); it != m_selected.End(); ++it)
					emittersArray.PushBack(*it);

				menu.Append( ID_EDIT_CLONE, TXT("Clone") );
				menu.Connect( ID_EDIT_CLONE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdEmitterGraphEditor::OnEmitterClone ), new ParticleEmittersArrayWrapper(emittersArray), this );
				menu.Append( ID_EDIT_COPY_TO_CLIPBOARD, TXT("Copy to clipboard") );
				menu.Connect( ID_EDIT_COPY_TO_CLIPBOARD, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdEmitterGraphEditor::OnCopyToClipboard ), new ParticleEmittersArrayWrapper(emittersArray), this );
				menu.Append( ID_EDIT_REMOVE, TXT("Remove") );
				menu.Connect( ID_EDIT_REMOVE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdEmitterGraphEditor::OnEmitterRemove ), new ParticleEmittersArrayWrapper(emittersArray), this );

				// Get the available classes
				TDynArray< CClass* > modificatorClasses;	
				SRTTI::GetInstance().EnumClasses( ClassID< IParticleModule >(), modificatorClasses );

				// Create subMenu
				wxMenu *subMenu = new wxMenu();

				// Create subSubMenu
				wxMenu *defaultMenu = new wxMenu();
				wxMenu *locationMenu = new wxMenu();
				wxMenu *rotationMenu = new wxMenu();
				wxMenu *materialMenu = new wxMenu();
				wxMenu *velocityMenu = new wxMenu();
				wxMenu *sizeMenu = new wxMenu();
				wxMenu *physicsMenu = new wxMenu();
				// Fill the list
				for( Int32 i = 0; i < (Int32)modificatorClasses.Size(); ++i )
				{		
					String name = modificatorClasses[i]->GetDefaultObject< IParticleModule >()->GetEditorName();
					String group = modificatorClasses[i]->GetDefaultObject< IParticleModule >()->GetEditorGroup();
					if ( !name.Empty() )
					{
						if ( group == TXT("Default") )
						{
							defaultMenu->Append( ID_EDIT_ADD_MODIFICATOR + i, name.AsChar(), wxEmptyString );
							menu.Connect( ID_EDIT_ADD_MODIFICATOR + i, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdEmitterGraphEditor::OnModificatorAdd ), new ParticleEmitterWrapper(emitter), this );
						}
						else if ( group == TXT("Location") )
						{
							locationMenu->Append( ID_EDIT_ADD_MODIFICATOR + i, name.AsChar(), wxEmptyString );
							menu.Connect( ID_EDIT_ADD_MODIFICATOR + i, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdEmitterGraphEditor::OnModificatorAdd ), new ParticleEmitterWrapper(emitter), this );
						}
						else if ( group == TXT("Rotation") )
						{
							rotationMenu->Append( ID_EDIT_ADD_MODIFICATOR + i, name.AsChar(), wxEmptyString );
							menu.Connect( ID_EDIT_ADD_MODIFICATOR + i, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdEmitterGraphEditor::OnModificatorAdd ), new ParticleEmitterWrapper(emitter), this );
						}
						else if ( group == TXT("Material") )
						{
							materialMenu->Append( ID_EDIT_ADD_MODIFICATOR + i, name.AsChar(), wxEmptyString );
							menu.Connect( ID_EDIT_ADD_MODIFICATOR + i, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdEmitterGraphEditor::OnModificatorAdd ), new ParticleEmitterWrapper(emitter), this );
						}
						else if ( group == TXT("Velocity") )
						{
							velocityMenu->Append( ID_EDIT_ADD_MODIFICATOR + i, name.AsChar(), wxEmptyString );
							menu.Connect( ID_EDIT_ADD_MODIFICATOR + i, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdEmitterGraphEditor::OnModificatorAdd ), new ParticleEmitterWrapper(emitter), this );
						}
						else if ( group == TXT("Size") )
						{
							sizeMenu->Append( ID_EDIT_ADD_MODIFICATOR + i, name.AsChar(), wxEmptyString );
							menu.Connect( ID_EDIT_ADD_MODIFICATOR + i, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdEmitterGraphEditor::OnModificatorAdd ), new ParticleEmitterWrapper(emitter), this );
						}
						else if ( group == TXT("Physics") )
						{
							physicsMenu->Append( ID_EDIT_ADD_MODIFICATOR + i, name.AsChar(), wxEmptyString );
							menu.Connect( ID_EDIT_ADD_MODIFICATOR + i, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdEmitterGraphEditor::OnModificatorAdd ), new ParticleEmitterWrapper(emitter), this );
						}
					}
				}

				subMenu->Append( wxID_ANY, TXT("Default"), defaultMenu);
				subMenu->Append( wxID_ANY, TXT("Location"), locationMenu);
				subMenu->Append( wxID_ANY, TXT("Rotation"), rotationMenu);
				subMenu->Append( wxID_ANY, TXT("Material"), materialMenu);
				subMenu->Append( wxID_ANY, TXT("Velocity"), velocityMenu);
				subMenu->Append( wxID_ANY, TXT("Size"), sizeMenu);
				subMenu->Append( wxID_ANY, TXT("Physics"), physicsMenu);
				menu.AppendSeparator();
				menu.Append( wxID_ANY, TXT("Add Modificator"), subMenu);

				menu.AppendSeparator();
				if ( emitter->IsEnabled() )
				{
					menu.AppendCheckItem( ID_EDIT_GLOBAL_DISABLE, TXT("Disable Selected") );
				}
				else
				{
					menu.AppendCheckItem( ID_EDIT_GLOBAL_DISABLE, TXT("Enable Selected") );
				}
				menu.Connect( ID_EDIT_GLOBAL_DISABLE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdEmitterGraphEditor::OnEmitterGlobalDisable ), new ParticleEmittersArrayWrapper(emittersArray), this );
			}
			else if (IsModificator(m_activeModule))
			{
				menu.Append(ID_EDIT_REMOVE_MODIFICATOR, String::Printf(TXT("Remove Modificator   %s"), m_activeModule->GetEditorName().AsChar()).AsChar());
				menu.Connect(ID_EDIT_REMOVE_MODIFICATOR, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdEmitterGraphEditor::OnModificatorRemove ), new ModificatorWrapper(GetEmitter(m_activeModule), GetModificator(m_activeModule)), this );

				//menu.Append(ID_EDIT_COPY_MODIFICATOR, String::Printf(TXT("Copy Modificator   %s"), m_activeModule->GetEditorName().AsChar()).AsChar());
				//menu.Connect(ID_EDIT_COPY_MODIFICATOR, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdEmitterGraphEditor::OnModificatorCopy ), new ModificatorWrapper(GetEmitter(m_activeModule), GetModificator(m_activeModule)), this );
			}
		}
	}
	else
	{
		menu.Append( ID_EDIT_ADD, TXT("Add Emitter") );
		menu.Connect( ID_EDIT_ADD, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdEmitterGraphEditor::OnEmitterAdd ), NULL, this );
		menu.AppendSeparator();
		menu.Append( ID_EDIT_RESET, TXT("Reset Layout") );
		menu.Connect( ID_EDIT_RESET, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdEmitterGraphEditor::OnLayoutReset ), NULL, this );
		menu.Append( ID_EDIT_FIT, TXT("Fit to window") );
		menu.Connect( ID_EDIT_FIT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdEmitterGraphEditor::OnFitToWindow ), NULL, this );
		menu.Append( ID_EDIT_PASTE_FROM_CLIPBOARD, TXT("Paste from clipboard") );
		menu.Connect( ID_EDIT_PASTE_FROM_CLIPBOARD, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdEmitterGraphEditor::OnPasteFromClipboard ), NULL, this );
	}

	PopupMenu(&menu);
}

CParticleEmitter *CEdEmitterGraphEditor::GetEmitter(IParticleModule *module) const
{
	if (CParticleEmitter *emitter = Cast< CParticleEmitter >(module))
	{
		return emitter;
	}

	if (module)
	{
		return module->GetEmitter();
	}

	return NULL;
}

void CEdEmitterGraphEditor::SelectModificator(IParticleModule *modificator)
{
	if (m_selectedModificator != modificator)
	{
		m_selected.Clear();
		m_selectedModificator = modificator;
		m_editor->UpdateProperties();
		m_editor->UpdateLodProperties();
	}
}

void CEdEmitterGraphEditor::SelectEmitter(CParticleEmitter *emitter, Bool select, Bool exclusive)
{
	ASSERT( emitter );
	m_selectedModificator = NULL;

	Bool updateNeeded = false;

	if ( select && exclusive && !m_selected.Empty() && !( m_selected.Size()==1 && *m_selected.Begin()==emitter) )
	{
		updateNeeded = true;
		m_selected.Clear();
	}

	// Update only if selection differs
	if ( IsEmitterSelected( emitter ) != select )
	{
		updateNeeded = true;

		if ( select )
		{
			m_selected.Insert( emitter );
		}
		else
		{
			m_selected.Erase( emitter );
		}
	}

	if ( updateNeeded )
	{
		m_editor->UpdateProperties();
		m_editor->UpdateLodProperties();
	}

}

void CEdEmitterGraphEditor::SelectAll(Bool select)
{
	m_selected.Clear();

	if (select)
	{
		for ( THashMap<CParticleEmitter *, EmitterLayoutInfo>::const_iterator it = m_layout.Begin(); it != m_layout.End(); ++it)
		{
			m_selected.Insert( it->m_first );
		}
	}

	m_editor->UpdateProperties();
	m_editor->UpdateLodProperties();
}

void CEdEmitterGraphEditor::SelectEmittersFromArea(const wxRect &area)
{
	const TDynArray< CParticleEmitter* >& emitters = m_particleSystem->GetEmitters();
	for (Uint32 i = 0; i < emitters.Size(); i++ )
	{
		// Get layout
		CParticleEmitter* emitter = emitters[i];

		// Select only if layout data is valid
		if (EmitterLayoutInfo *layout = m_layout.FindPtr(emitter))
		{
			// Selected ?
			wxPoint pos;
			emitter->GetPosition(pos.x, pos.y);
			const wxRect emitterRect(pos, layout->m_size);
			if (area.Intersects(emitterRect) && !IsEmitterSelected(emitter) )
			{
				SelectEmitter(emitter, true);
			}
		}
	}
}

void CEdEmitterGraphEditor::OnEmitterAdd( wxCommandEvent& event )
{
	// Ask for emitter name
	String emitterName = String::Printf( TXT("Emitter %d"), m_particleSystem->GetEmitters().Size() );
	if ( !InputBox( this, TXT("Add emitter"), TXT("Please give emitter a name"), emitterName ) )
	{
		return;
	}

	// Create particle emitter
	// Only if the name is unique
	TDynArray< CParticleEmitter* > particles = m_particleSystem->GetEmitters();
	for( Uint32 i=0; i<particles.Size(); ++i )
	{
		const String name = particles[i]->GetEditorName();
		if( name == emitterName )
		{
			wxMessageBox( TXT("Unable to create new particle emitter. That name is already used"), TXT("Error"), wxOK | wxICON_WARNING );
			return;
		}
	}
	
	CParticleEmitter* emitter = m_particleSystem->AddEmitter( ClassID<CParticleEmitter>(), emitterName );
	if ( !emitter )
	{
		wxMessageBox( TXT("Unable to create new particle emitter"), TXT("Error"), wxOK | wxICON_WARNING );
		return;
	}

	// Add default modificator to the newly created emitter
	IParticleModule* module = emitter->AddModule( ClassID< CParticleInitializerLifeTime >() );

	if ( CDiskFile* matFile = GDepot->FindFile( TXT("engine\\materials\\defaults\\flare.w2mg") ) )
	{
		matFile->Load();
		if ( IMaterial* mat = Cast< IMaterial >( matFile->GetResource() ) )
		{
			emitter->SetMaterial( mat );
		}
	}
	

	// Update layout
	UpdateEmitterLayout(emitter);
	wxPoint pos = ClientToCanvas(m_lastClickPoint);
	emitter->SetPosition(pos.x, pos.y);
	Repaint();
		
	// Notify editor
	m_editor->OnEmitterAdded( emitter );
}

void CEdEmitterGraphEditor::OnEmitterClone( wxCommandEvent& event )
{
	ASSERT( m_particleSystem );
	ParticleEmittersArrayWrapper* wrapper = (ParticleEmittersArrayWrapper *) event.m_callbackUserData;
	ASSERT( wrapper );

	for (Int32 i = 0; i < (Int32) wrapper->m_emitters.Size(); ++i)
	{
		CParticleEmitter* emitter = wrapper->m_emitters[i];
		ASSERT( emitter );

		// Ask for the emitter name
		if ( emitter )
		{
			// Assemble emitter name
			String emitterName = String( TXT("Copy of ") ) + emitter->GetEditorName();
			if ( !InputBox( this, TXT("Clone emitter"), TXT("Please give emitter a name"), emitterName ) )
			{
				return;
			}

			// Create particle emitter
			//String name = i > 0 ? String::Printf(TXT("%s %d", emitterName, i)) : emitterName;
			CParticleEmitter* clonedEmitter = m_particleSystem->CloneEmitter( emitter, emitterName );
			if ( !clonedEmitter )
			{
				wxMessageBox( TXT("Unable to clone particle emitter"), TXT("Error"), wxOK | wxICON_WARNING );
				return;
			}

			// Update layout
			EmitterLayoutInfo *layout = m_layout.FindPtr(emitter);
			ASSERT(layout);
			wxPoint pos;
			emitter->GetPosition(pos.x, pos.y);
			clonedEmitter->SetPosition(pos.x + layout->m_size.x / 2, pos.y + layout->m_size.y / 2);
			UpdateEmitterLayout(clonedEmitter);

			// Notify editor
			m_editor->OnEmitterAdded( clonedEmitter );
		}
	}

	Repaint();
}

void CEdEmitterGraphEditor::OnEmitterRemove( wxCommandEvent& event )
{
	ASSERT(m_particleSystem);
	ParticleEmittersArrayWrapper* wrapper = (ParticleEmittersArrayWrapper *) event.m_callbackUserData;
	ASSERT(wrapper);

	for (Int32 i = 0; i < (Int32)wrapper->m_emitters.Size(); ++i)
	{
		CParticleEmitter* emitter = wrapper->m_emitters[i];
		ASSERT( emitter );
		// Asking the user if they really would like to delete the emitter or not.
		if ( GFeedback->AskYesNo( TXT("Do you want to delete this emitter?\n'%s'?"), emitter->GetEditorName().AsChar() ) )
		{		
			// Notify main frame of the emitter removal
			m_editor->OnEmitterRemove( emitter );

			// Remove existing emitter
			m_particleSystem->RemoveEmitter( emitter );

			// Update layout
			m_layout.Erase( emitter );
		}
	}

	SelectAll(false);
	Repaint();
}

void CEdEmitterGraphEditor::OnEmitterGlobalDisable( wxCommandEvent& event )
{
	ASSERT(m_particleSystem);
	ParticleEmittersArrayWrapper* wrapper = (ParticleEmittersArrayWrapper *) event.m_callbackUserData;
	ASSERT(wrapper);
	ASSERT(m_activeModule && IsEmitter(m_activeModule));

	Bool newEnable = !m_activeModule->IsEnabled();

	for (Int32 i = 0; i < (Int32)wrapper->m_emitters.Size(); ++i)
	{
		CParticleEmitter* emitter = wrapper->m_emitters[i];
		ASSERT( emitter );
		emitter->SetEnable( newEnable );
		m_editor->OnEmitterChanged( emitter );
		emitter->ResetInstances();
	}

	SelectAll(false);
	Repaint();
}


void CEdEmitterGraphEditor::OnModificatorAdd( wxCommandEvent& event )
{
	ParticleEmitterWrapper* wrapper = (ParticleEmitterWrapper *) event.m_callbackUserData;
	ASSERT( wrapper );

	CParticleEmitter* emitter = wrapper->m_emitter;
	ASSERT( emitter );

	// Enumerate classes
	TDynArray< CClass* > modificatorClasses;
	SRTTI::GetInstance().EnumClasses( ClassID< IParticleModule >(), modificatorClasses );

	// The the class ID
	Int32 id = event.GetId() - ID_EDIT_ADD_MODIFICATOR;
	ASSERT( id >= 0 && id < (Int32) modificatorClasses.Size() );

	// Add modificator
	CClass *moduleClass = modificatorClasses[ id ];
	IParticleModule* module = emitter->AddModule( moduleClass );
	if ( !module )
	{
		wxMessageBox( TXT("Unable to add new particle module"), TXT("Warning"), wxOK | wxICON_WARNING );
		return;
	}

	// Update layout
	EmitterLayoutInfo *layout = m_layout.FindPtr(emitter);
	ASSERT(layout);
	UpdateEmitterLayout(emitter);

	// Notify editor
	m_editor->OnEmitterChanged( emitter );
	emitter->ResetInstances();

	// Repaint canvas
	Repaint();
}

void CEdEmitterGraphEditor::OnModificatorRemove( wxCommandEvent& event )
{
	ModificatorWrapper* wrapper = (ModificatorWrapper *) event.m_callbackUserData;
	ASSERT(wrapper);

	// Extract module from event
	CParticleEmitter *emitter = wrapper->m_emitter;
	IParticleModule *module = wrapper->m_module;
	ASSERT(emitter);
	ASSERT(module);

	// Perform removal
	RemoveModule( module );
}

void CEdEmitterGraphEditor::OnModificatorCopy( wxCommandEvent& event )
{
	ModificatorWrapper* wrapper = (ModificatorWrapper *) event.m_callbackUserData;
	ASSERT(wrapper);

	// Extract module from event
	CParticleEmitter *emitter = wrapper->m_emitter;
	IParticleModule *module = wrapper->m_module;
	ASSERT(emitter);
	ASSERT(module);

	// Perform removal
	//CopyModule( module );
}

void CEdEmitterGraphEditor::OnEditCurve( wxCommandEvent& event )
{
	IParticleModule *module = NULL;
	if (ModificatorWrapper *wrapper = dynamic_cast<ModificatorWrapper *>(event.m_callbackUserData))
		module = wrapper->m_module;
	else if (ParticleEmitterWrapper *wrapper = dynamic_cast<ParticleEmitterWrapper *>(event.m_callbackUserData))
		module = wrapper->m_emitter;
	ASSERT(module);

	// Get curves
	TDynArray<CurveParameter *> curves;
	module->GetCurves(curves);

	// The the class ID
	Int32 id = event.GetId() - ID_EDIT_CURVE;
	ASSERT(id >= 0 && id < (Int32) curves.Size());

	CParticleEmitter *emitter = GetEmitter(m_activeModule);
	String emitterName  = emitter->GetEditorName();
	String modName = m_activeModule->GetEditorName();
	const String moduleName = emitterName+TXT(":")+modName;

	m_editor->EditCurve(curves[id], moduleName);
}

void CEdEmitterGraphEditor::OnLayoutReset( wxCommandEvent& event )
{
	UpdateLayout();

	wxPoint position(10, 10);
	const TDynArray< CParticleEmitter* >& emitters = m_particleSystem->GetEmitters();
	for (Uint32 i = 0; i < emitters.Size(); ++i)
	{
		emitters[i]->SetPosition(position.x, position.y);

		EmitterLayoutInfo *layout = m_layout.FindPtr(emitters[i]);
		ASSERT(layout);
		position.x += layout->m_size.x + 5;
	}

	ScaleToClientView();
}

void CEdEmitterGraphEditor::OnFitToWindow( wxCommandEvent& event )
{
	ScaleToClientView();
}

void CEdEmitterGraphEditor::OnCopyToClipboard( wxCommandEvent& event )
{
	ASSERT( m_particleSystem );
	ParticleEmittersArrayWrapper* wrapper = (ParticleEmittersArrayWrapper *) event.m_callbackUserData;
	ASSERT( wrapper );

	m_emittersInClipboard.Clear();

	for (Int32 i = 0; i < (Int32) wrapper->m_emitters.Size(); ++i)
	{
		CParticleEmitter* emitter = wrapper->m_emitters[i];
		ASSERT( emitter );

		// Ask for the emitter name
		if ( emitter )
		{
			m_emittersInClipboard.PushBack( THandle< CParticleEmitter >( emitter ) );
		}
	}
}

void CEdEmitterGraphEditor::OnPasteFromClipboard( wxCommandEvent& event )
{
	Int32 xOffset = 0;
	// Iterate emitters in clipboard
	for (Int32 i = 0; i < (Int32) m_emittersInClipboard.Size(); ++i)
	{
		if ( m_emittersInClipboard[i].Get() )
		{
			CParticleEmitter* emitter = m_emittersInClipboard[i].Get();
			ASSERT( emitter );
			if ( emitter )
			{
				// Perform clone
				CParticleEmitter* clonedEmitter = m_particleSystem->CloneEmitter( emitter, emitter->GetEditorName() );

				if ( clonedEmitter )
				{
					// Update layout
					UpdateEmitterLayout( clonedEmitter );
					EmitterLayoutInfo *layout = m_layout.FindPtr( clonedEmitter );
					wxPoint pos = ClientToCanvas( m_lastClickPoint );
					xOffset += layout->m_size.x / 2;
					clonedEmitter->SetPosition( pos.x + xOffset, pos.y );
					
					// Notify editor
					m_editor->OnEmitterAdded( clonedEmitter );
				}
			}
		}
	}

	// Repaint canvas
	Repaint();
}

void CEdEmitterGraphEditor::RemoveModule( IParticleModule* module )
{
	ASSERT( module );
	ASSERT( IsModificator( module ) );

	// Get parent emitter
	CParticleEmitter *emitter = GetEmitter( module );

	// Let editor prepare for module removal
	m_editor->OnModuleRemove( module );

	// Remove from parent emitter
	emitter->RemoveModule( module );

	// Deselect any
	SelectModificator(NULL);

	// Update layout of parent emitter
	UpdateEmitterLayout( emitter );

	// Notify editor
	m_editor->OnEmitterChanged( emitter );

	// Repaint canvas
	Repaint();
}

Bool CEdEmitterGraphEditor::IsEmitter(IParticleModule *module) const 
{ 
	return module && module->IsA< CParticleEmitter >();
}

Bool CEdEmitterGraphEditor::IsModificator(IParticleModule *module) const 
{ 
	return module && !IsEmitter(module) && IsEmitter(module->GetEmitter()); 
}

Bool CEdEmitterGraphEditor::IsEmitterSelected(CParticleEmitter *emitter) const 
{ 
	return m_selected.Find(emitter) != m_selected.End(); 
}

IParticleModule * CEdEmitterGraphEditor::GetModificator(IParticleModule *module) const 
{ 
	return Cast< CParticleEmitter >( module );
}
