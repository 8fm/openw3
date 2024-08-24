//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CarSimJoystickMgr.h"
#include "CarSimHelpers.h"

namespace
{
	static const CString s_csIniFilename( L"CarSim.ini" );
	static const CString s_csJoystickSection( L"Joystick" );
	static const CString s_csUseJoystickKey( L"UseJoystick" );
	static const CString s_csJoystickAssignementKey( L"Assignment" );
	static const CString s_csTrue( L"True" );
	static const CString s_csFalse( L"False" );
}

CarSimJoystickMgr::CarSimJoystickMgr()
: m_pDI( NULL )
, m_pJoystick( NULL )
, m_hMainWnd( NULL )
, m_bUseJoytick( false )
, m_bGearUpPushed( false )
, m_bGearDownPushed( false )
, m_bIsThrottleAxisIdle( true )
, m_bIsAuxBusAxisIdle( true )
{
	m_assignement.m_joystickProductGuid = GUID_NULL;
	m_assignement.m_gearUpButtonIndex = 0;
	m_assignement.m_gearDownButtonIndex = 0;
	m_assignement.m_throttleAxisGuid = GUID_NULL;
	m_assignement.m_AuxBusAxisGuid = GUID_NULL;
}
	
CarSimJoystickMgr::~CarSimJoystickMgr()
{
	_ASSERT( m_pDI == NULL );
	_ASSERT( m_pJoystick == NULL );
}

void CarSimJoystickMgr::Init( HWND in_hMainWnd )
{
	m_hMainWnd = in_hMainWnd;

	Load();

    // Create a DInput object
    if( FAILED( DirectInput8Create( GetModuleHandle(NULL), DIRECTINPUT_VERSION, IID_IDirectInput8, (VOID**)&m_pDI, NULL ) ) )
        return;

    if( FAILED( m_pDI->EnumDevices( DI8DEVCLASS_GAMECTRL, FindAndCreateJoystickCallback, (VOID*)this, DIEDFL_ATTACHEDONLY ) ) )
        return;

    InitJoystick();
}
	
void CarSimJoystickMgr::Term()
{
	ClearJoystick();

    ReleaseClear( m_pDI );

	Save();
}

void CarSimJoystickMgr::OnActivate()
{
	if( m_pJoystick )
        m_pJoystick->Acquire();
}

void CarSimJoystickMgr::Poll( IJoystickAware* in_pJoystickAware )
{
    DIJOYSTATE2 js;           // DInput joystick state 

    if( m_pJoystick == NULL || in_pJoystickAware == NULL ) 
        return;

    // Poll the device to read the current state
    if( FAILED( m_pJoystick->Poll() ) )  
    {
        HRESULT hr = m_pJoystick->Acquire();
        while( hr == DIERR_INPUTLOST ) 
            hr = m_pJoystick->Acquire();

        return; 
    }

    // Get the input's device state
    if( FAILED( m_pJoystick->GetDeviceState( sizeof(DIJOYSTATE2), &js ) ) )
        return;

	LONG lThrottle = 0, lAuxBus = 0;

	if( m_assignement.m_throttleAxisGuid == GUID_XAxis )
		lThrottle = js.lX;
	else if( m_assignement.m_throttleAxisGuid == GUID_YAxis )
		lThrottle = js.lY;
	else if( m_assignement.m_throttleAxisGuid == GUID_ZAxis )
		lThrottle = js.lZ;
	else if( m_assignement.m_throttleAxisGuid == GUID_RxAxis )
		lThrottle = js.lRx;
	else if( m_assignement.m_throttleAxisGuid == GUID_RyAxis )
		lThrottle = js.lRy;
	else if( m_assignement.m_throttleAxisGuid == GUID_RzAxis )
		lThrottle = js.lRz;

	if( m_assignement.m_AuxBusAxisGuid == GUID_XAxis )
		lAuxBus = js.lX;
	else if( m_assignement.m_AuxBusAxisGuid == GUID_YAxis )
		lAuxBus = js.lY;
	else if( m_assignement.m_AuxBusAxisGuid == GUID_ZAxis )
		lAuxBus = js.lZ;
	else if( m_assignement.m_AuxBusAxisGuid == GUID_RxAxis )
		lAuxBus = js.lRx;
	else if( m_assignement.m_AuxBusAxisGuid == GUID_RyAxis )
		lAuxBus = js.lRy;
	else if( m_assignement.m_AuxBusAxisGuid == GUID_RzAxis )
		lAuxBus = js.lRz;

	if( ! m_bIsThrottleAxisIdle || lThrottle != 0 )
		in_pJoystickAware->OnAxisThrottle(  - (double)lThrottle / 1000.0 );

	m_bIsThrottleAxisIdle = lThrottle == 0;

	if( ! m_bIsAuxBusAxisIdle || lAuxBus != 0 )
		in_pJoystickAware->OnAxisAuxBusWet( fabs( (double)lAuxBus / 1000.0 ) );

	m_bIsAuxBusAxisIdle = lAuxBus == 0;


	// Check gear down button
	if( js.rgbButtons[m_assignement.m_gearDownButtonIndex] & 0x80 )
	{
		if( ! m_bGearDownPushed )
			in_pJoystickAware->OnButtonGearDown();

		m_bGearDownPushed = true;
	}
	else
	{
		m_bGearDownPushed = false;
	}

	// Check gear up button
	if( js.rgbButtons[m_assignement.m_gearUpButtonIndex] & 0x80 )
	{
		if( ! m_bGearUpPushed )
			in_pJoystickAware->OnButtonGearUp();

		m_bGearUpPushed = true;
	}
	else
	{
		m_bGearUpPushed = false;
	}
}

