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
// First Precompiled Shader at offset:[271]
// Embedded Data:
//  0x0000010f - Offset To First Precompiled Shader
//  0x00000050 - Original Shader Descriptor
//  0x0000010a - Original Shader Size
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
mul r0.x, r0.x, cb0[2].w
mul r0.xyzw, r0.xxxx, cb0[2].xyzw
add r2.x, -r1.w, l(1.000000)
mad r0.xyzw, r0.xyzw, r2.xxxx, r1.xyzw
mov r1.xyz, v1.xyzx
mov r1.w, l(1.000000)
mul r0.xyzw, r0.xyzw, r1.xyzw
mul r0.xyzw, r0.xyzw, v1.wwww
mad r0.xyzw, v0.xyzw, r0.wwww, r0.xyzw
mul o0.xyz, r0.wwww, r0.xyzx
mov o0.w, r0.w
ret 
Opaque Custom Data - XBOX Precompiled Shader
// Offset:[271], bundle is:[211] DWORDS.
// Disassembled Precompiled Shader Object:
// Shader Type               : (PS)
// Shader Hw Stage           : (PS)
// HLSL Source Filename      : f:\balazs.torok.stream2\dev\external\gfx4\Src\Render\D3D1x\Shaders\FSColorOuterBevelMul.fl11.hlsl
// HLSL Entrypoint Name      : main

//
// Shader Data:
// ----------------- PS Data ------------------------
; Input Semantic Mappings
;   [0] generic, usageIdx  0, channelMask 15, param0, paramSlot 0, DefaultVal={0,0,0,0};   [1] generic, usageIdx  1, channelMask 15, param1, paramSlot 1, DefaultVal={0,0,0,0};   [2] generic, usageIdx  2, channelMask  3, param2, paramSlot 2, DefaultVal={0,0,0,0}

