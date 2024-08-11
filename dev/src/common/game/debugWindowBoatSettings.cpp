#include "build.h"

#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS

#include "debugWindowBoatSettings.h"

#ifndef FINAL

#include "..\engine\redGuiPanel.h"
#include "..\engine\redGuiAdvancedSlider.h"
#include "..\engine\redGuiLabel.h"
#include "..\engine\redGuiTab.h"
#include "..\engine\redGuiControl.h"
#include "..\engine\redGuiScrollPanel.h"
#include "boatComponent.h"
#include "..\engine\redGuiGraphicContext.h"
#include "..\engine\redGuiManager.h"
#include "..\engine\redGuiCheckBox.h"
#include "boatBuoyancySystem.h"
#include "..\core\configVar.h"
#include "..\engine\redGuiButton.h"
#include "..\core\configVarSystem.h"

//////////////////////////////////////////////////////////////////////////

namespace DebugWindows
{

    //////////////////////////////////////////////////////////////////////////

    CDebugWindowBoatSettings::CDebugWindowBoatSettings(void)
        : RedGui::CRedGuiWindow( 40,40, 610, 600 )
    {
        SetCaption(TXT("Boat settings"));

        // Create panels
        RedGui::CRedGuiPanel * topPanel = new RedGui::CRedGuiPanel( 2, 2, 600, 510 );
        topPanel->SetAutoSize( false );
        AddChild( topPanel );

        RedGui::CRedGuiPanel * bottomPanel = new RedGui::CRedGuiPanel( 2, 512, 600, 40 );
        topPanel->SetAutoSize( false );
        AddChild( bottomPanel );

        // create tabs
        m_tabs = new RedGui::CRedGuiTab( 0, 0, 100, 100 );
        m_tabs->SetDock( RedGui::DOCK_Fill );
        m_tabs->SetMargin( Box2( 3, 3, 3, 3 ) );
        m_tabs->EventTabChanged.Bind( this, &CDebugWindowBoatSettings::NotifyOnTabChanged );
        topPanel->AddChild( m_tabs );
        
        // Add save buttons to lower panel
        m_saveButton = new RedGui::CRedGuiButton(0,0, 135, 20);
        m_saveButton->SetMargin(Box2(2, 2, 2, 0));
        m_saveButton->SetText(TXT("Save settings"));
        m_saveButton->EventButtonClicked.Bind(this, &CDebugWindowBoatSettings::NotifyOnButtonClicked);
        bottomPanel->AddChild(m_saveButton);
        m_saveButton->SetDock( RedGui::DOCK_Left );

        m_loadButton = new RedGui::CRedGuiButton(0,0, 135, 20);
        m_loadButton->SetMargin(Box2(2, 2, 2, 0));
        m_loadButton->SetText(TXT("Load settings"));
        m_loadButton->EventButtonClicked.Bind(this, &CDebugWindowBoatSettings::NotifyOnButtonClicked);
        bottomPanel->AddChild(m_loadButton);
        m_loadButton->SetDock( RedGui::DOCK_Left );

        //////////////////////////////////////////////////////////////////////////
        // General tab
        String name = TXT("Gears");
        m_tabs->AddTab( name );
        
        RedGui::CRedGuiControl* steeringTab = m_tabs->GetTabByName( name );
        if( steeringTab != nullptr )
        {
            CreateCheckBoxBelow(steeringTab, TXT("Sailing enabled"), &BoatConfig::cvBoatSailingEnabled );

            CreateSliderBelow( steeringTab, 0.1f, 15.0f, 0.1f,    TXT("Sailing Gear One MaxSpeed"),                    TXT("Float"), &BoatConfig::cvBoatSailingGearOneMaxSpeed         );
            CreateSliderBelow( steeringTab, 0.1f, 40.0f, 0.1f,    TXT("Sailing Gear One Acceleration"),                TXT("Float"), &BoatConfig::cvBoatSailingGearOneAcceleration     );
            CreateSliderBelow( steeringTab, 0.1f, 40.0f, 0.1f,    TXT("Sailing Gear One Deceleration"),                TXT("Float"), &BoatConfig::cvBoatSailingGearOneDeceleration     );
            
            CreateSliderBelow( steeringTab, 0.1f, 15.0f, 0.1f,    TXT("Sailing Gear Two MaxSpeed"),                    TXT("Float"), &BoatConfig::cvBoatSailingGearTwoMaxSpeed         );
            CreateSliderBelow( steeringTab, 0.1f, 40.0f, 0.1f,    TXT("Sailing Gear Two Acceleration"),                TXT("Float"), &BoatConfig::cvBoatSailingGearTwoAcceleration     );
            CreateSliderBelow( steeringTab, 0.1f, 40.0f, 0.1f,    TXT("Sailing Gear Two Deceleration"),                TXT("Float"), &BoatConfig::cvBoatSailingGearTwoDeceleration     );
            
            CreateSliderBelow( steeringTab, 0.1f, 15.0f, 0.1f,    TXT("Sailing Gear Three MaxSpeed"),                  TXT("Float"), &BoatConfig::cvBoatSailingGearThreeMaxSpeed       );
            CreateSliderBelow( steeringTab, 0.1f, 40.0f, 0.1f,    TXT("Sailing Gear Three Acceleration"),              TXT("Float"), &BoatConfig::cvBoatSailingGearThreeAcceleration   );
            CreateSliderBelow( steeringTab, 0.1f, 40.0f, 0.1f,    TXT("Sailing Gear Three Deceleration"),              TXT("Float"), &BoatConfig::cvBoatSailingGearThreeDeceleration   );
            
            CreateSliderBelow( steeringTab, 0.1f, 15.0f, 0.1f,    TXT("Sailing Gear Reverse MaxSpeed"),                TXT("Float"), &BoatConfig::cvBoatSailingGearReverseMaxSpeed     );
            CreateSliderBelow( steeringTab, 0.1f, 40.0f, 0.1f,    TXT("Sailing Gear Reverse Acceleration"),            TXT("Float"), &BoatConfig::cvBoatSailingGearReverseAcceleration );
            CreateSliderBelow( steeringTab, 0.1f, 40.0f, 0.1f,    TXT("Sailing Gear Reverse Deceleration"),            TXT("Float"), &BoatConfig::cvBoatSailingGearReverseDeceleration );
            
            CreateSliderBelow( steeringTab, 0.0f, 89.0f, 1.0f,    TXT("Wave climbing angle thresh 1"),                 TXT("Float"), &BoatConfig::cvBoatWaveClimbAngleThreshold1       );
            CreateSliderBelow( steeringTab, 0.0f, 89.0f, 1.0f,    TXT("Wave climbing angle thresh 2"),                 TXT("Float"), &BoatConfig::cvBoatWaveClimbAngleThreshold2       );
        }

        name = TXT("Turning");
        m_tabs->AddTab( name );
        {
            RedGui::CRedGuiControl* turningTab = m_tabs->GetTabByName( name );
            CreateSliderBelow( turningTab, 0.5f, 89.5f, 0.5f,    TXT("Turning zero speed steer angle"),               TXT("Float"), &BoatConfig::cvBoatTurningZeroSpeedSteerAngle     );
            CreateSliderBelow( turningTab, 0.5f, 89.5f, 0.5f,    TXT("Turning low speed steer angle"),                TXT("Float"), &BoatConfig::cvBoatTurningFristSpeedSteerAngle    );
            CreateSliderBelow( turningTab, 0.5f, 89.5f, 0.5f,    TXT("Turning max speed steer angle"),                TXT("Float"), &BoatConfig::cvBoatTurningSecondSpeedSteerAngle   );
            CreateSliderBelow( turningTab, 0.05f, 1.0f, 0.05f,   TXT("Turning propelling force ratio at zero angle"), TXT("Float"), &BoatConfig::cvBoatTurningZeroAngleForceCutoff    );
            CreateSliderBelow( turningTab, 0.1f, 2.0f, 0.1f,     TXT("Turning steer rotation speed"),                 TXT("Float"), &BoatConfig::cvBoatTurningSteerRotationTime       );
        }

        //////////////////////////////////////////////////////////////////////////
        // Buoyancy tab
        name = TXT("Buoyancy");
        m_tabs->AddTab( name );

        RedGui::CRedGuiControl* buoyancyTab = m_tabs->GetTabByName( name );
        if( buoyancyTab != nullptr )
        {
            CreateSliderBelow( buoyancyTab, 0.0f, 10.0f, 0.025f, TXT("Linear damping"),                 TXT("Float"), &BoatConfig::cvBoatLinearDamping                 );
            CreateSliderBelow( buoyancyTab, 0.0f, 10.0f, 0.025f, TXT("Angular damping"),                TXT("Float"), &BoatConfig::cvBoatAngularDamping                );

            CreateSliderBelow( buoyancyTab, 5.0f, 200.0f, 5.0f,  TXT("Sailing force scaller"),          TXT("Float"), &BoatConfig::cvBoatSailingForceScaller           );
            CreateSliderBelow( buoyancyTab, 0.0f, 100.0f, 1.0f,  TXT("Buoyancy torque scaller"),        TXT("Float"), &BoatConfig::cvBoatTorqueScaller                 );
            CreateSliderBelow( buoyancyTab, 0.0f, 600.0f, 10.0f, TXT("Buoyancy force scaller"),         TXT("Float"), &BoatConfig::cvBoatForceScaller                  );

            CreateSliderBelow( buoyancyTab, -1.0f, 1.0f, 0.1f,   TXT("Floating height"),                TXT("Float"), &BoatConfig::cvBoatFloatingHeight                );
            CreateSliderBelow( buoyancyTab, -4.0f, 4.0f, 0.1f,   TXT("Mass center Y position"),         TXT("Float"), &BoatConfig::cvBoatMassCenter                    );
            CreateSliderBelow( buoyancyTab, 1.0f, 100.0f, 1.0f,  TXT("Boat mass"),                      TXT("Float"), &BoatConfig::cvBoatMass                          );

            CreateSliderBelow( buoyancyTab, 0.0f, 3.0f, 1.0f,    TXT("Buoyancy probing precision"),     TXT("Float"), &BoatConfig::cvBoatBuoyancyWaterProbingPrecision );
            CreateSliderBelow( buoyancyTab, 0.0f, 1.5f, 0.025f,  TXT("Buoyancy fly threshold scaller"), TXT("Float"), &BoatConfig::cvBoatBuoyancyFlyThresholdScaller   );

            CreateSliderBelow( buoyancyTab, 0.01f, 5.0f, 0.01f,  TXT("Buoyancy points spread X"),       TXT("Float"), &BoatConfig::cvBoatBuoyancyPointsSpreadX         );
            CreateSliderBelow( buoyancyTab, 0.01f, 5.0f, 0.01f,  TXT("Buoyancy points spread Y"),       TXT("Float"), &BoatConfig::cvBoatBuoyancyPointsSpreadY         );

            CreateSliderBelow( buoyancyTab, 0.0f, 89.0f, 1.0f,   TXT("X tilt limit"),                   TXT("Float"), &BoatConfig::cvBoatTiltXLimit                    );
            CreateSliderBelow( buoyancyTab, 0.0f, 89.0f, 1.0f,   TXT("Y tilt limit"),                   TXT("Float"), &BoatConfig::cvBoatTiltYLimit                    );

            CreateSliderBelow( buoyancyTab, 0.0f, 5.0f, 0.05f,   TXT("Turning tilt Y speed"),           TXT("Float"), &BoatConfig::cvBoatTiltingYSpeed                 );
            CreateSliderBelow( buoyancyTab, 0.0f, 1.0f, 0.05f,   TXT("Turning tilt Y min %"),           TXT("Float"), &BoatConfig::cvBoatTiltingMinYTiltPercent        );
            CreateSliderBelow( buoyancyTab, 0.0f, 1.0f, 0.05f,   TXT("Turning tilt Y max %"),           TXT("Float"), &BoatConfig::cvBoatTiltingMaxYTiltPercent        );

            CreateSliderBelow( buoyancyTab, 0.1f, 5.0f,  0.1f,   TXT("Boat Z diff limit"),              TXT("Float"), &BoatConfig::cvBoatBuoyancyZDiffLimit            );
            CreateSliderBelow( buoyancyTab, 0.1f, 10.0f, 0.1f,   TXT("Boat Force UpBase"),              TXT("Float"), &BoatConfig::cvBoatForceUpBase                   );
            CreateSliderBelow( buoyancyTab, 0.1f, 10.0f, 0.1f,   TXT("Boat Force UpExpo"),              TXT("Float"), &BoatConfig::cvBoatForceUpExpo                   );
            CreateSliderBelow( buoyancyTab, 0.1f, 10.0f, 0.1f,   TXT("Boat Force DownBase"),            TXT("Float"), &BoatConfig::cvBoatForceDownBase                 );
            CreateSliderBelow( buoyancyTab, 0.1f, 10.0f, 0.1f,   TXT("Boat Force DownExpo"),            TXT("Float"), &BoatConfig::cvBoatForceDownExpo                 );
            CreateSliderBelow( buoyancyTab, 0.1f, 10.0f, 0.1f,   TXT("Boat Torque UpBase"),             TXT("Float"), &BoatConfig::cvBoatTorqueUpBase                  );
            CreateSliderBelow( buoyancyTab, 0.1f, 10.0f, 0.1f,   TXT("Boat Torque UpExpo"),             TXT("Float"), &BoatConfig::cvBoatTorqueUpExpo                  );
            CreateSliderBelow( buoyancyTab, 0.1f, 10.0f, 0.1f,   TXT("Boat Torque DownNormBase"),       TXT("Float"), &BoatConfig::cvBoatTorqueDownNormBase            );
            CreateSliderBelow( buoyancyTab, 0.1f, 10.0f, 0.1f,   TXT("Boat Torque DownNormExpo"),       TXT("Float"), &BoatConfig::cvBoatTorqueDownNormExpo            );
            CreateSliderBelow( buoyancyTab, 0.1f, 10.0f, 0.1f,   TXT("Boat Torque DownInAirBase"),      TXT("Float"), &BoatConfig::cvBoatTorqueDownInAirBase           );
            CreateSliderBelow( buoyancyTab, 0.1f, 10.0f, 0.1f,   TXT("Boat Torque DownInAirExpo"),      TXT("Float"), &BoatConfig::cvBoatTorqueDownInAirExpo           );
        }

        //////////////////////////////////////////////////////////////////////////
        // Hedgehog stuff
        name = TXT("Hedgehog");
        m_tabs->AddTab(name);
        RedGui::CRedGuiControl* hedgehogTab = m_tabs->GetTabByName(name);
        if( hedgehogTab != nullptr )
        {
            CreateSliderBelow( hedgehogTab, 0.1f,  5.0f,  0.1f,         TXT("Raycast Max Length"),            TXT("Float"), &BoatConfig::cvBoatHedgeRaycastMaxLength        );
            CreateSliderBelow( hedgehogTab, 0.1f,  5.0f,  0.1f,         TXT("Raycast Min Length"),            TXT("Float"), &BoatConfig::cvBoatHedgeRaycastMinLength        );
            CreateSliderBelow( hedgehogTab, 0.1f,  2.0f,  0.1f,         TXT("Raycast cutoff length"),         TXT("Float"), &BoatConfig::cvBoatHedgeRaycastCutoffLength     );

            CreateSliderBelow( hedgehogTab, 0.5f,  50.0f,  0.5f,        TXT("RepelForceMulti"),               TXT("Float"), &BoatConfig::cvBoatHedgeRepelForceMulti         );
            CreateSliderBelow( hedgehogTab, 100.0f,  10000.0f,  100.0f, TXT("RepelTorqueMulti"),              TXT("Float"), &BoatConfig::cvBoatHedgeRepelTorqueMulti        );
            CreateSliderBelow( hedgehogTab, 0.01f, 2.0f,  0.01f,        TXT("Input DamperSpeed"),             TXT("Float"), &BoatConfig::cvBoatHedgeInputDamperSpeed        );

            CreateSliderBelow( hedgehogTab, 0.01f, 1.0f,  0.01f,        TXT("Repel and normal damper speed"), TXT("Float"), &BoatConfig::cvBoatHedgeNormalNForceDamperSpeed );

            CreateSliderBelow( hedgehogTab, 1.0f,  10.0f, 1.0f,         TXT("Hard repel power"),              TXT("Float"), &BoatConfig::cvBoatHedgeHardRepelPower          );
            CreateSliderBelow( hedgehogTab, 1.0f,  10.0f, 1.0f,         TXT("Smooth repel power"),            TXT("Float"), &BoatConfig::cvBoatHedgeSmoothRepelPower        );

            CreateSliderBelow( hedgehogTab, 0.0f,  2.0f, 0.01f,         TXT("Ray scale X"),                   TXT("Float"), &BoatConfig::cvBoatHedgeRayScaleX               );
            CreateSliderBelow( hedgehogTab, 0.0f,  2.0f, 0.01f,         TXT("Ray scale Y"),                   TXT("Float"), &BoatConfig::cvBoatHedgeRayScaleY               );
            CreateSliderBelow( hedgehogTab, -2.0f,  2.0f, 0.01f,        TXT("Ray offset Y"),                  TXT("Float"), &BoatConfig::cvBoatHedgeRayOffsetY              );
        }

//         //////////////////////////////////////////////////////////////////////////
//         // Drowning tab
//         name = TXT("Drowning");
//         m_tabs->AddTab( name );
// 
//         RedGui::CRedGuiControl* drowningTab = m_tabs->GetTabByName( name );
//         if( drowningTab != nullptr )
//         {            
//             CreateCheckBoxBelow( drowningTab, TXT("Drowning enabled"), &BoatConfig::cvBoatDrowningEnabled );
// 
//             CreateSliderBelow( drowningTab, 0.025f, 10.0f, 0.025f, TXT("Drowning acceleration"), TXT("Float"), &BoatConfig::cvBoatDrowningAcceleration );
//             CreateSliderBelow( drowningTab, 1.0f, 20.0f, 1.0f,     TXT("Drowning propagation time"), TXT("Float"), &BoatConfig::cvBoatDrowningPropagationTime );
//             CreateSliderBelow( drowningTab, 1.0f, 20.0f, 1.0f,     TXT("Drowning systems shutdown time"), TXT("Float"), &BoatConfig::cvBoatDrowningSystemsShutdownTime );
// 
//             CreateSliderBelow( drowningTab, 1.0f, 20.0f, 1.0f, TXT("Destruction hit accept timeout"), TXT("Float"), &BoatConfig::cvBoatDestructionHitsAcceptTimeout );            
//             CreateSliderBelow( drowningTab, 0.5f, 10.0f, 0.5f, TXT("Destruction repel force multiplier"), TXT("Float"), &BoatConfig::cvBoatDestructionHitRepelForceMultiplier );
//         }
// 
//         //////////////////////////////////////////////////////////////////////////
//         // mashing tab
//         name = TXT("Mashing");
//         m_tabs->AddTab( name );
// 
//         RedGui::CRedGuiControl* mashingTab = m_tabs->GetTabByName( name );
//         if( mashingTab != nullptr )
//         {
//             CreateSliderBelow( mashingTab, 0.05f, 1.5f, 0.05f, TXT("Double tap time threshold"), TXT("Float"), &BoatConfig::cvBoatDoubleTapThreshold );
//             CreateSliderBelow( mashingTab, 0.05f, 1.5f, 0.05f, TXT("Gear candidate change threshold"), TXT("Float"), &BoatConfig::cvBoatGearCandidateSwitchTimeout );
//         }
// 
//         //////////////////////////////////////////////////////////////////////////
//         // path finding
//         name = TXT("PathFinding");
//         m_tabs->AddTab( name );
// 
//         RedGui::CRedGuiControl* pathfindingTab = m_tabs->GetTabByName( name );
//         if( pathfindingTab != nullptr )
//         {
//             CreateSliderBelow( pathfindingTab, 1.0f, 100.0f, 1.0f, TXT("Distance below which auto teleport is triggered"), TXT("Float"), &BoatConfig::cvBoatPathFindingOutFrustumMaxDistance );
//         }

        //////////////////////////////////////////////////////////////////////////
        // mashing tab
        name = TXT("Camera");
        m_tabs->AddTab( name );

        {

            RedGui::CRedGuiControl* cameraTab = m_tabs->GetTabByName( name );
            if( cameraTab != nullptr )
            {
                CreateCheckBoxBelow(cameraTab, TXT("Local Space Camera"), &BoatConfig::driverBoatLocalSpaceCamera );
                CreateSliderBelow( cameraTab, 30.0f, 80.0f, 1.0f, TXT("FOV while standing"), TXT("Float"), &BoatConfig::driverBoatFovStand, false );
                CreateSliderBelow( cameraTab, 30.0f, 80.0f, 1.0f, TXT("FOV while on R gear"), TXT("Float"), &BoatConfig::driverBoatFovReverse, false );           
                CreateSliderBelow( cameraTab, 30.0f, 80.0f, 1.0f, TXT("FOV while on 1st gear"), TXT("Float"), &BoatConfig::driverBoatFovGear1, false );
                CreateSliderBelow( cameraTab, 30.0f, 80.0f, 1.0f, TXT("FOV while on 2nd gear"), TXT("Float"), &BoatConfig::driverBoatFovGear2, false );
                CreateSliderBelow( cameraTab, 30.0f, 80.0f, 1.0f, TXT("FOV while on 3rd gear"), TXT("Float"), &BoatConfig::driverBoatFovGear3, false );

                CreateSliderBelow( cameraTab, 0.0f, 100.0f, 1.0f, TXT("Distance while standing"), TXT("Float"), &BoatConfig::driverBoatDistanceStand, false );
                CreateSliderBelow( cameraTab, 0.0f, 100.0f, 1.0f, TXT("Distance while on R gear"), TXT("Float"), &BoatConfig::driverBoatDistanceReverse, false );
                CreateSliderBelow( cameraTab, 0.0f, 100.0f, 1.0f, TXT("Distance while on 1st gear"), TXT("Float"), &BoatConfig::driverBoatDistanceGear1, false );
                CreateSliderBelow( cameraTab, 0.0f, 100.0f, 1.0f, TXT("Distance while on 2nd gear"), TXT("Float"), &BoatConfig::driverBoatDistanceGear2, false );
                CreateSliderBelow( cameraTab, 0.0f, 100.0f, 1.0f, TXT("Distance while on 3rd gear"), TXT("Float"), &BoatConfig::driverBoatDistanceGear3, false );

                CreateSliderBelow( cameraTab, -90.0f, 0.0f, 1.0f, TXT("Pitch while standing"), TXT("Float"), &BoatConfig::driverBoatPitchStand, false );
                CreateSliderBelow( cameraTab, -90.0f, 0.0f, 1.0f, TXT("Pitch while on R gear"), TXT("Float"), &BoatConfig::driverBoatPitchReverse, false );
                CreateSliderBelow( cameraTab, -90.0f, 0.0f, 1.0f, TXT("Pitch while on 1st gear"), TXT("Float"), &BoatConfig::driverBoatPitchGear1, false );
                CreateSliderBelow( cameraTab, -90.0f, 0.0f, 1.0f, TXT("Pitch while on 2nd gear"), TXT("Float"), &BoatConfig::driverBoatPitchGear2, false );
                CreateSliderBelow( cameraTab, -90.0f, 0.0f, 1.0f, TXT("Pitch while on 3rd gear"), TXT("Float"), &BoatConfig::driverBoatPitchGear3, false );

                CreateSliderBelow( cameraTab, -10.0f, 10.0f, 0.01f, TXT("Pivot offset up while on R gear"), TXT("Float"), &BoatConfig::driverBoatPivotOffsetUpReverse, false );
                CreateSliderBelow( cameraTab, -10.0f, 10.0f, 0.01f, TXT("Pivot offset up while on 1st gear"), TXT("Float"), &BoatConfig::driverBoatPivotOffsetUpGear1, false );
                CreateSliderBelow( cameraTab, -10.0f, 10.0f, 0.01f, TXT("Pivot offset up while on 2nd gear"), TXT("Float"), &BoatConfig::driverBoatPivotOffsetUpGear2, false );
                CreateSliderBelow( cameraTab, -10.0f, 10.0f, 0.01f, TXT("Pivot offset up while on 3rd gear"), TXT("Float"), &BoatConfig::driverBoatPivotOffsetUpGear3, false );

                CreateSliderBelow( cameraTab, -10.0f, 10.0f, 0.01f, TXT("Camera-sail offset while standing"), TXT("Float"), &BoatConfig::driverBoatCameraToSailOffsetStand, false );
                CreateSliderBelow( cameraTab, -10.0f, 10.0f, 0.01f, TXT("Camera-sail offset while on R gear"), TXT("Float"), &BoatConfig::driverBoatCameraToSailOffsetReverse, false );
                CreateSliderBelow( cameraTab, -10.0f, 10.0f, 0.01f, TXT("Camera-sail offset while on 1st gear"), TXT("Float"), &BoatConfig::driverBoatCameraToSailOffsetGear1, false );
                CreateSliderBelow( cameraTab, -10.0f, 10.0f, 0.01f, TXT("Camera-sail offset while on 2nd gear"), TXT("Float"), &BoatConfig::driverBoatCameraToSailOffsetGear2, false );
                CreateSliderBelow( cameraTab, -10.0f, 10.0f, 0.01f, TXT("Camera-sail offset while on 3rd gear"), TXT("Float"), &BoatConfig::driverBoatCameraToSailOffsetGear3, false );

                CreateSliderBelow( cameraTab, 0.01f, 10.0f, 0.01f, TXT("FOV adjust time factor"), TXT("Float"), &BoatConfig::driverBoatFovAdjustCoef, false );
                CreateSliderBelow( cameraTab, 0.01f, 10.0f, 0.01f, TXT("Distance adjust time factor"), TXT("Float"), &BoatConfig::driverBoatDistanceAdjustCoef, false );
                CreateSliderBelow( cameraTab, 0.01f, 10.0f, 0.01f, TXT("Pitch adjust time factor"), TXT("Float"), &BoatConfig::driverBoatPitchAdjustCoef, false );
                CreateSliderBelow( cameraTab, 0.01f, 10.0f, 0.01f, TXT("Pivot offset adjust time factor"), TXT("Float"), &BoatConfig::driverBoatOffsetAdjustCoef, false );
                CreateSliderBelow( cameraTab, 0.01f, 10.0f, 0.01f, TXT("Camera-sail offset adjust time factor"), TXT("Float"), &BoatConfig::driverBoatCameraToSailOffsetAdjustCoef, false );
            }
        }

        name = TXT("Passenger Camera");
        m_tabs->AddTab( name );

        {
            RedGui::CRedGuiControl* cameraTab = m_tabs->GetTabByName( name );
            if( cameraTab != nullptr )
            {
                CreateSliderBelow( cameraTab, 30.0f, 80.0f, 1.0f, TXT("FOV while standing"), TXT("Float"), &BoatConfig::passengerBoatFovStand, false );
                CreateSliderBelow( cameraTab, 30.0f, 80.0f, 1.0f, TXT("FOV while on R gear"), TXT("Float"), &BoatConfig::passengerBoatFovReverse, false );           
                CreateSliderBelow( cameraTab, 30.0f, 80.0f, 1.0f, TXT("FOV while on 1st gear"), TXT("Float"), &BoatConfig::passengerBoatFovGear1, false );
                CreateSliderBelow( cameraTab, 30.0f, 80.0f, 1.0f, TXT("FOV while on 2nd gear"), TXT("Float"), &BoatConfig::passengerBoatFovGear2, false );
                CreateSliderBelow( cameraTab, 30.0f, 80.0f, 1.0f, TXT("FOV while on 3rd gear"), TXT("Float"), &BoatConfig::passengerBoatFovGear3, false );

                CreateSliderBelow( cameraTab, 0.0f, 100.0f, 1.0f, TXT("Distance while standing"), TXT("Float"), &BoatConfig::passengerBoatDistanceStand, false );
                CreateSliderBelow( cameraTab, 0.0f, 100.0f, 1.0f, TXT("Distance while on R gear"), TXT("Float"), &BoatConfig::passengerBoatDistanceReverse, false );
                CreateSliderBelow( cameraTab, 0.0f, 100.0f, 1.0f, TXT("Distance while on 1st gear"), TXT("Float"), &BoatConfig::passengerBoatDistanceGear1, false );
                CreateSliderBelow( cameraTab, 0.0f, 100.0f, 1.0f, TXT("Distance while on 2nd gear"), TXT("Float"), &BoatConfig::passengerBoatDistanceGear2, false );
                CreateSliderBelow( cameraTab, 0.0f, 100.0f, 1.0f, TXT("Distance while on 3rd gear"), TXT("Float"), &BoatConfig::passengerBoatDistanceGear3, false );

                CreateSliderBelow( cameraTab, -90.0f, 0.0f, 1.0f, TXT("Pitch while standing"), TXT("Float"), &BoatConfig::passengerBoatPitchStand, false );
                CreateSliderBelow( cameraTab, -90.0f, 0.0f, 1.0f, TXT("Pitch while on R gear"), TXT("Float"), &BoatConfig::passengerBoatPitchReverse, false );
                CreateSliderBelow( cameraTab, -90.0f, 0.0f, 1.0f, TXT("Pitch while on 1st gear"), TXT("Float"), &BoatConfig::passengerBoatPitchGear1, false );
                CreateSliderBelow( cameraTab, -90.0f, 0.0f, 1.0f, TXT("Pitch while on 2nd gear"), TXT("Float"), &BoatConfig::passengerBoatPitchGear2, false );
                CreateSliderBelow( cameraTab, -90.0f, 0.0f, 1.0f, TXT("Pitch while on 3rd gear"), TXT("Float"), &BoatConfig::passengerBoatPitchGear3, false );

                CreateSliderBelow( cameraTab, -10.0f, 10.0f, 0.01f, TXT("Pivot offset up while on R gear"), TXT("Float"), &BoatConfig::passengerBoatPivotOffsetUpReverse, false );
                CreateSliderBelow( cameraTab, -10.0f, 10.0f, 0.01f, TXT("Pivot offset up while on 1st gear"), TXT("Float"), &BoatConfig::passengerBoatPivotOffsetUpGear1, false );
                CreateSliderBelow( cameraTab, -10.0f, 10.0f, 0.01f, TXT("Pivot offset up while on 2nd gear"), TXT("Float"), &BoatConfig::passengerBoatPivotOffsetUpGear2, false );
                CreateSliderBelow( cameraTab, -10.0f, 10.0f, 0.01f, TXT("Pivot offset up while on 3rd gear"), TXT("Float"), &BoatConfig::passengerBoatPivotOffsetUpGear3, false );

                CreateSliderBelow( cameraTab, -10.0f, 10.0f, 0.01f, TXT("Camera-sail offset while standing"), TXT("Float"), &BoatConfig::passengerBoatCameraToSailOffsetStand, false );
                CreateSliderBelow( cameraTab, -10.0f, 10.0f, 0.01f, TXT("Camera-sail offset while on R gear"), TXT("Float"), &BoatConfig::passengerBoatCameraToSailOffsetReverse, false );
                CreateSliderBelow( cameraTab, -10.0f, 10.0f, 0.01f, TXT("Camera-sail offset while on 1st gear"), TXT("Float"), &BoatConfig::passengerBoatCameraToSailOffsetGear1, false );
                CreateSliderBelow( cameraTab, -10.0f, 10.0f, 0.01f, TXT("Camera-sail offset while on 2nd gear"), TXT("Float"), &BoatConfig::passengerBoatCameraToSailOffsetGear2, false );
                CreateSliderBelow( cameraTab, -10.0f, 10.0f, 0.01f, TXT("Camera-sail offset while on 3rd gear"), TXT("Float"), &BoatConfig::passengerBoatCameraToSailOffsetGear3, false );

                CreateSliderBelow( cameraTab, 0.01f, 10.0f, 0.01f, TXT("FOV adjust time factor"), TXT("Float"), &BoatConfig::passengerBoatFovAdjustCoef, false );
                CreateSliderBelow( cameraTab, 0.01f, 10.0f, 0.01f, TXT("Distance adjust time factor"), TXT("Float"), &BoatConfig::passengerBoatDistanceAdjustCoef, false );
                CreateSliderBelow( cameraTab, 0.01f, 10.0f, 0.01f, TXT("Pitch adjust time factor"), TXT("Float"), &BoatConfig::passengerBoatPitchAdjustCoef, false );
                CreateSliderBelow( cameraTab, 0.01f, 10.0f, 0.01f, TXT("Pivot offset adjust time factor"), TXT("Float"), &BoatConfig::passengerBoatOffsetAdjustCoef, false );
                CreateSliderBelow( cameraTab, 0.01f, 10.0f, 0.01f, TXT("Camera-sail offset adjust time factor"), TXT("Float"), &BoatConfig::passengerBoatCameraToSailOffsetAdjustCoef, false );
            }
        }

        name = TXT("DEBUG");
        m_tabs->AddTab( name );
        {
            RedGui::CRedGuiControl* debugTab = m_tabs->GetTabByName( name );
            if( debugTab != nullptr )
            {
                CreateSliderBelow( debugTab, 0.01f, 1.0f, 0.01f, TXT("Physx thread synchronization damper time"), TXT("Float"), &BoatConfig::cvPhysxThreadDamperTime );
            }
        }

        m_tabs->SetActiveTab( 0 );
    }

