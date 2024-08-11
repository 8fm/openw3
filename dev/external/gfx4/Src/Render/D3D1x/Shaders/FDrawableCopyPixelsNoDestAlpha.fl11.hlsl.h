#if 0
//
// Generated by Microsoft (R) HLSL Shader Compiler For Durango 9.30.12098.0
//
//
///
// Resource Bindings:
//
// Name                                 Type  Format         Dim Slot Elements
// ------------------------------ ---------- ------- ----------- ---- --------
// sampler_tex[0]                    sampler      NA          NA    0        1
// sampler_tex[1]                    sampler      NA          NA    1        1
// tex[0]                            texture  float4          2d    0        1
// tex[1]                            texture  float4          2d    1        1
//
//
//
// Input signature:
//
// Name                 Index   Mask Register SysValue  Format   Used
// -------------------- ----- ------ -------- -------- ------- ------
// TEXCOORD                 0   xy          0     NONE   float   xy  
// TEXCOORD                 1     zw        0     NONE   float     zw
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
// First Precompiled Shader at offset:[78]
// Embedded Data:
//  0x0000004e - Offset To First Precompiled Shader
//  0x00000050 - Original Shader Descriptor
//  0x00000049 - Original Shader Size
dcl_globalFlags refactoringAllowed
dcl_sampler s0, mode_default
dcl_sampler s1, mode_default
dcl_resource_texture2d (float,float,float,float) t0
dcl_resource_texture2d (float,float,float,float) t1
dcl_input_ps linear v0.xy
dcl_input_ps linear v0.zw
dcl_output o0.xyzw
dcl_temps 2
sample_indexable(texture2d)(float,float,float,float) r0.xyz, v0.xyxx, t0.xyzw, s0
sample_indexable(texture2d)(float,float,float,float) r1.xyzw, v0.zwzz, t1.xyzw, s1
add r1.xyz, -r0.xyzx, r1.xyzx
mad o0.xyz, r1.wwww, r1.xyzx, r0.xyzx
mov o0.w, l(1.000000)
ret 
Opaque Custom Data - XBOX Precompiled Shader
// Offset:[78], bundle is:[158] DWORDS.
// Disassembled Precompiled Shader Object:
// Shader Type               : (PS)
// Shader Hw Stage           : (PS)
// HLSL Source Filename      : f:\balazs.torok.stream2\dev\external\gfx4\Src\Render\D3D1x\Shaders\FDrawableCopyPixelsNoDestAlpha.fl11.hlsl
// HLSL Entrypoint Name      : main

//
// Shader Data:
// ----------------- PS Data ------------------------
; Input Semantic Mappings
;   [0] generic, usageIdx  0, channelMask 15, param0, paramSlot 0, DefaultVal={0,0,0,0}

codeLenInByte        = 132;Bytes

; launchModeFlags    = 0x0000000E
srdTableSupport      = TRUE
immediateSupportExt  = TRUE
srdTableSupportExt   = TRUE

