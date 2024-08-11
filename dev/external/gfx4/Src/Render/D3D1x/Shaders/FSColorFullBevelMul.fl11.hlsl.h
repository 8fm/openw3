#if 0
//
// Generated by Microsoft (R) HLSL Shader Compiler For Durango 9.30.12098.0
//
//
///
// Buffer Definitions: 
//
// cbuffer Constants
// {
//
//   float4 fsize;                      // Offset:    0 Size:    16
//   float4 offset;                     // Offset:   16 Size:    16
//   float4 scolor;                     // Offset:   32 Size:    16
//   float4 srctexscale;                // Offset:   48 Size:    16
//   float4 texscale;                   // Offset:   64 Size:    16
//
// }
//
//
// Resource Bindings:
//
// Name                                 Type  Format         Dim Slot Elements
// ------------------------------ ---------- ------- ----------- ---- --------
// sampler_srctex                    sampler      NA          NA    0        1
// sampler_tex                       sampler      NA          NA    1        1
// srctex                            texture  float4          2d    0        1
// tex                               texture  float4          2d    1        1
// Constants                         cbuffer      NA          NA    0        1
//
//
//
// Input signature:
//
// Name                 Index   Mask Register SysValue  Format   Used
// -------------------- ----- ------ -------- -------- ------- ------
// TEXCOORD                 0   xyzw        0     NONE   float   xyzw
// TEXCOORD                 1   xyzw        1     NONE   float   xyzw
// TEXCOORD                 2   xy          2     NONE   float   xy  
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
// First Precompiled Shader at offset:[274]
// Embedded Data:
//  0x00000112 - Offset To First Precompiled Shader
//  0x00000050 - Original Shader Descriptor
//  0x0000010d - Original Shader Size
dcl_globalFlags refactoringAllowed
dcl_constantbuffer cb0[5], immediateIndexed
dcl_sampler s0, mode_default
dcl_sampler s1, mode_default
dcl_resource_texture2d (float,float,float,float) t0
dcl_resource_texture2d (float,float,float,float) t1
dcl_input_ps linear v0.xyzw
dcl_input_ps linear v1.xyzw
dcl_input_ps linear v2.xy
dcl_output o0.xyzw
dcl_temps 3
mov r0.x, l(0)
mov r0.y, -cb0[0].x
loop 
  lt r0.z, cb0[0].x, r0.y
  breakc_nz r0.z
  add r1.x, r0.y, cb0[1].x
  mov r2.x, r0.x
  mov r2.y, -cb0[0].y
  loop 
    lt r0.z, cb0[0].y, r2.y
    breakc_nz r0.z
    add r1.y, r2.y, cb0[1].y
    mad r0.zw, r1.xxxy, cb0[4].xxxy, v2.xxxy
    sample_l_indexable(texture2d)(float,float,float,float) r0.z, r0.zwzz, t1.xywz, s1, l(0.000000)
    add r2.x, r0.z, r2.x
    add r2.y, r2.y, l(1.000000)
  endloop 
  mov r0.x, r2.x
  add r0.y, r0.y, l(1.000000)
endloop 
mul r0.x, r0.x, cb0[0].w
mul_sat r0.x, r0.x, cb0[0].z
mul r0.yz, v2.xxyx, cb0[3].xxyx
sample_l_indexable(texture2d)(float,float,float,float) r1.xyzw, r0.yzyy, t0.xyzw, s0, l(0.000000)
mul r0.y, r0.x, cb0[2].w
mad r0.x, -r0.x, cb0[2].w, l(1.000000)
mul r1.xyzw, r0.xxxx, r1.xyzw
mad r0.xyzw, cb0[2].xyzw, r0.yyyy, r1.xyzw
mov r1.xyz, v1.xyzx
mov r1.w, l(1.000000)
mul r0.xyzw, r0.xyzw, r1.xyzw
mul r0.xyzw, r0.xyzw, v1.wwww
mad r0.xyzw, v0.xyzw, r0.wwww, r0.xyzw
mul o0.xyz, r0.wwww, r0.xyzx
mov o0.w, r0.w
ret 
Opaque Custom Data - XBOX Precompiled Shader
// Offset:[274], bundle is:[210] DWORDS.
// Disassembled Precompiled Shader Object:
// Shader Type               : (PS)
// Shader Hw Stage           : (PS)
// HLSL Source Filename      : f:\balazs.torok.stream2\dev\external\gfx4\Src\Render\D3D1x\Shaders\FSColorFullBevelMul.fl11.hlsl
// HLSL Entrypoint Name      : main

