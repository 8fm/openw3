/* SCE CONFIDENTIAL
 * Pad Library for PC Games 1.4.1
 * Copyright (C) 2014 Sony Computer Entertainment Inc.
 * All Rights Reserved.
 */


#ifndef _SCE_PAD_H
#define _SCE_PAD_H

#ifdef	_WIN32
#include "pad_types.h"
#include "pad_windows_static.h"
#else
#include <sys/types.h>
#include <stdbool.h>
#include <scetypes.h>
#include <user_service.h>
#endif	/* _WIN32 */

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ScePadButtonDataOffset {
	SCE_PAD_BUTTON_L3        = 0x00000002,
	SCE_PAD_BUTTON_R3        = 0x00000004,
	SCE_PAD_BUTTON_OPTIONS   = 0x00000008,
	SCE_PAD_BUTTON_UP        = 0x00000010,
	SCE_PAD_BUTTON_RIGHT     = 0x00000020,
	SCE_PAD_BUTTON_DOWN      = 0x00000040,
	SCE_PAD_BUTTON_LEFT      = 0x00000080,
	SCE_PAD_BUTTON_L2        = 0x00000100,
	SCE_PAD_BUTTON_R2        = 0x00000200,
	SCE_PAD_BUTTON_L1        = 0x00000400,
	SCE_PAD_BUTTON_R1        = 0x00000800,
	SCE_PAD_BUTTON_TRIANGLE  = 0x00001000,
	SCE_PAD_BUTTON_CIRCLE    = 0x00002000,
	SCE_PAD_BUTTON_CROSS     = 0x00004000,
	SCE_PAD_BUTTON_SQUARE    = 0x00008000,
	SCE_PAD_BUTTON_TOUCH_PAD = 0x00100000,
	SCE_PAD_BUTTON_INTERCEPTED = 0x80000000,
} ScePadButtonDataOffset;

/* This definietion is alias for support old style. */
#define SCE_PAD_BUTTON_START	SCE_PAD_BUTTON_OPTIONS


typedef struct ScePadAnalogStick {
	uint8_t x;
	uint8_t y;
} ScePadAnalogStick;

typedef struct ScePadAnalogButtons{
	uint8_t l2;
	uint8_t r2;
	uint8_t padding[2];
} ScePadAnalogButtons;

/**
 *E  
 *   @brief Maximum number of touch points.
 **/
#define SCE_PAD_MAX_TOUCH_NUM		2

/**
 *E  
 *   @brief device unique data size
 **/
#define SCE_PAD_MAX_DEVICE_UNIQUE_DATA_SIZE		12

/**
 *E  
 *   @brief Structure for touch point.
 *
 **/
typedef struct ScePadTouch {
	uint16_t  x;			/*E X position                */
	uint16_t  y;			/*E Y position                */
	uint8_t   id;			/*E Touch ID                  */
	uint8_t   reserve[3];	/*E reserved                  */
} ScePadTouch;

/**
 *E  
 *   @brief Structure for touch data.
 *
 **/
typedef struct ScePadTouchData {
	uint8_t touchNum;	/*E Number of touch reports */
	uint8_t reserve[7];	/*E reserved                */
	ScePadTouch touch[SCE_PAD_MAX_TOUCH_NUM];	/*E actual touch data */
} ScePadTouchData;

/**
 *E  
 *   @brief Structure for extension unit data.
 *
 **/
typedef struct ScePadExtensionUnitData {
	uint32_t extensionUnitId;
	uint8_t  reserve[1];
	uint8_t  dataLength;
	uint8_t  data[10];
} ScePadExtensionUnitData;

/**
 *E  
 *   @brief Structure for controller data.
 *   
 *   This data structure covers ORBIS standard controller.
 *
 **/
typedef struct ScePadData {
	uint32_t				buttons;		/*E Digital buttons       */
	ScePadAnalogStick		leftStick;		/*E Left stick            */
	ScePadAnalogStick		rightStick;		/*E Right stick           */
	ScePadAnalogButtons		analogButtons;	/*E Analog buttons(R2/L2) */
	SceFQuaternion			orientation;	/*E Controller orientation as a Quaternion <x,y,z,w>
	                                         *E   It is calculated as an accumulation value when controller is connected.
											 *E   It can reset by scePadResetOrientation function. */
	SceFVector3				acceleration;	/*E Acceleration of the controller(G).    */
	SceFVector3				angularVelocity;/*E Angular velocity of the controller(radians/s). */
	ScePadTouchData         touchData;		/*E Touch pad data. */
	bool					connected;		/*E Controller connection status. true:connected  false:removed */
	uint64_t                timestamp;		/*E System timestamp of this data(micro seconds). */
	ScePadExtensionUnitData extensionUnitData; /*E Data area for retrieve the extension unit. */
	uint8_t                 connectedCount;
	uint8_t                 reserve[2];	/*E Reserved area */
	uint8_t                 deviceUniqueDataLen;	/*E Device unique data length(for special controllers) */
	uint8_t                 deviceUniqueData[SCE_PAD_MAX_DEVICE_UNIQUE_DATA_SIZE];	/*E Device unique data(for special controllers)        */
} ScePadData;