userElementCount     = 1;
;  userElements[0]      = PTR_EXTENDED_USER_DATA, ReferencedExtUserElements=1, s[0:1]
extUserElementCount  = 4;
;  extUserElements 1[0] = IMM_RESOURCE, 0, offset 0:7 dwords
;  extUserElements 1[1] = IMM_RESOURCE, 1, offset 8:15 dwords
;  extUserElements 1[2] = IMM_SAMPLER, 0, offset 16:19 dwords
;  extUserElements 1[3] = IMM_SAMPLER, 1, offset 20:23 dwords
NumVgprs             = 8;
NumSgprs             = 30;
FloatMode            = 192;
IeeeMode             = 0;
bFlatPtr32           = 0;
ScratchSize          = 0 dwords/thread;
LDSByteSize          = 0 bytes/workgroup (compile time only);
ScratchWaveOffsetReg = s65535;
; texSamplerUsage         = 0x00000003
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
; SPI_PS_IN_CONTROL       = 0x00000001
SPIC:NUM_INTERP             = 1
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
  s_mov_b64     s[28:29], exec                          // 000000000000: BE9C047E
  s_wqm_b64     exec, exec                              // 000000000004: BEFE0A7E
  s_mov_b32     m0, s2                                  // 000000000008: BEFC0302
  s_load_dwordx8  s[4:11], s[0:1], 0x00                 // 00000000000C: C0C20100
  s_load_dwordx8  s[12:19], s[0:1], 0x08                // 000000000010: C0C60108
  s_load_dwordx8  s[20:27], s[0:1], 0x10                // 000000000014: C0CA0110
  v_interp_p1_f32  v2, v0, attr0.x                      // 000000000018: C8080000
  v_interp_p1_f32  v3, v0, attr0.y                      // 00000000001C: C80C0100
  v_interp_p1_f32  v4, v0, attr0.z                      // 000000000020: C8100200
  v_interp_p1_f32  v5, v0, attr0.w                      // 000000000024: C8140300
  v_interp_p2_f32  v2, v1, attr0.x                      // 000000000028: C8090001
  v_interp_p2_f32  v3, v1, attr0.y                      // 00000000002C: C80D0101
  v_interp_p2_f32  v4, v1, attr0.z                      // 000000000030: C8110201
  v_interp_p2_f32  v5, v1, attr0.w                      // 000000000034: C8150301
  s_waitcnt     lgkmcnt(0)                              // 000000000038: BF8C007F
  image_sample  v[1:4], v[2:3], s[4:11], s[20:23] dmask:0x7 // 00000000003C: F0800700 00A10102
  image_sample  v[4:7], v[4:5], s[12:19], s[24:27] dmask:0xf // 000000000044: F0800F00 00C30404
  s_waitcnt     vmcnt(0) & lgkmcnt(15)                  // 00000000004C: BF8C0F70
  v_subrev_f32  v0, v1, v4                              // 000000000050: 0A000901
  v_subrev_f32  v4, v2, v5                              // 000000000054: 0A080B02
  v_subrev_f32  v5, v3, v6                              // 000000000058: 0A0A0D03
  v_mac_legacy_f32  v1, v7, v0                          // 00000000005C: 0C020107
  v_mac_legacy_f32  v2, v7, v4                          // 000000000060: 0C040907
  v_mac_legacy_f32  v3, v7, v5                          // 000000000064: 0C060B07
  v_mov_b32     v0, 1.0                                 // 000000000068: 7E0002F2
  s_mov_b64     exec, s[28:29]                          // 00000000006C: BEFE041C
  v_cvt_pkrtz_f16_f32  v1, v1, v2                       // 000000000070: 5E020501
  v_cvt_pkrtz_f16_f32  v0, v3, v0                       // 000000000074: 5E000103
  exp           mrt0, v1, v1, v0, v0 compr vm           // 000000000078: F800140F 00000001
  s_endpgm                                              // 000000000080: BF810000
end


// Approximately 6 instruction slots used
#endif