//
// Shader Data:
// ----------------- PS Data ------------------------
; Input Semantic Mappings
;   [0] generic, usageIdx  0, channelMask 15, param0, paramSlot 0, DefaultVal={0,0,0,0};   [1] generic, usageIdx  1, channelMask 15, param1, paramSlot 1, DefaultVal={0,0,0,0};   [2] generic, usageIdx  2, channelMask  3, param2, paramSlot 2, DefaultVal={0,0,0,0}

codeLenInByte        = 412;Bytes

; launchModeFlags    = 0x0000000E
srdTableSupport      = TRUE
immediateSupportExt  = TRUE
srdTableSupportExt   = TRUE

userElementCount     = 1;
;  userElements[0]      = PTR_EXTENDED_USER_DATA, ReferencedExtUserElements=1, s[0:1]
extUserElementCount  = 5;
;  extUserElements 1[0] = IMM_RESOURCE, 0, offset 0:7 dwords
;  extUserElements 1[1] = IMM_RESOURCE, 1, offset 8:15 dwords
;  extUserElements 1[2] = IMM_SAMPLER, 0, offset 16:19 dwords
;  extUserElements 1[3] = IMM_SAMPLER, 1, offset 20:23 dwords
;  extUserElements 1[4] = IMM_CONST_BUFFER, 0, offset 24:27 dwords
NumVgprs             = 13;
NumSgprs             = 26;
FloatMode            = 192;
IeeeMode             = 0;
bFlatPtr32           = 0;
ScratchSize          = 0 dwords/thread;
LDSByteSize          = 0 bytes/workgroup (compile time only);
ScratchWaveOffsetReg = s65535;
; texSamplerUsage         = 0x00000003
; constBufUsage           = 0x00000001

; SPI_SHADER_PGM_RSRC2_PS = 0x00000004
SSPRP:SCRATCH_EN            = 0
SSPRP:USER_SGPR             = 2
SSPRP:TRAP_PRESENT          = 0
SSPRP:WAVE_CNT_EN           = 0
SSPRP:EXTRA_LDS_SIZE        = 0
SSPRP:EXCP_EN               = 0
; SPI_SHADER_Z_FORMAT     = 0x00000000
SPZF:Z_EXPORT_FORMAT        = 0; SPI_SHADER_ZERO
; SPI_PS_IN_CONTROL       = 0x00000003
SPIC:NUM_INTERP             = 3
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
  s_load_dwordx8  s[4:11], s[0:1], 0x14                 // 000000000004: C0C20114
  s_waitcnt     lgkmcnt(0)                              // 000000000008: BF8C007F
  s_buffer_load_dwordx4  s[12:15], s[8:11], 0x00        // 00000000000C: C2860900
  v_mov_b32     v2, 0                                   // 000000000010: 7E040280
  s_load_dwordx8  s[16:23], s[0:1], 0x08                // 000000000014: C0C80108
  s_waitcnt     lgkmcnt(0)                              // 000000000018: BF8C007F
  v_max_f32     v3, -s12, -s12                          // 00000000001C: D2200003 6000180C
label_0009:
  v_mov_b32     v4, s12                                 // 000000000024: 7E08020C
  v_cmp_gt_f32  vcc, v3, v4                             // 000000000028: 7C080903
  s_cbranch_vccnz  label_0027                           // 00000000002C: BF87001B
  s_buffer_load_dwordx2  s[2:3], s[8:11], 0x04          // 000000000030: C2410904
  s_waitcnt     lgkmcnt(0)                              // 000000000034: BF8C007F
  v_add_f32     v4, s2, v3                              // 000000000038: 06080602
  v_max_f32     v5, -s13, -s13                          // 00000000003C: D2200005 60001A0D
label_0011:
  v_mov_b32     v6, s13                                 // 000000000044: 7E0C020D
  v_cmp_gt_f32  vcc, v5, v6                             // 000000000048: 7C080D05
  s_cbranch_vccnz  label_0025                           // 00000000004C: BF870011
  s_buffer_load_dwordx2  s[24:25], s[8:11], 0x10        // 000000000050: C24C0910
  v_interp_p1_f32  v7, v0, attr2.x                      // 000000000054: C81C0800
  v_interp_p1_f32  v8, v0, attr2.y                      // 000000000058: C8200900
  v_interp_p2_f32  v7, v1, attr2.x                      // 00000000005C: C81D0801
  v_interp_p2_f32  v8, v1, attr2.y                      // 000000000060: C8210901
  v_add_f32     v6, s3, v5                              // 000000000064: 060C0A03
  s_waitcnt     lgkmcnt(0)                              // 000000000068: BF8C007F
  v_mad_legacy_f32  v9, v4, s24, v7                     // 00000000006C: D2800009 041C3104
  v_mad_legacy_f32  v10, v6, s25, v8                    // 000000000074: D280000A 04203306
  image_sample_lz  v[6:9], v[9:10], s[16:23], s[4:7] dmask:0x8 // 00000000007C: F09C0800 00240609
  s_waitcnt     vmcnt(0) & lgkmcnt(15)                  // 000000000084: BF8C0F70
  v_add_f32     v2, v2, v6                              // 000000000088: 06040D02
  v_add_f32     v5, 1.0, v5                             // 00000000008C: 060A0AF2
  s_branch      label_0011                              // 000000000090: BF82FFEC
