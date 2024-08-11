#include "build.h"
#include "dialogEditor_hacks.h"
#include "dialogEditor.h"
#include "dialogPreview.h"
#include "../../common/core/engineTransform.h"
#include "../../common/game/storySceneCameraDefinition.h"
#include "../../common/game/storySceneEventCustomCamera.h"
#include "../../common/engine/environmentAreaParams.h"
#include "../../common/engine/environmentManager.h"
#include "../../common/game/storySceneUtils.h"
#include "../../common/game/storySceneEventCustomCameraInstance.h"
#include "../../common/game/storySceneEventCameraBlend.h"

#ifdef DEBUG_SCENES_2
#pragma optimize("",off)
#endif

void CEdSceneEditor_Hacks::OnPreview_ViewportTick_ApplyDof( CEdSceneEditor* editor )
{
	if( editor->m_camera.IsPreviewMode() )
	{
		return;
	}
	StorySceneCameraDefinition* camDef = editor->m_camera.GetSelectedDefinition();
	CWorld* previewWorld = editor->GetWorld();
	if ( camDef && previewWorld )
	{
		CAreaEnvironmentParams areaEnvironmentParams = previewWorld->GetEnvironmentManager()->GetCurrentAreaEnvironmentParams();
		const Float importTime = previewWorld->GetEnvironmentManager()->GetCurrentGameTime().ToFloat() / (24.f * 60.f * 60.f);
		CEnvDepthOfFieldParameters dofParams;
		dofParams.m_activated = true;

		//because of camera.cpp line 267
		Float  dofBlurDistNear  = camDef->m_dofFocusDistNear - Max( 0.f, camDef->m_dofBlurDistNear );
		Float  dofFocusDistFar  = camDef->m_dofFocusDistNear  + Max( 0.f, camDef->m_dofFocusDistFar );
		Float  dofBlurDistFar   = camDef->m_dofFocusDistFar  + Max( 0.f, camDef->m_dofBlurDistFar );


		dofParams.m_nearBlurDist.Clear().AddPoint( 0.0f, SSimpleCurvePoint::BuildValueScalar( dofBlurDistNear ) );
		dofParams.m_nearFocusDist.Clear().AddPoint( 0.0f, SSimpleCurvePoint::BuildValueScalar( camDef->m_dofFocusDistNear ) );
		dofParams.m_farFocusDist.Clear().AddPoint( 0.0f, SSimpleCurvePoint::BuildValueScalar( dofFocusDistFar ) );
		dofParams.m_farBlurDist.Clear().AddPoint( 0.0f, SSimpleCurvePoint::BuildValueScalar( dofBlurDistFar ) );
		dofParams.m_intensity.Clear().AddPoint( 0.0f, SSimpleCurvePoint::BuildValueScalar( camDef->m_dofIntensity ) );
		areaEnvironmentParams.m_depthOfField.ImportDayPointValue( areaEnvironmentParams.m_depthOfField, dofParams, 1.f, importTime );
		previewWorld->GetEnvironmentManager()->EnableForcedAreaEnvironment( areaEnvironmentParams );
		previewWorld->GetEnvironmentManager()->DisableForceAreaEnvironment();

		CGameEnvironmentParams& params = previewWorld->GetEnvironmentManager()->GetGameEnvironmentParams();
		params.m_displaySettings.m_forceCutsceneDofMode = true;
	}
}

namespace
{
	EngineTransform ExtractAndApplyScale( const EngineTransform& _inputTransSS, const EngineTransform& scenePlacement, const Matrix& scenePlacementMatWSInv )
	{
		EngineTransform inputTransSS( _inputTransSS );
		inputTransSS.RemoveScale();

		const Matrix matWS = StorySceneUtils::CalcWSFromSS( inputTransSS, scenePlacement );

		Matrix mat2;
		Vector scale;
		matWS.ExtractScale( mat2, scale );
		mat2.SetTranslation( matWS.GetTranslation() );

		Matrix newMatSS;
		static Bool TEST = true;
		if ( TEST )
		{
			newMatSS = Matrix::Mul( scenePlacementMatWSInv, mat2 );
		}
		else
		{
			newMatSS = Matrix::Mul( matWS, scenePlacementMatWSInv );
		}

		EngineTransform outTransSS;
		outTransSS.Init( newMatSS );
		outTransSS.RemoveScale();
		return outTransSS;
	}
}

#ifdef DEBUG_SCENES_2
#pragma optimize("",on)
#endif