bool CarSimJoystickMgr::GetAllJoystick( DeviceVector& out_rDeviceVector )
{
	out_rDeviceVector.clear();
	
    if( m_pDI == NULL || FAILED( m_pDI->EnumDevices( DI8DEVCLASS_GAMECTRL, GetAllJoystickCallback, (VOID*)&out_rDeviceVector, DIEDFL_ATTACHEDONLY ) ) )
        return false;
	
	return ! out_rDeviceVector.empty();
}

bool CarSimJoystickMgr::GetAllJoystickButtons( const DIDEVICEINSTANCE& in_rJoystick, DeviceObjectVector& out_rButtonVector )
{
	return GetAllJoystickObjects( in_rJoystick, out_rButtonVector, false );
}

bool CarSimJoystickMgr::GetAllJoystickAxis( const DIDEVICEINSTANCE& in_rJoystick, DeviceObjectVector& out_rAxisVector )
{
	return GetAllJoystickObjects( in_rJoystick, out_rAxisVector, true );
}

bool CarSimJoystickMgr::GetJoystickAssignment( JoystickAssignment& out_rAssignement ) const
{
	out_rAssignement = m_assignement;

	return m_bUseJoytick;
}

void CarSimJoystickMgr::SetJoystickAssignement(  bool in_bUseJoystick, const JoystickAssignment& in_rAssignement  )
{
	ClearJoystick();

	m_bUseJoytick = in_bUseJoystick;
	m_assignement = in_rAssignement;

	if( FAILED( m_pDI->EnumDevices( DI8DEVCLASS_GAMECTRL, FindAndCreateJoystickCallback, (VOID*)this, DIEDFL_ATTACHEDONLY ) ) )
        return;
		
	InitJoystick();
}

void CarSimJoystickMgr::Load()
{
	TCHAR szBuffer[_MAX_PATH] = {0};
	::GetPrivateProfileString( s_csJoystickSection, s_csUseJoystickKey, s_csFalse, szBuffer, _MAX_PATH, s_csIniFilename );
	m_bUseJoytick = s_csTrue == szBuffer;

	::GetPrivateProfileStruct( s_csJoystickSection, s_csJoystickAssignementKey, (VOID*)&m_assignement, sizeof( JoystickAssignment ), s_csIniFilename );
}

void CarSimJoystickMgr::Save()
{
	if( ::WritePrivateProfileString( s_csJoystickSection, s_csUseJoystickKey, m_bUseJoytick? s_csTrue : s_csFalse, s_csIniFilename ) )	
	{
		::WritePrivateProfileStruct( s_csJoystickSection, s_csJoystickAssignementKey, (VOID*)&m_assignement, sizeof( JoystickAssignment ), s_csIniFilename );
	}
}

