/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "curvePropertyEditor.h"
#include "../../common/engine/curve.h"
#include "../../common/game/storySceneEvent.h"
#include "../../common/game/storySceneEventAnimClip.h"
#include "../../common/game/storySceneLine.h"
#include "voice.h"
#include "../../common/engine/localizationManager.h"

//RED_DEFINE_STATIC_NAME( Damp_curve )

BEGIN_EVENT_TABLE( CEdCurvePropertyEditorDialog, wxDialog )
	EVT_BUTTON( XRCID("buttOk"), CEdCurvePropertyEditorDialog::OnOk )
END_EVENT_TABLE()


CEdCurvePropertyEditorDialog::CEdCurvePropertyEditorDialog( wxWindow* parent, TDynArray< SCurveData* >& curves )
	: m_curveEditor(NULL)
{
	{
		// Load designed frame from resource
		wxXmlResource::Get()->LoadDialog( this, NULL, TXT("PropertyCurveDialog") );

		// Create Curve Editor and place it in CurvePanel
		{
			wxPanel* rp = XRCCTRL( *this, "CurvePanel", wxPanel );
			wxBoxSizer* sizer1 = new wxBoxSizer( wxVERTICAL );		
			m_curveEditor = new CEdCurveEditor( rp );
			sizer1->Add( m_curveEditor, 1, wxEXPAND, 0 );
			rp->SetSizer( sizer1 );		
			rp->Layout();
		}

		//SetSizer(mainSizer);
		Layout();

		m_curve = NULL;

		Float min , max;
		if ( curves.Size() > 0 )
		{
			if ( curves[0]->Size() )
			{
				min = curves[0]->GetMinTime();
				max = curves[0]->GetMaxTime();
			}
			m_curveEditor->AddCurve( curves[0], TXT("X"), true, Vector( 0, 1.0f, 0 ), NULL, Color( 255, 0, 0 ) );
		}
		if ( curves.Size() > 1 )
		{
			m_curveEditor->AddCurve( curves[1], TXT("Y"), true, Vector( 0, 1.0f, 0 ), NULL, Color( 0, 255, 0 ) );
		}
		if ( curves.Size() > 2 )
		{
			m_curveEditor->AddCurve( curves[2], TXT("Z"), true, Vector( 0, 1.0f, 0 ),  NULL, Color( 0, 0, 255 ) );
		}
		if ( curves.Size() > 3 )
		{
			m_curveEditor->AddCurve( curves[3], TXT("W") );
		}
		for ( Uint32 i = 4; i < curves.Size(); ++i )
		{
			m_curveEditor->AddCurve( curves[i], TXT("Curve") );
		}

		for ( Uint32 i = 1; i < curves.Size(); ++i )
		{
			if ( curves[i]->Size() )
			{
				min = Min( min, curves[i]->GetMinTime() );
				max = Max( max, curves[i]->GetMaxTime() );
			}
		}

		SetActiveRegion( min, max );

		Show();
	}
}

CEdCurvePropertyEditorDialog::CEdCurvePropertyEditorDialog( wxWindow* parent, CCurve* curve )
	: m_curveEditor(NULL)
{
	// Load designed frame from resource
	wxXmlResource::Get()->LoadDialog( this, NULL, TXT("PropertyCurveDialog") );

	// Create Curve Editor and place it in CurvePanel
	{
		wxPanel* rp = XRCCTRL( *this, "CurvePanel", wxPanel );
		wxBoxSizer* sizer1 = new wxBoxSizer( wxVERTICAL );		
		m_curveEditor = new CEdCurveEditor( rp );
		sizer1->Add( m_curveEditor, 1, wxEXPAND, 0 );
		rp->SetSizer( sizer1 );		
		rp->Layout();
	}

	//SetSizer(mainSizer);
	Layout();

	m_curve = curve;

	m_curveEditor->AddCurve( curve, TXT("Damp curve"));

	Float min, max;
	m_curve->GetTimesMinMax( min, max );

	SetActiveRegion( min, max );

	Show();
}

CEdCurvePropertyEditorDialog::~CEdCurvePropertyEditorDialog()
{
}

void CEdCurvePropertyEditorDialog::SetActiveRegion( Float start, Float end )
{
	m_curveEditor->SetActiveRegion( start, end );
}

void CEdCurvePropertyEditorDialog::OnOk(wxCommandEvent& event )
{
	EndDialog(1);
}

//////////////////////////////////////////////////////////////////////////

CCurvePropertyEditor::CCurvePropertyEditor( CPropertyItem* propertyItem )
	: ICustomPropertyEditor( propertyItem )
{	
	m_icon = SEdResources::GetInstance().LoadBitmap( TXT("IMG_PB_CURVE") );
}

void CCurvePropertyEditor::CreateControls( const wxRect &propRect, TDynArray< wxControl* >& outSpawnedControls )
{
	// Add editor button
	m_propertyItem->AddButton( m_icon, wxCommandEventHandler( CCurvePropertyEditor::OnEditorDialog ), this );
}

void CCurvePropertyEditor::CloseControls()
{
}

Bool CCurvePropertyEditor::GrabValue( String& displayData )
{
	return false;
}

Bool CCurvePropertyEditor::SaveValue()
{
	return true;
}

void CCurvePropertyEditor::OnEditorDialog( wxCommandEvent &event )
{
	OnOpenCurveEditor();
}

void CCurvePropertyEditor::OnOpenCurveEditor()
{
	if ( CCurve* curve = m_propertyItem->GetParentObject( 0 ).As< CCurve >() )
	{
		CEdCurvePropertyEditorDialog dlg(m_propertyItem->GetPage(), curve);
		dlg.ShowModal();
	}
}

