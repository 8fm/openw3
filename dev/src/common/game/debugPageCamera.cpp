/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "../core/depot.h"
#include "../core/debugPageHandler.h"
#include "../core/debugPageHTMLDoc.h"
#include "../core/httpResponseData.h"
#include "../core/objectMap.h"
#include "../engine/cameraDirector.h"
#include "../engine/freeCamera.h"
#include "../engine/camera.h"

#ifndef NO_DEBUG_PAGES

#include "commonGame.h"
//-----

/// camera debug page
class CDebugPageCamera : public IDebugPageHandlerHTML
{
public:
	CDebugPageCamera()
		: IDebugPageHandlerHTML( "/camera/" )
	{}

private:
	// IDebugPageHandler interface
	virtual StringAnsi GetTitle() const override { return "Camera"; }
	virtual StringAnsi GetCategory() const override { return "Game"; }
	virtual Bool HasIndexPage() const override { return GGame && GGame->GetActiveWorld(); }

	//! Async command
	virtual CHTTPResponseData* OnCommand( const class CBasicURL& fullURL, const class CBasicURL& relativeUrl, const StringAnsi& content ) override
	{
		// parse the params
		const CBasicURL params = CBasicURL::ParseParams( CBasicURL::Decode( content ) );

		// basic shit
		Int32 testValue=-1;
		StringAnsi testText;
		if ( params.GetKey("__goto", testText) )
		{
			LOG_CORE( TXT("HTTP Camera Teleportation request: '%hs'"), testText.AsChar() );

			Vector cameraPos;
			EulerAngles cameraRot;
			if ( ParseCameraViewString( ANSI_TO_UNICODE( testText.AsChar() ), cameraPos, cameraRot ) )
			{
				GGame->EnableFreeCamera( true ); 
				GGame->SetFreeCameraWorldPosition( cameraPos );
				GGame->SetFreeCameraWorldRotation( cameraRot );
				return new CHTTPResponseData( CHTTPResponseData::eResultCode_OK );
			}
			else
			{
				return new CHTTPResponseData( CHTTPResponseData::eResultCode_BadRequest );
			}
		}
		else if ( params.GetKey( "__focus", testValue ) )
		{
			// get node by index
			CObjectsMap::ObjectIndexer indexer( GObjectsMap );
			CNode* node = Cast< CNode >( indexer.GetObject( testValue ) );
			if ( !node )
				return new CHTTPResponseData( CHTTPResponseData::eResultCode_BadRequest );

			// calculate best view position
			Vector cameraCenter;
			Float cameraDist = 1.0f;
			const EulerAngles cameraAngles( 0.0f, -45.0f, 45.0f );
			if ( node->IsA< CEntity >() )
			{
				const CEntity* entity = static_cast< const CEntity* >( node );
				const Box box = entity->CalcBoundingBox();

				cameraCenter = box.CalcCenter();
				cameraDist = box.CalcExtents().Mag3() * 1.4f;
			}

			// setup view position in free camera to see the selected entity
			const Vector cameraPos = cameraCenter - (cameraAngles.TransformVector( Vector::EY ) * cameraDist);

			// setup camera
			GGame->EnableFreeCamera( true ); 
			GGame->SetFreeCameraWorldPosition( cameraPos );
			GGame->SetFreeCameraWorldRotation( cameraAngles );

			// serviced
			return new CHTTPResponseData( CHTTPResponseData::eResultCode_OK );
		}

		// not handled
		return nullptr;
	}

