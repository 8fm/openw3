#if 0
//
// Generated by Microsoft (R) HLSL Shader Compiler For Durango 9.30.12098.0
//
//
///
//
// Input signature:
//
// Name                 Index   Mask Register SysValue  Format   Used
// -------------------- ----- ------ -------- -------- ------- ------
// COLOR                    0   xyzw        0     NONE   float   xyzw
// COLOR                    1   xyzw        1     NONE   float      w
//
//
// Output signature:
//
// Name                 Index   Mask Register SysValue  Format   Used
// -------------------- ----- ------ -------- -------- ------- ------
// SV_Target                0   xyzw        0   TARGET   float   xyzw
//
ps_5_0
Opaque Custom Data - XBOX Precompiled Shader Header
// First Precompiled Shader at offset:[30]
// Embedded Data:
//  0x0000001e - Offset To First Precompiled Shader
//  0x00000050 - Original Shader Descriptor
//  0x00000019 - Original Shader Size
dcl_globalFlags refactoringAllowed
dcl_input_ps linear v0.xyzw
dcl_input_ps linear v1.w
dcl_output o0.xyzw
mul o0.w, v0.w, v1.w
mov o0.xyz, v0.xyzx
ret 
Opaque Custom Data - XBOX Precompiled Shader
// Offset:[30], bundle is:[127] DWORDS.
// Disassembled Precompiled Shader Object:
// Shader Type               : (PS)
// Shader Hw Stage           : (PS)
// HLSL Source Filename      : f:\balazs.torok.stream2\dev\external\gfx4\Src\Render\D3D1x\Shaders\FVertexEAlpha.fl11.hlsl
// HLSL Entrypoint Name      : main

//
// Shader Data:
// ----------------- PS Data ------------------------
; Input Semantic Mappings
;   [0] generic, usageIdx  0, channelMask 15, param0, paramSlot 0, DefaultVal={0,0,0,0};   [1] generic, usageIdx  1, channelMask  8, param1, paramSlot 1, DefaultVal={0,0,0,0}

codeLenInByte        = 68;Bytes

; launchModeFlags    = 0x0000000E
srdTableSupport      = TRUE
immediateSupportExt  = TRUE
srdTableSupportExt   = TRUE

userElementCount     = 0;
extUserElementCount  = 0;
NumVgprs             = 5;
NumSgprs             = 4;
FloatMode            = 192;
IeeeMode             = 0;
bFlatPtr32           = 0;
ScratchSize          = 0 dwords/thread;
LDSByteSize          = 0 bytes/workgroup (compile time only);
ScratchWaveOffsetReg = s65535;
; texSamplerUsage         = 0x00000000
; constBufUsage           = 0x00000000

; SPI_SHADER_PGM_RSRC2_PS = 0x00000004
SSPRP:SCRATCH_EN            = 0
SSPRP:USER_SGPR             = 2
SSPRP:TRAP_PRESENT          = 0
SSPRP:WAVE_CNT_EN           = 0
SSPRP:EXTRA_LDS_SIZE        = 0
SSPRP:EXCP_EN               = 0
; SPI_SHADER_Z_FORMAT     = 0x00000000
SPZF:Z_EXPORT_FORMAT        = 0; SPI_SHADER_ZERO
; SPI_PS_IN_CONTROL       = 0x00000002
SPIC:NUM_INTERP             = 2
SPIC:PARAM_GEN              = 0
SPIC:FOG_ADDR               = 0
SPIC:BC_OPTIMIZE_DISABLE    = 0
SPIC:PASS_FOG_THROUGH_PS    = 0
; SPI_PS_INPUT_ADDR       = 0x00000002
SPIA:PERSP_CENTER_ENA       = 1
; DB_SHADER_CONTROL       = 0x00000010
DB:Z_ORDER                  = 1
DB:CONSERVATIVE_Z_EXPORT    = 0; EXPORT_ANY_Z
; CB_SHADER_MASK          = 0x0000000F
CB:OUTPUT0_ENABLE           = 15

