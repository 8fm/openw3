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
// COLOR                    0   xyzw        0     NONE   float   x  w
// TEXCOORD                 0   xyzw        1     NONE   float      w
// TEXCOORD                 1   xyzw        2     NONE   float      w
// TEXCOORD                 2   xy          3     NONE   float   xy  
// TEXCOORD                 3     zw        3     NONE   float     zw
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
// First Precompiled Shader at offset:[105]
// Embedded Data:
//  0x00000069 - Offset To First Precompiled Shader
//  0x00000050 - Original Shader Descriptor
//  0x00000064 - Original Shader Size
dcl_globalFlags refactoringAllowed
dcl_sampler s0, mode_default
dcl_sampler s1, mode_default
dcl_resource_texture2d (float,float,float,float) t0
dcl_resource_texture2d (float,float,float,float) t1
dcl_input_ps linear v0.xw
dcl_input_ps linear v1.w
dcl_input_ps linear v2.w
dcl_input_ps linear v3.xy
dcl_input_ps linear v3.zw
dcl_output o0.xyzw
dcl_temps 1
sample_indexable(texture2d)(float,float,float,float) r0.x, v3.xyxx, t0.wxyz, s0
sample_indexable(texture2d)(float,float,float,float) r0.y, v3.zwzz, t1.xwyz, s1
add r0.x, -r0.y, r0.x
mad r0.x, v0.x, r0.x, r0.y
mul r0.x, r0.x, v2.w
mad r0.x, v1.w, r0.x, r0.x
mul o0.xyzw, r0.xxxx, v0.wwww
ret 
Opaque Custom Data - XBOX Precompiled Shader
// Offset:[105], bundle is:[160] DWORDS.
// Disassembled Precompiled Shader Object:
// Shader Type               : (PS)
// Shader Hw Stage           : (PS)
// HLSL Source Filename      : f:\balazs.torok.stream2\dev\external\gfx4\Src\Render\D3D1x\Shaders\FTexTGTexTGCxformAcEAlphaInv.fl11.hlsl
// HLSL Entrypoint Name      : main

//
// Shader Data:
// ----------------- PS Data ------------------------
; Input Semantic Mappings
;   [0] generic, usageIdx  0, channelMask  9, param0, paramSlot 0, DefaultVal={0,0,0,0};   [1] generic, usageIdx  1, channelMask  8, param1, paramSlot 1, DefaultVal={0,0,0,0};   [2] generic, usageIdx  2, channelMask  8, param2, paramSlot 2, DefaultVal={0,0,0,0};   [3] generic, usageIdx  3, channelMask 15, param3, paramSlot 3, DefaultVal={0,0,0,0}

codeLenInByte        = 160;Bytes

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
; SPI_PS_IN_CONTROL       = 0x00000004
SPIC:NUM_INTERP             = 4
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
  s_load_dwordx8  s[20:27], s[0:1], 0x10                // 000000000010: C0CA0110
  v_interp_p1_f32  v2, v0, attr3.x                      // 000000000014: C8080C00
  v_interp_p1_f32  v3, v0, attr3.y                      // 000000000018: C80C0D00
  v_interp_p2_f32  v2, v1, attr3.x                      // 00000000001C: C8090C01
  v_interp_p2_f32  v3, v1, attr3.y                      // 000000000020: C80D0D01
  s_waitcnt     lgkmcnt(0)                              // 000000000024: BF8C007F
  image_sample  v[2:5], v[2:3], s[4:11], s[20:23] dmask:0x8 // 000000000028: F0800800 00A10202
  s_load_dwordx8  s[12:19], s[0:1], 0x08                // 000000000030: C0C60108
  v_interp_p1_f32  v4, v0, attr3.z                      // 000000000034: C8100E00
  v_interp_p1_f32  v5, v0, attr3.w                      // 000000000038: C8140F00
  v_interp_p2_f32  v4, v1, attr3.z                      // 00000000003C: C8110E01
  v_interp_p2_f32  v5, v1, attr3.w                      // 000000000040: C8150F01
  s_waitcnt     lgkmcnt(0)                              // 000000000044: BF8C007F
  image_sample  v[3:6], v[4:5], s[12:19], s[24:27] dmask:0x8 // 000000000048: F0800800 00C30304
  v_interp_p1_f32  v4, v0, attr0.x                      // 000000000050: C8100000
  v_interp_p2_f32  v4, v1, attr0.x                      // 000000000054: C8110001
  s_waitcnt     vmcnt(0) & lgkmcnt(15)                  // 000000000058: BF8C0F70
  v_sub_f32     v2, v2, v3                              // 00000000005C: 08040702
  v_interp_p1_f32  v5, v0, attr2.w                      // 000000000060: C8140B00
  v_mac_legacy_f32  v3, v4, v2                          // 000000000064: 0C060504
  v_interp_p1_f32  v2, v0, attr1.w                      // 000000000068: C8080700
  v_interp_p2_f32  v5, v1, attr2.w                      // 00000000006C: C8150B01
  v_interp_p1_f32  v0, v0, attr0.w                      // 000000000070: C8000300
  v_interp_p2_f32  v2, v1, attr1.w                      // 000000000074: C8090701
  v_mul_legacy_f32  v3, v3, v5                          // 000000000078: 0E060B03
  v_interp_p2_f32  v0, v1, attr0.w                      // 00000000007C: C8010301
  v_mac_legacy_f32  v3, v2, v3                          // 000000000080: 0C060702
  v_mul_legacy_f32  v0, v3, v0                          // 000000000084: 0E000103
  s_mov_b64     exec, s[28:29]                          // 000000000088: BEFE041C
  v_cvt_pkrtz_f16_f32  v0, v0, v0                       // 00000000008C: 5E000100
  s_nop         0x0000                                  // 000000000090: BF800000
  exp           mrt0, v0, v0, v0, v0 compr vm           // 000000000094: F800140F 00000000
  s_endpgm                                              // 00000000009C: BF810000