codeLenInByte        = 408;Bytes

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
NumVgprs             = 14;
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
  s_waitcnt     lgkmcnt(0)                              // 0000000000DC: BF8C007F
  v_mul_legacy_f32  v2, s3, v2                          // 0000000000E0: 0E040403
  v_interp_p1_f32  v11, v0, attr1.x                     // 0000000000E4: C82C0400
  v_mul_legacy_f32  v7, s0, v2                          // 0000000000E8: 0E0E0400
  v_mul_legacy_f32  v8, s1, v2                          // 0000000000EC: 0E100401
  v_mul_legacy_f32  v9, s2, v2                          // 0000000000F0: 0E120402
  v_interp_p1_f32  v12, v0, attr1.y                     // 0000000000F4: C8300500
  s_waitcnt     vmcnt(0) & lgkmcnt(15)                  // 0000000000F8: BF8C0F70
  v_sub_f32     v10, 1.0, v6                            // 0000000000FC: 08140CF2
  v_interp_p1_f32  v13, v0, attr1.z                     // 000000000100: C8340600
  v_mul_legacy_f32  v2, s3, v2                          // 000000000104: 0E040403
  v_interp_p2_f32  v11, v1, attr1.x                     // 000000000108: C82D0401
  v_mac_legacy_f32  v3, v7, v10                         // 00000000010C: 0C061507
  v_interp_p2_f32  v12, v1, attr1.y                     // 000000000110: C8310501
  v_mac_legacy_f32  v6, v2, v10                         // 000000000114: 0C0C1502
  v_mac_legacy_f32  v4, v8, v10                         // 000000000118: 0C081508
  v_interp_p2_f32  v13, v1, attr1.z                     // 00000000011C: C8350601
  v_mul_legacy_f32  v2, v3, v11                         // 000000000120: 0E041703
  v_mac_legacy_f32  v5, v9, v10                         // 000000000124: 0C0A1509
  v_mul_legacy_f32  v3, v4, v12                         // 000000000128: 0E061904
  v_interp_p1_f32  v7, v0, attr1.w                      // 00000000012C: C81C0700
  v_mul_legacy_f32  v4, v5, v13                         // 000000000130: 0E081B05
  v_interp_p1_f32  v5, v0, attr0.x                      // 000000000134: C8140000
  v_interp_p1_f32  v8, v0, attr0.y                      // 000000000138: C8200100
  v_interp_p1_f32  v9, v0, attr0.z                      // 00000000013C: C8240200
  v_interp_p2_f32  v7, v1, attr1.w                      // 000000000140: C81D0701
  v_interp_p1_f32  v0, v0, attr0.w                      // 000000000144: C8000300
  v_interp_p2_f32  v5, v1, attr0.x                      // 000000000148: C8150001
  v_interp_p2_f32  v8, v1, attr0.y                      // 00000000014C: C8210101
  v_interp_p2_f32  v9, v1, attr0.z                      // 000000000150: C8250201
  v_mul_legacy_f32  v2, v2, v7                          // 000000000154: 0E040F02
  v_mul_legacy_f32  v3, v3, v7                          // 000000000158: 0E060F03
  v_mul_legacy_f32  v4, v4, v7                          // 00000000015C: 0E080F04
  v_mul_legacy_f32  v6, v6, v7                          // 000000000160: 0E0C0F06
  v_interp_p2_f32  v0, v1, attr0.w                      // 000000000164: C8010301
  v_mac_legacy_f32  v2, v5, v6                          // 000000000168: 0C040D05
  v_mac_legacy_f32  v3, v8, v6                          // 00000000016C: 0C060D08
  v_mac_legacy_f32  v4, v9, v6                          // 000000000170: 0C080D09
  v_mac_legacy_f32  v6, v0, v6                          // 000000000174: 0C0C0D00
  v_mul_legacy_f32  v0, v2, v6                          // 000000000178: 0E000D02
  v_mul_legacy_f32  v1, v3, v6                          // 00000000017C: 0E020D03
  v_mul_legacy_f32  v2, v4, v6                          // 000000000180: 0E040D04
  v_cvt_pkrtz_f16_f32  v0, v0, v1                       // 000000000184: 5E000300
  v_cvt_pkrtz_f16_f32  v1, v2, v6                       // 000000000188: 5E020D02
  exp           mrt0, v0, v0, v1, v1 compr vm           // 00000000018C: F800140F 00000100
  s_endpgm                                              // 000000000194: BF810000
end


// Approximately 36 instruction slots used
#endif