label_0025:
  v_add_f32     v3, 1.0, v3                             // 000000000094: 060606F2
  s_branch      label_0009                              // 000000000098: BF82FFE2
label_0027:
  s_buffer_load_dwordx2  s[2:3], s[8:11], 0x0c          // 00000000009C: C241090C
  v_interp_p1_f32  v3, v0, attr2.x                      // 0000000000A0: C80C0800
  v_interp_p1_f32  v4, v0, attr2.y                      // 0000000000A4: C8100900
  s_load_dwordx8  s[16:23], s[0:1], 0x00                // 0000000000A8: C0C80100
  s_load_dwordx4  s[4:7], s[0:1], 0x10                  // 0000000000AC: C0820110
  v_interp_p2_f32  v3, v1, attr2.x                      // 0000000000B0: C80D0801
  v_interp_p2_f32  v4, v1, attr2.y                      // 0000000000B4: C8110901
  s_waitcnt     lgkmcnt(0)                              // 0000000000B8: BF8C007F
  v_mul_legacy_f32  v3, s2, v3                          // 0000000000BC: 0E060602
  v_mul_legacy_f32  v4, s3, v4                          // 0000000000C0: 0E080803
  image_sample_lz  v[3:6], v[3:4], s[16:23], s[4:7] dmask:0xf // 0000000000C4: F09C0F00 00240303
  s_buffer_load_dwordx4  s[0:3], s[8:11], 0x08          // 0000000000CC: C2800908
  v_mul_legacy_f32  v2, s15, v2                         // 0000000000D0: 0E04040F
  v_mul_legacy_f32  v2, s14, v2 clamp                   // 0000000000D4: D20E0802 0002040E
  v_interp_p1_f32  v8, v0, attr1.x                      // 0000000000DC: C8200400
  s_waitcnt     lgkmcnt(0)                              // 0000000000E0: BF8C007F
  v_mad_legacy_f32  v7, -v2, s3, 1.0                    // 0000000000E4: D2800007 23C80702
  v_interp_p1_f32  v9, v0, attr1.y                      // 0000000000EC: C8240500
  v_mul_legacy_f32  v2, s3, v2                          // 0000000000F0: 0E040403
  s_waitcnt     vmcnt(0) & lgkmcnt(15)                  // 0000000000F4: BF8C0F70
  v_mul_legacy_f32  v3, v3, v7                          // 0000000000F8: 0E060F03
  v_mul_legacy_f32  v4, v4, v7                          // 0000000000FC: 0E080F04
  v_mul_legacy_f32  v5, v5, v7                          // 000000000100: 0E0A0F05
  v_interp_p1_f32  v10, v0, attr1.z                     // 000000000104: C8280600
  v_mul_legacy_f32  v6, v6, v7                          // 000000000108: 0E0C0F06
  v_interp_p2_f32  v8, v1, attr1.x                      // 00000000010C: C8210401
  v_mac_legacy_f32  v3, s0, v2                          // 000000000110: 0C060400
  v_mac_legacy_f32  v4, s1, v2                          // 000000000114: 0C080401
  v_mac_legacy_f32  v5, s2, v2                          // 000000000118: 0C0A0402
  v_interp_p2_f32  v9, v1, attr1.y                      // 00000000011C: C8250501
  v_mac_legacy_f32  v6, s3, v2                          // 000000000120: 0C0C0403
  v_interp_p2_f32  v10, v1, attr1.z                     // 000000000124: C8290601
  v_mul_legacy_f32  v2, v3, v8                          // 000000000128: 0E041103
  v_mul_legacy_f32  v3, v4, v9                          // 00000000012C: 0E061304
  v_interp_p1_f32  v7, v0, attr1.w                      // 000000000130: C81C0700
  v_mul_legacy_f32  v4, v5, v10                         // 000000000134: 0E081505
  v_interp_p1_f32  v5, v0, attr0.x                      // 000000000138: C8140000
  v_interp_p1_f32  v8, v0, attr0.y                      // 00000000013C: C8200100
  v_interp_p1_f32  v9, v0, attr0.z                      // 000000000140: C8240200
  v_interp_p2_f32  v7, v1, attr1.w                      // 000000000144: C81D0701
  v_interp_p1_f32  v0, v0, attr0.w                      // 000000000148: C8000300
  v_interp_p2_f32  v5, v1, attr0.x                      // 00000000014C: C8150001
  v_interp_p2_f32  v8, v1, attr0.y                      // 000000000150: C8210101
  v_interp_p2_f32  v9, v1, attr0.z                      // 000000000154: C8250201
  v_mul_legacy_f32  v2, v2, v7                          // 000000000158: 0E040F02
  v_mul_legacy_f32  v3, v3, v7                          // 00000000015C: 0E060F03
  v_mul_legacy_f32  v4, v4, v7                          // 000000000160: 0E080F04
  v_mul_legacy_f32  v6, v6, v7                          // 000000000164: 0E0C0F06
  v_interp_p2_f32  v0, v1, attr0.w                      // 000000000168: C8010301
  v_mac_legacy_f32  v2, v5, v6                          // 00000000016C: 0C040D05
  v_mac_legacy_f32  v3, v8, v6                          // 000000000170: 0C060D08
  v_mac_legacy_f32  v4, v9, v6                          // 000000000174: 0C080D09
  v_mac_legacy_f32  v6, v0, v6                          // 000000000178: 0C0C0D00
  v_mul_legacy_f32  v0, v2, v6                          // 00000000017C: 0E000D02
  v_mul_legacy_f32  v1, v3, v6                          // 000000000180: 0E020D03
  v_mul_legacy_f32  v2, v4, v6                          // 000000000184: 0E040D04
  v_cvt_pkrtz_f16_f32  v0, v0, v1                       // 000000000188: 5E000300
  v_cvt_pkrtz_f16_f32  v1, v2, v6                       // 00000000018C: 5E020D02
  exp           mrt0, v0, v0, v1, v1 compr vm           // 000000000190: F800140F 00000100
  s_endpgm                                              // 000000000198: BF810000