/**
 *E
 *  @brief Initialize function
 *
 *  This function initialize the libpad.
 *  When you use this library, please call this function first. 
 *  
 *  @retval (==SCE_OK) : success
 *          (< SCE_OK) : error codes
 **/
int scePadInit(void);

/**
 *E
 *  @brief parameter for the scePadOpen function.
 *
 *  It is reserved now. 
 **/
typedef struct ScePadOpenParam{
	uint8_t reserve[8]; /* reserved */
} ScePadOpenParam;


/**
 * E
 *  Type of device
 **/

/* Personal user */
#define SCE_PAD_PORT_TYPE_STANDARD			0 	/* for standard controller */
#define SCE_PAD_PORT_TYPE_SPECIAL			2	/* for special controller */

/* SYSTEM user(SCE_USER_SERVICE_USER_ID_SYSTEM) */
#define SCE_PAD_PORT_TYPE_REMOTE_CONTROL	16	/* for remote control(CEC remote control) */

/**
 *E
 *  @brief It is a function which opens the controller port. 
 *         When it succeeds, this function returns handle to access controller. 
 *
 *  @param [in] userId : The UserID which opens controller.
 *                        
 *  @param [in] type   : Type of device
 *                       - SCE_PAD_PORT_TYPE_STANDARD
 *                       - SCE_PAD_PORT_TYPE_SPECIAL
 *                       - SCE_PAD_PORT_TYPE_REMOTE_CONTROL (SYSTEM user only)
 *
 *  @param [in] index  : It is a parameter for future extension.
 *                       Please specify '0' currently.
 *
 *  @param [in] pParam : It is a parameter for future extension.
 *                       Please specify "NULL" or pointer of ScePadOpenParam. 
 *  
 *  @retval (>= 0) : handle to access the controler
 *          (< 0)  : error codes
 **/
int scePadOpen(SceUserServiceUserId userId, int32_t type, int32_t index, const ScePadOpenParam* pParam);

/**
 *E
 *  @brief Close the controller port.
 *
 *  @param [in] handle
 *  
 *  @retval (> SCE_OK) : success
 *          (< 0) : error codes
 **/
int scePadClose(int32_t handle);

/**
 *E  
 *  @brief Retrieves the latest controller data states.
 *
 *  @param [in]  handle : handle to access controller.
 *  @param [out] pData  : Pointer to retrieve the latest state.
 *  
 *  @retval (== SCE_OK) : success
 *          (< 0)       : error codes
 *
 **/
int scePadReadState(int32_t handle, ScePadData *pData);

/**
 *E  
 *   @brief Maximum history data by scePadRead function.
 **/
#define SCE_PAD_MAX_DATA_NUM	64

/**
 *E  
 *  @brief Retrieves the data history of controller data.
 *
 *  @param [in]  handle : handle to access controller.
 *  @param [out] pData  : Pointer to the top of history buffer.
 *  @param [in]  num    : Number of history buffer (1 ~ SCE_PAD_MAX_DATA_NUM).
 *  
 *  @retval (>= 0) : The acquired actual number of data.
 *                   '0' returns in the following case.
 *                      - Controller data is not updated from the previous read.
 *          (< 0)  : error codes
 *
 **/
int scePadRead(int32_t handle, ScePadData *pData, int32_t num);

/**
 *E  
 *  @brief Set motion sensor states.
 *
 *  @param [in]  handle  : handle to access controller.
 *  @param [out] bEnable : true  - Start motion sensor calculation.(default value is "true")
 *                       : false - Stop motion sensor calculation.
 *  
 *  @retval (== SCE_OK) : success
 *          (< 0)       : error codes
 *
 **/
int scePadSetMotionSensorState(int32_t handle, bool bEnable);

/**
 *E  
 *  @brief Set tilt correction filter settings.
 *
 *  @param [in]  handle  : handle to access controller.
 *  @param [out] bEnable : true  - Enable tilt correction. (default value is "false")
 *                       : false - Disable tilt correction.
 *  
 *  @retval (== SCE_OK) : success
 *          (< 0)       : error codes
 *
 **/
int scePadSetTiltCorrectionState(int32_t handle, bool bEnable);

/**
 *E  
 *  @brief Set deadbanding filter settings of angular velocity.
 *
 *  @param [in]  handle  : handle to access controller.
 *  @param [out] bEnable : true  - Enable deadbanding.(default value is "false")
 *                       : false - Disable deadbanding.
 *  
 *  @retval (== SCE_OK) : success
 *          (< 0)       : error codes
 *
 **/
int scePadSetAngularVelocityDeadbandState(int32_t handle, bool bEnable);

/**
 *E  
 *  @brief Reset device orientation to identity.
 *
 *  @param [in]  handle  : handle to access controller.
 *  
 *  @retval (== SCE_OK) : success
 *          (< 0)       : error codes
 *
 **/
int scePadResetOrientation(int32_t handle);

/**
 *E
 *  @brief parameter for control the dual motor.
 **/