const BYTE pBinary_D3D1xFL11X_FSColorOuterBevelMul[] =
{
     68,  88,  66,  67, 235,  70, 
    121, 128,  18, 168, 203, 123, 
    255,  40, 148, 244, 125,  51, 
     20, 174,   1,   0,   0,   0, 
    136,  11,   0,   0,   5,   0, 
      0,   0,  52,   0,   0,   0, 
    188,   2,   0,   0,  32,   3, 
      0,   0,  84,   3,   0,   0, 
    236,  10,   0,   0,  82,  68, 
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
     83,  72,  69,  88, 144,   7, 
      0,   0,  80,   0,   0,   0, 
    228,   1,   0,   0,  53,  16, 
      0,   0,   5,   0,   0,   0, 
     15,   1,   0,   0,  80,   0, 
      0,   0,  10,   1,   0,   0, 
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
     18,   0,  16,   0,   0,   0, 
      0,   0,  10,   0,  16,   0, 
      0,   0,   0,   0,  58, 128, 
     32,   0,   0,   0,   0,   0, 
      2,   0,   0,   0,  56,   0, 
      0,   8, 242,   0,  16,   0, 
      0,   0,   0,   0,   6,   0, 
     16,   0,   0,   0,   0,   0, 
     70, 142,  32,   0,   0,   0, 
      0,   0,   2,   0,   0,   0, 
      0,   0,   0,   8,  18,   0, 
     16,   0,   2,   0,   0,   0, 
     58,   0,  16, 128,  65,   0, 
      0,   0,   1,   0,   0,   0, 
      1,  64,   0,   0,   0,   0, 
    128,  63,  50,   0,   0,   9, 
    242,   0,  16,   0,   0,   0, 
      0,   0,  70,  14,  16,   0, 
      0,   0,   0,   0,   6,   0, 
     16,   0,   2,   0,   0,   0, 
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
     53,  16,   0,   0, 213,   0, 
      0,   0,  21,   0,   1,   0, 
     41, 147,  11,   0, 120,   1, 
    237,  86,  65, 107,  19,  65, 
     20, 126,  51,  59, 153, 206, 
    198, 217, 217,  40,   9, 173, 
    154, 106, 212,  70,  68, 180, 
     24,  91,  47, 158, 170,  22, 
     47, 106,   5,  11,  30, 202, 
     82, 146, 106,  98, 197, 216, 
    148, 180, 149,  32,  37,  86, 
     15, 234, 193, 131, 224, 165, 
    135, 122, 246, 234, 209, 147, 
    169,  57, 205,  81, 138, 191, 
    192, 147,  96, 145, 210, 131, 
    120,  16, 244, 189, 205, 174, 
      6, 209,  91,  65, 193, 126, 
    240, 205, 155, 111, 223, 204, 
    155,  55,  59, 203, 206, 123, 
    182,   3,  66, 156,  27, 203, 
     94,  32,  59,  50, 187, 239, 
    224,  17,   1, 144, 192,  62, 
     67, 146,  37,  52,  20, 181, 
      0, 179,  73, 106,   1, 142, 
     80,  67,   3,  56, 128,  65, 
    179,   7, 185, 140, 122,   4, 
     45, 249,  98, 118,  96, 223, 
    228, 209, 119,  24, 123, 187, 
    145, 159,  49, 126,   9, 237, 
     85,  36, 133,  29,  69, 246, 
     34, 113,  72,   8,  12, 249, 
      3,  30,  53, 136, 216,  23, 
    195,  65,  30,  68,  82, 126, 
     52, 190,  15, 233,  34,   9, 
    228, 235, 142,   1, 121, 106, 
    126, 143,  73,  36, 229, 255, 
     39,  80, 110, 113,  44, 202, 
    129, 242,  36, 208, 126, 255, 
     54,  58, 239, 164, 147,  29, 
    237, 153, 116, 124,  78,   4, 
     58, 143, 110, 208,  62, 227, 
     71, 169, 232,  28, 227, 243, 
    205,  69, 154, 198, 208, 222, 
     86, 145, 127, 194,  55,  68, 
    150,  58,  93, 136,  53, 229, 
     33, 193, 199,  22,  45, 219, 
     15,  57, 178, 252,  80, 248, 
     93, 252,  47, 192, 207,  59, 
      4, 189,  11,  58, 157,  20, 
    178, 243,  70,  58,  32,  63, 
    125, 255,   0,  75,  45, 226, 
    211,  62, 248,  66,  42, 214, 
    236,  23, 237, 144,  14,  15, 
    174,  75,  75, 148,  14,  76, 
    114, 143,  79, 250, 217, 200, 
    239,  64, 128,  58, 232, 210, 
     19, 168,  39, 186, 116,  17, 
    117, 177,  75,  79, 161, 158, 
    250, 161, 163, 248, 126, 184, 
     30, 151, 116, 206, 248, 176, 
    249,  47, 115,  59, 199, 173, 
    225, 118, 142,  91, 195, 237, 
     28, 183, 140,  33, 184, 243, 
    245, 117, 154, 181,  87, 239, 
    193, 147,  22, 184,  15, 219, 
     75,  92,  52,  21, 179, 161, 
    118,  32, 183, 166, 251, 160, 
    168, 185, 106,  58, 174,  90, 
    220,  11, 143,  90, 194,  61, 
    211,  38,  31, 151,  74,  38, 
    208, 239, 237, 129, 162, 199, 
    117,  51, 225, 169, 197, 157, 
    232,  79, 185,  23, 219, 160, 
    178,  22, 220, 156, 101, 170, 
    223,  50, 247, 128, 117, 146, 
     90, 210,  28,  23, 150, 214, 
     68,  33,  43, 146, 104, 229, 
     80,  78, 128,  90, 217, 112, 
    229,   0, 204, 250,  24, 207, 
     19, 114,  51, 153, 148, 235, 
    223,  30, 180,  54, 165, 148, 
    239, 209, 106,  92,  11, 148, 
    198,  88,  41,  11, 152,  83, 
    138,  61,  88, 101, 202, 195, 
    152,  59, 109,  39,   7, 105, 
     28, 165,  12, 248,  43,  27, 
    142,  51,   0, 202,  93, 106, 
    251,  66,  24, 174, 204, 154, 
     17,  28, 104, 140, 131,  26, 
    196,  49,  11, 194,  24,  38, 
     82, 134, 139,  93,   6,  18, 
     39,  44, 173, 185, 169, 211, 
     10, 228, 176, 165,  49,  76, 
     28, 183,  61,  25, 169,  89, 
    162,  96, 121,  70, 107, 149, 
     81, 154, 201,  83, 214, 233, 
     21, 198, 205,  36, 181, 216, 
     45,  13, 244, 100, 109,  98, 
     47, 174,   7, 105, 204,  39, 
    103, 129,  15,  88, 214, 211, 
    111, 241,  63, 111,  25, 100, 
     44,  99,   7,  44, 227, 121, 
    203, 125,  97,  28,  95,  26, 
    225,  43,  35, 125, 109, 152, 
    195, 108, 194,  19,  90, 121, 
     82, 187, 158, 210, 224, 105, 
    205,  61,  48, 142, 199, 141, 
    240,  48, 191, 248, 158,  73, 
    199, 247, 196, 253,  22, 213, 
    113,  97,  23, 209, 169,  64, 
    126,   2,  75, 144, 208,  69, 
     13, 149,  35, 164, 195,   7, 
      8, 186,  11, 241, 254,  11, 
     53,  49, 141,  36,  29, 199, 
    160,  58, 142, 244, 114,  84, 
    143,  60, 142, 108,  52,  15, 
    235,  19, 154, 245,  19, 175, 
     34, 255, 203, 200, 210, 122, 
     84, 210, 142, 127,  60, 250, 
    233, 197, 243, 233,  15, 239, 
    162, 231, 111,  35,  75,  49, 
    168, 187, 176, 242,  34,  63, 
     22, 213, 190, 235, 145, 111, 
     10,  73, 243,  43, 167, 131, 
    169,  82, 181, 116, 119, 110, 
    112, 190,  86, 175, 221,  26, 
    156, 155, 175, 151,  75, 183, 
     79,   6, 215, 203, 119, 130, 
    114,  99, 190,  92, 159,  41, 
     85, 131,  27, 149, 198, 112, 
     48,  94, 191,  22,  92,  41, 
    207,  92,  47, 215, 131, 209, 
    161, 209,  66,  35,  24, 159, 
     46, 161, 152,  11, 206, 143, 
    159, 171,  85, 107, 245, 203, 
     11,  56, 250, 108, 249,  78, 
    185, 122, 105, 161,  58,  88, 
    169,  22,  10, 131, 211, 213, 
    185,  42, 174, 209,  65,  37, 
    202, 128, 170, 176,  30, 228, 
    237, 210, 205,  25, 248,  14, 
    183, 120,  92, 237,   0,   0, 
     83,  84,  65,  84, 148,   0, 
      0,   0,  36,   0,   0,   0, 
      3,   0,   0,   0,   0,   0, 
      0,   0,   4,   0,   0,   0, 
     19,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      1,   0,   0,   0,   2,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   2,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      8,   0,   0,   0,   0,   0, 
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
