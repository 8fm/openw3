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
// First Precompiled Shader at offset:[102]
// Embedded Data:
//  0x00000066 - Offset To First Precompiled Shader
//  0x00000050 - Original Shader Descriptor
//  0x00000061 - Original Shader Size
dcl_globalFlags refactoringAllowed
dcl_sampler s0, mode_default
dcl_sampler s1, mode_default
dcl_resource_texture2d (float,float,float,float) t0
dcl_resource_texture2d (float,float,float,float) t1
dcl_input_ps linear v0.xy
dcl_input_ps linear v0.zw
dcl_output o0.xyzw
dcl_temps 3
sample_indexable(texture2d)(float,float,float,float) r0.xyzw, v0.zwzz, t1.xyzw, s1
add r1.x, -r0.w, l(1.000000)
sample_indexable(texture2d)(float,float,float,float) r2.xyzw, v0.xyxx, t0.xyzw, s0
mad r1.x, r2.w, r1.x, r0.w
div r0.w, r0.w, r1.x
mov o0.w, r1.x
add r0.xyz, r0.xyzx, -r2.xyzx
mad o0.xyz, r0.wwww, r0.xyzx, r2.xyzx
ret 
Opaque Custom Data - XBOX Precompiled Shader
// Offset:[102], bundle is:[161] DWORDS.
// Disassembled Precompiled Shader Object:
// Shader Type               : (PS)
// Shader Hw Stage           : (PS)
// HLSL Source Filename      : f:\balazs.torok.stream2\dev\external\gfx4\Src\Render\D3D1x\Shaders\FDrawableCopyPixelsMergeAlpha.fl11.hlsl
// HLSL Entrypoint Name      : main

//
// Shader Data:
// ----------------- PS Data ------------------------
; Input Semantic Mappings
;   [0] generic, usageIdx  0, channelMask 15, param0, paramSlot 0, DefaultVal={0,0,0,0}

codeLenInByte        = 156;Bytes

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
NumVgprs             = 13;
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
  s_load_dwordx8  s[4:11], s[0:1], 0x08                 // 00000000000C: C0C20108
  s_load_dwordx8  s[12:19], s[0:1], 0x10                // 000000000010: C0C60110
  v_interp_p1_f32  v2, v0, attr0.z                      // 000000000014: C8080200
  v_interp_p1_f32  v3, v0, attr0.w                      // 000000000018: C80C0300
  v_interp_p2_f32  v2, v1, attr0.z                      // 00000000001C: C8090201
  v_interp_p2_f32  v3, v1, attr0.w                      // 000000000020: C80D0301
  s_waitcnt     lgkmcnt(0)                              // 000000000024: BF8C007F
  image_sample  v[5:8], v[2:3], s[4:11], s[16:19] dmask:0xf // 000000000028: F0800F00 00810502
  s_load_dwordx8  s[20:27], s[0:1], 0x00                // 000000000030: C0CA0100
  v_interp_p1_f32  v9, v0, attr0.x                      // 000000000034: C8240000
  v_interp_p1_f32  v10, v0, attr0.y                     // 000000000038: C8280100
  v_interp_p2_f32  v9, v1, attr0.x                      // 00000000003C: C8250001
  v_interp_p2_f32  v10, v1, attr0.y                     // 000000000040: C8290101
  s_waitcnt     lgkmcnt(0)                              // 000000000044: BF8C007F
  image_sample  v[0:3], v[9:10], s[20:27], s[12:15] dmask:0xf // 000000000048: F0800F00 00650009
  s_waitcnt     vmcnt(1) & lgkmcnt(15)                  // 000000000050: BF8C0F71
  v_sub_f32     v4, 1.0, v8                             // 000000000054: 080810F2
  s_waitcnt     vmcnt(0) & lgkmcnt(15)                  // 000000000058: BF8C0F70
  v_mad_legacy_f32  v3, v3, v4, v8                      // 00000000005C: D2800003 04220903
  v_rcp_f32     v4, v3                                  // 000000000064: 7E085503
  v_mul_f32     v4, v8, v4                              // 000000000068: 10080908
  v_sub_f32     v5, v5, v0                              // 00000000006C: 080A0105
  v_sub_f32     v6, v6, v1                              // 000000000070: 080C0306
  v_sub_f32     v7, v7, v2                              // 000000000074: 080E0507
  v_mac_legacy_f32  v0, v4, v5                          // 000000000078: 0C000B04
  v_mac_legacy_f32  v1, v4, v6                          // 00000000007C: 0C020D04
  v_mac_legacy_f32  v2, v4, v7                          // 000000000080: 0C040F04
  s_mov_b64     exec, s[28:29]                          // 000000000084: BEFE041C
  v_cvt_pkrtz_f16_f32  v0, v0, v1                       // 000000000088: 5E000300
  v_cvt_pkrtz_f16_f32  v1, v2, v3                       // 00000000008C: 5E020702
  exp           mrt0, v0, v0, v1, v1 compr vm           // 000000000090: F800140F 00000100
  s_endpgm                                              // 000000000098: BF810000