// Shader Instructions:
shader main
  asic(CI)
  type(PS)
                                                            // s_ps_state in s0
  s_mov_b32     m0, s2                                  // 000000000000: BEFC0302
  v_interp_p1_f32  v2, v0, attr0.w                      // 000000000004: C8080300
  v_interp_p1_f32  v3, v0, attr1.w                      // 000000000008: C80C0700
  v_interp_p2_f32  v2, v1, attr0.w                      // 00000000000C: C8090301
  v_interp_p2_f32  v3, v1, attr1.w                      // 000000000010: C80D0701
  v_mul_legacy_f32  v2, v2, v3                          // 000000000014: 0E040702
  v_interp_p1_f32  v3, v0, attr0.x                      // 000000000018: C80C0000
  v_interp_p1_f32  v4, v0, attr0.y                      // 00000000001C: C8100100
  v_interp_p1_f32  v0, v0, attr0.z                      // 000000000020: C8000200
  v_interp_p2_f32  v3, v1, attr0.x                      // 000000000024: C80D0001
  v_interp_p2_f32  v4, v1, attr0.y                      // 000000000028: C8110101
  v_interp_p2_f32  v0, v1, attr0.z                      // 00000000002C: C8010201
  v_cvt_pkrtz_f16_f32  v1, v3, v4                       // 000000000030: 5E020903
  v_cvt_pkrtz_f16_f32  v0, v0, v2                       // 000000000034: 5E000500
  exp           mrt0, v1, v1, v0, v0 compr vm           // 000000000038: F800140F 00000001
  s_endpgm                                              // 000000000040: BF810000
end


// Approximately 3 instruction slots used
#endif