//////////////////////////////////////////////////////////////////////////

CBaseCurveDataPropertyEditor::CBaseCurveDataPropertyEditor(  CPropertyItem* propertyItem  )
	: CCurvePropertyEditor( propertyItem )
{

}

ICurveDataOwner* CBaseCurveDataPropertyEditor::GetOwner()
{
	ICurveDataOwner* owner = nullptr;
	
	if ( !owner )
	{ 
		owner = dynamic_cast< ICurveDataOwner* >( m_propertyItem->GetRootObject( 0 ).As< CObject >() );
	}
	if ( !owner )
	{ 
		owner = dynamic_cast< ICurveDataOwner* >( m_propertyItem->GetRootObject( 0 ).As< CExtAnimEvent >() );
	}
	if ( !owner )
	{ 
		owner = dynamic_cast< ICurveDataOwner* >( m_propertyItem->GetRootObject( 0 ).As< CStorySceneEvent >() );
	}

	// if ( !owner )
	// Write you stuff here. We can not use CObject always...

	return owner;
}

void CBaseCurveDataPropertyEditor::OnOpenCurveEditor()
{
	ICurveDataOwner* owner = GetOwner();
	if ( owner )
	{
		if ( SCurveData* curve = owner->GetCurveData() )
		{
			TDynArray< SCurveData* > curves;
			curves.PushBack( curve );

			CEdCurvePropertyEditorDialog dlg( m_propertyItem->GetPage(), curves );
			dlg.ShowModal();
		}
		else if ( TDynArray< SCurveData* >* curves = owner->GetCurvesData() )
		{
			CEdCurvePropertyEditorDialog dlg( m_propertyItem->GetPage(), *curves );
			dlg.ShowModal();
		}

		owner->OnCurveChanged();
	}
}

//////////////////////////////////////////////////////////////////////////

CVoiceCurveDataPropertyEditor::CVoiceCurveDataPropertyEditor(  CPropertyItem* propertyItem  )
	: CCurvePropertyEditor( propertyItem )
{
	m_iconImport = SEdResources::GetInstance().LoadBitmap( TXT("IMG_PB_ADD") );
}

void CVoiceCurveDataPropertyEditor::CreateControls( const wxRect &propRect, TDynArray< wxControl* >& outSpawnedControls )
{
	CCurvePropertyEditor::CreateControls( propRect, outSpawnedControls );

	m_propertyItem->AddButton( m_iconImport, wxCommandEventHandler( CVoiceCurveDataPropertyEditor::OnImportCurveData ), this );
}

void CVoiceCurveDataPropertyEditor::OnOpenCurveEditor()
{
	CStorySceneEventAnimClip* evt = m_propertyItem->GetRootObject( 0 ).As< CStorySceneEventAnimClip >();

	SCurveData& curve = evt->GetVoiceWeightCurve().m_curve;
	TDynArray< SCurveData* > curves;
	curves.PushBack( &curve );

	CEdCurvePropertyEditorDialog dlg( m_propertyItem->GetPage(), curves );
	dlg.ShowModal();
}

#include "dialogEditor.h"

void CVoiceCurveDataPropertyEditor::OnImportCurveData( wxCommandEvent &event )
{
	CStorySceneEventAnimClip* evt = m_propertyItem->GetRootObject( 0 ).As< CStorySceneEventAnimClip >();

	SVoiceWeightCurve& vcurve = evt->GetVoiceWeightCurve();
	SCurveData& curve = vcurve.m_curve;
	curve.Clear();

	if ( const CStorySceneLine* line = Cast< const CStorySceneLine >( evt->GetSceneElement() ) )
	{
		const String& currentLocale = SLocalizationManager::GetInstance().GetCurrentLocale();

		const String pathWav = SEdLipsyncCreator::GetInstance().GetWavPath( line->GetVoiceFileName(), currentLocale.ToLower() );

		const Float evtDuration = evt->GetDurationProperty(); // TODO: confirm this is ok - we're in "edit" mode, section is approved
															  // so duration property == instance property
		const Float lineDuration = line->CalculateDuration( currentLocale );

		if ( evtDuration > 0.f && lineDuration > 0.f )
		{
			TDynArray< Float > data;
			if ( SEdLipsyncCreator::GetInstance().CreateWavAna( pathWav, data ) && data.Size() > 2 )
			{
				TDynArray< Float > dataF;
				dataF.Resize( data.Size() );

				static Int32 windowSize = 5;
				for ( Uint32 i=0; i<data.Size(); ++i )
				{
					Int32 count = 0;
					Float accValue = 0.f;
					
					Int32 minIt = Max< Int32 >( i - windowSize, 0 );
					Int32 maxIt = Min< Int32 >( i + windowSize, data.SizeInt() );

					for ( Int32 it=minIt; it<maxIt; ++it )
					{
						accValue += data[ it ];
						count += 1;
					}

					const Float avgValue = accValue / (Float)count;

					dataF[ i ] = avgValue;
				}

				const Float timeStep = lineDuration / (Float)(data.SizeInt()-1);
				for ( Uint32 i=0; i<data.Size(); ++i )
				{
					curve.AddPoint( (Float)i*timeStep, dataF[ i ] );
				}

				const Float s = lineDuration / evtDuration;

				evt->SetStartPosition( 0.f );
				evt->SetAnimationClipStart( 0.f );
				evt->SetAnimationClipEnd( evtDuration );
				evt->SetAnimationStretch( s );
			}
		}
	}
}