void CarSimJoystickMgr::InitJoystick()
{
	// Make sure we got a joystick
    if( m_pJoystick == NULL )
        return;

    if( FAILED( m_pJoystick->SetDataFormat( &c_dfDIJoystick2 ) ) )
        return;

    if( FAILED( m_pJoystick->SetCooperativeLevel( m_hMainWnd, DISCL_EXCLUSIVE | DISCL_FOREGROUND ) ) )
        return;

    m_pJoystick->EnumObjects( InitAxisCallback, (VOID*)this, /*DIDFT_AXIS*/DIDFT_ALL );
}

void CarSimJoystickMgr::ClearJoystick()
{
	if( m_pJoystick ) 
        m_pJoystick->Unacquire();
    
    ReleaseClear( m_pJoystick );

	m_bGearUpPushed = false;
	m_bGearDownPushed = false;
	m_bIsThrottleAxisIdle = true;
	m_bIsAuxBusAxisIdle = true;
}

bool CarSimJoystickMgr::GetAllJoystickObjects( const DIDEVICEINSTANCE& in_rJoystick, DeviceObjectVector& out_rObjectVector, bool in_bAxis )
{
	out_rObjectVector.clear();

	if( m_pDI == NULL )
		return false;

	LPDIRECTINPUTDEVICE8 pJoystick = NULL;

	if( FAILED( m_pDI->CreateDevice( in_rJoystick.guidInstance, &pJoystick, NULL ) ) )
		return false;

	_ASSERT( pJoystick );

	bool retVal = false;
	if( SUCCEEDED( pJoystick->EnumObjects( GetAllObjectsCallback, (VOID*)&out_rObjectVector, in_bAxis? DIDFT_AXIS : DIDFT_BUTTON ) ) )
	{
		retVal = ! out_rObjectVector.empty();
	}

	ReleaseClear( pJoystick );

	return retVal;
}

BOOL CALLBACK CarSimJoystickMgr::FindAndCreateJoystickCallback( const DIDEVICEINSTANCE* pdidInstance, VOID* pContext )
{
	CarSimJoystickMgr* pThis = (CarSimJoystickMgr*)pContext;
	_ASSERT( pThis );

	if( ! pThis->m_bUseJoytick )
		return DIENUM_STOP;

	if( pdidInstance->guidProduct == pThis->m_assignement.m_joystickProductGuid )
	{
		if( SUCCEEDED( pThis->m_pDI->CreateDevice( pdidInstance->guidInstance, &(pThis->m_pJoystick), NULL ) ) )
			return DIENUM_STOP;
	}

    return DIENUM_CONTINUE;
}

BOOL CALLBACK CarSimJoystickMgr::InitAxisCallback( const DIDEVICEOBJECTINSTANCE* pdidoi, VOID* pContext )
{
	CarSimJoystickMgr* pThis = (CarSimJoystickMgr*)pContext;
	_ASSERT( pThis && pThis->m_pJoystick );

    // For axes that are returned, set the DIPROP_RANGE property for the
    // enumerated axis in order to scale min/max values.
    if( pdidoi->dwType & DIDFT_AXIS )
    {
        DIPROPRANGE diprg; 
        diprg.diph.dwSize       = sizeof(DIPROPRANGE); 
        diprg.diph.dwHeaderSize = sizeof(DIPROPHEADER); 
        diprg.diph.dwHow        = DIPH_BYID; 
        diprg.diph.dwObj        = pdidoi->dwType; // Specify the enumerated axis
        diprg.lMin              = -1000; 
        diprg.lMax              = +1000; 
    
        // Set the range for the axis
        if( FAILED( pThis->m_pJoystick->SetProperty( DIPROP_RANGE, &diprg.diph ) ) ) 
            return DIENUM_STOP;
         
    }

    return DIENUM_CONTINUE;
}

BOOL CALLBACK CarSimJoystickMgr::GetAllJoystickCallback( const DIDEVICEINSTANCE* pdidInstance, VOID* pContext )
{
	DeviceVector* pVector = (DeviceVector*)pContext;

	pVector->push_back( *pdidInstance );

    return DIENUM_CONTINUE;
}

BOOL CALLBACK CarSimJoystickMgr::GetAllObjectsCallback( const DIDEVICEOBJECTINSTANCE* pdidInstance, VOID* pContext )
{
	DeviceObjectVector* pVector = (DeviceObjectVector*)pContext;

	pVector->push_back( *pdidInstance );

	return DIENUM_CONTINUE;
}