end


// Approximately 8 instruction slots used
#endif

const BYTE pBinary_D3D1xFL11X_FTexTGTexTGCxformAcEAlphaInv[] =
{
     68,  88,  66,  67, 176,  38, 
     39, 137, 159, 227,  72, 211, 
     69, 153, 181, 212,  44, 235, 
     27,  27,   1,   0,   0,   0, 
    252,   6,   0,   0,   5,   0, 
      0,   0,  52,   0,   0,   0, 
     96,   1,   0,   0, 248,   1, 
      0,   0,  44,   2,   0,   0, 
     96,   6,   0,   0,  82,  68, 
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
     71,  78, 144,   0,   0,   0, 
      5,   0,   0,   0,   8,   0, 
      0,   0, 128,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   3,   0,   0,   0, 
      0,   0,   0,   0,  15,   9, 
      0,   0, 134,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   3,   0,   0,   0, 
      1,   0,   0,   0,  15,   8, 
      0,   0, 134,   0,   0,   0, 
      1,   0,   0,   0,   0,   0, 
      0,   0,   3,   0,   0,   0, 
      2,   0,   0,   0,  15,   8, 
      0,   0, 134,   0,   0,   0, 
      2,   0,   0,   0,   0,   0, 
      0,   0,   3,   0,   0,   0, 
      3,   0,   0,   0,   3,   3, 
      0,   0, 134,   0,   0,   0, 
      3,   0,   0,   0,   0,   0, 
      0,   0,   3,   0,   0,   0, 
      3,   0,   0,   0,  12,  12, 
      0,   0,  67,  79,  76,  79, 
     82,   0,  84,  69,  88,  67, 
     79,  79,  82,  68,   0, 171, 
     79,  83,  71,  78,  44,   0, 
      0,   0,   1,   0,   0,   0, 
      8,   0,   0,   0,  32,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   3,   0, 
      0,   0,   0,   0,   0,   0, 
     15,   0,   0,   0,  83,  86, 
     95,  84,  97, 114, 103, 101, 
    116,   0, 171, 171,  83,  72, 
     69,  88,  44,   4,   0,   0, 
     80,   0,   0,   0,  11,   1, 
      0,   0,  53,  16,   0,   0, 
      5,   0,   0,   0, 105,   0, 
      0,   0,  80,   0,   0,   0, 
    100,   0,   0,   0, 106,   8, 
      0,   1,  90,   0,   0,   3, 
      0,  96,  16,   0,   0,   0, 
      0,   0,  90,   0,   0,   3, 
      0,  96,  16,   0,   1,   0, 
      0,   0,  88,  24,   0,   4, 
      0, 112,  16,   0,   0,   0, 
      0,   0,  85,  85,   0,   0, 
     88,  24,   0,   4,   0, 112, 
     16,   0,   1,   0,   0,   0, 
     85,  85,   0,   0,  98,  16, 
      0,   3, 146,  16,  16,   0, 
      0,   0,   0,   0,  98,  16, 
      0,   3, 130,  16,  16,   0, 
      1,   0,   0,   0,  98,  16, 
      0,   3, 130,  16,  16,   0, 
      2,   0,   0,   0,  98,  16, 
      0,   3,  50,  16,  16,   0, 
      3,   0,   0,   0,  98,  16, 
      0,   3, 194,  16,  16,   0, 
      3,   0,   0,   0, 101,   0, 
      0,   3, 242,  32,  16,   0, 
      0,   0,   0,   0, 104,   0, 
      0,   2,   1,   0,   0,   0, 
     69,   0,   0, 139, 194,   0, 
      0, 128,  67,  85,  21,   0, 
     18,   0,  16,   0,   0,   0, 
      0,   0,  70,  16,  16,   0, 
      3,   0,   0,   0,  54, 121, 
     16,   0,   0,   0,   0,   0, 
      0,  96,  16,   0,   0,   0, 
      0,   0,  69,   0,   0, 139, 
    194,   0,   0, 128,  67,  85, 
     21,   0,  34,   0,  16,   0, 
      0,   0,   0,   0, 230,  26, 
     16,   0,   3,   0,   0,   0, 
    198, 121,  16,   0,   1,   0, 
      0,   0,   0,  96,  16,   0, 
      1,   0,   0,   0,   0,   0, 
      0,   8,  18,   0,  16,   0, 
      0,   0,   0,   0,  26,   0, 
     16, 128,  65,   0,   0,   0, 
      0,   0,   0,   0,  10,   0, 
     16,   0,   0,   0,   0,   0, 
     50,   0,   0,   9,  18,   0, 
     16,   0,   0,   0,   0,   0, 
     10,  16,  16,   0,   0,   0, 
      0,   0,  10,   0,  16,   0, 
      0,   0,   0,   0,  26,   0, 
     16,   0,   0,   0,   0,   0, 
     56,   0,   0,   7,  18,   0, 
     16,   0,   0,   0,   0,   0, 
     10,   0,  16,   0,   0,   0, 
      0,   0,  58,  16,  16,   0, 
      2,   0,   0,   0,  50,   0, 
      0,   9,  18,   0,  16,   0, 
      0,   0,   0,   0,  58,  16, 
     16,   0,   1,   0,   0,   0, 
     10,   0,  16,   0,   0,   0, 
      0,   0,  10,   0,  16,   0, 
      0,   0,   0,   0,  56,   0, 
      0,   7, 242,  32,  16,   0, 
      0,   0,   0,   0,   6,   0, 
     16,   0,   0,   0,   0,   0, 
    246,  31,  16,   0,   0,   0, 
      0,   0,  62,   0,   0,   1, 
     53,  16,   0,   0, 162,   0, 
      0,   0,  21,   0,   1,   0, 
     41, 147,  10,   0, 120,   1, 
    237,  86, 207, 107,  19,  65, 
     20, 126, 179, 155, 110, 146, 
    230, 103,  49, 149,   8,  69, 
     20,  21, 114,  10, 164, 122, 
    234, 193, 182,  88, 149,  94, 
     60, 216, 158, 100,  17, 183, 
    237, 198, 168, 249,  81,  54, 
    161,   4, 193,  24, 193,  99, 
    133, 128,  87,  15, 245,  38, 
    130, 224,  85, 144,  36,  84, 
    208,  65, 252,   3, 252,   3, 
    188, 235,  73,  60,  72, 244, 
    125, 155, 153, 152, 148, 246, 
     22,  80,  36,  31, 124, 239, 
    237, 183, 111, 246, 237, 155, 
    153, 101, 223,  60, 157,  38, 
     31, 133, 114, 239,  45, 252, 
    227, 197, 207, 123,  45,  65, 
     52, 197, 215, 236, 124,  15, 
    212,  67, 176,  60,  46,  12, 
     75, 148, 129, 193,   0, 131, 
      8, 161, 147, 204,  61, 230, 
     18,  19,  49, 205,  62, 150, 
    186,  79, 216, 166, 152,  22, 
    115, 129, 159,  89, 101, 127, 
    154, 105,  50, 143,  51,  35, 
     76, 164,   3,  56,  60,  64, 
      0, 134, 161, 253,  48, 144, 
     75,  35, 201,  68,  46,   0, 
    126,  56,   7, 157, 131,  57, 
     28, 103, 153, 106, 106, 135, 
      2, 181, 233,  92, 168,  15, 
    117,   2, 152, 239, 223,  70, 
    127, 189, 250,  86, 207,  93, 
    239,  19, 128, 253,  24,  70, 
    156, 217,  31,  77, 148,  86, 
    251, 168, 215,  53, 163,  52, 
     30, 199, 220, 186, 204, 163, 
    240, 139, 145, 198, 197,  16, 
    180,  70,  62, 139, 250, 201, 
     44, 145, 166,  83, 240,  70, 
    198, 255,  46,  44, 115, 145, 
    110, 177, 255, 223, 161, 215, 
     20,  30, 223,  14, 190, 205, 
      4,  83,   3, 247, 207,  48, 
    137, 154,  29, 176, 149, 166, 
     31,  80,  90, 243, 178, 141, 
    104, 243,  40,  45, 232,  38, 
    116,  98, 110, 160, 237,   3, 
    250, 198,   1, 205, 203,  63, 
    162,  55,  70,  52,  95, 251, 
     90, 229, 199,  62, 179, 107, 
    252, 203, 156, 212,  56,  30, 
     78, 106,  28,  15,  39,  53, 
    142, 141,  62,  26, 129, 103, 
    237, 198, 116, 175, 109, 152, 
     63, 219,  36, 246, 187,  73, 
    241, 177,  75, 209, 144, 164, 
     88,  84, 138, 104,  88, 138, 
     88,  76,  62, 164, 221,  14, 
    133, 154, 223,  12, 227,  57, 
    133, 196, 251,  46, 197, 147, 
    146,  18,  41,  41, 226,  51, 
     82,  36, 102,   7, 241, 128, 
    249, 142,  51,  38, 165, 160, 
     25, 185, 157, 216, 237,  24, 
    193,  64, 136,  34,  41,  25, 
    152, 178, 162,  20,  12,  73, 
     17, 153, 149, 220,  72, 165, 
      8, 134, 165,  25, 177, 226, 
    194,  20, 210,   8,  90,  81, 
     83,  80, 124,  46, 208, 227, 
    247, 171, 255, 109,  74, 255, 
     47,  31, 117,  78, 176, 213, 
     61,  85, 159,  19,  52, 184, 
    151, 250,  33,  24, 244,  85, 
    104, 255,   6,   3,  61, 129, 
    251, 128, 175,  65, 156, 141, 
    160,  91, 170,  15,  55, 149, 
     87, 227, 184, 119,  99, 212, 
     31, 188,  86, 241,  23, 202, 
     35,  63, 206,  38,  31,  30, 
    188, 244,  94, 237,  95,  60, 
    246,  73, 221, 151, 202,  35, 
      7, 142, 122,  95, 223, 124, 
    159, 191, 166, 206, 124,  95, 
     84, 236,  46,  19, 207, 231, 
     23, 236,  13, 167, 232, 220, 
    175, 102, 107,  21, 175, 114, 
     47,  91, 173, 121, 174,  83, 
    154, 183, 183, 220,  29, 219, 
    173, 215,  92, 175, 236,  20, 
    237, 219, 249, 250,   5, 123, 
    205, 219, 180, 175, 187, 229, 
     45, 215, 179,  87, 206, 175, 
    228, 234, 246,  90, 193,  97, 
     81, 181, 175, 172, 187, 245, 
    245, 171, 190, 185,  84, 207, 
     87, 188, 210, 242, 230, 229, 
    229, 226, 118, 193,  89,  45, 
    239, 100, 243, 197,  92,  46, 
     91,  40,  86, 139, 252, 174, 
     62, 242, 170,  18, 156,  50, 
    131, 204, 146, 115, 167,  76, 
    191,   1, 230, 131,  57,  70, 
     83,  84,  65,  84, 148,   0, 
      0,   0,   8,   0,   0,   0, 
      1,   0,   0,   0,   0,   0, 
      0,   0,   6,   0,   0,   0, 
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
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0
};