const BYTE pBinary_D3D1xFL11X_FVertexEAlpha[] =
{
     68,  88,  66,  67, 139, 154, 
    135, 105, 182, 217, 225,  63, 
    106,  99, 211, 178,  74,  60, 
    166,  93,   1,   0,   0,   0, 
     80,   4,   0,   0,   5,   0, 
      0,   0,  52,   0,   0,   0, 
    180,   0,   0,   0, 252,   0, 
      0,   0,  48,   1,   0,   0, 
    180,   3,   0,   0,  82,  68, 
     69,  70, 120,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
     60,   0,   0,   0,   0,   5, 
    255, 255,   0, 129,   0,   0, 
     60,   0,   0,   0,  82,  68, 
     49,  49,  60,   0,   0,   0, 
     24,   0,   0,   0,  32,   0, 
      0,   0,  40,   0,   0,   0, 
     36,   0,   0,   0,  12,   0, 
      0,   0,   0,   0,   0,   0, 
     77, 105,  99, 114, 111, 115, 
    111, 102, 116,  32,  40,  82, 
     41,  32,  72,  76,  83,  76, 
     32,  83, 104,  97, 100, 101, 
    114,  32,  67, 111, 109, 112, 
    105, 108, 101, 114,  32,  70, 
    111, 114,  32,  68, 117, 114, 
     97, 110, 103, 111,  32,  57, 
     46,  51,  48,  46,  49,  50, 
     48,  57,  56,  46,  48,   0, 
     73,  83,  71,  78,  64,   0, 
      0,   0,   2,   0,   0,   0, 
      8,   0,   0,   0,  56,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   3,   0, 
      0,   0,   0,   0,   0,   0, 
     15,  15,   0,   0,  56,   0, 
      0,   0,   1,   0,   0,   0, 
      0,   0,   0,   0,   3,   0, 
      0,   0,   1,   0,   0,   0, 
     15,   8,   0,   0,  67,  79, 
     76,  79,  82,   0, 171, 171, 
     79,  83,  71,  78,  44,   0, 
      0,   0,   1,   0,   0,   0, 
      8,   0,   0,   0,  32,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   3,   0, 
      0,   0,   0,   0,   0,   0, 
     15,   0,   0,   0,  83,  86, 
     95,  84,  97, 114, 103, 101, 
    116,   0, 171, 171,  83,  72, 
     69,  88, 124,   2,   0,   0, 
     80,   0,   0,   0, 159,   0, 
      0,   0,  53,  16,   0,   0, 
      5,   0,   0,   0,  30,   0, 
      0,   0,  80,   0,   0,   0, 
     25,   0,   0,   0, 106,   8, 
      0,   1,  98,  16,   0,   3, 
    242,  16,  16,   0,   0,   0, 
      0,   0,  98,  16,   0,   3, 
    130,  16,  16,   0,   1,   0, 
      0,   0, 101,   0,   0,   3, 
    242,  32,  16,   0,   0,   0, 
      0,   0,  56,   0,   0,   7, 
    130,  32,  16,   0,   0,   0, 
      0,   0,  58,  16,  16,   0, 
      0,   0,   0,   0,  58,  16, 
     16,   0,   1,   0,   0,   0, 
     54,   0,   0,   5, 114,  32, 
     16,   0,   0,   0,   0,   0, 
     70,  18,  16,   0,   0,   0, 
      0,   0,  62,   0,   0,   1, 
     53,  16,   0,   0, 129,   0, 
      0,   0,  21,   0,   1,   0, 
     41, 216,   9,   0, 120,   1, 
    237,  86,  77, 107,  19,  65, 
     24, 158, 217, 141,  77, 162, 
     73, 211, 130, 135,  28, 122, 
     40, 182,   7,  79, 209, 216, 
     34, 197,  83, 133, 216,  31, 
     96, 193,  67,  29,  66,  54, 
    102,  99, 164, 219, 180, 108, 
     66,  89,  60,  20, 189, 120, 
     44, 133, 254,  12, 237,  47, 
    168, 216,  30, 231, 230, 197, 
    131,  71,   5, 127,  68, 192, 
    130, 125, 158, 157,  25,  88, 
    218, 136, 151, 128, 130, 121, 
    224, 121, 223, 125, 102, 102, 
    223, 249, 228, 157, 249,  90, 
     20,  41, 190, 247,  90,  63, 
    233,  63,  44, 233, 102,   7, 
    254,   6,  40, 173,  39, 146, 
      2, 173,  16,  31, 173, 191, 
     75, 195,   6, 158, 105, 227, 
    131,  13, 112,  29, 100, 157, 
    163, 193, 250, 249,  18,  44, 
    155, 207, 128, 135, 224,   6, 
     88,   1,  89,  86,   6,  17, 
     38, 253, 190,  10,  87,  54, 
    174, 142, 125,  58, 220, 164, 
    177, 184, 214, 118, 153, 102, 
     60, 230,  64,  55, 199, 113, 
    224, 216,  28,  24, 151, 227, 
     36, 114,  52, 127,  25, 215, 
    230,   9, 184, 125,  34, 184, 
     31,  89, 204, 210, 252,   1, 
     92,  11, 206, 237,  28, 252, 
     29, 126,   1, 244, 227, 192, 
    245, 153,  73, 119,  22,  94, 
     86, 197,  34,  63, 254,  51, 
    184, 179, 193, 181,  32, 121, 
    198, 204, 138,  24, 176, 190, 
      4,  10, 241, 230, 140,  60, 
    170, 138,  17, 149, 211,  88, 
    182, 145,  57, 219,  70, 251, 
    212, 105, 208, 140,  70,  96, 
    191, 232,  53, 177,  97, 205, 
    202, 130,  24, 241,  44,  64, 
     43, 104, 149, 209,  91, 208, 
     91,  25, 221, 130, 110, 101, 
    116,  27, 186, 237, 180, 139, 
     95,  49, 253, 121, 118, 159, 
     15, 254, 101,  78, 199,  56, 
     25,  78, 199,  56,  25,  78, 
    199,  56,  49, 166, 240, 252, 
    139,  79, 194,  47, 104, 145, 
     47, 105, 233,  23, 181, 204, 
    151, 181, 151, 207, 225,  42, 
     43, 105,  33, 231,  52,  18, 
    172, 150, 162, 172, 165, 156, 
    215, 210, 147,  26,  57, 205, 
    228, 196, 219,  46, 167, 189, 
     61, 251,  97, 239, 196,  47, 
    214,  51,  31,  51, 157, 206, 
     94, 185,  65, 115, 246,  45, 
    116,  97, 219, 209, 221,   2, 
    191, 221, 191, 243, 238, 164, 
     24, 108,  47, 218, 250,   5, 
    235,  25, 131, 239, 142, 207, 
     15,  79, 239,  29, 219, 178, 
     53, 235, 159, 131, 252, 191, 
    251,  72, 181, 131,  40, 120, 
     61, 168,  13, 119, 227, 221, 
    237, 218,  96,  24, 135, 193, 
    206,   3, 213,   9, 247,  85, 
    152,  12, 195, 184,  31,  68, 
    234, 101,  55,  89,  85, 155, 
    241,  11, 245,  52, 236, 119, 
    194,  88,  53,  86,  26, 245, 
     68, 109, 246,   2, 136, 129, 
    218, 120,  22, 198, 195,  48, 
    121, 242,  56, 218, 235,   5, 
    181, 110,  84, 175, 215, 122, 
    209,  32,  66, 112, 131, 247, 
    182,  75,  94, 221, 121, 112, 
     39, 120, 213,  23, 151,  47, 
    116,  14,  58,   0,   0,   0, 
     83,  84,  65,  84, 148,   0, 
      0,   0,   3,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   3,   0,   0,   0, 
      1,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      1,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      1,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0
};
