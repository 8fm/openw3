#pragma once

enum EAsyncCheckResult
{
	ASR_InProgress,
	ASR_ReadyTrue,
	ASR_ReadyFalse,
	ASR_Failed
};

BEGIN_ENUM_RTTI( EAsyncCheckResult )
	ENUM_OPTION( ASR_InProgress );
	ENUM_OPTION( ASR_ReadyTrue );
	ENUM_OPTION( ASR_ReadyFalse );
	ENUM_OPTION( ASR_Failed );
END_ENUM_RTTI()