    //////////////////////////////////////////////////////////////////////////

    CDebugWindowBoatSettings::~CDebugWindowBoatSettings(void)
    {   
    }

    //////////////////////////////////////////////////////////////////////////

    Bool CDebugWindowBoatSettings::RegisterBoat( CBoatComponent* ptr )
    {
        return m_registeredBoats.PushBackUnique( ptr );
    }

    //////////////////////////////////////////////////////////////////////////
    
    Bool CDebugWindowBoatSettings::DeRegisterBoat( CBoatComponent* ptr )
    {
        // Remove this boat from debug pages
        auto it = Find( m_registeredBoats.Begin(), m_registeredBoats.End(), ptr );        
        if( it != m_registeredBoats.End() )
        {
            m_registeredBoats.Erase( it );
            return true;
        }

        return false;
    }

    //////////////////////////////////////////////////////////////////////////

    void CDebugWindowBoatSettings::CreateSliderBelow( RedGui::CRedGuiControl* parent, Float min, Float max, Float step, const String& name, const String& userDataType, RedGui::RedGuiAny userData, Bool addToList )
    {
        RedGui::CRedGuiPanel* panel = new RedGui::CRedGuiPanel( 0, 0, 100, 40 );
        panel->SetDock( RedGui::DOCK_Top );
        panel->SetMargin( Box2( 5, 5, 5, 5 ) );
        panel->SetBorderVisible( false );
        parent->AddChild( panel );
        {
            // Slider
            RedGui::CRedGuiAdvancedSlider* slider = new RedGui::CRedGuiAdvancedSlider( 0, 0, 300, 20 );
            
            if( addToList )
            {
                m_guiSliders.PushBack( slider );
            }

            slider->SetDock( RedGui::DOCK_Left );
            slider->SetMinValue( min );
            slider->SetMaxValue( max );
            slider->SetStepValue( step );

            // Set slider value
            if( userDataType == TXT("Float") )
            {
                Config::TConfigVar<Float>* var = (Config::TConfigVar<Float>*)userData;
                slider->SetValue( var->Get() );
            }
            else if( userDataType == TXT("Int") )
            {
                Config::TConfigVar<Int32>* var = (Config::TConfigVar<Int32>*)userData;
                slider->SetValue( (Float)var->Get() );
            }
            else
            {
                ASSERT( TXT("Data type unreckognized") );
            }

            slider->SetUserString( TXT("Type"), userDataType );
            slider->SetUserData( userData );
            slider->EventScroll.Bind( this, &CDebugWindowBoatSettings::NotifyOnSliderChanged );

            panel->AddChild( slider );

            // Label
            RedGui::CRedGuiLabel* label = new RedGui::CRedGuiLabel( 0, 0, 300, 20 );
            label->SetText( name );
            label->SetDock( RedGui::DOCK_Left );
            label->SetMargin( Box2( 10, 0, 0, 0 ) );

            panel->AddChild( label );
        }
    }