end


// Approximately 36 instruction slots used
#endif

const BYTE pBinary_D3D1xFL11X_FSColorFullBevelMul[] =
{
     68,  88,  66,  67, 196, 188, 
    180,  73, 193,  10, 178,  62, 
     92, 205, 191,  78,   6, 170, 
     26, 114,   1,   0,   0,   0, 
    144,  11,   0,   0,   5,   0, 
      0,   0,  52,   0,   0,   0, 
    188,   2,   0,   0,  32,   3, 
      0,   0,  84,   3,   0,   0, 
    244,  10,   0,   0,  82,  68, 
     69,  70, 128,   2,   0,   0, 
      1,   0,   0,   0,  12,   1, 
      0,   0,   5,   0,   0,   0, 
     60,   0,   0,   0,   0,   5, 
    255, 255,   0, 129,   0,   0, 
     67,   2,   0,   0,  82,  68, 
     49,  49,  60,   0,   0,   0, 
     24,   0,   0,   0,  32,   0, 
      0,   0,  40,   0,   0,   0, 
     36,   0,   0,   0,  12,   0, 
      0,   0,   0,   0,   0,   0, 
    220,   0,   0,   0,   3,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      1,   0,   0,   0,   1,   0, 
      0,   0, 235,   0,   0,   0, 
      3,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   1,   0, 
      0,   0,   1,   0,   0,   0, 
      1,   0,   0,   0, 247,   0, 
      0,   0,   2,   0,   0,   0, 
      5,   0,   0,   0,   4,   0, 
      0,   0, 255, 255, 255, 255, 
      0,   0,   0,   0,   1,   0, 
      0,   0,  13,   0,   0,   0, 
    254,   0,   0,   0,   2,   0, 
      0,   0,   5,   0,   0,   0, 
      4,   0,   0,   0, 255, 255, 
    255, 255,   1,   0,   0,   0, 
      1,   0,   0,   0,  13,   0, 
      0,   0,   2,   1,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   1,   0,   0,   0, 
      0,   0,   0,   0, 115,  97, 
    109, 112, 108, 101, 114,  95, 
    115, 114,  99, 116, 101, 120, 
      0, 115,  97, 109, 112, 108, 
    101, 114,  95, 116, 101, 120, 
      0, 115, 114,  99, 116, 101, 
    120,   0, 116, 101, 120,   0, 
     67, 111, 110, 115, 116,  97, 
    110, 116, 115,   0,   2,   1, 
      0,   0,   5,   0,   0,   0, 
     36,   1,   0,   0,  80,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0, 236,   1, 
      0,   0,   0,   0,   0,   0, 
     16,   0,   0,   0,   2,   0, 
      0,   0, 252,   1,   0,   0, 
      0,   0,   0,   0, 255, 255, 
    255, 255,   0,   0,   0,   0, 
    255, 255, 255, 255,   0,   0, 
      0,   0,  32,   2,   0,   0, 
     16,   0,   0,   0,  16,   0, 
      0,   0,   2,   0,   0,   0, 
    252,   1,   0,   0,   0,   0, 
      0,   0, 255, 255, 255, 255, 
      0,   0,   0,   0, 255, 255, 
    255, 255,   0,   0,   0,   0, 
     39,   2,   0,   0,  32,   0, 
      0,   0,  16,   0,   0,   0, 
      2,   0,   0,   0, 252,   1, 
      0,   0,   0,   0,   0,   0, 
    255, 255, 255, 255,   0,   0, 
      0,   0, 255, 255, 255, 255, 
      0,   0,   0,   0,  46,   2, 
      0,   0,  48,   0,   0,   0, 
     16,   0,   0,   0,   2,   0, 
      0,   0, 252,   1,   0,   0, 
      0,   0,   0,   0, 255, 255, 
    255, 255,   0,   0,   0,   0, 
    255, 255, 255, 255,   0,   0, 
      0,   0,  58,   2,   0,   0, 
     64,   0,   0,   0,  16,   0, 
      0,   0,   2,   0,   0,   0, 
    252,   1,   0,   0,   0,   0, 
      0,   0, 255, 255, 255, 255, 
      0,   0,   0,   0, 255, 255, 
    255, 255,   0,   0,   0,   0, 
    102, 115, 105, 122, 101,   0, 
    102, 108, 111,  97, 116,  52, 
      0, 171, 171, 171,   1,   0, 
      3,   0,   1,   0,   4,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
    242,   1,   0,   0, 111, 102, 
    102, 115, 101, 116,   0, 115, 
     99, 111, 108, 111, 114,   0, 
    115, 114,  99, 116, 101, 120, 
    115,  99,  97, 108, 101,   0, 
    116, 101, 120, 115,  99,  97, 
    108, 101,   0,  77, 105,  99, 
    114, 111, 115, 111, 102, 116, 
     32,  40,  82,  41,  32,  72, 
     76,  83,  76,  32,  83, 104, 
     97, 100, 101, 114,  32,  67, 
    111, 109, 112, 105, 108, 101, 
    114,  32,  70, 111, 114,  32, 
     68, 117, 114,  97, 110, 103, 
    111,  32,  57,  46,  51,  48, 
     46,  49,  50,  48,  57,  56, 
     46,  48,   0, 171,  73,  83, 
     71,  78,  92,   0,   0,   0, 
      3,   0,   0,   0,   8,   0, 
      0,   0,  80,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   3,   0,   0,   0, 
      0,   0,   0,   0,  15,  15, 
      0,   0,  80,   0,   0,   0, 
      1,   0,   0,   0,   0,   0, 
      0,   0,   3,   0,   0,   0, 
      1,   0,   0,   0,  15,  15, 
      0,   0,  80,   0,   0,   0, 
      2,   0,   0,   0,   0,   0, 
      0,   0,   3,   0,   0,   0, 
      2,   0,   0,   0,   3,   3, 
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
     83,  72,  69,  88, 152,   7, 
      0,   0,  80,   0,   0,   0, 
    230,   1,   0,   0,  53,  16, 
      0,   0,   5,   0,   0,   0, 
     18,   1,   0,   0,  80,   0, 
      0,   0,  13,   1,   0,   0, 
    106,   8,   0,   1,  89,   0, 
      0,   4,  70, 142,  32,   0, 
      0,   0,   0,   0,   5,   0, 
      0,   0,  90,   0,   0,   3, 
      0,  96,  16,   0,   0,   0, 
      0,   0,  90,   0,   0,   3, 
      0,  96,  16,   0,   1,   0, 
      0,   0,  88,  24,   0,   4, 
      0, 112,  16,   0,   0,   0, 
      0,   0,  85,  85,   0,   0, 
     88,  24,   0,   4,   0, 112, 
     16,   0,   1,   0,   0,   0, 
     85,  85,   0,   0,  98,  16, 
      0,   3, 242,  16,  16,   0, 
      0,   0,   0,   0,  98,  16, 
      0,   3, 242,  16,  16,   0, 
      1,   0,   0,   0,  98,  16, 
      0,   3,  50,  16,  16,   0, 
      2,   0,   0,   0, 101,   0, 
      0,   3, 242,  32,  16,   0, 
      0,   0,   0,   0, 104,   0, 
      0,   2,   3,   0,   0,   0, 
     54,   0,   0,   5,  18,   0, 
     16,   0,   0,   0,   0,   0, 
      1,  64,   0,   0,   0,   0, 
      0,   0,  54,   0,   0,   7, 
     34,   0,  16,   0,   0,   0, 
      0,   0,  10, 128,  32, 128, 
     65,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
     48,   0,   0,   1,  49,   0, 
      0,   8,  66,   0,  16,   0, 
      0,   0,   0,   0,  10, 128, 
     32,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,  26,   0, 
     16,   0,   0,   0,   0,   0, 
      3,   0,   4,   3,  42,   0, 
     16,   0,   0,   0,   0,   0, 
      0,   0,   0,   8,  18,   0, 
     16,   0,   1,   0,   0,   0, 
     26,   0,  16,   0,   0,   0, 
      0,   0,  10, 128,  32,   0, 
      0,   0,   0,   0,   1,   0, 
      0,   0,  54,   0,   0,   5, 
     18,   0,  16,   0,   2,   0, 
      0,   0,  10,   0,  16,   0, 
      0,   0,   0,   0,  54,   0, 
      0,   7,  34,   0,  16,   0, 
      2,   0,   0,   0,  26, 128, 
     32, 128,  65,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,  48,   0,   0,   1, 
     49,   0,   0,   8,  66,   0, 
     16,   0,   0,   0,   0,   0, 
     26, 128,  32,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
     26,   0,  16,   0,   2,   0, 
      0,   0,   3,   0,   4,   3, 
     42,   0,  16,   0,   0,   0, 
      0,   0,   0,   0,   0,   8, 
     34,   0,  16,   0,   1,   0, 
      0,   0,  26,   0,  16,   0, 
      2,   0,   0,   0,  26, 128, 
     32,   0,   0,   0,   0,   0, 
      1,   0,   0,   0,  50,   0, 
      0,  10, 194,   0,  16,   0, 
      0,   0,   0,   0,   6,   4, 
     16,   0,   1,   0,   0,   0, 
      6, 132,  32,   0,   0,   0, 
      0,   0,   4,   0,   0,   0, 
      6,  20,  16,   0,   2,   0, 
      0,   0,  72,   0,   0, 141, 
    194,   0,   0, 128,  67,  85, 
     21,   0,  66,   0,  16,   0, 
      0,   0,   0,   0, 230,  10, 
     16,   0,   0,   0,   0,   0, 
     70, 123,  16,   0,   1,   0, 
      0,   0,   0,  96,  16,   0, 
      1,   0,   0,   0,   1,  64, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   7,  18,   0, 
     16,   0,   2,   0,   0,   0, 
     42,   0,  16,   0,   0,   0, 
      0,   0,  10,   0,  16,   0, 
      2,   0,   0,   0,   0,   0, 
      0,   7,  34,   0,  16,   0, 
      2,   0,   0,   0,  26,   0, 
     16,   0,   2,   0,   0,   0, 
      1,  64,   0,   0,   0,   0, 
    128,  63,  22,   0,   0,   1, 
     54,   0,   0,   5,  18,   0, 
     16,   0,   0,   0,   0,   0, 
     10,   0,  16,   0,   2,   0, 
      0,   0,   0,   0,   0,   7, 
     34,   0,  16,   0,   0,   0, 
      0,   0,  26,   0,  16,   0, 
      0,   0,   0,   0,   1,  64, 
      0,   0,   0,   0, 128,  63, 
     22,   0,   0,   1,  56,   0, 
      0,   8,  18,   0,  16,   0, 
      0,   0,   0,   0,  10,   0, 
     16,   0,   0,   0,   0,   0, 
     58, 128,  32,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
     56,  32,   0,   8,  18,   0, 
     16,   0,   0,   0,   0,   0, 
     10,   0,  16,   0,   0,   0, 
      0,   0,  42, 128,  32,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,  56,   0,   0,   8, 
     98,   0,  16,   0,   0,   0, 
      0,   0,   6,  17,  16,   0, 
      2,   0,   0,   0,   6, 129, 
     32,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,  72,   0, 
      0, 141, 194,   0,   0, 128, 
     67,  85,  21,   0, 242,   0, 
     16,   0,   1,   0,   0,   0, 
    150,   5,  16,   0,   0,   0, 
      0,   0,  70, 126,  16,   0, 
      0,   0,   0,   0,   0,  96, 
     16,   0,   0,   0,   0,   0, 
      1,  64,   0,   0,   0,   0, 
      0,   0,  56,   0,   0,   8, 
     34,   0,  16,   0,   0,   0, 
      0,   0,  10,   0,  16,   0, 
      0,   0,   0,   0,  58, 128, 
     32,   0,   0,   0,   0,   0, 
      2,   0,   0,   0,  50,   0, 
      0,  11,  18,   0,  16,   0, 
      0,   0,   0,   0,  10,   0, 
     16, 128,  65,   0,   0,   0, 
      0,   0,   0,   0,  58, 128, 
     32,   0,   0,   0,   0,   0, 
      2,   0,   0,   0,   1,  64, 
      0,   0,   0,   0, 128,  63, 
     56,   0,   0,   7, 242,   0, 
     16,   0,   1,   0,   0,   0, 
      6,   0,  16,   0,   0,   0, 
      0,   0,  70,  14,  16,   0, 
      1,   0,   0,   0,  50,   0, 
      0,  10, 242,   0,  16,   0, 
      0,   0,   0,   0,  70, 142, 
     32,   0,   0,   0,   0,   0, 
      2,   0,   0,   0,  86,   5, 
     16,   0,   0,   0,   0,   0, 
     70,  14,  16,   0,   1,   0, 
      0,   0,  54,   0,   0,   5, 
    114,   0,  16,   0,   1,   0, 
      0,   0,  70,  18,  16,   0, 
      1,   0,   0,   0,  54,   0, 
      0,   5, 130,   0,  16,   0, 
      1,   0,   0,   0,   1,  64, 
      0,   0,   0,   0, 128,  63, 
     56,   0,   0,   7, 242,   0, 
     16,   0,   0,   0,   0,   0, 
     70,  14,  16,   0,   0,   0, 
      0,   0,  70,  14,  16,   0, 
      1,   0,   0,   0,  56,   0, 
      0,   7, 242,   0,  16,   0, 
      0,   0,   0,   0,  70,  14, 
     16,   0,   0,   0,   0,   0, 
    246,  31,  16,   0,   1,   0, 
      0,   0,  50,   0,   0,   9, 
    242,   0,  16,   0,   0,   0, 
      0,   0,  70,  30,  16,   0, 
      0,   0,   0,   0, 246,  15, 
     16,   0,   0,   0,   0,   0, 
     70,  14,  16,   0,   0,   0, 
      0,   0,  56,   0,   0,   7, 
    114,  32,  16,   0,   0,   0, 
      0,   0, 246,  15,  16,   0, 
      0,   0,   0,   0,  70,   2, 
     16,   0,   0,   0,   0,   0, 
     54,   0,   0,   5, 130,  32, 
     16,   0,   0,   0,   0,   0, 
     58,   0,  16,   0,   0,   0, 
      0,   0,  62,   0,   0,   1, 
     53,  16,   0,   0, 212,   0, 
      0,   0,  21,   0,   1,   0, 
     41, 150,  11,   0, 120,   1, 
    237,  86,  49,  79,  20,  65, 
     20, 126,  51,  59,  59,  55, 
    123, 206, 238, 130,  30,  17, 
    241, 136, 135,  28,  10, 205, 
     37,   8, 177, 176,  82,  33, 
     84,  90,  40, 177,  49,  27, 
    194,  33, 135,  16,  23,  48, 
    199,  65, 208, 144,  19,  41, 
    180, 161, 176, 209, 138, 196, 
     63, 160, 149,  86, 134, 120, 
    120, 213, 212, 196, 194, 218, 
    218, 130,  80, 105,  98, 196, 
    247, 246, 118, 241,  98, 176, 
     35, 209,  68, 190, 228, 155, 
     55, 223, 190, 153, 247, 222, 
    237, 108, 110, 222, 139,  99, 
     16, 225, 221, 241, 161, 215, 
    100, 207,  94, 124,  85,  29, 
     20,   0,  54, 206,  25, 146, 
     44,  97,  89, 209,   8,  80, 
     73, 211,   8, 208,  75,   3, 
     45, 224,   0,  46, 154,  14, 
    228,   6, 234, 203, 104, 201, 
    151, 176,   1, 243, 225,  60, 
    250, 206, 225, 236,  20, 242, 
      7, 198,  47, 162, 189, 133, 
    116, 144, 195, 200, 147,  72, 
     92,  18,   1,  67, 238, 131, 
     98,  19,  18,  95,   2,  11, 
    217, 133, 148,  72,  90, 223, 
    142, 164,  88,   4, 242,  53, 
    199, 128,  30,  26,  14, 198, 
     24,  50, 201, 113,  16, 168, 
    182,  36,  22, 213,  64, 117, 
     18, 232, 247, 254, 109,  52, 
    222,  73, 163,  58, 250, 205, 
    164, 147, 115,  34, 208, 121, 
     52, 195,  67,  38, 143,  50, 
    241,  57,  38, 231, 155, 143, 
     53, 189,  11, 250, 109,  91, 
    200,  63,  97,  15, 145, 165, 
     73,  19,  18,  77, 117,  72, 
    240, 113,  68, 203, 206,  64, 
    142,  44, 239, 142, 190, 139, 
    255,   5, 248, 121,  71, 160, 
    119,  65, 167, 211, 130, 108, 
    188, 145,   6, 200,  63, 129, 
      4,  88, 173,  17, 159, 181, 
    195,  87,  82, 137, 102, 191, 
    105, 139, 116, 116, 112,  77, 
    154,  62, 124,  11, 198, 184, 
    203, 199, 252, 108, 236, 183, 
     32,  64,  29,  52, 233, 219, 
    168, 111,  55, 233, 113, 212, 
    227,  77, 122,   2, 245, 196, 
    190, 142, 227, 251,  81,  62, 
     46, 233, 156, 241,  97, 245, 
     95, 230,  81, 141, 135, 195, 
    163,  26,  15, 135,  71,  53, 
     30,  26,  35, 112, 235, 251, 
    251,  12, 171, 111,  61, 130, 
    245,  26,  56,  79, 234, 171, 
     92,  84,  21,  51, 145, 182, 
     32, 183, 173, 219,  97,  92, 
    115,  85, 181,  28, 181, 114, 
     26, 158, 214, 132, 115, 165, 
     78,  62,  46, 149, 180, 209, 
    239, 118, 192, 184, 203, 117, 
    213, 118, 213,  74,  43, 250, 
     91, 156, 107, 117,  80,  89, 
      3,  78, 206,  48, 213, 105, 
    152, 211, 101, 172, 180, 150, 
    180, 199, 129, 213, 109, 209, 
    159,  21, 105, 180, 114,  32, 
     39,  64, 109, 236,  56,  50, 
     15, 247, 125, 140, 231,  10, 
    185, 155,  78, 203,  47, 123, 
    107, 181,  93,  41, 229, 103, 
    180,  26, 115, 129, 210,  24, 
    171, 197,   0, 214, 212, 194, 
    214, 182, 152, 114,  49, 102, 
    171, 105, 212,  32,  61,  75, 
     41,  15, 252, 141,  29, 203, 
    202, 131, 114,  86, 235, 190, 
     16,  30,  87, 222, 182,  39, 
    240, 102,  16, 185, 104,  93, 
     10, 243, 241, 148, 233,   6, 
     59, 111,  44, 244,  83,  62, 
    203, 151, 158, 240, 149, 103, 
    251, 105,  15, 100, 175, 145, 
    190, 246, 152, 232,  50,  32, 
    164, 102,  66, 105,  46, 210, 
    154, 217,  61, 184,  94, 107, 
     38, 251, 140, 213,  42,  60, 
    113,  66, 122, 144, 202,  26, 
    187,  13, 115,  66,   6, 107, 
    202,  25, 224, 121, 195,  82, 
    157,   6, 255, 235,  13, 131, 
     54, 195,  88, 151,  97, 188, 
    199, 112,  95, 120,  73, 142, 
     40, 182, 197, 140, 237,  10, 
    173,  92, 169,  29,  87, 105, 
    112, 181, 230,  46, 120, 150, 
    203,  61, 225,  10,  15, 247, 
     55, 238, 154,  76, 114,  87, 
     60, 174,  81,  47,  23,  77, 
     17, 116, 207,  53,   3, 219, 
    144, 200,  69,   3, 181,  36, 
    164, 163,   7,   8, 186,  15, 
    241,  14, 140,  52,  49, 131, 
     36, 157, 196, 160,  94, 142, 
    244,  70, 220, 147, 172, 199, 
     54, 222, 135, 125,  12, 237, 
    250, 133, 205, 216, 255,  54, 
    182, 148, 143, 218, 218, 177, 
    189,   7, 125,  47, 123,  55, 
    191, 125, 138, 159, 127, 140, 
     45, 197, 160, 169, 253, 230, 
    121, 241,  70, 220, 255, 238, 
    196,  62, 234,  75, 105, 255, 
    212, 165,  96, 162,  24,  22, 
     31,  46,  20,  42, 243, 229, 
    249, 123, 133, 133,  74, 185, 
     84, 156, 189,  16,  76, 150, 
    150, 130, 210, 114, 165,  84, 
    158,  43, 134, 193, 221, 169, 
    229, 193,  96, 180, 124,  39, 
    184,  89, 154, 155,  44, 149, 
    131, 225, 129, 225, 254, 229, 
     96, 116, 186, 136,  98,  33, 
     24,  25,  29, 154,  15, 231, 
    203,  35, 139,  97, 120, 181, 
    180,  84,  10, 175,  47, 134, 
    133, 169, 176, 191, 191,  48, 
     29,  46, 132, 152, 162, 129, 
    153, 184,   0, 106, 196,  82, 
    200, 217, 226, 204,  28, 252, 
      4,  18, 121,  94,  30,   0, 
      0,   0,  83,  84,  65,  84, 
    148,   0,   0,   0,  36,   0, 
      0,   0,   3,   0,   0,   0, 
      0,   0,   0,   0,   4,   0, 
      0,   0,  19,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   1,   0,   0,   0, 
      2,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   2,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   8,   0,   0,   0, 
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
      0,   0
};