const BYTE pBinary_D3D1xFL11X_FDrawableCopyPixelsNoDestAlpha[] =
{
     68,  88,  66,  67, 240, 224, 
     35, 252, 167,   2, 227, 108, 
    252,  80,  15, 186, 115,  62, 
    210, 215,   1,   0,   0,   0, 
     60,   6,   0,   0,   5,   0, 
      0,   0,  52,   0,   0,   0, 
     96,   1,   0,   0, 172,   1, 
      0,   0, 224,   1,   0,   0, 
    160,   5,   0,   0,  82,  68, 
     69,  70,  36,   1,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   4,   0,   0,   0, 
     60,   0,   0,   0,   0,   5, 
    255, 255,   0, 129,   0,   0, 
    232,   0,   0,   0,  82,  68, 
     49,  49,  60,   0,   0,   0, 
     24,   0,   0,   0,  32,   0, 
      0,   0,  40,   0,   0,   0, 
     36,   0,   0,   0,  12,   0, 
      0,   0,   0,   0,   0,   0, 
    188,   0,   0,   0,   3,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      1,   0,   0,   0,   1,   0, 
      0,   0, 203,   0,   0,   0, 
      3,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   1,   0, 
      0,   0,   1,   0,   0,   0, 
      1,   0,   0,   0, 218,   0, 
      0,   0,   2,   0,   0,   0, 
      5,   0,   0,   0,   4,   0, 
      0,   0, 255, 255, 255, 255, 
      0,   0,   0,   0,   1,   0, 
      0,   0,  13,   0,   0,   0, 
    225,   0,   0,   0,   2,   0, 
      0,   0,   5,   0,   0,   0, 
      4,   0,   0,   0, 255, 255, 
    255, 255,   1,   0,   0,   0, 
      1,   0,   0,   0,  13,   0, 
      0,   0, 115,  97, 109, 112, 
    108, 101, 114,  95, 116, 101, 
    120,  91,  48,  93,   0, 115, 
     97, 109, 112, 108, 101, 114, 
     95, 116, 101, 120,  91,  49, 
     93,   0, 116, 101, 120,  91, 
     48,  93,   0, 116, 101, 120, 
     91,  49,  93,   0,  77, 105, 
     99, 114, 111, 115, 111, 102, 
    116,  32,  40,  82,  41,  32, 
     72,  76,  83,  76,  32,  83, 
    104,  97, 100, 101, 114,  32, 
     67, 111, 109, 112, 105, 108, 
    101, 114,  32,  70, 111, 114, 
     32,  68, 117, 114,  97, 110, 
    103, 111,  32,  57,  46,  51, 
     48,  46,  49,  50,  48,  57, 
     56,  46,  48,   0,  73,  83, 
     71,  78,  68,   0,   0,   0, 
      2,   0,   0,   0,   8,   0, 
      0,   0,  56,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   3,   0,   0,   0, 
      0,   0,   0,   0,   3,   3, 
      0,   0,  56,   0,   0,   0, 
      1,   0,   0,   0,   0,   0, 
      0,   0,   3,   0,   0,   0, 
      0,   0,   0,   0,  12,  12, 
      0,   0,  84,  69,  88,  67, 
     79,  79,  82,  68,   0, 171, 
    171, 171,  79,  83,  71,  78, 
     44,   0,   0,   0,   1,   0, 
      0,   0,   8,   0,   0,   0, 
     32,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,   0,   0, 
      0,   0,  15,   0,   0,   0, 
     83,  86,  95,  84,  97, 114, 
    103, 101, 116,   0, 171, 171, 
     83,  72,  69,  88, 184,   3, 
      0,   0,  80,   0,   0,   0, 
    238,   0,   0,   0,  53,  16, 
      0,   0,   5,   0,   0,   0, 
     78,   0,   0,   0,  80,   0, 
      0,   0,  73,   0,   0,   0, 
    106,   8,   0,   1,  90,   0, 
      0,   3,   0,  96,  16,   0, 
      0,   0,   0,   0,  90,   0, 
      0,   3,   0,  96,  16,   0, 
      1,   0,   0,   0,  88,  24, 
      0,   4,   0, 112,  16,   0, 
      0,   0,   0,   0,  85,  85, 
      0,   0,  88,  24,   0,   4, 
      0, 112,  16,   0,   1,   0, 
      0,   0,  85,  85,   0,   0, 
     98,  16,   0,   3,  50,  16, 
     16,   0,   0,   0,   0,   0, 
     98,  16,   0,   3, 194,  16, 
     16,   0,   0,   0,   0,   0, 
    101,   0,   0,   3, 242,  32, 
     16,   0,   0,   0,   0,   0, 
    104,   0,   0,   2,   2,   0, 
      0,   0,  69,   0,   0, 139, 
    194,   0,   0, 128,  67,  85, 
     21,   0, 114,   0,  16,   0, 
      0,   0,   0,   0,  70,  16, 
     16,   0,   0,   0,   0,   0, 
     70, 126,  16,   0,   0,   0, 
      0,   0,   0,  96,  16,   0, 
      0,   0,   0,   0,  69,   0, 
      0, 139, 194,   0,   0, 128, 
     67,  85,  21,   0, 242,   0, 
     16,   0,   1,   0,   0,   0, 
    230,  26,  16,   0,   0,   0, 
      0,   0,  70, 126,  16,   0, 
      1,   0,   0,   0,   0,  96, 
     16,   0,   1,   0,   0,   0, 
      0,   0,   0,   8, 114,   0, 
     16,   0,   1,   0,   0,   0, 
     70,   2,  16, 128,  65,   0, 
      0,   0,   0,   0,   0,   0, 
     70,   2,  16,   0,   1,   0, 
      0,   0,  50,   0,   0,   9, 
    114,  32,  16,   0,   0,   0, 
      0,   0, 246,  15,  16,   0, 
      1,   0,   0,   0,  70,   2, 
     16,   0,   1,   0,   0,   0, 
     70,   2,  16,   0,   0,   0, 
      0,   0,  54,   0,   0,   5, 
    130,  32,  16,   0,   0,   0, 
      0,   0,   1,  64,   0,   0, 
      0,   0, 128,  63,  62,   0, 
      0,   1,  53,  16,   0,   0, 
    160,   0,   0,   0,  21,   0, 
      1,   0,  41, 121,  10,   0, 
    120,   1, 237,  86, 207, 107, 
     19,  65,  20, 126, 179, 187, 
    109, 146, 237, 182,  73,  37, 
     66, 197, 224,  65,  20,   4, 
     33,  18, 237, 201,  83, 197, 
     32,  34,  69, 196, 222, 202, 
     82, 186,  49,  27,  83, 186, 
    249, 193, 110, 208, 212,  67, 
    140, 224,  65,  36,   7,  17, 
    143,  94,  60, 120, 243, 224, 
     63,  96,  18,  84, 100, 241, 
    224, 221, 255, 162, 120,  16, 
    145,  82, 125, 223, 238,  76, 
    187, 136, 222,   2,  10, 230, 
    131, 239, 189, 249, 230, 199, 
    219, 183,  51, 187,  51, 179, 
     99,  82, 132,  71, 110, 237, 
     41, 252, 240, 241, 217,  55, 
    167,   4, 209,  12, 151, 217, 
     69,  30, 232, 166,  97, 137, 
     86,  51, 176,  68, 103,  96, 
    208,  65,  35,  66, 211,   9, 
    230,  67, 230,  10,  19, 109, 
    138,  49,  86, 198,  61, 182, 
      5, 166, 197, 252, 206, 227, 
    174, 178,  63, 206, 212, 153, 
    139,  76, 164, 129, 112,   0, 
    135,  60,   0, 218,   1,   3, 
     38,   1, 244, 149,  41,  69, 
     64,  89, 245, 133,  87, 177, 
     34, 156, 134, 249,  61, 144, 
    119,  50, 206, 175,  64, 110, 
     42,  31, 196, 148, 211,  21, 
    141, 251, 219, 136, 223,  49, 
    182, 234, 221, 213,  58,   1, 
     88, 143,  36,  22, 152, 113, 
    111, 162,  61, 217,  79, 205, 
    171,  37, 215,  21, 213, 120, 
    183,  49, 243,  79, 248, 193, 
     88,  66,  33,   1, 165,  17, 
    127, 150, 178, 108, 255,  95, 
    168,  57, 197,  92, 224, 219, 
    201,  49, 147,  51, 130, 118, 
    252,  11,  68, 253,  17, 248, 
    100, 137, 190,  65,  41,  45, 
     88,  99, 172, 210,  58, 116, 
    244,  17,  38,  52,  43,  49, 
    163, 109, 232, 130,  54, 178, 
    133,   3, 109, 179, 182,  19, 
    122, 157, 245, 122,  66, 111, 
    178, 222,  76, 232,  10, 235, 
    138, 210,  42, 126,  54, 122, 
    158,  30, 173,  51,  87, 226, 
    215, 253, 103,  57, 205, 113, 
     50, 156, 230,  56,  25,  78, 
    115, 156,  24,  35, 244, 140, 
    231, 195, 158, 185,  63, 212, 
    244, 189,  33, 137, 183, 227, 
    180, 248,  48, 206, 137, 143, 
    124,  60, 165,  67,  18,  86, 
     72,  90,  46,  36,  61,  31, 
     10, 202, 132,  66, 204, 135, 
     66,  91,  12, 133, 126,  52, 
    188,  79, 131,  17, 165, 250, 
    187, 154, 120, 193, 155, 111, 
    127, 215,  48, 222,  81,  59, 
     59,  24, 137,  12, 153, 218, 
     92, 218, 212, 231,  77,  51, 
     37,  52,  43, 149,  49, 172, 
    212, 220, 172, 245,  69, 163, 
     94, 193, 216,  31, 242, 158, 
     24, 239, 169, 121, 181,  39, 
     62,  24,  29,  99,  27, 151, 
     15, 239,   2,  10, 124,  94, 
     70,  77,  48,  56,  59, 161, 
    163,  10,   6, 246, 125, 222, 
    235,  35,  13, 230, 153, 208, 
     29, 121, 214,  86, 165, 151, 
    253, 248, 124,  70, 175,  67, 
     60, 147, 237,   3, 233,  17, 
    127, 142, 249, 233, 229, 251, 
    207,  95, 143,  92,  27, 189, 
    150, 245, 175, 164,  71,  12, 
    220,  79,  22,  26, 231,  78, 
     46, 203, 139,  74,  40, 219, 
     60,  38, 198, 215,  46, 218, 
     21, 199, 115, 238,   5, 197, 
     78, 203, 111, 109,  23, 131, 
    142, 239,  58, 141, 243, 118, 
    213, 189,  99, 187, 221, 142, 
    235,  55,  29, 207, 190,  93, 
    235,  46, 219, 107, 254,  45, 
    251, 166, 219, 172, 186, 190, 
     93, 190,  80,  46, 117, 237, 
    181, 186, 195,  34, 176, 175, 
    148, 125, 231, 174,  83, 241, 
    220, 203, 173, 246, 206, 141, 
    173, 174, 235,   5, 215,  91, 
    101,  55, 232,  92, 242, 218, 
    117, 167,  88, 243,  74, 165, 
     98, 221,  11, 240, 196,  24, 
    171,  50,  23, 220,  37,  83, 
    204, 134, 179, 213, 164, 159, 
    147,  95,  42, 233,   0,   0, 
     83,  84,  65,  84, 148,   0, 
      0,   0,   6,   0,   0,   0, 
      2,   0,   0,   0,   0,   0, 
      0,   0,   3,   0,   0,   0, 
      2,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      1,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   2,   0, 
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