    //////////////////////////////////////////////////////////////////////////

    void CDebugWindowBoatSettings::CreateCheckBoxBelow( RedGui::CRedGuiControl* parent, const String& name, RedGui::RedGuiAny userData )
    {
        RedGui::CRedGuiCheckBox* checkBox = new RedGui::CRedGuiCheckBox( 0, 0, 100, 25 );

        m_guiCheckboxes.PushBack( checkBox );

        checkBox->SetMargin( Box2( 5, 5, 5, 5 ) );
        checkBox->SetDock( RedGui::DOCK_Top );
        checkBox->SetText( name );
        
        Config::TConfigVar<Bool>* var = (Config::TConfigVar<Bool>*)userData;
        checkBox->SetChecked( var->Get() );

        checkBox->SetUserString( TXT("Type"), TXT("Bool") );
        checkBox->SetUserData( userData );
        checkBox->EventCheckedChanged.Bind( this, &CDebugWindowBoatSettings::NotifyOnCheckedChanged );
        parent->AddChild( checkBox );
    }

    //////////////////////////////////////////////////////////////////////////

    void CDebugWindowBoatSettings::NotifyOnSliderChanged( RedGui::CRedGuiEventPackage& eventPackage, Float value )
    {
        RedGui::CRedGuiControl* sender = eventPackage.GetEventSender();

        String type = sender->GetUserString( TXT("Type") );

        if( type == TXT("Float") )
        {
            Config::TConfigVar<Float>* var = sender->GetUserData< Config::TConfigVar<Float> >();
            var->Set( value );
        }
        else if( type == TXT("Int") )
        {
            Config::TConfigVar<Int32>* var = sender->GetUserData< Config::TConfigVar<Int32> >();
            var->Set( (Int32)value );
        }
    }