	//! Process command, commands are always passed via URLs, return NULL if not handled
	virtual bool OnFillPage( const class CBasicURL& fullURL, const class CBasicURL& relativeURL, class CDebugPageHTMLDocument& doc ) override
	{
		// no layers
		if ( !GGame || !GGame->GetActiveWorld() )
		{
			doc << "<span class=\"error\">No active world</span>";
			return true;
		}

		// reference camera position
		String cameraString;
		{
			CDebugPageHTMLInfoBlock info( doc, "Current camera" );

			if ( GGame->IsFreeCameraEnabled() )
			{
				const auto pos = GGame->GetFreeCamera().GetPosition();
				const auto rot = GGame->GetFreeCamera().GetRotation();
				cameraString = TXT("[[") + ToString( pos ) + TXT("|") + ToString( rot ) + TXT("]]");
			}
			else
			{
				const auto pos = GGame->GetActiveWorld()->GetCameraDirector()->GetCameraPosition();
				const auto rot = GGame->GetActiveWorld()->GetCameraDirector()->GetCameraRotation();
				cameraString = TXT("[[") + ToString( pos ) + TXT("|") + ToString( rot ) + TXT("]]");
			}

			info.Info( "Camera reference string: ").Write( UNICODE_TO_ANSI( cameraString.AsChar() ) );
		}

		// free camera
		if ( GGame->IsFreeCameraEnabled() )
		{
			CDebugPageHTMLInfoBlock info( doc, "Free camera" );

			const auto pos = GGame->GetFreeCamera().GetPosition();
			const auto rot = GGame->GetFreeCamera().GetRotation();
			info.Info( "Position: " ).Writef( "[%1.3f, %1.3f, %1.3f]", pos.X, pos.Y, pos.Z );
			info.Info( "Rotation: " ).Writef( "(Pitch=%1.3f, Yaw=%1.3f, Roll=%1.3f)", rot.Pitch, rot.Yaw, rot.Roll );
			info.Info( "FOV: " ).Writef( "%1.1f", GGame->GetFreeCamera().GetFOV() );
		}

		// general state
		{
			CDebugPageHTMLInfoBlock info( doc, "Normal camera" );

			const auto pos = GGame->GetActiveWorld()->GetCameraDirector()->GetCameraPosition();
			const auto rot = GGame->GetActiveWorld()->GetCameraDirector()->GetCameraRotation();
			info.Info( "Position: " ).Writef( "[%1.3f, %1.3f, %1.3f]", pos.X, pos.Y, pos.Z );
			info.Info( "Rotation: " ).Writef( "(Pitch=%1.3f, Yaw=%1.3f, Roll=%1.3f)", rot.Pitch, rot.Yaw, rot.Roll );
		}

		// teleport window
		{
			// teleport window
			{
				CDebugPageHTMLInfoBlock info( doc, "Teleport camera" );
				CDebugPageHTMLFormHelperAJAX form( doc, "camera" );

				doc.Write("Enter camera position to teleport to:<br>");

				form.InputBox( "__goto", "", UNICODE_TO_ANSI( cameraString.AsChar() ), 50 );
				form.SubmitButton( "Go!" );
			}
		}

		// stack
		{
			// get proxies
			TDynArray< const CCameraProxy* > proxies;
			GGame->GetActiveWorld()->GetCameraDirector()->GetCameraProxiesForDebugOnly( proxies );

			// camera state
			if ( !proxies.Empty() )
			{
				for ( Uint32 index=0; index<proxies.Size(); ++index )
				{
					CDebugPageHTMLInfoBlock info( doc, "Camera stack: Camera %d", index );

					auto& cameraProxy = const_cast< CCameraProxy& >( *proxies[ index ] );

					// general stuff
					info.Info( "Weight: " ).Writef( "%1.3f", cameraProxy.GetWeight() );
					info.Info( "Valid: " ).Write( cameraProxy.IsValid() ? "Yes" : "No" );
					info.Info( "Abandoned: " ).Writef( cameraProxy.IsAbandoned() ? "Yes" : "No" );

					// proxy owner
					if ( cameraProxy.GetParent() && cameraProxy.GetParent()->IsA< CObject >() )
					{
						info.Info( "Parent: " ).LinkObject( (const CObject*) cameraProxy.GetParent().Get() );
					}

					//  camera data
					if ( cameraProxy.GetCamera() )
					{
						info.Info( "Camera object: " ).Writef( "0x%016llX", (Uint64) cameraProxy.GetCamera() );

						ICamera::Data cameraData;
						if ( cameraProxy.GetCamera()->GetData( cameraData ) )
						{
							info.Info( "Position: " ).Writef( "[%1.3f, %1.3f, %1.3f]", cameraData.m_position.X, cameraData.m_position.Y, cameraData.m_position.Z );
							info.Info( "Rotation: " ).Writef( "(Pitch=%1.3f, Yaw=%1.3f, Roll=%1.3f)", cameraData.m_rotation.Pitch, cameraData.m_rotation.Yaw, cameraData.m_rotation.Roll );
							info.Info( "HasFocus: " ).Write( cameraData.m_hasFocus ? "Yes" : "No" );
							info.Info( "Focus: " ).Writef( "[%1.3f, %1.3f, %1.3f]", cameraData.m_focus.X, cameraData.m_focus.Y, cameraData.m_focus.Z );
							info.Info( "FOV: " ).Writef( "%1.3f", cameraData.m_fov );
							info.Info( "NearPlane: " ).Writef( "%1.3f", cameraData.m_nearPlane );
							info.Info( "FarPlane: " ).Writef( "%1.3f", cameraData.m_farPlane );
							info.Info( "ForceNearPlane: " ).Writef( "%1.3f", cameraData.m_forceNearPlane );
							info.Info( "ForceFarPlane: " ).Writef( "%1.3f", cameraData.m_forceFarPlane );
						}
					}
				}
			}
		}

		return true;
	}
};

//-----

void InitCameraDebugPages()
{
	new CDebugPageCamera();
}

//-----


#endif