end


// Approximately 9 instruction slots used
#endif

const BYTE pBinary_D3D1xFL11X_FDrawableCopyPixelsMergeAlpha[] =
{
     68,  88,  66,  67,  24,  93, 
     51, 121,  55,  29, 116, 220, 
    132, 111,  75,  46, 183,   5, 
     42, 237,   1,   0,   0,   0, 
    168,   6,   0,   0,   5,   0, 
      0,   0,  52,   0,   0,   0, 
     96,   1,   0,   0, 172,   1, 
      0,   0, 224,   1,   0,   0, 
     12,   6,   0,   0,  82,  68, 
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
     83,  72,  69,  88,  36,   4, 
      0,   0,  80,   0,   0,   0, 
      9,   1,   0,   0,  53,  16, 
      0,   0,   5,   0,   0,   0, 
    102,   0,   0,   0,  80,   0, 
      0,   0,  97,   0,   0,   0, 
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
    104,   0,   0,   2,   3,   0, 
      0,   0,  69,   0,   0, 139, 
    194,   0,   0, 128,  67,  85, 
     21,   0, 242,   0,  16,   0, 
      0,   0,   0,   0, 230,  26, 
     16,   0,   0,   0,   0,   0, 
     70, 126,  16,   0,   1,   0, 
      0,   0,   0,  96,  16,   0, 
      1,   0,   0,   0,   0,   0, 
      0,   8,  18,   0,  16,   0, 
      1,   0,   0,   0,  58,   0, 
     16, 128,  65,   0,   0,   0, 
      0,   0,   0,   0,   1,  64, 
      0,   0,   0,   0, 128,  63, 
     69,   0,   0, 139, 194,   0, 
      0, 128,  67,  85,  21,   0, 
    242,   0,  16,   0,   2,   0, 
      0,   0,  70,  16,  16,   0, 
      0,   0,   0,   0,  70, 126, 
     16,   0,   0,   0,   0,   0, 
      0,  96,  16,   0,   0,   0, 
      0,   0,  50,   0,   0,   9, 
     18,   0,  16,   0,   1,   0, 
      0,   0,  58,   0,  16,   0, 
      2,   0,   0,   0,  10,   0, 
     16,   0,   1,   0,   0,   0, 
     58,   0,  16,   0,   0,   0, 
      0,   0,  14,   0,   0,   7, 
    130,   0,  16,   0,   0,   0, 
      0,   0,  58,   0,  16,   0, 
      0,   0,   0,   0,  10,   0, 
     16,   0,   1,   0,   0,   0, 
     54,   0,   0,   5, 130,  32, 
     16,   0,   0,   0,   0,   0, 
     10,   0,  16,   0,   1,   0, 
      0,   0,   0,   0,   0,   8, 
    114,   0,  16,   0,   0,   0, 
      0,   0,  70,   2,  16,   0, 
      0,   0,   0,   0,  70,   2, 
     16, 128,  65,   0,   0,   0, 
      2,   0,   0,   0,  50,   0, 
      0,   9, 114,  32,  16,   0, 
      0,   0,   0,   0, 246,  15, 
     16,   0,   0,   0,   0,   0, 
     70,   2,  16,   0,   0,   0, 
      0,   0,  70,   2,  16,   0, 
      2,   0,   0,   0,  62,   0, 
      0,   1,  53,  16,   0,   0, 
    163,   0,   0,   0,  21,   0, 
      1,   0,  41, 144,  10,   0, 
    120,   1, 237,  86, 207, 107, 
     19,  65,  20, 126, 251,  35, 
    201,  38, 221, 116, 115, 232, 
    161,  98,  81, 136,  45, 214, 
     75,  32,  81,  16,  60,  85, 
     13,  98,  17,  65,  44, 122, 
     40,  75, 233, 166, 217,  52, 
    165, 219,  36, 110, 130,  70, 
    145,  52,   5, 111, 230, 208, 
     63, 160,  71, 241, 212, 127, 
    193,  36, 136, 202, 226, 209, 
    131, 103,  79, 158, 197, 147, 
      7, 169, 190, 111, 119,  54, 
     93, 196, 222,   2,  21, 204, 
      7, 223, 123, 243, 237, 155, 
    121, 243, 102,   6, 102, 118, 
     63,  69,  62, 150,  95,  23, 
     94, 194, 127,  46,  60,  60, 
    251,  66,  34, 138, 113, 155, 
    157, 239, 129, 182,   6,  75, 
     84,  78, 194,  18,  45, 194, 
    160, 131,  76, 148, 102, 119, 
    142, 121, 192,  92,  98,  34, 
     22,  50, 192, 210, 176, 203, 
    118, 134,  57, 205, 204, 242, 
    152, 219, 236, 207,  51,  85, 
     38, 190, 235,  76, 164,   3, 
     56,  60,  66,  56,  63, 250, 
     69, 129, 190,  40,  61,  28, 
    131, 242,  20,  38,   0,  31, 
    205,  65,  11,  48, 127, 199, 
      5,  38, 234,  63,   9, 168, 
     45, 204, 133, 185,  80,  39, 
    128, 245, 158,  54, 130, 181, 
      7, 213, 133, 107,  15, 207, 
      9, 192, 121,  68, 129, 189, 
     15, 198, 240, 186, 196,  57, 
    134, 251,  58,  47, 116, 120, 
    150,  67, 230,  73, 248, 197, 
    152,  69,  35, 130,  80,  35, 
    127, 156,  12, 182, 255,  47, 
    194,  61, 197,  94, 224, 116, 
     50, 204, 232, 142,  32, 158, 
    101,  18, 117,   7, 224, 254, 
     44, 253, 128,  10, 181, 244, 
    135,  86, 160, 145,  44, 170, 
    113, 224,  10, 173, 201,   9, 
    121, 205, 152,  19, 113, 133, 
     76, 214, 102,  68, 175, 178, 
     94, 141, 232, 117, 214, 235, 
     17,  93,  98,  93,  26, 105, 
    145, 223, 240, 231, 147,  21, 
    156,  51, 127, 236, 252, 203, 
    156, 212,  56,  30,  78, 106, 
     28,  15,  39,  53, 142, 141, 
     62,  58, 234,  65, 191, 147, 
     58, 234, 203, 202, 207, 190, 
     38, 189,  29, 102, 164,  15, 
     67, 146,  53, 143,  20, 221, 
    147, 228, 164,  39,  41, 105, 
    111, 151, 122,   3,  50, 186, 
    223, 228, 216,  30,  95, 186, 
     31, 249, 233, 154, 247,  72, 
     90, 244,  36,  90, 240,  36, 
    233, 210,  40, 158,  36, 155, 
     30,  25, 189, 193, 247, 140, 
    166,  53, 216,  43, 212, 253, 
    164,  36, 179, 170, 242,  64, 
    235, 104,  73,  45,  19, 147, 
     82,  90,  92, 209, 181,  68, 
    108,  90,  83, 167,  72,  87, 
    211, 178, 174,  26, 170,  62, 
    167,  30, 245, 249, 174,  12, 
    238, 218, 153, 240, 174, 220, 
     27, 156,  97, 235,  55,  25, 
    193,  43, 124,  12, 126, 134, 
    253,  16,  12, 158, 100, 104, 
    255,   3,   3, 239,   1, 191, 
      1, 190,   6, 241, 143,   1, 
    221,  19, 111, 240, 115, 225, 
     69,  63, 126, 183, 209, 235, 
     24, 135,  34, 254,  74, 120, 
    228, 159,  98, 198, 151,  15, 
     47, 222, 120, 127, 245, 141, 
     39, 190, 191,  19,  30,  57, 
    240, 175, 244, 213,  40, 238, 
    222,  65, 131, 241,  69, 196, 
    182, 153,  24,  95, 185, 102, 
    150,  44, 199, 122, 214, 204, 
    181, 234, 110, 125,  59, 215, 
    108, 185, 182, 181,  83,  48, 
    203, 246,  99, 211, 110, 183, 
    108, 183, 102,  57, 230, 102, 
    165, 125, 197,  92, 113,  55, 
    204, 251, 118, 173, 108, 187, 
    102, 241, 114,  49, 223,  54, 
     87, 170,  22, 139, 166, 121, 
    171, 232,  90,  79, 172, 146, 
     99, 223, 172,  55, 158, 222, 
    219, 106, 219,  78, 243, 174, 
    237, 110, 218, 215, 157,  70, 
    213, 202,  85, 156, 124,  62, 
     87, 117, 154,  14,  79,  22, 
     96,  67, 148, 130,  95, 188, 
      4, 115, 199, 218, 170, 209, 
    111, 179, 166,  46,  78,   0, 
     83,  84,  65,  84, 148,   0, 
      0,   0,   9,   0,   0,   0, 
      3,   0,   0,   0,   0,   0, 
      0,   0,   3,   0,   0,   0, 
      5,   0,   0,   0,   0,   0, 
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