    //////////////////////////////////////////////////////////////////////////

    void CDebugWindowBoatSettings::NotifyOnCheckedChanged( RedGui::CRedGuiEventPackage& eventPackage, Bool value )
    {
        CRedGuiControl* sender = eventPackage.GetEventSender();

        Config::TConfigVar<Bool>* var = sender->GetUserData< Config::TConfigVar<Bool> >();
        var->Set( value );
    }

    //////////////////////////////////////////////////////////////////////////

    void CDebugWindowBoatSettings::NotifyOnTabChanged( RedGui::CRedGuiEventPackage& eventPackage, RedGui::CRedGuiControl* selectedTab )
    {
    }

    //////////////////////////////////////////////////////////////////////////

    void CDebugWindowBoatSettings::NotifyOnButtonClicked( RedGui::CRedGuiEventPackage& eventPackage )
    {
        CRedGuiControl* sender = eventPackage.GetEventSender();

        if( eventPackage.GetEventSender() == m_loadButton )
        {
            // Reload from config file
            SConfig::GetInstance().Reload();

            // Refresh all slider ans check boxes
            const Uint32 sliders = m_guiSliders.Size();
            for( Uint32 i=0; i<sliders; ++i)
            {
                RedGui::CRedGuiAdvancedSlider* slider = m_guiSliders[i];

                if( slider == nullptr )
                {
                    continue;
                }

                // Set slider value for different data types
                const String& type = slider->GetUserString( TXT("Type") );
                if( type == TXT("Float") )
                {
                    Config::TConfigVar<Float>* var = slider->GetUserData< Config::TConfigVar<Float> >();
                    if( var != nullptr )
                    {
                        slider->SetValue( var->Get() );
                    }
                }
                else if( type == TXT("Int") )
                {
                    Config::TConfigVar<Int32>* var = slider->GetUserData< Config::TConfigVar<Int32> >();
                    if( var != nullptr )
                    {
                        slider->SetValue( (Float)var->Get() );
                    }
                }
            }

            const Uint32 chkboxs = m_guiCheckboxes.Size();
            for( Uint32 i=0; i<chkboxs; ++i)
            {
                RedGui::CRedGuiCheckBox* box = m_guiCheckboxes[i];

                if( box == nullptr )
                {
                    continue;
                }

                // Update checkbox 
                Config::TConfigVar<Bool>* var = box->GetUserData< Config::TConfigVar<Bool> >();
                if( var != nullptr )
                {
                    box->SetChecked( var->Get() );
                }
            }
        }
        else if( eventPackage.GetEventSender() == m_saveButton )
        {
            SConfig::GetInstance().Save();
        }
    }

    //////////////////////////////////////////////////////////////////////////

    void CDebugWindowBoatSettings::OnWindowClosed( CRedGuiControl* control )
    {
        // Unpause game
        //GGame->Unpause( TXT( "CDebugWindowVehicleViewer" ) );

        SetEnabled(false);

        const Uint32 count = m_registeredBoats.Size();
        for( Uint32 i=0; i<count; ++i )
        {
            m_registeredBoats[i]->OnDebugPageClosed();
        }
    }

    //////////////////////////////////////////////////////////////////////////

    void CDebugWindowBoatSettings::OnWindowOpened( CRedGuiControl* control )
    {
        SetEnabled(true);

        // Pause game
        //GGame->Pause( TXT( "CDebugWindowVehicleViewer" ) );
    }

    //////////////////////////////////////////////////////////////////////////
} // namespace DebugWindows

//////////////////////////////////////////////////////////////////////////

#endif  // FINAL
#endif	// NO_DEBUG_WINDOWS
#endif	// NO_RED_GUI
