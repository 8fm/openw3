#ifndef _H_ENGINE_NAMES_REGISTRY
#define _H_ENGINE_NAMES_REGISTRY

#if !defined( REGISTER_NAME )
#define REGISTER_NAME( name_ ) RED_DECLARE_NAME( name_ )
#define REGISTER_NAMED_NAME( varname_, string_ ) RED_DECLARE_NAMED_NAME( varname_, string_ )
#define REGISTER_NOT_REGISTERED
#endif

REGISTER_NAME( OnUserSignedIn )
REGISTER_NAME( OnUserSignedOut )
REGISTER_NAME( OnAccountPickerOpened )
REGISTER_NAME( OnAccountPickerClosed )
REGISTER_NAME( OnControllerReconnected )
REGISTER_NAME( OnControllerDisconnected )
REGISTER_NAME( OnSignInStarted )
REGISTER_NAME( OnSignInCancelled )

REGISTER_NAME( OnInputDeviceChanged )

REGISTER_NAME( SoundSystem )
REGISTER_NAME( SoundEventSaveData )
REGISTER_NAME( SavedSoundEvent )

REGISTER_NAME ( Boat )

REGISTER_NAME( EyesIdle )
REGISTER_NAME( Rest )
REGISTER_NAME( True_ );
REGISTER_NAME( False_ );

REGISTER_NAME( FakeAxisInput )
REGISTER_NAME( Standard )
REGISTER_NAME( InputSettings )
REGISTER_NAMED_NAME( AnyBindingName, "$any" )
REGISTER_NAMED_NAME( SingleBindingName, "$single" )
REGISTER_NAME( RefreshOutputMonitor )
REGISTER_NAME( ListAvailableMonitorCount )
REGISTER_NAME( fullscreen )

REGISTER_NAME( ReloadBaseTree )
REGISTER_NAME( PreReloadBaseTree )
REGISTER_NAME( PostReloadBaseTree )

REGISTER_NAME( MoveForward )
REGISTER_NAME( MoveBackward )
REGISTER_NAME( MoveLeft )
REGISTER_NAME( MoveRight )
REGISTER_NAME( PreserveSystemGamma )

REGISTER_NAME( ps4pad )
REGISTER_NAME( xpad )
REGISTER_NAME( keyboardmouse )
REGISTER_NAME( tablet )

REGISTER_NAME( SectionChanged )
REGISTER_NAME( SectionFinished )
REGISTER_NAME( AudioTrackStarted )

REGISTER_NAME( forceAutoHideDistance )
REGISTER_NAME( Dismemberment )

#if defined( REGISTER_NOT_REGISTERED )
#undef REGISTER_NAME
#undef REGISTER_NAMED_NAME
#undef REGISTER_NOT_REGISTERED
#endif

#endif
 