typedef struct ScePadVibrationParam{
	uint8_t largeMotor;		/*E large(left) motor(0:stop 1~255:rotate).  */
	uint8_t smallMotor;		/*E small(right) motor(0:stop 1~255:rotate). */
} ScePadVibrationParam;

/**
 *E  
 *  @brief Set the parameter to control the vibration effect.
 *
 *  @param [in] handle  : handle to access controller.
 *  @param [in] pParam  : Parameter for control the motor.
 *  
 *  @retval (== SCE_OK) : success
 *          (< 0)       : error codes
 *
 **/
int scePadSetVibration(int32_t handle, const ScePadVibrationParam *pParam);


/**
 *E  
 *   @brief Structure for Controller color
 **/
typedef struct ScePadColor{
	uint8_t r;	/*E   Red   */
	uint8_t g;	/*E   Green */
	uint8_t b;	/*E   Blue  */
	uint8_t reserve[1];
} ScePadColor;

typedef ScePadColor ScePadLightBarParam;

/**
 *E  
 *  @brief Set the parameter to control the Light Bar.
 *
 *  @param [in] handle  : handle to access controller.
 *  @param [in] pParam  : Parameter for control the Light Bar.
 *  
 *  @retval (== SCE_OK) : success
 *          (< 0)       : error codes
 *
 **/
int scePadSetLightBar(int32_t handle, const ScePadLightBarParam *pParam);

/**
 *E  
 *  @brief Reset the Light bar to default color(User Color) of the controller.
 *         
 *
 *  @param [in] handle  : handle to access controller.
 *  
 *  @retval (== SCE_OK) : success
 *          (< 0)       : error codes
 *
 **/
int scePadResetLightBar(int32_t handle);

/**
 *E  
 *   @brief Structure for touch pad information
 **/
typedef struct ScePadTouchPadInformation{
	float pixelDensity; /* (dot/mm) */
	struct{
		uint16_t x;
		uint16_t y;
	}resolution;
}ScePadTouchPadInformation;

/**
 *E  
 *   @brief Structure for stick information
 **/
typedef struct ScePadStickInformation{
	uint8_t deadZoneLeft;  /* deadzone of left stick */
	uint8_t deadZoneRight; /* deadzone of right stick */
}ScePadStickInformation;

/*E Connection type of the controller */
#define SCE_PAD_CONNECTION_TYPE_LOCAL	0	/*E access from local connection */
#define SCE_PAD_CONNECTION_TYPE_REMOTE	1	/*E access from remote connection (remote play access) */

/**
 *E  
 *   @brief Structure for controller feature information.
 **/
typedef struct ScePadControllerInformation{
	ScePadTouchPadInformation touchPadInfo;
	ScePadStickInformation stickInfo;
	uint8_t connectionType;
	uint8_t connectedCount;
	bool    connected;
	uint8_t reserve[12];
}ScePadControllerInformation;

/**
 *E  
 *  @brief Retrieves feature information of the controller.
 *
 *  @param [in] handle  : handle to access controller.
 *  @param [out] pInfo  : pointer for retrieve controller feature information.
 *  
 *  @retval (== SCE_OK) : success
 *          (< 0)       : error codes
 *
 **/
int scePadGetControllerInformation(int32_t handle, ScePadControllerInformation *pInfo);

/*
 * Error number definition
 */

/**
 * @j 不正な引数 @ej
 * @e invalid argument @ee
 */
#define SCE_PAD_ERROR_INVALID_ARG				-2137915391	/* 0x80920001 */

/**
 * @j 不正なポート @ej
 * @e invalid port @ee
 */
#define SCE_PAD_ERROR_INVALID_PORT				-2137915390	/* 0x80920002 */

/**
 * @j 不正なハンドル @ej
 * @e invalid handle @ee
 */
#define SCE_PAD_ERROR_INVALID_HANDLE			-2137915389	/* 0x80920003 */

/**
 * @j すでにオープン済みのポートの再オープン @ej
 * @e already opned the port. @ee
 */
#define SCE_PAD_ERROR_ALREADY_OPENED			-2137915388	/* 0x80920004 */

/**
 * @j ライブラリ未初期化 @ej
 * @e library uninitialized. @ee
 */
#define SCE_PAD_ERROR_NOT_INITIALIZED			-2137915387	/* 0x80920005 */

/**
 * @j ライトバーの設定値が不正 @ej
 * @e invalid light bar setting. @ee
 */
#define SCE_PAD_ERROR_INVALID_LIGHTBAR_SETTING	-2137915386	/* 0x80920006 */

/**
 * @j 指定したハンドルにデバイスが接続されていない @ej
 * @e device is not connected of the handle. @ee
 */
#define SCE_PAD_ERROR_DEVICE_NOT_CONNECTED		-2137915385	/* 0x80920007 */

/**
 * @j 致命的なエラー @ej
 * @e fatal error @ee
 */
#define SCE_PAD_ERROR_FATAL						-2137915137	/* 0x809200FF */


#ifdef __cplusplus
}
#endif

#endif //end define _SCE_PAD_H
