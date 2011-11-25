/*
 * THIS FILE IS AUTO-GENERATED. DO NOT EDIT.
 * Compiled for x86-32 bit mode.
 *
 * You must include ncopcode_desc.h before this file.
 */

static const NaClOp g_Operands[305] = {
  /* 0 */ { E_Operand, NACL_OPFLAG(OpUse) | NACL_OPFLAG(OpSet) | NACL_OPFLAG(OpDest), "$Eb" },
  /* 1 */ { G_Operand, NACL_OPFLAG(OpUse), "$Gb" },
  /* 2 */ { E_Operand, NACL_OPFLAG(OpUse) | NACL_OPFLAG(OpSet) | NACL_OPFLAG(OpDest), "$Ev" },
  /* 3 */ { G_Operand, NACL_OPFLAG(OpUse), "$Gv" },
  /* 4 */ { G_Operand, NACL_OPFLAG(OpUse) | NACL_OPFLAG(OpSet) | NACL_OPFLAG(OpDest), "$Gb" },
  /* 5 */ { E_Operand, NACL_OPFLAG(OpUse), "$Eb" },
  /* 6 */ { G_Operand, NACL_OPFLAG(OpUse) | NACL_OPFLAG(OpSet) | NACL_OPFLAG(OpDest), "$Gv" },
  /* 7 */ { E_Operand, NACL_OPFLAG(OpUse), "$Ev" },
  /* 8 */ { RegAL, NACL_OPFLAG(OpUse) | NACL_OPFLAG(OpSet) | NACL_OPFLAG(OpDest), "%al" },
  /* 9 */ { I_Operand, NACL_OPFLAG(OpUse), "$Ib" },
  /* 10 */ { RegREAX, NACL_OPFLAG(OpUse) | NACL_OPFLAG(OpSet) | NACL_OPFLAG(OpDest), "$rAXv" },
  /* 11 */ { I_Operand, NACL_OPFLAG(OpUse), "$Iz" },
  /* 12 */ { E_Operand, NACL_OPFLAG(OpUse), "$Eb" },
  /* 13 */ { G_Operand, NACL_OPFLAG(OpUse), "$Gb" },
  /* 14 */ { E_Operand, NACL_OPFLAG(OpUse), "$Ev" },
  /* 15 */ { G_Operand, NACL_OPFLAG(OpUse), "$Gv" },
  /* 16 */ { G_Operand, NACL_OPFLAG(OpUse), "$Gb" },
  /* 17 */ { E_Operand, NACL_OPFLAG(OpUse), "$Eb" },
  /* 18 */ { G_Operand, NACL_OPFLAG(OpUse), "$Gv" },
  /* 19 */ { E_Operand, NACL_OPFLAG(OpUse), "$Ev" },
  /* 20 */ { RegAL, NACL_OPFLAG(OpUse), "%al" },
  /* 21 */ { I_Operand, NACL_OPFLAG(OpUse), "$Ib" },
  /* 22 */ { RegREAX, NACL_OPFLAG(OpUse), "$rAXv" },
  /* 23 */ { I_Operand, NACL_OPFLAG(OpUse), "$Iz" },
  /* 24 */ { RegRECX, NACL_OPFLAG(OpUse) | NACL_OPFLAG(OpSet) | NACL_OPFLAG(OpDest), "$rCXv" },
  /* 25 */ { RegREDX, NACL_OPFLAG(OpUse) | NACL_OPFLAG(OpSet) | NACL_OPFLAG(OpDest), "$rDXv" },
  /* 26 */ { RegREBX, NACL_OPFLAG(OpUse) | NACL_OPFLAG(OpSet) | NACL_OPFLAG(OpDest), "$rBXv" },
  /* 27 */ { RegRESP, NACL_OPFLAG(OpUse) | NACL_OPFLAG(OpSet) | NACL_OPFLAG(OpDest), "$rSPv" },
  /* 28 */ { RegREBP, NACL_OPFLAG(OpUse) | NACL_OPFLAG(OpSet) | NACL_OPFLAG(OpDest), "$rBPv" },
  /* 29 */ { RegRESI, NACL_OPFLAG(OpUse) | NACL_OPFLAG(OpSet) | NACL_OPFLAG(OpDest), "$rSIv" },
  /* 30 */ { RegREDI, NACL_OPFLAG(OpUse) | NACL_OPFLAG(OpSet) | NACL_OPFLAG(OpDest), "$rDIv" },
  /* 31 */ { RegESP, NACL_OPFLAG(OpUse) | NACL_OPFLAG(OpSet) | NACL_OPFLAG(OpImplicit), "{%esp}" },
  /* 32 */ { G_OpcodeBase, NACL_OPFLAG(OpUse), "$r8v" },
  /* 33 */ { RegESP, NACL_OPFLAG(OpUse) | NACL_OPFLAG(OpSet) | NACL_OPFLAG(OpImplicit), "{%esp}" },
  /* 34 */ { G_OpcodeBase, NACL_OPFLAG(OpSet) | NACL_OPFLAG(OpDest), "$r8v" },
  /* 35 */ { RegESP, NACL_OPFLAG(OpUse) | NACL_OPFLAG(OpSet) | NACL_OPFLAG(OpImplicit), "{%esp}" },
  /* 36 */ { I_Operand, NACL_OPFLAG(OpUse), "$Iz" },
  /* 37 */ { G_Operand, NACL_OPFLAG(OpSet) | NACL_OPFLAG(OpDest), "$Gv" },
  /* 38 */ { E_Operand, NACL_OPFLAG(OpUse), "$Ev" },
  /* 39 */ { I_Operand, NACL_OPFLAG(OpUse), "$Iz" },
  /* 40 */ { RegESP, NACL_OPFLAG(OpUse) | NACL_OPFLAG(OpSet) | NACL_OPFLAG(OpImplicit), "{%esp}" },
  /* 41 */ { I_Operand, NACL_OPFLAG(OpUse), "$Ib" },
  /* 42 */ { G_Operand, NACL_OPFLAG(OpSet) | NACL_OPFLAG(OpDest), "$Gv" },
  /* 43 */ { E_Operand, NACL_OPFLAG(OpUse), "$Ev" },
  /* 44 */ { I_Operand, NACL_OPFLAG(OpUse), "$Ib" },
  /* 45 */ { RegEIP, NACL_OPFLAG(OpSet), "%eip" },
  /* 46 */ { J_Operand, NACL_OPFLAG(OpUse) | NACL_OPFLAG(OperandNear) | NACL_OPFLAG(OperandRelative), "$Jb" },
  /* 47 */ { E_Operand, NACL_OPFLAG(OpUse), "$Eb" },
  /* 48 */ { I_Operand, NACL_OPFLAG(OpUse), "$Ib" },
  /* 49 */ { E_Operand, NACL_OPFLAG(OpUse) | NACL_OPFLAG(OpSet) | NACL_OPFLAG(OpDest), "$Eb" },
  /* 50 */ { I_Operand, NACL_OPFLAG(OpUse), "$Ib" },
  /* 51 */ { E_Operand, NACL_OPFLAG(OpUse) | NACL_OPFLAG(OpSet) | NACL_OPFLAG(OpDest), "$Ev" },
  /* 52 */ { I_Operand, NACL_OPFLAG(OpUse), "$Iz" },
  /* 53 */ { E_Operand, NACL_OPFLAG(OpUse) | NACL_OPFLAG(OpSet) | NACL_OPFLAG(OpDest), "$Ev" },
  /* 54 */ { I_Operand, NACL_OPFLAG(OpUse), "$Ib" },
  /* 55 */ { E_Operand, NACL_OPFLAG(OpUse) | NACL_OPFLAG(OpSet) | NACL_OPFLAG(OpDest), "$Eb" },
  /* 56 */ { G_Operand, NACL_OPFLAG(OpUse) | NACL_OPFLAG(OpSet) | NACL_OPFLAG(OpDest), "$Gb" },
  /* 57 */ { E_Operand, NACL_OPFLAG(OpUse) | NACL_OPFLAG(OpSet) | NACL_OPFLAG(OpDest), "$Ev" },
  /* 58 */ { G_Operand, NACL_OPFLAG(OpUse) | NACL_OPFLAG(OpSet) | NACL_OPFLAG(OpDest), "$Gv" },
  /* 59 */ { E_Operand, NACL_OPFLAG(OpSet) | NACL_OPFLAG(OpDest), "$Eb" },
  /* 60 */ { G_Operand, NACL_OPFLAG(OpUse), "$Gb" },
  /* 61 */ { E_Operand, NACL_OPFLAG(OpSet) | NACL_OPFLAG(OpDest), "$Ev" },
  /* 62 */ { G_Operand, NACL_OPFLAG(OpUse), "$Gv" },
  /* 63 */ { G_Operand, NACL_OPFLAG(OpSet) | NACL_OPFLAG(OpDest), "$Gb" },
  /* 64 */ { E_Operand, NACL_OPFLAG(OpUse), "$Eb" },
  /* 65 */ { G_Operand, NACL_OPFLAG(OpSet), "$Gv" },
  /* 66 */ { M_Operand, NACL_OPFLAG(OpAddress), "$M" },
  /* 67 */ { RegESP, NACL_OPFLAG(OpUse) | NACL_OPFLAG(OpSet) | NACL_OPFLAG(OpImplicit), "{%esp}" },
  /* 68 */ { E_Operand, NACL_OPFLAG(OpSet) | NACL_OPFLAG(OpDest), "$Ev" },
  /* 69 */ { G_OpcodeBase, NACL_OPFLAG(OpUse) | NACL_OPFLAG(OpSet) | NACL_OPFLAG(OpDest), "$r8v" },
  /* 70 */ { RegREAX, NACL_OPFLAG(OpUse) | NACL_OPFLAG(OpSet) | NACL_OPFLAG(OpDest), "$rAXv" },
  /* 71 */ { RegEAX, NACL_OPFLAG(OpSet) | NACL_OPFLAG(OperandSignExtends_v), "%eax" },
  /* 72 */ { RegAX, NACL_OPFLAG(OpUse), "%ax" },
  /* 73 */ { RegAX, NACL_OPFLAG(OpSet), "%ax" },
  /* 74 */ { RegAL, NACL_OPFLAG(OpUse), "%al" },
  /* 75 */ { RegEDX, NACL_OPFLAG(OpSet), "%edx" },
  /* 76 */ { RegEAX, NACL_OPFLAG(OpUse), "%eax" },
  /* 77 */ { RegDX, NACL_OPFLAG(OpSet), "%dx" },
  /* 78 */ { RegAX, NACL_OPFLAG(OpUse), "%ax" },
  /* 79 */ { RegAL, NACL_OPFLAG(OpSet) | NACL_OPFLAG(OpDest), "%al" },
  /* 80 */ { O_Operand, NACL_OPFLAG(OpUse), "$Ob" },
  /* 81 */ { RegREAX, NACL_OPFLAG(OpSet) | NACL_OPFLAG(OpDest), "$rAXv" },
  /* 82 */ { O_Operand, NACL_OPFLAG(OpUse), "$Ov" },
  /* 83 */ { O_Operand, NACL_OPFLAG(OpSet) | NACL_OPFLAG(OpDest), "$Ob" },
  /* 84 */ { RegAL, NACL_OPFLAG(OpUse), "%al" },
  /* 85 */ { O_Operand, NACL_OPFLAG(OpSet) | NACL_OPFLAG(OpDest), "$Ov" },
  /* 86 */ { RegREAX, NACL_OPFLAG(OpUse), "$rAXv" },
  /* 87 */ { RegES_EDI, NACL_OPFLAG(OpSet), "$Yb" },
  /* 88 */ { RegDS_EDI, NACL_OPFLAG(OpUse), "$Xb" },
  /* 89 */ { RegES_EDI, NACL_OPFLAG(OpSet), "$Yvd" },
  /* 90 */ { RegDS_EDI, NACL_OPFLAG(OpUse), "$Xvd" },
  /* 91 */ { RegES_EDI, NACL_OPFLAG(OpSet), "$Yvw" },
  /* 92 */ { RegDS_EDI, NACL_OPFLAG(OpUse), "$Xvw" },
  /* 93 */ { RegDS_EDI, NACL_OPFLAG(OpUse), "$Xb" },
  /* 94 */ { RegES_EDI, NACL_OPFLAG(OpUse), "$Yb" },
  /* 95 */ { RegDS_EDI, NACL_OPFLAG(OpUse), "$Xvd" },
  /* 96 */ { RegES_EDI, NACL_OPFLAG(OpUse), "$Yvd" },
  /* 97 */ { RegDS_EDI, NACL_OPFLAG(OpUse), "$Xvw" },
  /* 98 */ { RegES_EDI, NACL_OPFLAG(OpUse), "$Yvw" },
  /* 99 */ { RegES_EDI, NACL_OPFLAG(OpSet), "$Yb" },
  /* 100 */ { RegAL, NACL_OPFLAG(OpUse), "%al" },
  /* 101 */ { RegES_EDI, NACL_OPFLAG(OpSet), "$Yvd" },
  /* 102 */ { RegEAX, NACL_OPFLAG(OpUse), "$rAXvd" },
  /* 103 */ { RegES_EDI, NACL_OPFLAG(OpSet), "$Yvw" },
  /* 104 */ { RegAX, NACL_OPFLAG(OpUse), "$rAXvw" },
  /* 105 */ { RegAL, NACL_OPFLAG(OpSet), "%al" },
  /* 106 */ { RegDS_EDI, NACL_OPFLAG(OpUse), "$Xb" },
  /* 107 */ { RegEAX, NACL_OPFLAG(OpSet), "$rAXvd" },
  /* 108 */ { RegDS_EDI, NACL_OPFLAG(OpUse), "$Xvd" },
  /* 109 */ { RegAX, NACL_OPFLAG(OpSet), "$rAXvw" },
  /* 110 */ { RegDS_EDI, NACL_OPFLAG(OpUse), "$Xvw" },
  /* 111 */ { RegAL, NACL_OPFLAG(OpUse), "%al" },
  /* 112 */ { RegES_EDI, NACL_OPFLAG(OpUse), "$Yb" },
  /* 113 */ { RegEAX, NACL_OPFLAG(OpUse), "$rAXvd" },
  /* 114 */ { RegES_EDI, NACL_OPFLAG(OpUse), "$Yvd" },
  /* 115 */ { RegAX, NACL_OPFLAG(OpUse), "$rAXvw" },
  /* 116 */ { RegES_EDI, NACL_OPFLAG(OpUse), "$Yvw" },
  /* 117 */ { G_OpcodeBase, NACL_OPFLAG(OpSet) | NACL_OPFLAG(OpDest), "$r8b" },
  /* 118 */ { I_Operand, NACL_OPFLAG(OpUse), "$Ib" },
  /* 119 */ { G_OpcodeBase, NACL_OPFLAG(OpSet) | NACL_OPFLAG(OpDest), "$r8v" },
  /* 120 */ { I_Operand, NACL_OPFLAG(OpUse), "$Iv" },
  /* 121 */ { E_Operand, NACL_OPFLAG(OpSet) | NACL_OPFLAG(OpDest), "$Eb" },
  /* 122 */ { I_Operand, NACL_OPFLAG(OpUse), "$Ib" },
  /* 123 */ { E_Operand, NACL_OPFLAG(OpSet) | NACL_OPFLAG(OpDest), "$Ev" },
  /* 124 */ { I_Operand, NACL_OPFLAG(OpUse), "$Iz" },
  /* 125 */ { E_Operand, NACL_OPFLAG(OpUse) | NACL_OPFLAG(OpSet) | NACL_OPFLAG(OpDest), "$Eb" },
  /* 126 */ { Const_1, NACL_OPFLAG(OpUse), "1" },
  /* 127 */ { E_Operand, NACL_OPFLAG(OpUse) | NACL_OPFLAG(OpSet) | NACL_OPFLAG(OpDest), "$Ev" },
  /* 128 */ { Const_1, NACL_OPFLAG(OpUse), "1" },
  /* 129 */ { E_Operand, NACL_OPFLAG(OpUse) | NACL_OPFLAG(OpSet) | NACL_OPFLAG(OpDest), "$Eb" },
  /* 130 */ { RegCL, NACL_OPFLAG(OpUse), "%cl" },
  /* 131 */ { E_Operand, NACL_OPFLAG(OpUse) | NACL_OPFLAG(OpSet) | NACL_OPFLAG(OpDest), "$Ev" },
  /* 132 */ { RegCL, NACL_OPFLAG(OpUse), "%cl" },
  /* 133 */ { Mv_Operand, NACL_OPFLAG(OpUse), "$Md" },
  /* 134 */ { Mw_Operand, NACL_OPFLAG(OpSet) | NACL_OPFLAG(OpDest), "$Mw" },
  /* 135 */ { M_Operand, NACL_OPFLAG(OpSet) | NACL_OPFLAG(OpDest), "$M" },
  /* 136 */ { Mw_Operand, NACL_OPFLAG(OpUse), "$Mw" },
  /* 137 */ { M_Operand, NACL_OPFLAG(OpUse), "$M" },
  /* 138 */ { Mv_Operand, NACL_OPFLAG(OpSet) | NACL_OPFLAG(OpDest), "$Md" },
  /* 139 */ { Mo_Operand, NACL_OPFLAG(OpUse), "$Mq" },
  /* 140 */ { Mo_Operand, NACL_OPFLAG(OpSet) | NACL_OPFLAG(OpDest), "$Mq" },
  /* 141 */ { RegEIP, NACL_OPFLAG(OpSet), "%eip" },
  /* 142 */ { RegECX, NACL_OPFLAG(OpUse), "%ecx" },
  /* 143 */ { J_Operand, NACL_OPFLAG(OpUse) | NACL_OPFLAG(OperandNear) | NACL_OPFLAG(OperandRelative), "$Jb" },
  /* 144 */ { RegEIP, NACL_OPFLAG(OpSet), "%eip" },
  /* 145 */ { RegCX, NACL_OPFLAG(OpUse), "%cx" },
  /* 146 */ { J_Operand, NACL_OPFLAG(OpUse) | NACL_OPFLAG(OperandNear) | NACL_OPFLAG(OperandRelative), "$Jb" },
  /* 147 */ { RegEIP, NACL_OPFLAG(OpUse) | NACL_OPFLAG(OpSet) | NACL_OPFLAG(OpImplicit), "{%eip}" },
  /* 148 */ { RegESP, NACL_OPFLAG(OpUse) | NACL_OPFLAG(OpSet) | NACL_OPFLAG(OpImplicit), "{%esp}" },
  /* 149 */ { J_Operand, NACL_OPFLAG(OpUse) | NACL_OPFLAG(OperandNear) | NACL_OPFLAG(OperandRelative), "$Jz" },
  /* 150 */ { RegEIP, NACL_OPFLAG(OpSet), "%eip" },
  /* 151 */ { J_Operand, NACL_OPFLAG(OpUse) | NACL_OPFLAG(OperandNear) | NACL_OPFLAG(OperandRelative), "$Jz" },
  /* 152 */ { RegAX, NACL_OPFLAG(OpSet), "%ax" },
  /* 153 */ { RegAL, NACL_OPFLAG(OpUse), "%al" },
  /* 154 */ { E_Operand, NACL_OPFLAG(OpUse), "$Eb" },
  /* 155 */ { RegREDX, NACL_OPFLAG(OpSet), "%redx" },
  /* 156 */ { RegREAX, NACL_OPFLAG(OpUse) | NACL_OPFLAG(OpSet), "%reax" },
  /* 157 */ { E_Operand, NACL_OPFLAG(OpUse), "$Ev" },
  /* 158 */ { RegESP, NACL_OPFLAG(OpUse) | NACL_OPFLAG(OpSet) | NACL_OPFLAG(OpImplicit), "{%esp}" },
  /* 159 */ { E_Operand, NACL_OPFLAG(OpUse), "$Ev" },
  /* 160 */ { RegEIP, NACL_OPFLAG(OpSet), "%eip" },
  /* 161 */ { E_Operand, NACL_OPFLAG(OpUse) | NACL_OPFLAG(OperandNear), "$Ev" },
  /* 162 */ { RegEIP, NACL_OPFLAG(OpUse) | NACL_OPFLAG(OpSet) | NACL_OPFLAG(OpImplicit), "{%eip}" },
  /* 163 */ { RegESP, NACL_OPFLAG(OpUse) | NACL_OPFLAG(OpSet) | NACL_OPFLAG(OpImplicit), "{%esp}" },
  /* 164 */ { E_Operand, NACL_OPFLAG(OpUse) | NACL_OPFLAG(OperandNear), "$Ev" },
  /* 165 */ { Ew_Operand, NACL_OPFLAG(OpUse), "$Ew" },
  /* 166 */ { Mb_Operand, NACL_EMPTY_OPFLAGS, "$Mb" },
  /* 167 */ { Mmx_G_Operand, NACL_EMPTY_OPFLAGS, "$Pq" },
  /* 168 */ { Mmx_E_Operand, NACL_EMPTY_OPFLAGS, "$Qq" },
  /* 169 */ { I_Operand, NACL_EMPTY_OPFLAGS, "$Ib" },
  /* 170 */ { Xmm_E_Operand, NACL_OPFLAG(OpUse), "$Wps" },
  /* 171 */ { Xmm_E_Operand, NACL_OPFLAG(OpSet) | NACL_OPFLAG(OpDest), "$Wps" },
  /* 172 */ { Xmm_E_Operand, NACL_OPFLAG(OpUse), "$VRq" },
  /* 173 */ { Xmm_Eo_Operand, NACL_OPFLAG(OpUse), "$Wq" },
  /* 174 */ { Mmx_E_Operand, NACL_OPFLAG(OpUse), "$Qq" },
  /* 175 */ { Mdq_Operand, NACL_OPFLAG(OpSet) | NACL_OPFLAG(OpDest), "$Mdq" },
  /* 176 */ { Xmm_E_Operand, NACL_OPFLAG(OpUse), "$Wss" },
  /* 177 */ { RegEAX, NACL_OPFLAG(OpSet), "%eax" },
  /* 178 */ { RegEDX, NACL_OPFLAG(OpSet), "%edx" },
  /* 179 */ { Gv_Operand, NACL_OPFLAG(OpSet) | NACL_OPFLAG(OpDest), "$Gd" },
  /* 180 */ { Xmm_E_Operand, NACL_OPFLAG(OpUse), "$VRps" },
  /* 181 */ { Xmm_E_Operand, NACL_OPFLAG(OpUse), "$Wdq" },
  /* 182 */ { Mmx_E_Operand, NACL_OPFLAG(OpUse), "$Qd" },
  /* 183 */ { E_Operand, NACL_OPFLAG(OpUse), "$Ed/q/d" },
  /* 184 */ { Mmx_E_Operand, NACL_OPFLAG(OpUse), "$Qq" },
  /* 185 */ { I_Operand, NACL_OPFLAG(OpUse), "$Ib" },
  /* 186 */ { Mmx_E_Operand, NACL_OPFLAG(OpUse) | NACL_OPFLAG(OpSet) | NACL_OPFLAG(OpDest), "$PRq" },
  /* 187 */ { I_Operand, NACL_OPFLAG(OpUse), "$Ib" },
  /* 188 */ { E_Operand, NACL_OPFLAG(OpSet) | NACL_OPFLAG(OpDest), "$Ed/q/d" },
  /* 189 */ { Mmx_E_Operand, NACL_OPFLAG(OpSet) | NACL_OPFLAG(OpDest), "$Qq" },
  /* 190 */ { RegEBX, NACL_OPFLAG(OpSet), "%ebx" },
  /* 191 */ { RegEDX, NACL_OPFLAG(OpSet), "%edx" },
  /* 192 */ { RegEAX, NACL_OPFLAG(OpUse) | NACL_OPFLAG(OpSet), "%eax" },
  /* 193 */ { RegECX, NACL_OPFLAG(OpUse) | NACL_OPFLAG(OpSet), "%ecx" },
  /* 194 */ { E_Operand, NACL_OPFLAG(OpSet) | NACL_OPFLAG(OpDest), "$Ev" },
  /* 195 */ { G_Operand, NACL_OPFLAG(OpUse), "$Gv" },
  /* 196 */ { I_Operand, NACL_OPFLAG(OpUse), "$Ib" },
  /* 197 */ { E_Operand, NACL_OPFLAG(OpSet) | NACL_OPFLAG(OpDest), "$Ev" },
  /* 198 */ { G_Operand, NACL_OPFLAG(OpUse), "$Gv" },
  /* 199 */ { RegCL, NACL_OPFLAG(OpUse), "%cl" },
  /* 200 */ { E_Operand, NACL_OPFLAG(OpUse) | NACL_OPFLAG(OpSet) | NACL_OPFLAG(OpDest), "$Ev" },
  /* 201 */ { G_Operand, NACL_OPFLAG(OpUse), "$Gv" },
  /* 202 */ { I_Operand, NACL_OPFLAG(OpUse), "$Ib" },
  /* 203 */ { E_Operand, NACL_OPFLAG(OpUse) | NACL_OPFLAG(OpSet) | NACL_OPFLAG(OpDest), "$Ev" },
  /* 204 */ { G_Operand, NACL_OPFLAG(OpUse), "$Gv" },
  /* 205 */ { RegCL, NACL_OPFLAG(OpUse), "%cl" },
  /* 206 */ { Mb_Operand, NACL_OPFLAG(OpUse), "$Mb" },
  /* 207 */ { RegAL, NACL_OPFLAG(OpUse) | NACL_OPFLAG(OpSet), "%al" },
  /* 208 */ { E_Operand, NACL_OPFLAG(OpUse) | NACL_OPFLAG(OpSet) | NACL_OPFLAG(OpDest), "$Eb" },
  /* 209 */ { G_Operand, NACL_OPFLAG(OpUse) | NACL_OPFLAG(OpSet), "$Gb" },
  /* 210 */ { RegREAX, NACL_OPFLAG(OpUse) | NACL_OPFLAG(OpSet), "$rAXv" },
  /* 211 */ { E_Operand, NACL_OPFLAG(OpUse) | NACL_OPFLAG(OpSet) | NACL_OPFLAG(OpDest), "$Ev" },
  /* 212 */ { G_Operand, NACL_OPFLAG(OpUse) | NACL_OPFLAG(OpSet), "$Gv" },
  /* 213 */ { G_Operand, NACL_OPFLAG(OpSet) | NACL_OPFLAG(OpDest), "$Gv" },
  /* 214 */ { Eb_Operand, NACL_OPFLAG(OpUse), "$Eb" },
  /* 215 */ { G_Operand, NACL_OPFLAG(OpSet) | NACL_OPFLAG(OpDest), "$Gv" },
  /* 216 */ { Ew_Operand, NACL_OPFLAG(OpUse), "$Ew" },
  /* 217 */ { Xmm_E_Operand, NACL_OPFLAG(OpUse), "$Wps" },
  /* 218 */ { I_Operand, NACL_OPFLAG(OpUse), "$Ib" },
  /* 219 */ { M_Operand, NACL_OPFLAG(OpSet) | NACL_OPFLAG(OpDest), "$Md/q" },
  /* 220 */ { G_Operand, NACL_OPFLAG(OpUse), "$Gd/q" },
  /* 221 */ { E_Operand, NACL_OPFLAG(OpUse), "$Rd/q/Mw" },
  /* 222 */ { I_Operand, NACL_OPFLAG(OpUse), "$Ib" },
  /* 223 */ { Gv_Operand, NACL_OPFLAG(OpSet) | NACL_OPFLAG(OpDest), "$Gd" },
  /* 224 */ { Mmx_E_Operand, NACL_OPFLAG(OpUse), "$PRq" },
  /* 225 */ { I_Operand, NACL_OPFLAG(OpUse), "$Ib" },
  /* 226 */ { RegEDX, NACL_OPFLAG(OpUse) | NACL_OPFLAG(OpSet), "%edx" },
  /* 227 */ { RegEAX, NACL_OPFLAG(OpUse) | NACL_OPFLAG(OpSet), "%eax" },
  /* 228 */ { Mo_Operand, NACL_OPFLAG(OpUse) | NACL_OPFLAG(OpSet) | NACL_OPFLAG(OpDest), "$Mq" },
  /* 229 */ { RegDS_EDI, NACL_OPFLAG(OpSet), "$Xvd" },
  /* 230 */ { Mmx_E_Operand, NACL_OPFLAG(OpUse), "$PRq" },
  /* 231 */ { Xmm_E_Operand, NACL_OPFLAG(OpUse), "$Wsd" },
  /* 232 */ { Xmm_E_Operand, NACL_OPFLAG(OpSet) | NACL_OPFLAG(OpDest), "$Wsd" },
  /* 233 */ { E_Operand, NACL_OPFLAG(OpUse), "$Ed/q" },
  /* 234 */ { G_Operand, NACL_OPFLAG(OpSet) | NACL_OPFLAG(OpDest), "$Gd/q" },
  /* 235 */ { Xmm_E_Operand, NACL_OPFLAG(OpUse), "$Wsd" },
  /* 236 */ { Xmm_Eo_Operand, NACL_OPFLAG(OpUse), "$Wq" },
  /* 237 */ { I_Operand, NACL_OPFLAG(OpUse), "$Ib" },
  /* 238 */ { Xmm_E_Operand, NACL_OPFLAG(OpUse), "$VRq" },
  /* 239 */ { I_Operand, NACL_OPFLAG(OpUse), "$Ib" },
  /* 240 */ { I2_Operand, NACL_OPFLAG(OpUse), "$Ib" },
  /* 241 */ { Xmm_E_Operand, NACL_OPFLAG(OpUse), "$VRdq" },
  /* 242 */ { Xmm_E_Operand, NACL_OPFLAG(OpUse), "$Wsd" },
  /* 243 */ { I_Operand, NACL_OPFLAG(OpUse), "$Ib" },
  /* 244 */ { Xmm_E_Operand, NACL_OPFLAG(OpUse), "$Wpd" },
  /* 245 */ { Mdq_Operand, NACL_OPFLAG(OpUse), "$Mdq" },
  /* 246 */ { Xmm_E_Operand, NACL_OPFLAG(OpSet) | NACL_OPFLAG(OpDest), "$Wss" },
  /* 247 */ { G_Operand, NACL_OPFLAG(OpSet) | NACL_OPFLAG(OpDest), "$Gd/q" },
  /* 248 */ { Xmm_E_Operand, NACL_OPFLAG(OpUse), "$Wss" },
  /* 249 */ { Xmm_E_Operand, NACL_OPFLAG(OpSet) | NACL_OPFLAG(OpDest), "$Wdq" },
  /* 250 */ { Xmm_E_Operand, NACL_OPFLAG(OpUse), "$Wss" },
  /* 251 */ { I_Operand, NACL_OPFLAG(OpUse), "$Ib" },
  /* 252 */ { Xmm_E_Operand, NACL_OPFLAG(OpSet) | NACL_OPFLAG(OpDest), "$Wpd" },
  /* 253 */ { Gv_Operand, NACL_OPFLAG(OpSet) | NACL_OPFLAG(OpDest), "$Gd" },
  /* 254 */ { Xmm_E_Operand, NACL_OPFLAG(OpUse), "$VRpd" },
  /* 255 */ { Xmm_E_Operand, NACL_OPFLAG(OpUse), "$Wdq" },
  /* 256 */ { I_Operand, NACL_OPFLAG(OpUse), "$Ib" },
  /* 257 */ { Xmm_E_Operand, NACL_OPFLAG(OpUse) | NACL_OPFLAG(OpSet) | NACL_OPFLAG(OpDest), "$VRdq" },
  /* 258 */ { I_Operand, NACL_OPFLAG(OpUse), "$Ib" },
  /* 259 */ { Xmm_E_Operand, NACL_OPFLAG(OpUse), "$Wpd" },
  /* 260 */ { I_Operand, NACL_OPFLAG(OpUse), "$Ib" },
  /* 261 */ { Gv_Operand, NACL_OPFLAG(OpSet) | NACL_OPFLAG(OpDest), "$Gd" },
  /* 262 */ { Xmm_E_Operand, NACL_OPFLAG(OpUse), "$VRdq" },
  /* 263 */ { I_Operand, NACL_OPFLAG(OpUse), "$Ib" },
  /* 264 */ { Xmm_Eo_Operand, NACL_OPFLAG(OpSet) | NACL_OPFLAG(OpDest), "$Wq" },
  /* 265 */ { RegDS_EDI, NACL_OPFLAG(OpSet), "$Xvd" },
  /* 266 */ { Xmm_E_Operand, NACL_OPFLAG(OpUse), "$VRdq" },
  /* 267 */ { G_Operand, NACL_OPFLAG(OpSet) | NACL_OPFLAG(OpDest), "$Gv" },
  /* 268 */ { M_Operand, NACL_OPFLAG(OpUse), "$Mv" },
  /* 269 */ { M_Operand, NACL_OPFLAG(OpSet) | NACL_OPFLAG(OpDest), "$Mv" },
  /* 270 */ { G_Operand, NACL_OPFLAG(OpUse), "$Gv" },
  /* 271 */ { Xmm_E_Operand, NACL_OPFLAG(OpUse), "$Udq/Mq" },
  /* 272 */ { Xmm_E_Operand, NACL_OPFLAG(OpUse), "$Udq/Md" },
  /* 273 */ { Xmm_E_Operand, NACL_OPFLAG(OpUse), "$Udq/Mw" },
  /* 274 */ { Gv_Operand, NACL_OPFLAG(OpUse) | NACL_OPFLAG(OpSet) | NACL_OPFLAG(OpDest), "$Gd" },
  /* 275 */ { E_Operand, NACL_OPFLAG(OpUse), "$Eb" },
  /* 276 */ { Gv_Operand, NACL_OPFLAG(OpUse) | NACL_OPFLAG(OpSet) | NACL_OPFLAG(OpDest), "$Gd" },
  /* 277 */ { E_Operand, NACL_OPFLAG(OpUse), "$Ev" },
  /* 278 */ { Ev_Operand, NACL_OPFLAG(OpSet) | NACL_OPFLAG(OpDest), "$Rd/Mb" },
  /* 279 */ { I_Operand, NACL_OPFLAG(OpUse), "$Ib" },
  /* 280 */ { Ev_Operand, NACL_OPFLAG(OpSet) | NACL_OPFLAG(OpDest), "$Rd/Mw" },
  /* 281 */ { I_Operand, NACL_OPFLAG(OpUse), "$Ib" },
  /* 282 */ { E_Operand, NACL_OPFLAG(OpSet) | NACL_OPFLAG(OpDest), "$Ed/q/d" },
  /* 283 */ { I_Operand, NACL_OPFLAG(OpUse), "$Ib" },
  /* 284 */ { Ev_Operand, NACL_OPFLAG(OpSet) | NACL_OPFLAG(OpDest), "$Ed" },
  /* 285 */ { I_Operand, NACL_OPFLAG(OpUse), "$Ib" },
  /* 286 */ { E_Operand, NACL_OPFLAG(OpUse), "$Rd/q/Mb" },
  /* 287 */ { I_Operand, NACL_OPFLAG(OpUse), "$Ib" },
  /* 288 */ { Xmm_E_Operand, NACL_OPFLAG(OpUse), "$Udq/Md" },
  /* 289 */ { I_Operand, NACL_OPFLAG(OpUse), "$Ib" },
  /* 290 */ { E_Operand, NACL_OPFLAG(OpUse), "$Ed/q/d" },
  /* 291 */ { I_Operand, NACL_OPFLAG(OpUse), "$Ib" },
  /* 292 */ { RegREAX, NACL_OPFLAG(OpSet), "$rAXv" },
  /* 293 */ { RegREDX, NACL_OPFLAG(OpSet), "$rDXv" },
  /* 294 */ { Xmm_E_Operand, NACL_OPFLAG(OpUse), "$Wdq" },
  /* 295 */ { I_Operand, NACL_OPFLAG(OpUse), "$Ib" },
  /* 296 */ { RegRECX, NACL_OPFLAG(OpSet), "$rCXv" },
  /* 297 */ { RegREAX, NACL_OPFLAG(OpSet), "$rAXv" },
  /* 298 */ { RegREDX, NACL_OPFLAG(OpSet), "$rDXv" },
  /* 299 */ { Xmm_E_Operand, NACL_OPFLAG(OpUse), "$Wdq" },
  /* 300 */ { I_Operand, NACL_OPFLAG(OpUse), "$Ib" },
  /* 301 */ { RegRECX, NACL_OPFLAG(OpSet), "$rCXv" },
  /* 302 */ { Xmm_E_Operand, NACL_OPFLAG(OpUse), "$Wdq" },
  /* 303 */ { I_Operand, NACL_OPFLAG(OpUse), "$Ib" },
  /* 304 */ { RegAX, NACL_OPFLAG(OpSet) | NACL_OPFLAG(OpDest), "%ax" },
};

static const NaClInst g_Opcodes[574] = {
  /* 0 */
  { NACLi_INVALID,
    NACL_EMPTY_IFLAGS,
    InstInvalid, 0x00, 0, 0, NACL_OPCODE_NULL_OFFSET  },
  /* 1 */
  { NACLi_386,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeLockable) | NACL_IFLAG(OperandSize_b),
    InstAdd, 0x00, 2, 0, NACL_OPCODE_NULL_OFFSET  },
  /* 2 */
  { NACLi_386,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeLockable) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v),
    InstAdd, 0x00, 2, 2, NACL_OPCODE_NULL_OFFSET  },
  /* 3 */
  { NACLi_386,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeLockable) | NACL_IFLAG(OperandSize_b),
    InstAdd, 0x00, 2, 4, NACL_OPCODE_NULL_OFFSET  },
  /* 4 */
  { NACLi_386,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeLockable) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v),
    InstAdd, 0x00, 2, 6, NACL_OPCODE_NULL_OFFSET  },
  /* 5 */
  { NACLi_386,
    NACL_IFLAG(OpcodeHasImmed_b) | NACL_IFLAG(OpcodeLockable),
    InstAdd, 0x00, 2, 8, NACL_OPCODE_NULL_OFFSET  },
  /* 6 */
  { NACLi_386,
    NACL_IFLAG(OpcodeHasImmed_z) | NACL_IFLAG(OpcodeLockable) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v),
    InstAdd, 0x00, 2, 10, NACL_OPCODE_NULL_OFFSET  },
  /* 7 */
  { NACLi_386,
    NACL_IFLAG(NaClIllegal) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 0, 0, NACL_OPCODE_NULL_OFFSET  },
  /* 8 */
  { NACLi_386,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeLockable) | NACL_IFLAG(OperandSize_b),
    InstOr, 0x00, 2, 0, NACL_OPCODE_NULL_OFFSET  },
  /* 9 */
  { NACLi_386,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeLockable) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v),
    InstOr, 0x00, 2, 2, NACL_OPCODE_NULL_OFFSET  },
  /* 10 */
  { NACLi_386,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeLockable) | NACL_IFLAG(OperandSize_b),
    InstOr, 0x00, 2, 4, NACL_OPCODE_NULL_OFFSET  },
  /* 11 */
  { NACLi_386,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeLockable) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v),
    InstOr, 0x00, 2, 6, NACL_OPCODE_NULL_OFFSET  },
  /* 12 */
  { NACLi_386,
    NACL_IFLAG(OpcodeHasImmed_b) | NACL_IFLAG(OpcodeLockable),
    InstOr, 0x00, 2, 8, NACL_OPCODE_NULL_OFFSET  },
  /* 13 */
  { NACLi_386,
    NACL_IFLAG(OpcodeHasImmed_z) | NACL_IFLAG(OpcodeLockable) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v),
    InstOr, 0x00, 2, 10, NACL_OPCODE_NULL_OFFSET  },
  /* 14 */
  { NACLi_INVALID,
    NACL_IFLAG(NaClIllegal),
    InstInvalid, 0x00, 0, 0, NACL_OPCODE_NULL_OFFSET  },
  /* 15 */
  { NACLi_386,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeLockable) | NACL_IFLAG(OperandSize_b) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 2, 0, NACL_OPCODE_NULL_OFFSET  },
  /* 16 */
  { NACLi_386,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeLockable) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 2, 2, NACL_OPCODE_NULL_OFFSET  },
  /* 17 */
  { NACLi_386,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeLockable) | NACL_IFLAG(OperandSize_b) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 2, 4, NACL_OPCODE_NULL_OFFSET  },
  /* 18 */
  { NACLi_386,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeLockable) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 2, 6, NACL_OPCODE_NULL_OFFSET  },
  /* 19 */
  { NACLi_386,
    NACL_IFLAG(OpcodeHasImmed_b) | NACL_IFLAG(OpcodeLockable) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 2, 8, NACL_OPCODE_NULL_OFFSET  },
  /* 20 */
  { NACLi_386,
    NACL_IFLAG(OpcodeHasImmed_z) | NACL_IFLAG(OpcodeLockable) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 2, 10, NACL_OPCODE_NULL_OFFSET  },
  /* 21 */
  { NACLi_386,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeLockable) | NACL_IFLAG(OperandSize_b),
    InstAnd, 0x00, 2, 0, NACL_OPCODE_NULL_OFFSET  },
  /* 22 */
  { NACLi_386,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeLockable) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v),
    InstAnd, 0x00, 2, 2, NACL_OPCODE_NULL_OFFSET  },
  /* 23 */
  { NACLi_386,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeLockable) | NACL_IFLAG(OperandSize_b),
    InstAnd, 0x00, 2, 4, NACL_OPCODE_NULL_OFFSET  },
  /* 24 */
  { NACLi_386,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeLockable) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v),
    InstAnd, 0x00, 2, 6, NACL_OPCODE_NULL_OFFSET  },
  /* 25 */
  { NACLi_386,
    NACL_IFLAG(OpcodeHasImmed_b) | NACL_IFLAG(OpcodeLockable),
    InstAnd, 0x00, 2, 8, NACL_OPCODE_NULL_OFFSET  },
  /* 26 */
  { NACLi_386,
    NACL_IFLAG(OpcodeHasImmed_z) | NACL_IFLAG(OpcodeLockable) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v),
    InstAnd, 0x00, 2, 10, NACL_OPCODE_NULL_OFFSET  },
  /* 27 */
  { NACLi_386,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeLockable) | NACL_IFLAG(OperandSize_b),
    InstSub, 0x00, 2, 0, NACL_OPCODE_NULL_OFFSET  },
  /* 28 */
  { NACLi_386,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeLockable) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v),
    InstSub, 0x00, 2, 2, NACL_OPCODE_NULL_OFFSET  },
  /* 29 */
  { NACLi_386,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeLockable) | NACL_IFLAG(OperandSize_b),
    InstSub, 0x00, 2, 4, NACL_OPCODE_NULL_OFFSET  },
  /* 30 */
  { NACLi_386,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeLockable) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v),
    InstSub, 0x00, 2, 6, NACL_OPCODE_NULL_OFFSET  },
  /* 31 */
  { NACLi_386,
    NACL_IFLAG(OpcodeHasImmed_b) | NACL_IFLAG(OpcodeLockable),
    InstSub, 0x00, 2, 8, NACL_OPCODE_NULL_OFFSET  },
  /* 32 */
  { NACLi_386,
    NACL_IFLAG(OpcodeHasImmed_z) | NACL_IFLAG(OpcodeLockable) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v),
    InstSub, 0x00, 2, 10, NACL_OPCODE_NULL_OFFSET  },
  /* 33 */
  { NACLi_386,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OperandSize_b) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 2, 12, NACL_OPCODE_NULL_OFFSET  },
  /* 34 */
  { NACLi_386,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 2, 14, NACL_OPCODE_NULL_OFFSET  },
  /* 35 */
  { NACLi_386,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OperandSize_b) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 2, 16, NACL_OPCODE_NULL_OFFSET  },
  /* 36 */
  { NACLi_386,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 2, 18, NACL_OPCODE_NULL_OFFSET  },
  /* 37 */
  { NACLi_386,
    NACL_IFLAG(OpcodeHasImmed_b) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 2, 20, NACL_OPCODE_NULL_OFFSET  },
  /* 38 */
  { NACLi_386,
    NACL_IFLAG(OpcodeHasImmed_z) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 2, 22, NACL_OPCODE_NULL_OFFSET  },
  /* 39 */
  { NACLi_386,
    NACL_IFLAG(OpcodeLockable) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 1, 10, NACL_OPCODE_NULL_OFFSET  },
  /* 40 */
  { NACLi_386,
    NACL_IFLAG(OpcodeLockable) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 1, 24, NACL_OPCODE_NULL_OFFSET  },
  /* 41 */
  { NACLi_386,
    NACL_IFLAG(OpcodeLockable) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 1, 25, NACL_OPCODE_NULL_OFFSET  },
  /* 42 */
  { NACLi_386,
    NACL_IFLAG(OpcodeLockable) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 1, 26, NACL_OPCODE_NULL_OFFSET  },
  /* 43 */
  { NACLi_386,
    NACL_IFLAG(OpcodeLockable) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 1, 27, NACL_OPCODE_NULL_OFFSET  },
  /* 44 */
  { NACLi_386,
    NACL_IFLAG(OpcodeLockable) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 1, 28, NACL_OPCODE_NULL_OFFSET  },
  /* 45 */
  { NACLi_386,
    NACL_IFLAG(OpcodeLockable) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 1, 29, NACL_OPCODE_NULL_OFFSET  },
  /* 46 */
  { NACLi_386,
    NACL_IFLAG(OpcodeLockable) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 1, 30, NACL_OPCODE_NULL_OFFSET  },
  /* 47 */
  { NACLi_386,
    NACL_IFLAG(OpcodePlusR) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v),
    InstPush, 0x00, 2, 31, NACL_OPCODE_NULL_OFFSET  },
  /* 48 */
  { NACLi_386,
    NACL_IFLAG(OpcodePlusR) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v),
    InstPush, 0x01, 2, 31, NACL_OPCODE_NULL_OFFSET  },
  /* 49 */
  { NACLi_386,
    NACL_IFLAG(OpcodePlusR) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v),
    InstPush, 0x02, 2, 31, NACL_OPCODE_NULL_OFFSET  },
  /* 50 */
  { NACLi_386,
    NACL_IFLAG(OpcodePlusR) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v),
    InstPush, 0x03, 2, 31, NACL_OPCODE_NULL_OFFSET  },
  /* 51 */
  { NACLi_386,
    NACL_IFLAG(OpcodePlusR) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v),
    InstPush, 0x04, 2, 31, NACL_OPCODE_NULL_OFFSET  },
  /* 52 */
  { NACLi_386,
    NACL_IFLAG(OpcodePlusR) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v),
    InstPush, 0x05, 2, 31, NACL_OPCODE_NULL_OFFSET  },
  /* 53 */
  { NACLi_386,
    NACL_IFLAG(OpcodePlusR) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v),
    InstPush, 0x06, 2, 31, NACL_OPCODE_NULL_OFFSET  },
  /* 54 */
  { NACLi_386,
    NACL_IFLAG(OpcodePlusR) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v),
    InstPush, 0x07, 2, 31, NACL_OPCODE_NULL_OFFSET  },
  /* 55 */
  { NACLi_386,
    NACL_IFLAG(OpcodePlusR) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v),
    InstPop, 0x00, 2, 33, NACL_OPCODE_NULL_OFFSET  },
  /* 56 */
  { NACLi_386,
    NACL_IFLAG(OpcodePlusR) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v),
    InstPop, 0x01, 2, 33, NACL_OPCODE_NULL_OFFSET  },
  /* 57 */
  { NACLi_386,
    NACL_IFLAG(OpcodePlusR) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v),
    InstPop, 0x02, 2, 33, NACL_OPCODE_NULL_OFFSET  },
  /* 58 */
  { NACLi_386,
    NACL_IFLAG(OpcodePlusR) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v),
    InstPop, 0x03, 2, 33, NACL_OPCODE_NULL_OFFSET  },
  /* 59 */
  { NACLi_386,
    NACL_IFLAG(OpcodePlusR) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v),
    InstPop, 0x04, 2, 33, NACL_OPCODE_NULL_OFFSET  },
  /* 60 */
  { NACLi_386,
    NACL_IFLAG(OpcodePlusR) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v),
    InstPop, 0x05, 2, 33, NACL_OPCODE_NULL_OFFSET  },
  /* 61 */
  { NACLi_386,
    NACL_IFLAG(OpcodePlusR) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v),
    InstPop, 0x06, 2, 33, NACL_OPCODE_NULL_OFFSET  },
  /* 62 */
  { NACLi_386,
    NACL_IFLAG(OpcodePlusR) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v),
    InstPop, 0x07, 2, 33, NACL_OPCODE_NULL_OFFSET  },
  /* 63 */
  { NACLi_386,
    NACL_IFLAG(OperandSize_v) | NACL_IFLAG(NaClIllegal) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 0, 0, NACL_OPCODE_NULL_OFFSET  },
  /* 64 */
  { NACLi_386,
    NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(NaClIllegal) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 0, 0, 63  },
  /* 65 */
  { NACLi_386,
    NACL_IFLAG(ModRmModIsnt0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(NaClIllegal) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 0, 0, NACL_OPCODE_NULL_OFFSET  },
  /* 66 */
  { NACLi_SYSTEM,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(NaClIllegal) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 0, 0, NACL_OPCODE_NULL_OFFSET  },
  /* 67 */
  { NACLi_386,
    NACL_IFLAG(OpcodeHasImmed_z) | NACL_IFLAG(OpcodeAllowsData16),
    InstPush, 0x00, 2, 35, NACL_OPCODE_NULL_OFFSET  },
  /* 68 */
  { NACLi_386,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeHasImmed_z) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 3, 37, NACL_OPCODE_NULL_OFFSET  },
  /* 69 */
  { NACLi_386,
    NACL_IFLAG(OpcodeHasImmed_b),
    InstPush, 0x00, 2, 40, NACL_OPCODE_NULL_OFFSET  },
  /* 70 */
  { NACLi_386,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeHasImmed_b) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 3, 42, NACL_OPCODE_NULL_OFFSET  },
  /* 71 */
  { NACLi_386,
    NACL_IFLAG(OpcodeAllowsRep) | NACL_IFLAG(OperandSize_b) | NACL_IFLAG(NaClIllegal) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 0, 0, NACL_OPCODE_NULL_OFFSET  },
  /* 72 */
  { NACLi_386,
    NACL_IFLAG(OpcodeAllowsRep) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(NaClIllegal) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 0, 0, NACL_OPCODE_NULL_OFFSET  },
  /* 73 */
  { NACLi_386,
    NACL_IFLAG(OpcodeAllowsRep) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(NaClIllegal) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 0, 0, 72  },
  /* 74 */
  { NACLi_386,
    NACL_IFLAG(OpcodeHasImmed) | NACL_IFLAG(OperandSize_b) | NACL_IFLAG(ConditionalJump) | NACL_IFLAG(PartialInstruction),
    InstDontCareCondJump, 0x00, 2, 45, NACL_OPCODE_NULL_OFFSET  },
  /* 75 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeHasImmed) | NACL_IFLAG(OperandSize_b) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x07, 2, 47, NACL_OPCODE_NULL_OFFSET  },
  /* 76 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeHasImmed) | NACL_IFLAG(OpcodeLockable) | NACL_IFLAG(OperandSize_b) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x06, 2, 49, 75  },
  /* 77 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeHasImmed) | NACL_IFLAG(OpcodeLockable) | NACL_IFLAG(OperandSize_b),
    InstSub, 0x05, 2, 49, 76  },
  /* 78 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeHasImmed) | NACL_IFLAG(OpcodeLockable) | NACL_IFLAG(OperandSize_b),
    InstAnd, 0x04, 2, 49, 77  },
  /* 79 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeHasImmed) | NACL_IFLAG(OpcodeLockable) | NACL_IFLAG(OperandSize_b) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x03, 2, 49, 78  },
  /* 80 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeHasImmed) | NACL_IFLAG(OpcodeLockable) | NACL_IFLAG(OperandSize_b) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x02, 2, 49, 79  },
  /* 81 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeHasImmed) | NACL_IFLAG(OpcodeLockable) | NACL_IFLAG(OperandSize_b),
    InstOr, 0x01, 2, 49, 80  },
  /* 82 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeHasImmed) | NACL_IFLAG(OpcodeLockable) | NACL_IFLAG(OperandSize_b),
    InstAdd, 0x00, 2, 49, 81  },
  /* 83 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeHasImmed_z) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x07, 2, 38, NACL_OPCODE_NULL_OFFSET  },
  /* 84 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeHasImmed_z) | NACL_IFLAG(OpcodeLockable) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x06, 2, 51, 83  },
  /* 85 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeHasImmed_z) | NACL_IFLAG(OpcodeLockable) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v),
    InstSub, 0x05, 2, 51, 84  },
  /* 86 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeHasImmed_z) | NACL_IFLAG(OpcodeLockable) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v),
    InstAnd, 0x04, 2, 51, 85  },
  /* 87 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeHasImmed_z) | NACL_IFLAG(OpcodeLockable) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x03, 2, 51, 86  },
  /* 88 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeHasImmed_z) | NACL_IFLAG(OpcodeLockable) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x02, 2, 51, 87  },
  /* 89 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeHasImmed_z) | NACL_IFLAG(OpcodeLockable) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v),
    InstOr, 0x01, 2, 51, 88  },
  /* 90 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeHasImmed_z) | NACL_IFLAG(OpcodeLockable) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v),
    InstAdd, 0x00, 2, 51, 89  },
  /* 91 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeHasImmed) | NACL_IFLAG(OperandSize_b) | NACL_IFLAG(NaClIllegal) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x07, 0, 0, NACL_OPCODE_NULL_OFFSET  },
  /* 92 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeHasImmed) | NACL_IFLAG(OpcodeLockable) | NACL_IFLAG(OperandSize_b) | NACL_IFLAG(NaClIllegal) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x06, 0, 0, 91  },
  /* 93 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeHasImmed) | NACL_IFLAG(OpcodeLockable) | NACL_IFLAG(OperandSize_b) | NACL_IFLAG(NaClIllegal) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x05, 0, 0, 92  },
  /* 94 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeHasImmed) | NACL_IFLAG(OpcodeLockable) | NACL_IFLAG(OperandSize_b) | NACL_IFLAG(NaClIllegal) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x04, 0, 0, 93  },
  /* 95 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeHasImmed) | NACL_IFLAG(OpcodeLockable) | NACL_IFLAG(OperandSize_b) | NACL_IFLAG(NaClIllegal) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x03, 0, 0, 94  },
  /* 96 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeHasImmed) | NACL_IFLAG(OpcodeLockable) | NACL_IFLAG(OperandSize_b) | NACL_IFLAG(NaClIllegal) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x02, 0, 0, 95  },
  /* 97 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeHasImmed) | NACL_IFLAG(OpcodeLockable) | NACL_IFLAG(OperandSize_b) | NACL_IFLAG(NaClIllegal) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x01, 0, 0, 96  },
  /* 98 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeHasImmed) | NACL_IFLAG(OpcodeLockable) | NACL_IFLAG(OperandSize_b) | NACL_IFLAG(NaClIllegal) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 0, 0, 97  },
  /* 99 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeHasImmed_b) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x07, 2, 43, NACL_OPCODE_NULL_OFFSET  },
  /* 100 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeHasImmed_b) | NACL_IFLAG(OpcodeLockable) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x06, 2, 53, 99  },
  /* 101 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeHasImmed_b) | NACL_IFLAG(OpcodeLockable) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v),
    InstSub, 0x05, 2, 53, 100  },
  /* 102 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeHasImmed_b) | NACL_IFLAG(OpcodeLockable) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v),
    InstAnd, 0x04, 2, 53, 101  },
  /* 103 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeHasImmed_b) | NACL_IFLAG(OpcodeLockable) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x03, 2, 53, 102  },
  /* 104 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeHasImmed_b) | NACL_IFLAG(OpcodeLockable) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x02, 2, 53, 103  },
  /* 105 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeHasImmed_b) | NACL_IFLAG(OpcodeLockable) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v),
    InstOr, 0x01, 2, 53, 104  },
  /* 106 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeHasImmed_b) | NACL_IFLAG(OpcodeLockable) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v),
    InstAdd, 0x00, 2, 53, 105  },
  /* 107 */
  { NACLi_386,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeLockable) | NACL_IFLAG(OperandSize_b) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 2, 55, NACL_OPCODE_NULL_OFFSET  },
  /* 108 */
  { NACLi_386,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeLockable) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 2, 57, NACL_OPCODE_NULL_OFFSET  },
  /* 109 */
  { NACLi_386,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OperandSize_b),
    InstMov, 0x00, 2, 59, NACL_OPCODE_NULL_OFFSET  },
  /* 110 */
  { NACLi_386,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v),
    InstMov, 0x00, 2, 61, NACL_OPCODE_NULL_OFFSET  },
  /* 111 */
  { NACLi_386,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OperandSize_b),
    InstMov, 0x00, 2, 63, NACL_OPCODE_NULL_OFFSET  },
  /* 112 */
  { NACLi_386,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v),
    InstMov, 0x00, 2, 37, NACL_OPCODE_NULL_OFFSET  },
  /* 113 */
  { NACLi_386,
    NACL_IFLAG(ModRmRegSOperand) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(NaClIllegal) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 0, 0, NACL_OPCODE_NULL_OFFSET  },
  /* 114 */
  { NACLi_386,
    NACL_IFLAG(ModRmModIsnt0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v),
    InstLea, 0x00, 2, 65, NACL_OPCODE_NULL_OFFSET  },
  /* 115 */
  { NACLi_386,
    NACL_IFLAG(ModRmRegSOperand) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(NaClIllegal) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 0, 0, NACL_OPCODE_NULL_OFFSET  },
  /* 116 */
  { NACLi_INVALID,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(NaClIllegal),
    InstInvalid, 0x00, 0, 0, NACL_OPCODE_NULL_OFFSET  },
  /* 117 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v),
    InstPop, 0x00, 2, 67, 116  },
  /* 118 */
  { NACLi_386,
    NACL_IFLAG(OpcodePlusR) | NACL_IFLAG(OpcodeLockable) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 2, 69, NACL_OPCODE_NULL_OFFSET  },
  /* 119 */
  { NACLi_386,
    NACL_IFLAG(OpcodePlusR) | NACL_IFLAG(OpcodeLockable) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x01, 2, 69, NACL_OPCODE_NULL_OFFSET  },
  /* 120 */
  { NACLi_386,
    NACL_IFLAG(OpcodePlusR) | NACL_IFLAG(OpcodeLockable) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x02, 2, 69, NACL_OPCODE_NULL_OFFSET  },
  /* 121 */
  { NACLi_386,
    NACL_IFLAG(OpcodePlusR) | NACL_IFLAG(OpcodeLockable) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x03, 2, 69, NACL_OPCODE_NULL_OFFSET  },
  /* 122 */
  { NACLi_386,
    NACL_IFLAG(OpcodePlusR) | NACL_IFLAG(OpcodeLockable) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x04, 2, 69, NACL_OPCODE_NULL_OFFSET  },
  /* 123 */
  { NACLi_386,
    NACL_IFLAG(OpcodePlusR) | NACL_IFLAG(OpcodeLockable) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x05, 2, 69, NACL_OPCODE_NULL_OFFSET  },
  /* 124 */
  { NACLi_386,
    NACL_IFLAG(OpcodePlusR) | NACL_IFLAG(OpcodeLockable) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x06, 2, 69, NACL_OPCODE_NULL_OFFSET  },
  /* 125 */
  { NACLi_386,
    NACL_IFLAG(OpcodePlusR) | NACL_IFLAG(OpcodeLockable) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x07, 2, 69, NACL_OPCODE_NULL_OFFSET  },
  /* 126 */
  { NACLi_386,
    NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 2, 71, NACL_OPCODE_NULL_OFFSET  },
  /* 127 */
  { NACLi_386,
    NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 2, 73, 126  },
  /* 128 */
  { NACLi_386,
    NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 2, 75, NACL_OPCODE_NULL_OFFSET  },
  /* 129 */
  { NACLi_386,
    NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 2, 77, 128  },
  /* 130 */
  { NACLi_386,
    NACL_IFLAG(OpcodeHasImmed_p) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(NaClIllegal) | NACL_IFLAG(JumpInstruction) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 0, 0, NACL_OPCODE_NULL_OFFSET  },
  /* 131 */
  { NACLi_386,
    NACL_IFLAG(OpcodeHasImmed_v) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(NaClIllegal) | NACL_IFLAG(JumpInstruction) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 0, 0, 130  },
  /* 132 */
  { NACLi_X87,
    NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 0, 0, NACL_OPCODE_NULL_OFFSET  },
  /* 133 */
  { NACLi_386,
    NACL_IFLAG(OpcodeHasImmed_Addr) | NACL_IFLAG(OperandSize_b),
    InstMov, 0x00, 2, 79, NACL_OPCODE_NULL_OFFSET  },
  /* 134 */
  { NACLi_386,
    NACL_IFLAG(OpcodeHasImmed_Addr) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v),
    InstMov, 0x00, 2, 81, NACL_OPCODE_NULL_OFFSET  },
  /* 135 */
  { NACLi_386,
    NACL_IFLAG(OpcodeHasImmed_Addr) | NACL_IFLAG(OperandSize_b),
    InstMov, 0x00, 2, 83, NACL_OPCODE_NULL_OFFSET  },
  /* 136 */
  { NACLi_386,
    NACL_IFLAG(OpcodeHasImmed_Addr) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v),
    InstMov, 0x00, 2, 85, NACL_OPCODE_NULL_OFFSET  },
  /* 137 */
  { NACLi_386,
    NACL_IFLAG(OpcodeAllowsRep) | NACL_IFLAG(OperandSize_b) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 2, 87, NACL_OPCODE_NULL_OFFSET  },
  /* 138 */
  { NACLi_386,
    NACL_IFLAG(OpcodeAllowsRep) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 2, 89, NACL_OPCODE_NULL_OFFSET  },
  /* 139 */
  { NACLi_386,
    NACL_IFLAG(OpcodeAllowsRep) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 2, 91, 138  },
  /* 140 */
  { NACLi_386,
    NACL_IFLAG(OpcodeAllowsRep) | NACL_IFLAG(OpcodeAllowsRepne) | NACL_IFLAG(OperandSize_b) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 2, 93, NACL_OPCODE_NULL_OFFSET  },
  /* 141 */
  { NACLi_386,
    NACL_IFLAG(OpcodeAllowsRep) | NACL_IFLAG(OpcodeAllowsRepne) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 2, 95, NACL_OPCODE_NULL_OFFSET  },
  /* 142 */
  { NACLi_386,
    NACL_IFLAG(OpcodeAllowsRep) | NACL_IFLAG(OpcodeAllowsRepne) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 2, 97, 141  },
  /* 143 */
  { NACLi_386,
    NACL_IFLAG(OpcodeAllowsRep) | NACL_IFLAG(OperandSize_b) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 2, 99, NACL_OPCODE_NULL_OFFSET  },
  /* 144 */
  { NACLi_386,
    NACL_IFLAG(OpcodeAllowsRep) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 2, 101, NACL_OPCODE_NULL_OFFSET  },
  /* 145 */
  { NACLi_386,
    NACL_IFLAG(OpcodeAllowsRep) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 2, 103, 144  },
  /* 146 */
  { NACLi_386,
    NACL_IFLAG(OpcodeAllowsRep) | NACL_IFLAG(OperandSize_b) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 2, 105, NACL_OPCODE_NULL_OFFSET  },
  /* 147 */
  { NACLi_386,
    NACL_IFLAG(OpcodeAllowsRep) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 2, 107, NACL_OPCODE_NULL_OFFSET  },
  /* 148 */
  { NACLi_386,
    NACL_IFLAG(OpcodeAllowsRep) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 2, 109, 147  },
  /* 149 */
  { NACLi_386,
    NACL_IFLAG(OpcodeAllowsRep) | NACL_IFLAG(OpcodeAllowsRepne) | NACL_IFLAG(OperandSize_b) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 2, 111, NACL_OPCODE_NULL_OFFSET  },
  /* 150 */
  { NACLi_386,
    NACL_IFLAG(OpcodeAllowsRep) | NACL_IFLAG(OpcodeAllowsRepne) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 2, 113, NACL_OPCODE_NULL_OFFSET  },
  /* 151 */
  { NACLi_386,
    NACL_IFLAG(OpcodeAllowsRep) | NACL_IFLAG(OpcodeAllowsRepne) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 2, 115, 150  },
  /* 152 */
  { NACLi_386,
    NACL_IFLAG(OpcodeHasImmed) | NACL_IFLAG(OpcodePlusR) | NACL_IFLAG(OperandSize_b),
    InstMov, 0x00, 2, 117, NACL_OPCODE_NULL_OFFSET  },
  /* 153 */
  { NACLi_386,
    NACL_IFLAG(OpcodeHasImmed) | NACL_IFLAG(OpcodePlusR) | NACL_IFLAG(OperandSize_b),
    InstMov, 0x01, 2, 117, NACL_OPCODE_NULL_OFFSET  },
  /* 154 */
  { NACLi_386,
    NACL_IFLAG(OpcodeHasImmed) | NACL_IFLAG(OpcodePlusR) | NACL_IFLAG(OperandSize_b),
    InstMov, 0x02, 2, 117, NACL_OPCODE_NULL_OFFSET  },
  /* 155 */
  { NACLi_386,
    NACL_IFLAG(OpcodeHasImmed) | NACL_IFLAG(OpcodePlusR) | NACL_IFLAG(OperandSize_b),
    InstMov, 0x03, 2, 117, NACL_OPCODE_NULL_OFFSET  },
  /* 156 */
  { NACLi_386,
    NACL_IFLAG(OpcodeHasImmed) | NACL_IFLAG(OpcodePlusR) | NACL_IFLAG(OperandSize_b),
    InstMov, 0x04, 2, 117, NACL_OPCODE_NULL_OFFSET  },
  /* 157 */
  { NACLi_386,
    NACL_IFLAG(OpcodeHasImmed) | NACL_IFLAG(OpcodePlusR) | NACL_IFLAG(OperandSize_b),
    InstMov, 0x05, 2, 117, NACL_OPCODE_NULL_OFFSET  },
  /* 158 */
  { NACLi_386,
    NACL_IFLAG(OpcodeHasImmed) | NACL_IFLAG(OpcodePlusR) | NACL_IFLAG(OperandSize_b),
    InstMov, 0x06, 2, 117, NACL_OPCODE_NULL_OFFSET  },
  /* 159 */
  { NACLi_386,
    NACL_IFLAG(OpcodeHasImmed) | NACL_IFLAG(OpcodePlusR) | NACL_IFLAG(OperandSize_b),
    InstMov, 0x07, 2, 117, NACL_OPCODE_NULL_OFFSET  },
  /* 160 */
  { NACLi_386,
    NACL_IFLAG(OpcodeHasImmed) | NACL_IFLAG(OpcodePlusR) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v),
    InstMov, 0x00, 2, 119, NACL_OPCODE_NULL_OFFSET  },
  /* 161 */
  { NACLi_386,
    NACL_IFLAG(OpcodeHasImmed) | NACL_IFLAG(OpcodePlusR) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v),
    InstMov, 0x01, 2, 119, NACL_OPCODE_NULL_OFFSET  },
  /* 162 */
  { NACLi_386,
    NACL_IFLAG(OpcodeHasImmed) | NACL_IFLAG(OpcodePlusR) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v),
    InstMov, 0x02, 2, 119, NACL_OPCODE_NULL_OFFSET  },
  /* 163 */
  { NACLi_386,
    NACL_IFLAG(OpcodeHasImmed) | NACL_IFLAG(OpcodePlusR) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v),
    InstMov, 0x03, 2, 119, NACL_OPCODE_NULL_OFFSET  },
  /* 164 */
  { NACLi_386,
    NACL_IFLAG(OpcodeHasImmed) | NACL_IFLAG(OpcodePlusR) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v),
    InstMov, 0x04, 2, 119, NACL_OPCODE_NULL_OFFSET  },
  /* 165 */
  { NACLi_386,
    NACL_IFLAG(OpcodeHasImmed) | NACL_IFLAG(OpcodePlusR) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v),
    InstMov, 0x05, 2, 119, NACL_OPCODE_NULL_OFFSET  },
  /* 166 */
  { NACLi_386,
    NACL_IFLAG(OpcodeHasImmed) | NACL_IFLAG(OpcodePlusR) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v),
    InstMov, 0x06, 2, 119, NACL_OPCODE_NULL_OFFSET  },
  /* 167 */
  { NACLi_386,
    NACL_IFLAG(OpcodeHasImmed) | NACL_IFLAG(OpcodePlusR) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v),
    InstMov, 0x07, 2, 119, NACL_OPCODE_NULL_OFFSET  },
  /* 168 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeHasImmed) | NACL_IFLAG(OperandSize_b) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x07, 2, 49, NACL_OPCODE_NULL_OFFSET  },
  /* 169 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeHasImmed) | NACL_IFLAG(OperandSize_b) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x06, 2, 49, 168  },
  /* 170 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeHasImmed) | NACL_IFLAG(OperandSize_b) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x05, 2, 49, 169  },
  /* 171 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeHasImmed) | NACL_IFLAG(OperandSize_b) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x04, 2, 49, 170  },
  /* 172 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeHasImmed) | NACL_IFLAG(OperandSize_b) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x03, 2, 49, 171  },
  /* 173 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeHasImmed) | NACL_IFLAG(OperandSize_b) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x02, 2, 49, 172  },
  /* 174 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeHasImmed) | NACL_IFLAG(OperandSize_b) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x01, 2, 49, 173  },
  /* 175 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeHasImmed) | NACL_IFLAG(OperandSize_b) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 2, 49, 174  },
  /* 176 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeHasImmed_b) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x07, 2, 53, NACL_OPCODE_NULL_OFFSET  },
  /* 177 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeHasImmed_b) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x06, 2, 53, 176  },
  /* 178 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeHasImmed_b) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x05, 2, 53, 177  },
  /* 179 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeHasImmed_b) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x04, 2, 53, 178  },
  /* 180 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeHasImmed_b) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x03, 2, 53, 179  },
  /* 181 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeHasImmed_b) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x02, 2, 53, 180  },
  /* 182 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeHasImmed_b) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x01, 2, 53, 181  },
  /* 183 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeHasImmed_b) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 2, 53, 182  },
  /* 184 */
  { NACLi_386,
    NACL_IFLAG(OpcodeHasImmed_w) | NACL_IFLAG(NaClIllegal) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 0, 0, NACL_OPCODE_NULL_OFFSET  },
  /* 185 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeHasImmed) | NACL_IFLAG(OperandSize_b),
    InstMov, 0x00, 2, 121, 116  },
  /* 186 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeHasImmed_z) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v),
    InstMov, 0x00, 2, 123, 116  },
  /* 187 */
  { NACLi_386,
    NACL_IFLAG(OpcodeHasImmed_w) | NACL_IFLAG(OpcodeHasImmed2_b) | NACL_IFLAG(NaClIllegal) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 0, 0, NACL_OPCODE_NULL_OFFSET  },
  /* 188 */
  { NACLi_RETURN,
    NACL_IFLAG(OpcodeHasImmed_w) | NACL_IFLAG(NaClIllegal) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 0, 0, NACL_OPCODE_NULL_OFFSET  },
  /* 189 */
  { NACLi_RETURN,
    NACL_IFLAG(NaClIllegal) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 0, 0, NACL_OPCODE_NULL_OFFSET  },
  /* 190 */
  { NACLi_SYSTEM,
    NACL_IFLAG(NaClIllegal) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 0, 0, NACL_OPCODE_NULL_OFFSET  },
  /* 191 */
  { NACLi_386,
    NACL_IFLAG(OpcodeHasImmed_b) | NACL_IFLAG(NaClIllegal) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 0, 0, NACL_OPCODE_NULL_OFFSET  },
  /* 192 */
  { NACLi_SYSTEM,
    NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(NaClIllegal) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 0, 0, NACL_OPCODE_NULL_OFFSET  },
  /* 193 */
  { NACLi_SYSTEM,
    NACL_IFLAG(OperandSize_v) | NACL_IFLAG(NaClIllegal) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 0, 0, 192  },
  /* 194 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OperandSize_b) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x07, 2, 125, NACL_OPCODE_NULL_OFFSET  },
  /* 195 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OperandSize_b) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x06, 2, 125, 194  },
  /* 196 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OperandSize_b) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x05, 2, 125, 195  },
  /* 197 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OperandSize_b) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x04, 2, 125, 196  },
  /* 198 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OperandSize_b) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x03, 2, 125, 197  },
  /* 199 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OperandSize_b) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x02, 2, 125, 198  },
  /* 200 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OperandSize_b) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x01, 2, 125, 199  },
  /* 201 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OperandSize_b) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 2, 125, 200  },
  /* 202 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x07, 2, 127, NACL_OPCODE_NULL_OFFSET  },
  /* 203 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x06, 2, 127, 202  },
  /* 204 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x05, 2, 127, 203  },
  /* 205 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x04, 2, 127, 204  },
  /* 206 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x03, 2, 127, 205  },
  /* 207 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x02, 2, 127, 206  },
  /* 208 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x01, 2, 127, 207  },
  /* 209 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 2, 127, 208  },
  /* 210 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OperandSize_b) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x07, 2, 129, NACL_OPCODE_NULL_OFFSET  },
  /* 211 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OperandSize_b) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x06, 2, 129, 210  },
  /* 212 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OperandSize_b) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x05, 2, 129, 211  },
  /* 213 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OperandSize_b) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x04, 2, 129, 212  },
  /* 214 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OperandSize_b) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x03, 2, 129, 213  },
  /* 215 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OperandSize_b) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x02, 2, 129, 214  },
  /* 216 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OperandSize_b) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x01, 2, 129, 215  },
  /* 217 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OperandSize_b) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 2, 129, 216  },
  /* 218 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x07, 2, 131, NACL_OPCODE_NULL_OFFSET  },
  /* 219 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x06, 2, 131, 218  },
  /* 220 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x05, 2, 131, 219  },
  /* 221 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x04, 2, 131, 220  },
  /* 222 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x03, 2, 131, 221  },
  /* 223 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x02, 2, 131, 222  },
  /* 224 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x01, 2, 131, 223  },
  /* 225 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 2, 131, 224  },
  /* 226 */
  { NACLi_X87,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(ModRmModIsnt0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x07, 1, 133, NACL_OPCODE_NULL_OFFSET  },
  /* 227 */
  { NACLi_X87,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(ModRmModIsnt0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x06, 1, 133, 226  },
  /* 228 */
  { NACLi_X87,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(ModRmModIsnt0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x05, 1, 133, 227  },
  /* 229 */
  { NACLi_X87,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(ModRmModIsnt0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x04, 1, 133, 228  },
  /* 230 */
  { NACLi_X87,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(ModRmModIsnt0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x03, 1, 133, 229  },
  /* 231 */
  { NACLi_X87,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(ModRmModIsnt0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x02, 1, 133, 230  },
  /* 232 */
  { NACLi_X87,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(ModRmModIsnt0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x01, 1, 133, 231  },
  /* 233 */
  { NACLi_X87,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(ModRmModIsnt0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 1, 133, 232  },
  /* 234 */
  { NACLi_X87,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(ModRmModIsnt0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x07, 1, 134, NACL_OPCODE_NULL_OFFSET  },
  /* 235 */
  { NACLi_X87,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(ModRmModIsnt0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x06, 1, 135, 234  },
  /* 236 */
  { NACLi_X87,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(ModRmModIsnt0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x05, 1, 136, 235  },
  /* 237 */
  { NACLi_X87,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(ModRmModIsnt0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x04, 1, 137, 236  },
  /* 238 */
  { NACLi_X87,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(ModRmModIsnt0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x03, 1, 138, 237  },
  /* 239 */
  { NACLi_X87,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(ModRmModIsnt0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x02, 1, 138, 238  },
  /* 240 */
  { NACLi_INVALID,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(NaClIllegal),
    InstInvalid, 0x01, 0, 0, 239  },
  /* 241 */
  { NACLi_X87,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(ModRmModIsnt0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 1, 133, 240  },
  /* 242 */
  { NACLi_X87,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(ModRmModIsnt0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x07, 1, 135, NACL_OPCODE_NULL_OFFSET  },
  /* 243 */
  { NACLi_INVALID,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(NaClIllegal),
    InstInvalid, 0x06, 0, 0, 242  },
  /* 244 */
  { NACLi_X87,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(ModRmModIsnt0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x05, 1, 137, 243  },
  /* 245 */
  { NACLi_INVALID,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(NaClIllegal),
    InstInvalid, 0x04, 0, 0, 244  },
  /* 246 */
  { NACLi_X87,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(ModRmModIsnt0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x03, 1, 138, 245  },
  /* 247 */
  { NACLi_X87,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(ModRmModIsnt0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x02, 1, 138, 246  },
  /* 248 */
  { NACLi_X87,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(ModRmModIsnt0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x01, 1, 138, 247  },
  /* 249 */
  { NACLi_X87,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(ModRmModIsnt0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 1, 133, 248  },
  /* 250 */
  { NACLi_X87,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(ModRmModIsnt0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x07, 1, 139, NACL_OPCODE_NULL_OFFSET  },
  /* 251 */
  { NACLi_X87,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(ModRmModIsnt0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x06, 1, 139, 250  },
  /* 252 */
  { NACLi_X87,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(ModRmModIsnt0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x05, 1, 139, 251  },
  /* 253 */
  { NACLi_X87,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(ModRmModIsnt0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x04, 1, 139, 252  },
  /* 254 */
  { NACLi_X87,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(ModRmModIsnt0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x03, 1, 139, 253  },
  /* 255 */
  { NACLi_X87,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(ModRmModIsnt0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x02, 1, 139, 254  },
  /* 256 */
  { NACLi_X87,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(ModRmModIsnt0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x01, 1, 139, 255  },
  /* 257 */
  { NACLi_X87,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(ModRmModIsnt0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 1, 139, 256  },
  /* 258 */
  { NACLi_INVALID,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(NaClIllegal),
    InstInvalid, 0x05, 0, 0, 235  },
  /* 259 */
  { NACLi_X87,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(ModRmModIsnt0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x04, 1, 137, 258  },
  /* 260 */
  { NACLi_X87,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(ModRmModIsnt0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x03, 1, 140, 259  },
  /* 261 */
  { NACLi_X87,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(ModRmModIsnt0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x02, 1, 140, 260  },
  /* 262 */
  { NACLi_X87,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(ModRmModIsnt0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x01, 1, 140, 261  },
  /* 263 */
  { NACLi_X87,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(ModRmModIsnt0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 1, 139, 262  },
  /* 264 */
  { NACLi_X87,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(ModRmModIsnt0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x07, 1, 136, NACL_OPCODE_NULL_OFFSET  },
  /* 265 */
  { NACLi_X87,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(ModRmModIsnt0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x06, 1, 136, 264  },
  /* 266 */
  { NACLi_X87,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(ModRmModIsnt0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x05, 1, 136, 265  },
  /* 267 */
  { NACLi_X87,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(ModRmModIsnt0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x04, 1, 136, 266  },
  /* 268 */
  { NACLi_X87,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(ModRmModIsnt0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x03, 1, 136, 267  },
  /* 269 */
  { NACLi_X87,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(ModRmModIsnt0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x02, 1, 136, 268  },
  /* 270 */
  { NACLi_X87,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(ModRmModIsnt0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x01, 1, 136, 269  },
  /* 271 */
  { NACLi_X87,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(ModRmModIsnt0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 1, 136, 270  },
  /* 272 */
  { NACLi_X87,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(ModRmModIsnt0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x06, 1, 135, 242  },
  /* 273 */
  { NACLi_X87,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(ModRmModIsnt0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x05, 1, 137, 272  },
  /* 274 */
  { NACLi_X87,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(ModRmModIsnt0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x04, 1, 137, 273  },
  /* 275 */
  { NACLi_X87,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(ModRmModIsnt0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x03, 1, 134, 274  },
  /* 276 */
  { NACLi_X87,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(ModRmModIsnt0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x02, 1, 134, 275  },
  /* 277 */
  { NACLi_X87,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(ModRmModIsnt0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x01, 1, 134, 276  },
  /* 278 */
  { NACLi_X87,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(ModRmModIsnt0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 1, 136, 277  },
  /* 279 */
  { NACLi_386,
    NACL_IFLAG(OpcodeHasImmed) | NACL_IFLAG(OperandSize_b) | NACL_IFLAG(JumpInstruction) | NACL_IFLAG(PartialInstruction),
    InstDontCareJump, 0x00, 2, 45, NACL_OPCODE_NULL_OFFSET  },
  /* 280 */
  { NACLi_386,
    NACL_IFLAG(OpcodeHasImmed) | NACL_IFLAG(OperandSize_b) | NACL_IFLAG(AddressSize_v) | NACL_IFLAG(ConditionalJump) | NACL_IFLAG(PartialInstruction),
    InstDontCareCondJump, 0x00, 3, 141, NACL_OPCODE_NULL_OFFSET  },
  /* 281 */
  { NACLi_386,
    NACL_IFLAG(OpcodeHasImmed) | NACL_IFLAG(OperandSize_b) | NACL_IFLAG(AddressSize_w) | NACL_IFLAG(ConditionalJump) | NACL_IFLAG(PartialInstruction),
    InstDontCareCondJump, 0x00, 3, 144, 280  },
  /* 282 */
  { NACLi_386,
    NACL_IFLAG(OpcodeHasImmed_b) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(NaClIllegal) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 0, 0, NACL_OPCODE_NULL_OFFSET  },
  /* 283 */
  { NACLi_386,
    NACL_IFLAG(OpcodeHasImmed_z) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(JumpInstruction),
    InstCall, 0x00, 3, 147, NACL_OPCODE_NULL_OFFSET  },
  /* 284 */
  { NACLi_386,
    NACL_IFLAG(OpcodeHasImmed_z) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(JumpInstruction) | NACL_IFLAG(PartialInstruction),
    InstDontCareJump, 0x00, 2, 150, NACL_OPCODE_NULL_OFFSET  },
  /* 285 */
  { NACLi_386,
    NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(NaClIllegal) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 0, 0, NACL_OPCODE_NULL_OFFSET  },
  /* 286 */
  { NACLi_386,
    NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 0, 0, NACL_OPCODE_NULL_OFFSET  },
  /* 287 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OperandSize_b) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x07, 3, 152, NACL_OPCODE_NULL_OFFSET  },
  /* 288 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OperandSize_b) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x06, 3, 152, 287  },
  /* 289 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OperandSize_b) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x05, 3, 152, 288  },
  /* 290 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OperandSize_b) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x04, 3, 152, 289  },
  /* 291 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeLockable) | NACL_IFLAG(OperandSize_b) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x03, 1, 0, 290  },
  /* 292 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeLockable) | NACL_IFLAG(OperandSize_b) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x02, 1, 0, 291  },
  /* 293 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeHasImmed) | NACL_IFLAG(OperandSize_b) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x01, 2, 47, 292  },
  /* 294 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeHasImmed) | NACL_IFLAG(OperandSize_b) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 2, 47, 293  },
  /* 295 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x07, 3, 155, NACL_OPCODE_NULL_OFFSET  },
  /* 296 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x06, 3, 155, 295  },
  /* 297 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x05, 3, 155, 296  },
  /* 298 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x04, 3, 155, 297  },
  /* 299 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeLockable) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x03, 1, 2, 298  },
  /* 300 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeLockable) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x02, 1, 2, 299  },
  /* 301 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeHasImmed_z) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x01, 2, 38, 300  },
  /* 302 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeHasImmed_z) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 2, 38, 301  },
  /* 303 */
  { NACLi_INVALID,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(NaClIllegal),
    InstInvalid, 0x07, 0, 0, NACL_OPCODE_NULL_OFFSET  },
  /* 304 */
  { NACLi_INVALID,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(NaClIllegal),
    InstInvalid, 0x06, 0, 0, 303  },
  /* 305 */
  { NACLi_INVALID,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(NaClIllegal),
    InstInvalid, 0x05, 0, 0, 304  },
  /* 306 */
  { NACLi_INVALID,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(NaClIllegal),
    InstInvalid, 0x04, 0, 0, 305  },
  /* 307 */
  { NACLi_INVALID,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(NaClIllegal),
    InstInvalid, 0x03, 0, 0, 306  },
  /* 308 */
  { NACLi_INVALID,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(NaClIllegal),
    InstInvalid, 0x02, 0, 0, 307  },
  /* 309 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeLockable) | NACL_IFLAG(OperandSize_b) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x01, 1, 0, 308  },
  /* 310 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeLockable) | NACL_IFLAG(OperandSize_b) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 1, 0, 309  },
  /* 311 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v),
    InstPush, 0x06, 2, 158, 303  },
  /* 312 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(ModRmModIsnt0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(NaClIllegal) | NACL_IFLAG(JumpInstruction) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x05, 0, 0, 311  },
  /* 313 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(JumpInstruction) | NACL_IFLAG(PartialInstruction),
    InstDontCareJump, 0x04, 2, 160, 312  },
  /* 314 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(ModRmModIsnt0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(NaClIllegal) | NACL_IFLAG(JumpInstruction) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x03, 0, 0, 313  },
  /* 315 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(JumpInstruction),
    InstCall, 0x02, 3, 162, 314  },
  /* 316 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeLockable) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x01, 1, 2, 315  },
  /* 317 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeLockable) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 1, 2, 316  },
  /* 318 */
  { NACLi_SYSTEM,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(NaClIllegal) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x05, 0, 0, 304  },
  /* 319 */
  { NACLi_SYSTEM,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(NaClIllegal) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x04, 0, 0, 318  },
  /* 320 */
  { NACLi_SYSTEM,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(NaClIllegal) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x03, 0, 0, 319  },
  /* 321 */
  { NACLi_SYSTEM,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(NaClIllegal) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x02, 0, 0, 320  },
  /* 322 */
  { NACLi_SYSTEM,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(NaClIllegal) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x01, 0, 0, 321  },
  /* 323 */
  { NACLi_SYSTEM,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(NaClIllegal) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 0, 0, 322  },
  /* 324 */
  { NACLi_SYSTEM,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(ModRmModIsnt0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(NaClIllegal) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x07, 0, 0, 303  },
  /* 325 */
  { NACLi_INVALID,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(NaClIllegal),
    InstLmsw, 0x06, 1, 165, 324  },
  /* 326 */
  { NACLi_INVALID,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(NaClIllegal),
    InstInvalid, 0x05, 0, 0, 325  },
  /* 327 */
  { NACLi_SYSTEM,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(NaClIllegal) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x04, 0, 0, 326  },
  /* 328 */
  { NACLi_INVALID,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(NaClIllegal),
    InstInvalid, 0x03, 0, 0, 327  },
  /* 329 */
  { NACLi_SVM,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeInModRmRm) | NACL_IFLAG(ModRmModIs0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(NaClIllegal) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x73, 0, 0, 328  },
  /* 330 */
  { NACLi_SVM,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeInModRmRm) | NACL_IFLAG(ModRmModIs0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(NaClIllegal) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x63, 0, 0, 329  },
  /* 331 */
  { NACLi_SVM,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeInModRmRm) | NACL_IFLAG(ModRmModIs0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(NaClIllegal) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x53, 0, 0, 330  },
  /* 332 */
  { NACLi_SVM,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeInModRmRm) | NACL_IFLAG(ModRmModIs0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(NaClIllegal) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x43, 0, 0, 331  },
  /* 333 */
  { NACLi_SVM,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeInModRmRm) | NACL_IFLAG(ModRmModIs0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(NaClIllegal) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x33, 0, 0, 332  },
  /* 334 */
  { NACLi_SVM,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeInModRmRm) | NACL_IFLAG(ModRmModIs0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(NaClIllegal) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x23, 0, 0, 333  },
  /* 335 */
  { NACLi_SVM,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeInModRmRm) | NACL_IFLAG(ModRmModIs0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(NaClIllegal) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x13, 0, 0, 334  },
  /* 336 */
  { NACLi_SYSTEM,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(ModRmModIsnt0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(NaClIllegal) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x03, 0, 0, 335  },
  /* 337 */
  { NACLi_SYSTEM,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(ModRmModIsnt0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(NaClIllegal) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x02, 0, 0, 336  },
  /* 338 */
  { NACLi_INVALID,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(NaClIllegal),
    InstInvalid, 0x01, 0, 0, 337  },
  /* 339 */
  { NACLi_SYSTEM,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeInModRmRm) | NACL_IFLAG(ModRmModIs0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(NaClIllegal) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x11, 0, 0, 338  },
  /* 340 */
  { NACLi_SYSTEM,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeInModRmRm) | NACL_IFLAG(ModRmModIs0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(NaClIllegal) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x01, 0, 0, 339  },
  /* 341 */
  { NACLi_SYSTEM,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(ModRmModIsnt0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(NaClIllegal) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x01, 0, 0, 340  },
  /* 342 */
  { NACLi_SYSTEM,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(ModRmModIsnt0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(NaClIllegal) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 0, 0, 341  },
  /* 343 */
  { NACLi_SYSTEM,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(NaClIllegal) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 0, 0, NACL_OPCODE_NULL_OFFSET  },
  /* 344 */
  { NACLi_3DNOW,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(ModRmModIsnt0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(NaClIllegal) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x07, 0, 0, NACL_OPCODE_NULL_OFFSET  },
  /* 345 */
  { NACLi_3DNOW,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(ModRmModIsnt0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(NaClIllegal) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x06, 0, 0, 344  },
  /* 346 */
  { NACLi_3DNOW,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(ModRmModIsnt0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(NaClIllegal) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x05, 0, 0, 345  },
  /* 347 */
  { NACLi_3DNOW,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(ModRmModIsnt0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(NaClIllegal) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x04, 0, 0, 346  },
  /* 348 */
  { NACLi_3DNOW,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(ModRmModIsnt0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x03, 1, 166, 347  },
  /* 349 */
  { NACLi_3DNOW,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(ModRmModIsnt0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(NaClIllegal) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x02, 0, 0, 348  },
  /* 350 */
  { NACLi_3DNOW,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(ModRmModIsnt0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x01, 1, 166, 349  },
  /* 351 */
  { NACLi_3DNOW,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(ModRmModIsnt0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 1, 166, 350  },
  /* 352 */
  { NACLi_3DNOW,
    NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 0, 0, NACL_OPCODE_NULL_OFFSET  },
  /* 353 */
  { NACLi_INVALID,
    NACL_IFLAG(Opcode0F0F) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeHasImmed_b) | NACL_IFLAG(NaClIllegal),
    InstInvalid, 0x00, 3, 167, NACL_OPCODE_NULL_OFFSET  },
  /* 354 */
  { NACLi_SSE,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 1, 170, NACL_OPCODE_NULL_OFFSET  },
  /* 355 */
  { NACLi_SSE,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 1, 171, NACL_OPCODE_NULL_OFFSET  },
  /* 356 */
  { NACLi_SSE,
    NACL_IFLAG(ModRmModIs0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 1, 172, NACL_OPCODE_NULL_OFFSET  },
  /* 357 */
  { NACLi_SSE,
    NACL_IFLAG(ModRmModIsnt0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 1, 139, 356  },
  /* 358 */
  { NACLi_SSE,
    NACL_IFLAG(ModRmModIsnt0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 1, 140, NACL_OPCODE_NULL_OFFSET  },
  /* 359 */
  { NACLi_SSE,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 1, 173, NACL_OPCODE_NULL_OFFSET  },
  /* 360 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x07, 0, 0, NACL_OPCODE_NULL_OFFSET  },
  /* 361 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x06, 0, 0, 360  },
  /* 362 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x05, 0, 0, 361  },
  /* 363 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x04, 0, 0, 362  },
  /* 364 */
  { NACLi_MMX,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(ModRmModIsnt0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x03, 1, 166, 363  },
  /* 365 */
  { NACLi_MMX,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(ModRmModIsnt0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x02, 1, 166, 364  },
  /* 366 */
  { NACLi_MMX,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(ModRmModIsnt0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x01, 1, 166, 365  },
  /* 367 */
  { NACLi_MMX,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(ModRmModIsnt0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 1, 166, 366  },
  /* 368 */
  { NACLi_386,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(NaClIllegal) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 0, 0, NACL_OPCODE_NULL_OFFSET  },
  /* 369 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 0, 0, 368  },
  /* 370 */
  { NACLi_SYSTEM,
    NACL_IFLAG(ModRmModIs0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(NaClIllegal) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 0, 0, NACL_OPCODE_NULL_OFFSET  },
  /* 371 */
  { NACLi_SSE,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 1, 174, NACL_OPCODE_NULL_OFFSET  },
  /* 372 */
  { NACLi_SSE,
    NACL_IFLAG(ModRmModIsnt0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 1, 175, NACL_OPCODE_NULL_OFFSET  },
  /* 373 */
  { NACLi_SSE,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 1, 176, NACL_OPCODE_NULL_OFFSET  },
  /* 374 */
  { NACLi_RDMSR,
    NACL_IFLAG(NaClIllegal) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 0, 0, NACL_OPCODE_NULL_OFFSET  },
  /* 375 */
  { NACLi_RDTSC,
    NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 2, 177, NACL_OPCODE_NULL_OFFSET  },
  /* 376 */
  { NACLi_SYSENTER,
    NACL_IFLAG(NaClIllegal) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 0, 0, NACL_OPCODE_NULL_OFFSET  },
  /* 377 */
  { NACLi_CMOV,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 2, 37, NACL_OPCODE_NULL_OFFSET  },
  /* 378 */
  { NACLi_SSE,
    NACL_IFLAG(ModRmModIs0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 2, 179, NACL_OPCODE_NULL_OFFSET  },
  /* 379 */
  { NACLi_SSE2,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 1, 170, NACL_OPCODE_NULL_OFFSET  },
  /* 380 */
  { NACLi_SSE2,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 1, 181, NACL_OPCODE_NULL_OFFSET  },
  /* 381 */
  { NACLi_MMX,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 1, 174, NACL_OPCODE_NULL_OFFSET  },
  /* 382 */
  { NACLi_MMX,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 1, 182, NACL_OPCODE_NULL_OFFSET  },
  /* 383 */
  { NACLi_MMX,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(SizeIgnoresData16) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 1, 183, NACL_OPCODE_NULL_OFFSET  },
  /* 384 */
  { NACLi_MMX,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeHasImmed_b) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 2, 184, NACL_OPCODE_NULL_OFFSET  },
  /* 385 */
  { NACLi_MMX,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(ModRmModIs0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeHasImmed_b) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x06, 2, 186, 303  },
  /* 386 */
  { NACLi_INVALID,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(NaClIllegal),
    InstInvalid, 0x05, 0, 0, 385  },
  /* 387 */
  { NACLi_MMX,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(ModRmModIs0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeHasImmed_b) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x04, 2, 186, 386  },
  /* 388 */
  { NACLi_INVALID,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(NaClIllegal),
    InstInvalid, 0x03, 0, 0, 387  },
  /* 389 */
  { NACLi_MMX,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(ModRmModIs0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeHasImmed_b) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x02, 2, 186, 388  },
  /* 390 */
  { NACLi_INVALID,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(NaClIllegal),
    InstInvalid, 0x01, 0, 0, 389  },
  /* 391 */
  { NACLi_INVALID,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(NaClIllegal),
    InstInvalid, 0x00, 0, 0, 390  },
  /* 392 */
  { NACLi_INVALID,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(NaClIllegal),
    InstInvalid, 0x04, 0, 0, 386  },
  /* 393 */
  { NACLi_INVALID,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(NaClIllegal),
    InstInvalid, 0x03, 0, 0, 392  },
  /* 394 */
  { NACLi_MMX,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(ModRmModIs0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeHasImmed_b) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x02, 2, 186, 393  },
  /* 395 */
  { NACLi_INVALID,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(NaClIllegal),
    InstInvalid, 0x01, 0, 0, 394  },
  /* 396 */
  { NACLi_INVALID,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(NaClIllegal),
    InstInvalid, 0x00, 0, 0, 395  },
  /* 397 */
  { NACLi_MMX,
    NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 0, 0, NACL_OPCODE_NULL_OFFSET  },
  /* 398 */
  { NACLi_MMX,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(SizeIgnoresData16) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 1, 188, NACL_OPCODE_NULL_OFFSET  },
  /* 399 */
  { NACLi_MMX,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 1, 189, NACL_OPCODE_NULL_OFFSET  },
  /* 400 */
  { NACLi_386,
    NACL_IFLAG(OpcodeHasImmed_z) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(ConditionalJump) | NACL_IFLAG(PartialInstruction),
    InstDontCareCondJump, 0x00, 2, 150, NACL_OPCODE_NULL_OFFSET  },
  /* 401 */
  { NACLi_386,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OperandSize_b) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 1, 59, NACL_OPCODE_NULL_OFFSET  },
  /* 402 */
  { NACLi_386,
    NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 4, 190, NACL_OPCODE_NULL_OFFSET  },
  /* 403 */
  { NACLi_386,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(NaClIllegal) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 0, 0, NACL_OPCODE_NULL_OFFSET  },
  /* 404 */
  { NACLi_386,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeHasImmed_b) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 3, 194, NACL_OPCODE_NULL_OFFSET  },
  /* 405 */
  { NACLi_386,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 3, 197, NACL_OPCODE_NULL_OFFSET  },
  /* 406 */
  { NACLi_386,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeLockable) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(NaClIllegal) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 0, 0, NACL_OPCODE_NULL_OFFSET  },
  /* 407 */
  { NACLi_386,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeHasImmed_b) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 3, 200, NACL_OPCODE_NULL_OFFSET  },
  /* 408 */
  { NACLi_386,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 3, 203, NACL_OPCODE_NULL_OFFSET  },
  /* 409 */
  { NACLi_SFENCE_CLFLUSH,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(ModRmModIsnt0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x07, 1, 206, NACL_OPCODE_NULL_OFFSET  },
  /* 410 */
  { NACLi_SFENCE_CLFLUSH,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeInModRmRm) | NACL_IFLAG(ModRmModIs0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x77, 0, 0, 409  },
  /* 411 */
  { NACLi_SFENCE_CLFLUSH,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeInModRmRm) | NACL_IFLAG(ModRmModIs0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x67, 0, 0, 410  },
  /* 412 */
  { NACLi_SFENCE_CLFLUSH,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeInModRmRm) | NACL_IFLAG(ModRmModIs0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x57, 0, 0, 411  },
  /* 413 */
  { NACLi_SFENCE_CLFLUSH,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeInModRmRm) | NACL_IFLAG(ModRmModIs0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x47, 0, 0, 412  },
  /* 414 */
  { NACLi_SFENCE_CLFLUSH,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeInModRmRm) | NACL_IFLAG(ModRmModIs0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x37, 0, 0, 413  },
  /* 415 */
  { NACLi_SFENCE_CLFLUSH,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeInModRmRm) | NACL_IFLAG(ModRmModIs0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x27, 0, 0, 414  },
  /* 416 */
  { NACLi_SFENCE_CLFLUSH,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeInModRmRm) | NACL_IFLAG(ModRmModIs0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x17, 0, 0, 415  },
  /* 417 */
  { NACLi_SFENCE_CLFLUSH,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeInModRmRm) | NACL_IFLAG(ModRmModIs0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x07, 0, 0, 416  },
  /* 418 */
  { NACLi_SSE2,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeInModRmRm) | NACL_IFLAG(ModRmModIs0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x76, 0, 0, 417  },
  /* 419 */
  { NACLi_SSE2,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeInModRmRm) | NACL_IFLAG(ModRmModIs0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x66, 0, 0, 418  },
  /* 420 */
  { NACLi_SSE2,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeInModRmRm) | NACL_IFLAG(ModRmModIs0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x56, 0, 0, 419  },
  /* 421 */
  { NACLi_SSE2,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeInModRmRm) | NACL_IFLAG(ModRmModIs0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x46, 0, 0, 420  },
  /* 422 */
  { NACLi_SSE2,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeInModRmRm) | NACL_IFLAG(ModRmModIs0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x36, 0, 0, 421  },
  /* 423 */
  { NACLi_SSE2,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeInModRmRm) | NACL_IFLAG(ModRmModIs0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x26, 0, 0, 422  },
  /* 424 */
  { NACLi_SSE2,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeInModRmRm) | NACL_IFLAG(ModRmModIs0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x16, 0, 0, 423  },
  /* 425 */
  { NACLi_SSE2,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeInModRmRm) | NACL_IFLAG(ModRmModIs0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x06, 0, 0, 424  },
  /* 426 */
  { NACLi_SSE2,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeInModRmRm) | NACL_IFLAG(ModRmModIs0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x75, 0, 0, 425  },
  /* 427 */
  { NACLi_SSE2,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeInModRmRm) | NACL_IFLAG(ModRmModIs0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x65, 0, 0, 426  },
  /* 428 */
  { NACLi_SSE2,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeInModRmRm) | NACL_IFLAG(ModRmModIs0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x55, 0, 0, 427  },
  /* 429 */
  { NACLi_SSE2,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeInModRmRm) | NACL_IFLAG(ModRmModIs0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x45, 0, 0, 428  },
  /* 430 */
  { NACLi_SSE2,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeInModRmRm) | NACL_IFLAG(ModRmModIs0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x35, 0, 0, 429  },
  /* 431 */
  { NACLi_SSE2,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeInModRmRm) | NACL_IFLAG(ModRmModIs0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x25, 0, 0, 430  },
  /* 432 */
  { NACLi_SSE2,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeInModRmRm) | NACL_IFLAG(ModRmModIs0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x15, 0, 0, 431  },
  /* 433 */
  { NACLi_SSE2,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeInModRmRm) | NACL_IFLAG(ModRmModIs0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x05, 0, 0, 432  },
  /* 434 */
  { NACLi_INVALID,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(NaClIllegal),
    InstInvalid, 0x04, 0, 0, 433  },
  /* 435 */
  { NACLi_SSE,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(ModRmModIsnt0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x03, 1, 138, 434  },
  /* 436 */
  { NACLi_SSE,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(ModRmModIsnt0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x02, 1, 133, 435  },
  /* 437 */
  { NACLi_FXSAVE,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(ModRmModIsnt0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(NaClIllegal) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x01, 0, 0, 436  },
  /* 438 */
  { NACLi_FXSAVE,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(ModRmModIsnt0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(NaClIllegal) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 0, 0, 437  },
  /* 439 */
  { NACLi_386,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 2, 6, NACL_OPCODE_NULL_OFFSET  },
  /* 440 */
  { NACLi_386,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeLockable) | NACL_IFLAG(OperandSize_b) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 3, 207, NACL_OPCODE_NULL_OFFSET  },
  /* 441 */
  { NACLi_386,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeLockable) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 3, 210, NACL_OPCODE_NULL_OFFSET  },
  /* 442 */
  { NACLi_386,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 2, 213, NACL_OPCODE_NULL_OFFSET  },
  /* 443 */
  { NACLi_386,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 2, 215, NACL_OPCODE_NULL_OFFSET  },
  /* 444 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeHasImmed_b) | NACL_IFLAG(OpcodeLockable) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(NaClIllegal) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x07, 0, 0, 116  },
  /* 445 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeHasImmed_b) | NACL_IFLAG(OpcodeLockable) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(NaClIllegal) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x06, 0, 0, 444  },
  /* 446 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeHasImmed_b) | NACL_IFLAG(OpcodeLockable) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(NaClIllegal) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x05, 0, 0, 445  },
  /* 447 */
  { NACLi_386,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeHasImmed_b) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(NaClIllegal) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x04, 0, 0, 446  },
  /* 448 */
  { NACLi_386,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 2, 37, NACL_OPCODE_NULL_OFFSET  },
  /* 449 */
  { NACLi_SSE,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeHasImmed_b) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 2, 217, NACL_OPCODE_NULL_OFFSET  },
  /* 450 */
  { NACLi_SSE2,
    NACL_IFLAG(ModRmModIsnt0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(SizeIgnoresData16) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 2, 219, NACL_OPCODE_NULL_OFFSET  },
  /* 451 */
  { NACLi_SSE,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeHasImmed_b) | NACL_IFLAG(SizeIgnoresData16) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 2, 221, NACL_OPCODE_NULL_OFFSET  },
  /* 452 */
  { NACLi_SSE41,
    NACL_IFLAG(ModRmModIs0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeHasImmed_b) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 3, 223, NACL_OPCODE_NULL_OFFSET  },
  /* 453 */
  { NACLi_CMPXCHG8B,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(ModRmModIsnt0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeLockable) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x01, 3, 226, 116  },
  /* 454 */
  { NACLi_386,
    NACL_IFLAG(OpcodePlusR) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 1, 69, NACL_OPCODE_NULL_OFFSET  },
  /* 455 */
  { NACLi_386,
    NACL_IFLAG(OpcodePlusR) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x01, 1, 69, NACL_OPCODE_NULL_OFFSET  },
  /* 456 */
  { NACLi_386,
    NACL_IFLAG(OpcodePlusR) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x02, 1, 69, NACL_OPCODE_NULL_OFFSET  },
  /* 457 */
  { NACLi_386,
    NACL_IFLAG(OpcodePlusR) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x03, 1, 69, NACL_OPCODE_NULL_OFFSET  },
  /* 458 */
  { NACLi_386,
    NACL_IFLAG(OpcodePlusR) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x04, 1, 69, NACL_OPCODE_NULL_OFFSET  },
  /* 459 */
  { NACLi_386,
    NACL_IFLAG(OpcodePlusR) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x05, 1, 69, NACL_OPCODE_NULL_OFFSET  },
  /* 460 */
  { NACLi_386,
    NACL_IFLAG(OpcodePlusR) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x06, 1, 69, NACL_OPCODE_NULL_OFFSET  },
  /* 461 */
  { NACLi_386,
    NACL_IFLAG(OpcodePlusR) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x07, 1, 69, NACL_OPCODE_NULL_OFFSET  },
  /* 462 */
  { NACLi_MMX,
    NACL_IFLAG(ModRmModIs0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 2, 223, NACL_OPCODE_NULL_OFFSET  },
  /* 463 */
  { NACLi_MMX,
    NACL_IFLAG(ModRmModIsnt0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 1, 140, NACL_OPCODE_NULL_OFFSET  },
  /* 464 */
  { NACLi_MMX,
    NACL_IFLAG(ModRmModIs0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 2, 229, NACL_OPCODE_NULL_OFFSET  },
  /* 465 */
  { NACLi_SSE2,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsRepne) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 1, 231, NACL_OPCODE_NULL_OFFSET  },
  /* 466 */
  { NACLi_SSE2,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsRepne) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 1, 232, NACL_OPCODE_NULL_OFFSET  },
  /* 467 */
  { NACLi_SSE3,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsRepne) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 1, 231, NACL_OPCODE_NULL_OFFSET  },
  /* 468 */
  { NACLi_INVALID,
    NACL_IFLAG(OpcodeAllowsRepne) | NACL_IFLAG(NaClIllegal),
    InstInvalid, 0x00, 0, 0, NACL_OPCODE_NULL_OFFSET  },
  /* 469 */
  { NACLi_SSE2,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsRepne) | NACL_IFLAG(SizeIgnoresData16) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 1, 233, NACL_OPCODE_NULL_OFFSET  },
  /* 470 */
  { NACLi_SSE4A,
    NACL_IFLAG(ModRmModIsnt0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsRepne) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 1, 140, NACL_OPCODE_NULL_OFFSET  },
  /* 471 */
  { NACLi_SSE2,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsRepne) | NACL_IFLAG(SizeIgnoresData16) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 2, 234, NACL_OPCODE_NULL_OFFSET  },
  /* 472 */
  { NACLi_SSE2,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeHasImmed_b) | NACL_IFLAG(OpcodeAllowsRepne) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 2, 236, NACL_OPCODE_NULL_OFFSET  },
  /* 473 */
  { NACLi_SSE4A,
    NACL_IFLAG(ModRmModIs0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeHasImmed_b) | NACL_IFLAG(OpcodeHasImmed2_b) | NACL_IFLAG(OpcodeAllowsRepne) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 3, 238, NACL_OPCODE_NULL_OFFSET  },
  /* 474 */
  { NACLi_SSE4A,
    NACL_IFLAG(ModRmModIs0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsRepne) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 1, 241, NACL_OPCODE_NULL_OFFSET  },
  /* 475 */
  { NACLi_SSE3,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsRepne) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 1, 170, NACL_OPCODE_NULL_OFFSET  },
  /* 476 */
  { NACLi_SSE2,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeHasImmed_b) | NACL_IFLAG(OpcodeAllowsRepne) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 2, 242, NACL_OPCODE_NULL_OFFSET  },
  /* 477 */
  { NACLi_SSE3,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsRepne) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 1, 244, NACL_OPCODE_NULL_OFFSET  },
  /* 478 */
  { NACLi_SSE2,
    NACL_IFLAG(ModRmModIs0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsRepne) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 1, 172, NACL_OPCODE_NULL_OFFSET  },
  /* 479 */
  { NACLi_SSE2,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsRepne) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 1, 244, NACL_OPCODE_NULL_OFFSET  },
  /* 480 */
  { NACLi_SSE3,
    NACL_IFLAG(ModRmModIsnt0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsRepne) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 1, 245, NACL_OPCODE_NULL_OFFSET  },
  /* 481 */
  { NACLi_SSE,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsRep) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 1, 176, NACL_OPCODE_NULL_OFFSET  },
  /* 482 */
  { NACLi_SSE,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsRep) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 1, 246, NACL_OPCODE_NULL_OFFSET  },
  /* 483 */
  { NACLi_SSE3,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsRep) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 1, 170, NACL_OPCODE_NULL_OFFSET  },
  /* 484 */
  { NACLi_INVALID,
    NACL_IFLAG(OpcodeAllowsRep) | NACL_IFLAG(NaClIllegal),
    InstInvalid, 0x00, 0, 0, NACL_OPCODE_NULL_OFFSET  },
  /* 485 */
  { NACLi_SSE,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsRep) | NACL_IFLAG(SizeIgnoresData16) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 1, 233, NACL_OPCODE_NULL_OFFSET  },
  /* 486 */
  { NACLi_SSE4A,
    NACL_IFLAG(ModRmModIsnt0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsRep) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 1, 138, NACL_OPCODE_NULL_OFFSET  },
  /* 487 */
  { NACLi_SSE,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsRep) | NACL_IFLAG(SizeIgnoresData16) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 2, 247, NACL_OPCODE_NULL_OFFSET  },
  /* 488 */
  { NACLi_SSE,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsRep) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 1, 170, NACL_OPCODE_NULL_OFFSET  },
  /* 489 */
  { NACLi_SSE2,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsRep) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 1, 176, NACL_OPCODE_NULL_OFFSET  },
  /* 490 */
  { NACLi_SSE2,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsRep) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 1, 170, NACL_OPCODE_NULL_OFFSET  },
  /* 491 */
  { NACLi_SSE2,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsRep) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 1, 181, NACL_OPCODE_NULL_OFFSET  },
  /* 492 */
  { NACLi_SSE2,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeHasImmed_b) | NACL_IFLAG(OpcodeAllowsRep) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 2, 236, NACL_OPCODE_NULL_OFFSET  },
  /* 493 */
  { NACLi_SSE2,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsRep) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 1, 173, NACL_OPCODE_NULL_OFFSET  },
  /* 494 */
  { NACLi_SSE2,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsRep) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 1, 249, NACL_OPCODE_NULL_OFFSET  },
  /* 495 */
  { NACLi_POPCNT,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsRep) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 2, 37, NACL_OPCODE_NULL_OFFSET  },
  /* 496 */
  { NACLi_LZCNT,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsRep) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 2, 37, NACL_OPCODE_NULL_OFFSET  },
  /* 497 */
  { NACLi_SSE,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeHasImmed_b) | NACL_IFLAG(OpcodeAllowsRep) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 2, 250, NACL_OPCODE_NULL_OFFSET  },
  /* 498 */
  { NACLi_SSE2,
    NACL_IFLAG(ModRmModIs0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsRep) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 1, 224, NACL_OPCODE_NULL_OFFSET  },
  /* 499 */
  { NACLi_SSE2,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(SizeIgnoresData16) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 1, 244, NACL_OPCODE_NULL_OFFSET  },
  /* 500 */
  { NACLi_SSE2,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(SizeIgnoresData16) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 1, 252, NACL_OPCODE_NULL_OFFSET  },
  /* 501 */
  { NACLi_SSE2,
    NACL_IFLAG(ModRmModIsnt0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(SizeIgnoresData16) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 1, 139, NACL_OPCODE_NULL_OFFSET  },
  /* 502 */
  { NACLi_SSE2,
    NACL_IFLAG(ModRmModIsnt0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(SizeIgnoresData16) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 1, 140, NACL_OPCODE_NULL_OFFSET  },
  /* 503 */
  { NACLi_SSE2,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(SizeIgnoresData16) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 1, 173, NACL_OPCODE_NULL_OFFSET  },
  /* 504 */
  { NACLi_SSE2,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(SizeIgnoresData16) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 1, 174, NACL_OPCODE_NULL_OFFSET  },
  /* 505 */
  { NACLi_SSE2,
    NACL_IFLAG(ModRmModIsnt0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(SizeIgnoresData16) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 1, 175, NACL_OPCODE_NULL_OFFSET  },
  /* 506 */
  { NACLi_SSE2,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(SizeIgnoresData16) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 1, 231, NACL_OPCODE_NULL_OFFSET  },
  /* 507 */
  { NACLi_SSE2,
    NACL_IFLAG(ModRmModIs0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(SizeIgnoresData16) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 2, 253, NACL_OPCODE_NULL_OFFSET  },
  /* 508 */
  { NACLi_INVALID,
    NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(SizeIgnoresData16) | NACL_IFLAG(NaClIllegal),
    InstInvalid, 0x00, 0, 0, NACL_OPCODE_NULL_OFFSET  },
  /* 509 */
  { NACLi_SSE2,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(SizeIgnoresData16) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 1, 170, NACL_OPCODE_NULL_OFFSET  },
  /* 510 */
  { NACLi_SSE2,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(SizeIgnoresData16) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 1, 181, NACL_OPCODE_NULL_OFFSET  },
  /* 511 */
  { NACLi_SSE2,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(SizeIgnoresData16) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 1, 183, NACL_OPCODE_NULL_OFFSET  },
  /* 512 */
  { NACLi_SSE2,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeHasImmed_b) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(SizeIgnoresData16) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 2, 255, NACL_OPCODE_NULL_OFFSET  },
  /* 513 */
  { NACLi_INVALID,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(SizeIgnoresData16) | NACL_IFLAG(NaClIllegal),
    InstInvalid, 0x07, 0, 0, NACL_OPCODE_NULL_OFFSET  },
  /* 514 */
  { NACLi_SSE2,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(ModRmModIs0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeHasImmed_b) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(SizeIgnoresData16) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x06, 2, 257, 513  },
  /* 515 */
  { NACLi_INVALID,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(SizeIgnoresData16) | NACL_IFLAG(NaClIllegal),
    InstInvalid, 0x05, 0, 0, 514  },
  /* 516 */
  { NACLi_SSE2,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(ModRmModIs0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeHasImmed_b) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(SizeIgnoresData16) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x04, 2, 257, 515  },
  /* 517 */
  { NACLi_INVALID,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(SizeIgnoresData16) | NACL_IFLAG(NaClIllegal),
    InstInvalid, 0x03, 0, 0, 516  },
  /* 518 */
  { NACLi_SSE2,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(ModRmModIs0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeHasImmed_b) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(SizeIgnoresData16) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x02, 2, 257, 517  },
  /* 519 */
  { NACLi_INVALID,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(SizeIgnoresData16) | NACL_IFLAG(NaClIllegal),
    InstInvalid, 0x01, 0, 0, 518  },
  /* 520 */
  { NACLi_INVALID,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(SizeIgnoresData16) | NACL_IFLAG(NaClIllegal),
    InstInvalid, 0x00, 0, 0, 519  },
  /* 521 */
  { NACLi_SSE2,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(ModRmModIs0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeHasImmed_b) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(SizeIgnoresData16) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x07, 2, 257, NACL_OPCODE_NULL_OFFSET  },
  /* 522 */
  { NACLi_SSE2,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(ModRmModIs0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeHasImmed_b) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(SizeIgnoresData16) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x06, 2, 257, 521  },
  /* 523 */
  { NACLi_INVALID,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(SizeIgnoresData16) | NACL_IFLAG(NaClIllegal),
    InstInvalid, 0x05, 0, 0, 522  },
  /* 524 */
  { NACLi_INVALID,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(SizeIgnoresData16) | NACL_IFLAG(NaClIllegal),
    InstInvalid, 0x04, 0, 0, 523  },
  /* 525 */
  { NACLi_SSE2,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(ModRmModIs0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeHasImmed_b) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(SizeIgnoresData16) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x03, 2, 257, 524  },
  /* 526 */
  { NACLi_SSE2,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(ModRmModIs0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeHasImmed_b) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(SizeIgnoresData16) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x02, 2, 257, 525  },
  /* 527 */
  { NACLi_INVALID,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(SizeIgnoresData16) | NACL_IFLAG(NaClIllegal),
    InstInvalid, 0x01, 0, 0, 526  },
  /* 528 */
  { NACLi_INVALID,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(SizeIgnoresData16) | NACL_IFLAG(NaClIllegal),
    InstInvalid, 0x00, 0, 0, 527  },
  /* 529 */
  { NACLi_INVALID,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(SizeIgnoresData16) | NACL_IFLAG(NaClIllegal),
    InstInvalid, 0x00, 0, 0, NACL_OPCODE_NULL_OFFSET  },
  /* 530 */
  { NACLi_SSE4A,
    NACL_IFLAG(OpcodeInModRm) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeHasImmed_b) | NACL_IFLAG(OpcodeHasImmed2_b) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(SizeIgnoresData16) | NACL_IFLAG(NaClIllegal) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 0, 0, 529  },
  /* 531 */
  { NACLi_SSE4A,
    NACL_IFLAG(ModRmModIs0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(SizeIgnoresData16) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 1, 241, NACL_OPCODE_NULL_OFFSET  },
  /* 532 */
  { NACLi_SSE2,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(SizeIgnoresData16) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 1, 188, NACL_OPCODE_NULL_OFFSET  },
  /* 533 */
  { NACLi_SSE2,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(SizeIgnoresData16) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 1, 249, NACL_OPCODE_NULL_OFFSET  },
  /* 534 */
  { NACLi_SSE2,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeHasImmed_b) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(SizeIgnoresData16) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 2, 259, NACL_OPCODE_NULL_OFFSET  },
  /* 535 */
  { NACLi_SSE,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeHasImmed_b) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(SizeIgnoresData16) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 2, 221, NACL_OPCODE_NULL_OFFSET  },
  /* 536 */
  { NACLi_SSE41,
    NACL_IFLAG(ModRmModIs0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeHasImmed_b) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(SizeIgnoresData16) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 3, 261, NACL_OPCODE_NULL_OFFSET  },
  /* 537 */
  { NACLi_SSE3,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(SizeIgnoresData16) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 1, 244, NACL_OPCODE_NULL_OFFSET  },
  /* 538 */
  { NACLi_SSE2,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(SizeIgnoresData16) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 1, 264, NACL_OPCODE_NULL_OFFSET  },
  /* 539 */
  { NACLi_SSE2,
    NACL_IFLAG(ModRmModIs0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(SizeIgnoresData16) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 2, 261, NACL_OPCODE_NULL_OFFSET  },
  /* 540 */
  { NACLi_SSE2,
    NACL_IFLAG(ModRmModIs0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(SizeIgnoresData16) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 2, 265, NACL_OPCODE_NULL_OFFSET  },
  /* 541 */
  { NACLi_E3DNOW,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 1, 174, NACL_OPCODE_NULL_OFFSET  },
  /* 542 */
  { NACLi_3DNOW,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 1, 174, NACL_OPCODE_NULL_OFFSET  },
  /* 543 */
  { NACLi_SSSE3,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 1, 174, NACL_OPCODE_NULL_OFFSET  },
  /* 544 */
  { NACLi_MOVBE,
    NACL_IFLAG(ModRmModIsnt0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 2, 267, NACL_OPCODE_NULL_OFFSET  },
  /* 545 */
  { NACLi_MOVBE,
    NACL_IFLAG(ModRmModIsnt0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 2, 269, NACL_OPCODE_NULL_OFFSET  },
  /* 546 */
  { NACLi_SSSE3,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(SizeIgnoresData16) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 1, 181, NACL_OPCODE_NULL_OFFSET  },
  /* 547 */
  { NACLi_SSE41,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(SizeIgnoresData16) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 1, 181, NACL_OPCODE_NULL_OFFSET  },
  /* 548 */
  { NACLi_SSE41,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(SizeIgnoresData16) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 1, 271, NACL_OPCODE_NULL_OFFSET  },
  /* 549 */
  { NACLi_SSE41,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(SizeIgnoresData16) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 1, 272, NACL_OPCODE_NULL_OFFSET  },
  /* 550 */
  { NACLi_SSE41,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(SizeIgnoresData16) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 1, 273, NACL_OPCODE_NULL_OFFSET  },
  /* 551 */
  { NACLi_SSE41,
    NACL_IFLAG(ModRmModIsnt0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(SizeIgnoresData16) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 1, 245, NACL_OPCODE_NULL_OFFSET  },
  /* 552 */
  { NACLi_SSE42,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(SizeIgnoresData16) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 1, 181, NACL_OPCODE_NULL_OFFSET  },
  /* 553 */
  { NACLi_VMX,
    NACL_IFLAG(ModRmModIsnt0x3) | NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(SizeIgnoresData16) | NACL_IFLAG(NaClIllegal) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 0, 0, NACL_OPCODE_NULL_OFFSET  },
  /* 554 */
  { NACLi_SSE42,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsRepne) | NACL_IFLAG(OperandSize_b) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 2, 274, NACL_OPCODE_NULL_OFFSET  },
  /* 555 */
  { NACLi_SSE42,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeAllowsRepne) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 2, 276, NACL_OPCODE_NULL_OFFSET  },
  /* 556 */
  { NACLi_SSSE3,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeHasImmed_b) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 2, 184, NACL_OPCODE_NULL_OFFSET  },
  /* 557 */
  { NACLi_SSE41,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeHasImmed_b) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(SizeIgnoresData16) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 2, 255, NACL_OPCODE_NULL_OFFSET  },
  /* 558 */
  { NACLi_SSE41,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeHasImmed_b) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(SizeIgnoresData16) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 2, 250, NACL_OPCODE_NULL_OFFSET  },
  /* 559 */
  { NACLi_SSE41,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeHasImmed_b) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(SizeIgnoresData16) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 2, 242, NACL_OPCODE_NULL_OFFSET  },
  /* 560 */
  { NACLi_SSSE3,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeHasImmed_b) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(SizeIgnoresData16) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 2, 255, NACL_OPCODE_NULL_OFFSET  },
  /* 561 */
  { NACLi_SSE41,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeHasImmed_b) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(SizeIgnoresData16) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 2, 278, NACL_OPCODE_NULL_OFFSET  },
  /* 562 */
  { NACLi_SSE41,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeHasImmed_b) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(SizeIgnoresData16) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 2, 280, NACL_OPCODE_NULL_OFFSET  },
  /* 563 */
  { NACLi_SSE41,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeHasImmed_b) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(SizeIgnoresData16) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 2, 282, NACL_OPCODE_NULL_OFFSET  },
  /* 564 */
  { NACLi_SSE41,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeHasImmed_b) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(SizeIgnoresData16) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 2, 284, NACL_OPCODE_NULL_OFFSET  },
  /* 565 */
  { NACLi_SSE41,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeHasImmed_b) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(SizeIgnoresData16) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 2, 286, NACL_OPCODE_NULL_OFFSET  },
  /* 566 */
  { NACLi_SSE41,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeHasImmed_b) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(SizeIgnoresData16) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 2, 288, NACL_OPCODE_NULL_OFFSET  },
  /* 567 */
  { NACLi_SSE41,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeHasImmed_b) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(SizeIgnoresData16) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 2, 290, NACL_OPCODE_NULL_OFFSET  },
  /* 568 */
  { NACLi_SSE42,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeHasImmed_b) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(SizeIgnoresData16) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 4, 292, NACL_OPCODE_NULL_OFFSET  },
  /* 569 */
  { NACLi_SSE42,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeHasImmed_b) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(SizeIgnoresData16) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 5, 296, NACL_OPCODE_NULL_OFFSET  },
  /* 570 */
  { NACLi_SSE42,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeHasImmed_b) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(SizeIgnoresData16) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 2, 255, NACL_OPCODE_NULL_OFFSET  },
  /* 571 */
  { NACLi_SSE42,
    NACL_IFLAG(OpcodeUsesModRm) | NACL_IFLAG(OpcodeHasImmed_b) | NACL_IFLAG(OpcodeAllowsData16) | NACL_IFLAG(SizeIgnoresData16) | NACL_IFLAG(OperandSize_v) | NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 3, 301, NACL_OPCODE_NULL_OFFSET  },
  /* 572 */
  { NACLi_X87,
    NACL_EMPTY_IFLAGS,
    InstInvalid, 0x00, 0, 0, NACL_OPCODE_NULL_OFFSET  },
  /* 573 */
  { NACLi_X87,
    NACL_IFLAG(PartialInstruction),
    InstDontCare, 0x00, 1, 304, NACL_OPCODE_NULL_OFFSET  },
};

static const NaClPrefixOpcodeArrayOffset g_LookupTable[2543] = {
  /*     0 */ 1, 2, 3, 4, 5, 6, 7, 7, 8, 9, 
  /*    10 */ 10, 11, 12, 13, 7, 14, 15, 16, 17, 18, 
  /*    20 */ 19, 20, 7, 7, 15, 16, 17, 18, 19, 20, 
  /*    30 */ 7, 7, 21, 22, 23, 24, 25, 26, 14, 7, 
  /*    40 */ 27, 28, 29, 30, 31, 32, 14, 7, 15, 16, 
  /*    50 */ 17, 18, 19, 20, 14, 7, 33, 34, 35, 36, 
  /*    60 */ 37, 38, 14, 7, 39, 40, 41, 42, 43, 44, 
  /*    70 */ 45, 46, 39, 40, 41, 42, 43, 44, 45, 46, 
  /*    80 */ 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 
  /*    90 */ 57, 58, 59, 60, 61, 62, 64, 64, 65, 66, 
  /*   100 */ 14, 14, 14, 14, 67, 68, 69, 70, 71, 73, 
  /*   110 */ 71, 73, 74, 74, 74, 74, 74, 74, 74, 74, 
  /*   120 */ 74, 74, 74, 74, 74, 74, 74, 74, 82, 90, 
  /*   130 */ 98, 106, 33, 34, 107, 108, 109, 110, 111, 112, 
  /*   140 */ 113, 114, 115, 117, 118, 119, 120, 121, 122, 123, 
  /*   150 */ 124, 125, 127, 129, 131, 132, 64, 64, 7, 7, 
  /*   160 */ 133, 134, 135, 136, 137, 139, 140, 142, 37, 38, 
  /*   170 */ 143, 145, 146, 148, 149, 151, 152, 153, 154, 155, 
  /*   180 */ 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 
  /*   190 */ 166, 167, 175, 183, 184, 7, 65, 65, 185, 186, 
  /*   200 */ 187, 7, 188, 189, 190, 191, 7, 193, 201, 209, 
  /*   210 */ 217, 225, 191, 191, 7, 7, 233, 241, 233, 249, 
  /*   220 */ 257, 263, 271, 278, 74, 74, 279, 281, 191, 282, 
  /*   230 */ 191, 282, 283, 284, 130, 279, 7, 285, 7, 285, 
  /*   240 */ 14, 7, 14, 14, 286, 286, 294, 302, 286, 286, 
  /*   250 */ 190, 190, 286, 286, 310, 317, 323, 342, 343, 343, 
  /*   260 */ 14, NACL_OPCODE_NULL_OFFSET, 190, NACL_OPCODE_NULL_OFFSET, 190, 190, 14, 7, 14, 351, 
  /*   270 */ 352, 353, 354, 355, 357, 358, 359, 359, 357, 358, 
  /*   280 */ 367, 368, 368, 368, 368, 368, 368, 369, 370, 370, 
  /*   290 */ 370, 370, 14, 14, 14, 14, 354, 355, 371, 372, 
  /*   300 */ 354, 354, 373, 354, 374, 375, 374, 190, 376, 376, 
  /*   310 */ 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 
  /*   320 */ 377, 377, 377, 377, 377, 377, 377, 377, 377, 377, 
  /*   330 */ 377, 377, 377, 377, 377, 377, 378, 354, 354, 354, 
  /*   340 */ 354, 354, 354, 354, 354, 354, 379, 380, 354, 354, 
  /*   350 */ 354, 354, 381, 381, 381, 381, 381, 381, 381, 381, 
  /*   360 */ 382, 382, 382, 381, 14, 14, 383, 381, 384, 391, 
  /*   370 */ 391, 396, 381, 381, 381, 397, 14, 14, 14, 14, 
  /*   380 */ 14, 14, 398, 399, 400, 400, 400, 400, 400, 400, 
  /*   390 */ 400, 400, 400, 400, 400, 400, 400, 400, 400, 400, 
  /*   400 */ 401, 401, 401, 401, 401, 401, 401, 401, 401, 401, 
  /*   410 */ 401, 401, 401, 401, 401, 401, 7, 7, 402, 403, 
  /*   420 */ 404, 405, 14, 14, 7, 7, 190, 406, 407, 408, 
  /*   430 */ 438, 439, 440, 441, 65, 406, 65, 65, 442, 443, 
  /*   440 */ 14, 116, 447, 406, 448, 448, 442, 443, 107, 108, 
  /*   450 */ 449, 450, 451, 452, 449, 453, 454, 455, 456, 457, 
  /*   460 */ 458, 459, 460, 461, 14, 381, 381, 381, 381, 381, 
  /*   470 */ 14, 462, 381, 381, 381, 381, 381, 381, 381, 381, 
  /*   480 */ 381, 381, 381, 381, 381, 381, 14, 463, 381, 381, 
  /*   490 */ 381, 381, 381, 381, 381, 381, 14, 381, 381, 381, 
  /*   500 */ 381, 381, 381, 464, 381, 381, 381, 381, 381, 381, 
  /*   510 */ 381, 14, NACL_OPCODE_NULL_OFFSET, 465, 466, 467, 468, 468, 468, 468, 
  /*   520 */ 468, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, 
  /*   530 */ NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, 468, 468, 469, 
  /*   540 */ 470, 471, 471, 468, 468, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, 
  /*   550 */ NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, 
  /*   560 */ NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, 
  /*   570 */ NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, 468, 465, 468, 
  /*   580 */ 468, 468, 468, 468, 468, 465, 465, 465, 468, 465, 
  /*   590 */ 465, 465, 465, 468, 468, 468, 468, 468, 468, 468, 
  /*   600 */ 468, 468, 468, 468, 468, 468, 468, 468, 468, 472, 
  /*   610 */ 468, 468, 468, 468, 468, 468, 468, 473, 474, 468, 
  /*   620 */ 468, 475, 475, 468, 468, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, 
  /*   630 */ NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, 
  /*   640 */ NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, 
  /*   650 */ NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, 
  /*   660 */ NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, 
  /*   670 */ NACL_OPCODE_NULL_OFFSET, 468, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, 
  /*   680 */ NACL_OPCODE_NULL_OFFSET, 468, 468, 468, 468, 468, 468, 468, 468, NACL_OPCODE_NULL_OFFSET, 
  /*   690 */ NACL_OPCODE_NULL_OFFSET, 476, 468, 468, 468, 468, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, 
  /*   700 */ NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, 477, 468, 468, 468, 468, 
  /*   710 */ 468, 478, 468, 468, 468, 468, 468, 468, 468, 468, 
  /*   720 */ 468, 468, 468, 468, 468, 468, 468, 479, 468, 468, 
  /*   730 */ 468, 468, 468, 468, 468, 468, 468, 480, 468, 468, 
  /*   740 */ 468, 468, 468, 468, 468, 468, 468, 468, 468, 468, 
  /*   750 */ 468, 468, 468, NACL_OPCODE_NULL_OFFSET, 481, 482, 483, 484, 484, 484, 
  /*   760 */ 483, 484, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, 
  /*   770 */ NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, 484, 484, 
  /*   780 */ 485, 486, 487, 487, 484, 484, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, 
  /*   790 */ NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, 
  /*   800 */ NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, 
  /*   810 */ NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, 484, 488, 
  /*   820 */ 481, 481, 484, 484, 484, 484, 481, 481, 489, 490, 
  /*   830 */ 481, 481, 481, 481, 484, 484, 484, 484, 484, 484, 
  /*   840 */ 484, 484, 484, 484, 484, 484, 484, 484, 484, 491, 
  /*   850 */ 492, 484, 484, 484, 484, 484, 484, 484, 484, 484, 
  /*   860 */ 484, 484, 484, 484, 493, 494, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, 
  /*   870 */ NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, 
  /*   880 */ NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, 
  /*   890 */ NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, 
  /*   900 */ NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, 
  /*   910 */ NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, 
  /*   920 */ NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, 495, 484, 484, 484, 484, 496, 484, 484, 
  /*   930 */ NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, 497, 484, 484, 484, 484, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, 
  /*   940 */ NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, 484, 484, 484, 484, 
  /*   950 */ 484, 484, 498, 484, 484, 484, 484, 484, 484, 484, 
  /*   960 */ 484, 484, 484, 484, 484, 484, 484, 484, 493, 484, 
  /*   970 */ 484, 484, 484, 484, 484, 484, 484, 484, 484, 484, 
  /*   980 */ 484, 484, 484, 484, 484, 484, 484, 484, 484, 484, 
  /*   990 */ 484, 484, 484, 484, NACL_OPCODE_NULL_OFFSET, 499, 500, 501, 502, 503, 
  /*  1000 */ 503, 501, 502, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, 
  /*  1010 */ NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, 499, 
  /*  1020 */ 500, 504, 505, 499, 499, 506, 506, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, 
  /*  1030 */ NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, 
  /*  1040 */ NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, 
  /*  1050 */ NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, 507, 
  /*  1060 */ 499, 508, 508, 499, 499, 499, 499, 499, 499, 499, 
  /*  1070 */ 509, 499, 499, 499, 499, 503, 503, 503, 510, 510, 
  /*  1080 */ 510, 510, 510, 503, 503, 503, 510, 503, 503, 511, 
  /*  1090 */ 510, 512, 520, 520, 528, 510, 510, 510, 508, 530, 
  /*  1100 */ 531, 508, 508, 499, 499, 532, 533, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, 
  /*  1110 */ NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, 
  /*  1120 */ NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, 
  /*  1130 */ NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, 
  /*  1140 */ NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, 
  /*  1150 */ NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, 508, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, 
  /*  1160 */ NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, 
  /*  1170 */ NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, 534, 508, 535, 536, 534, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, 
  /*  1180 */ NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, 537, 510, 510, 
  /*  1190 */ 510, 510, 510, 538, 539, 510, 510, 510, 510, 510, 
  /*  1200 */ 510, 510, 510, 510, 510, 510, 510, 510, 510, 510, 
  /*  1210 */ 505, 510, 510, 510, 510, 510, 510, 510, 510, 508, 
  /*  1220 */ 510, 510, 510, 510, 510, 510, 540, 510, 510, 510, 
  /*  1230 */ 510, 510, 510, 510, 508, NACL_OPCODE_NULL_OFFSET, 541, 542, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, 
  /*  1240 */ NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, 
  /*  1250 */ NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, 541, 542, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, 
  /*  1260 */ NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, 
  /*  1270 */ NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, 
  /*  1280 */ NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, 
  /*  1290 */ NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, 
  /*  1300 */ NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, 
  /*  1310 */ NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, 
  /*  1320 */ NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, 
  /*  1330 */ NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, 
  /*  1340 */ NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, 
  /*  1350 */ NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, 
  /*  1360 */ NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, 541, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, 541, NACL_OPCODE_NULL_OFFSET, 542, NACL_OPCODE_NULL_OFFSET, 
  /*  1370 */ NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, 542, NACL_OPCODE_NULL_OFFSET, 542, 542, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, 542, NACL_OPCODE_NULL_OFFSET, 
  /*  1380 */ NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, 542, NACL_OPCODE_NULL_OFFSET, 542, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, 542, NACL_OPCODE_NULL_OFFSET, 
  /*  1390 */ 542, 542, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, 542, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, 542, NACL_OPCODE_NULL_OFFSET, 
  /*  1400 */ 542, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, 542, NACL_OPCODE_NULL_OFFSET, 542, 542, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, 
  /*  1410 */ NACL_OPCODE_NULL_OFFSET, 541, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, 542, NACL_OPCODE_NULL_OFFSET, 543, 543, 543, 
  /*  1420 */ 543, 543, 543, 543, 543, 543, 543, 543, 543, 14, 
  /*  1430 */ 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 
  /*  1440 */ 14, 14, 14, 14, 14, 543, 543, 543, 14, 14, 
  /*  1450 */ 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 
  /*  1460 */ 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 
  /*  1470 */ 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 
  /*  1480 */ 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 
  /*  1490 */ 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 
  /*  1500 */ 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 
  /*  1510 */ 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 
  /*  1520 */ 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 
  /*  1530 */ 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 
  /*  1540 */ 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 
  /*  1550 */ 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 
  /*  1560 */ 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 
  /*  1570 */ 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 
  /*  1580 */ 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 
  /*  1590 */ 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 
  /*  1600 */ 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 
  /*  1610 */ 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 
  /*  1620 */ 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 
  /*  1630 */ 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 
  /*  1640 */ 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 
  /*  1650 */ 14, 14, 14, 14, 14, 14, 14, 544, 545, 14, 
  /*  1660 */ 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 
  /*  1670 */ 14, 14, 14, 546, 546, 546, 546, 546, 546, 546, 
  /*  1680 */ 546, 546, 546, 546, 546, 508, 508, 508, 508, 547, 
  /*  1690 */ 508, 508, 508, 547, 547, 508, 547, 508, 508, 508, 
  /*  1700 */ 508, 546, 546, 546, 508, 548, 549, 550, 548, 549, 
  /*  1710 */ 548, 508, 508, 547, 547, 551, 547, 508, 508, 508, 
  /*  1720 */ 508, 548, 549, 550, 548, 549, 548, 508, 552, 547, 
  /*  1730 */ 547, 547, 547, 547, 547, 547, 547, 547, 547, 508, 
  /*  1740 */ 508, 508, 508, 508, 508, 508, 508, 508, 508, 508, 
  /*  1750 */ 508, 508, 508, 508, 508, 508, 508, 508, 508, 508, 
  /*  1760 */ 508, 508, 508, 508, 508, 508, 508, 508, 508, 508, 
  /*  1770 */ 508, 508, 508, 508, 508, 508, 508, 508, 508, 508, 
  /*  1780 */ 508, 508, 508, 508, 508, 508, 508, 508, 508, 508, 
  /*  1790 */ 508, 508, 508, 508, 508, 508, 508, 508, 508, 508, 
  /*  1800 */ 508, 553, 553, 508, 508, 508, 508, 508, 508, 508, 
  /*  1810 */ 508, 508, 508, 508, 508, 508, 508, 508, 508, 508, 
  /*  1820 */ 508, 508, 508, 508, 508, 508, 508, 508, 508, 508, 
  /*  1830 */ 508, 508, 508, 508, 508, 508, 508, 508, 508, 508, 
  /*  1840 */ 508, 508, 508, 508, 508, 508, 508, 508, 508, 508, 
  /*  1850 */ 508, 508, 508, 508, 508, 508, 508, 508, 508, 508, 
  /*  1860 */ 508, 508, 508, 508, 508, 508, 508, 508, 508, 508, 
  /*  1870 */ 508, 508, 508, 508, 508, 508, 508, 508, 508, 508, 
  /*  1880 */ 508, 508, 508, 508, 508, 508, 508, 508, 508, 508, 
  /*  1890 */ 508, 508, 508, 508, 508, 508, 508, 508, 508, 508, 
  /*  1900 */ 508, 508, 508, 508, 508, 508, 508, 508, 508, 508, 
  /*  1910 */ 508, 508, 508, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, 508, 508, 508, 508, 508, 
  /*  1920 */ 508, 508, 508, 508, 508, 508, 508, 508, 508, NACL_OPCODE_NULL_OFFSET, 
  /*  1930 */ 554, 555, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, 556, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, 557, 557, 558, 
  /*  1940 */ 559, 557, 557, 557, 560, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, 561, 
  /*  1950 */ 562, 563, 564, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, 
  /*  1960 */ NACL_OPCODE_NULL_OFFSET, 565, 566, 567, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, 
  /*  1970 */ NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, 
  /*  1980 */ NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, 
  /*  1990 */ NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, 557, 557, 557, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, 
  /*  2000 */ NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, 
  /*  2010 */ NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, 
  /*  2020 */ NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, 568, 569, 570, 571, NACL_OPCODE_NULL_OFFSET, 
  /*  2030 */ NACL_OPCODE_NULL_OFFSET, 132, 132, 132, 132, 132, 132, 132, 132, 132, 
  /*  2040 */ 132, 132, 132, 132, 132, 132, 132, 132, 132, 132, 
  /*  2050 */ 132, 132, 132, 132, 132, 132, 132, 132, 132, 132, 
  /*  2060 */ 132, 132, 132, 132, 132, 132, 132, 132, 132, 132, 
  /*  2070 */ 132, 132, 132, 132, 132, 132, 132, 132, 132, 132, 
  /*  2080 */ 132, 132, 132, 132, 132, 132, 132, 132, 132, 132, 
  /*  2090 */ 132, 132, 132, 132, 132, NACL_OPCODE_NULL_OFFSET, 132, 132, 132, 132, 
  /*  2100 */ 132, 132, 132, 132, 132, 132, 132, 132, 132, 132, 
  /*  2110 */ 132, 132, 132, 14, 14, 14, 14, 14, 14, 14, 
  /*  2120 */ 14, 14, 14, 14, 14, 14, 14, 14, 132, 132, 
  /*  2130 */ 14, 14, 132, 132, 14, 14, 132, 132, 132, 132, 
  /*  2140 */ 132, 132, 132, 14, 132, 132, 132, 132, 132, 132, 
  /*  2150 */ 132, 132, 132, 132, 132, 132, 132, 132, 132, 132, 
  /*  2160 */ NACL_OPCODE_NULL_OFFSET, 132, 132, 132, 132, 132, 132, 132, 132, 132, 
  /*  2170 */ 132, 132, 132, 132, 132, 132, 132, 132, 132, 132, 
  /*  2180 */ 132, 132, 132, 132, 132, 132, 132, 132, 132, 132, 
  /*  2190 */ 132, 132, 132, 14, 14, 14, 14, 14, 14, 14, 
  /*  2200 */ 14, 14, 132, 14, 14, 14, 14, 14, 14, 14, 
  /*  2210 */ 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 
  /*  2220 */ 14, 14, 14, 14, 14, NACL_OPCODE_NULL_OFFSET, 132, 132, 132, 132, 
  /*  2230 */ 132, 132, 132, 132, 132, 132, 132, 132, 132, 132, 
  /*  2240 */ 132, 132, 132, 132, 132, 132, 132, 132, 132, 132, 
  /*  2250 */ 132, 132, 132, 132, 132, 132, 132, 132, 14, 14, 
  /*  2260 */ 132, 132, 14, 14, 14, 14, 132, 132, 132, 132, 
  /*  2270 */ 132, 132, 132, 132, 132, 132, 132, 132, 132, 132, 
  /*  2280 */ 132, 132, NACL_OPCODE_NULL_OFFSET, NACL_OPCODE_NULL_OFFSET, 132, 132, 132, 132, 132, 132, 
  /*  2290 */ 132, 132, 132, 132, 132, 132, 132, 132, 132, 132, 
  /*  2300 */ 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 
  /*  2310 */ 14, 14, 14, 14, 14, 14, 132, 132, 132, 132, 
  /*  2320 */ 132, 132, 132, 132, 132, 132, 132, 132, 132, 132, 
  /*  2330 */ 132, 132, 132, 132, 132, 132, 132, 132, 132, 132, 
  /*  2340 */ 132, 132, 132, 132, 132, 132, 132, 132, NACL_OPCODE_NULL_OFFSET, 132, 
  /*  2350 */ 132, 132, 132, 132, 132, 132, 132, 14, 14, 14, 
  /*  2360 */ 14, 14, 14, 14, 14, 132, 132, 132, 132, 132, 
  /*  2370 */ 132, 132, 132, 132, 132, 132, 132, 132, 132, 132, 
  /*  2380 */ 132, 132, 132, 132, 132, 132, 132, 132, 132, 132, 
  /*  2390 */ 132, 132, 132, 132, 132, 132, 132, 14, 14, 14, 
  /*  2400 */ 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 
  /*  2410 */ 14, 14, 14, NACL_OPCODE_NULL_OFFSET, 132, 132, 132, 132, 132, 132, 
  /*  2420 */ 132, 132, 132, 132, 132, 132, 132, 132, 132, 132, 
  /*  2430 */ 14, 14, 14, 14, 14, 14, 14, 14, 14, 132, 
  /*  2440 */ 14, 14, 14, 14, 14, 14, 132, 132, 132, 132, 
  /*  2450 */ 132, 132, 132, 132, 132, 132, 132, 132, 132, 132, 
  /*  2460 */ 132, 132, 132, 132, 132, 132, 132, 132, 132, 132, 
  /*  2470 */ 132, 132, 132, 132, 132, 132, 132, 132, NACL_OPCODE_NULL_OFFSET, 572, 
  /*  2480 */ 572, 572, 572, 572, 572, 572, 572, 572, 572, 572, 
  /*  2490 */ 572, 572, 572, 572, 572, 572, 572, 572, 572, 572, 
  /*  2500 */ 572, 572, 572, 572, 572, 572, 572, 572, 572, 572, 
  /*  2510 */ 572, 573, 14, 14, 14, 14, 14, 14, 14, 132, 
  /*  2520 */ 132, 132, 132, 132, 132, 132, 132, 132, 132, 132, 
  /*  2530 */ 132, 132, 132, 132, 132, 14, 14, 14, 14, 14, 
  /*  2540 */ 14, 14, 14, };

static const NaClPrefixOpcodeSelector g_PrefixOpcode[NaClInstPrefixEnumSize] = {
  /*             NoPrefix */ { 0 , 0x00, 0xff },
  /*             Prefix0F */ { 256 , 0x00, 0xff },
  /*           PrefixF20F */ { 512 , 0x0f, 0xff },
  /*           PrefixF30F */ { 753 , 0x0f, 0xff },
  /*           Prefix660F */ { 994 , 0x0f, 0xff },
  /*           Prefix0F0F */ { 1235 , 0x0b, 0xc0 },
  /*           Prefix0F38 */ { 1417 , 0x00, 0xff },
  /*         Prefix660F38 */ { 1673 , 0x00, 0xff },
  /*         PrefixF20F38 */ { 1929 , 0xef, 0xf2 },
  /*           Prefix0F3A */ { 1933 , 0x0e, 0x10 },
  /*         Prefix660F3A */ { 1936 , 0x07, 0x64 },
  /*             PrefixD8 */ { 2030 , 0xbf, 0xff },
  /*             PrefixD9 */ { 2095 , 0xbf, 0xff },
  /*             PrefixDA */ { 2160 , 0xbf, 0xff },
  /*             PrefixDB */ { 2225 , 0xbf, 0xf8 },
  /*             PrefixDC */ { 2283 , 0xbf, 0xff },
  /*             PrefixDD */ { 2348 , 0xbf, 0xff },
  /*             PrefixDE */ { 2413 , 0xbf, 0xff },
  /*             PrefixDF */ { 2478 , 0xbf, 0xff },
};

static const uint32_t kNaClPrefixTable[NCDTABLESIZE] = {
  /* 0x00-0x0f */
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
  /* 0x10-0x1f */
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
  /* 0x20-0x2f */
  0, 0, 0, 0, 0, 0, kPrefixSEGES, 0, 0, 0, 0, 0, 0, 0, kPrefixSEGCS, 0, 
  /* 0x30-0x3f */
  0, 0, 0, 0, 0, 0, kPrefixSEGSS, 0, 0, 0, 0, 0, 0, 0, kPrefixSEGDS, 0, 
  /* 0x40-0x4f */
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
  /* 0x50-0x5f */
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
  /* 0x60-0x6f */
  0, 0, 0, 0, kPrefixSEGFS, kPrefixSEGGS, kPrefixDATA16, kPrefixADDR16, 0, 0, 0, 0, 0, 0, 0, 0, 
  /* 0x70-0x7f */
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
  /* 0x80-0x8f */
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
  /* 0x90-0x9f */
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
  /* 0xa0-0xaf */
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
  /* 0xb0-0xbf */
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
  /* 0xc0-0xcf */
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
  /* 0xd0-0xdf */
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
  /* 0xe0-0xef */
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
  /* 0xf0-0xff */
  kPrefixLOCK, 0, kPrefixREPNE, kPrefixREP, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
};

static const NaClInstNode g_OpcodeSeq[95] = {
  /* 0 */
  { 0x0f,
    NACL_OPCODE_NULL_OFFSET,
    g_OpcodeSeq + 1,
    g_OpcodeSeq + 20,
  },
  /* 1 */
  { 0x0b,
    286,
    NULL,
    g_OpcodeSeq + 2,
  },
  /* 2 */
  { 0x1f,
    NACL_OPCODE_NULL_OFFSET,
    g_OpcodeSeq + 3,
    NULL,
  },
  /* 3 */
  { 0x00,
    286,
    NULL,
    g_OpcodeSeq + 4,
  },
  /* 4 */
  { 0x40,
    NACL_OPCODE_NULL_OFFSET,
    g_OpcodeSeq + 5,
    g_OpcodeSeq + 6,
  },
  /* 5 */
  { 0x00,
    286,
    NULL,
    NULL,
  },
  /* 6 */
  { 0x44,
    NACL_OPCODE_NULL_OFFSET,
    g_OpcodeSeq + 7,
    g_OpcodeSeq + 9,
  },
  /* 7 */
  { 0x00,
    NACL_OPCODE_NULL_OFFSET,
    g_OpcodeSeq + 8,
    NULL,
  },
  /* 8 */
  { 0x00,
    286,
    NULL,
    NULL,
  },
  /* 9 */
  { 0x80,
    NACL_OPCODE_NULL_OFFSET,
    g_OpcodeSeq + 10,
    g_OpcodeSeq + 14,
  },
  /* 10 */
  { 0x00,
    NACL_OPCODE_NULL_OFFSET,
    g_OpcodeSeq + 11,
    NULL,
  },
  /* 11 */
  { 0x00,
    NACL_OPCODE_NULL_OFFSET,
    g_OpcodeSeq + 12,
    NULL,
  },
  /* 12 */
  { 0x00,
    NACL_OPCODE_NULL_OFFSET,
    g_OpcodeSeq + 13,
    NULL,
  },
  /* 13 */
  { 0x00,
    286,
    NULL,
    NULL,
  },
  /* 14 */
  { 0x84,
    NACL_OPCODE_NULL_OFFSET,
    g_OpcodeSeq + 15,
    NULL,
  },
  /* 15 */
  { 0x00,
    NACL_OPCODE_NULL_OFFSET,
    g_OpcodeSeq + 16,
    NULL,
  },
  /* 16 */
  { 0x00,
    NACL_OPCODE_NULL_OFFSET,
    g_OpcodeSeq + 17,
    NULL,
  },
  /* 17 */
  { 0x00,
    NACL_OPCODE_NULL_OFFSET,
    g_OpcodeSeq + 18,
    NULL,
  },
  /* 18 */
  { 0x00,
    NACL_OPCODE_NULL_OFFSET,
    g_OpcodeSeq + 19,
    NULL,
  },
  /* 19 */
  { 0x00,
    286,
    NULL,
    NULL,
  },
  /* 20 */
  { 0x66,
    NACL_OPCODE_NULL_OFFSET,
    g_OpcodeSeq + 21,
    g_OpcodeSeq + 92,
  },
  /* 21 */
  { 0x0f,
    NACL_OPCODE_NULL_OFFSET,
    g_OpcodeSeq + 22,
    g_OpcodeSeq + 32,
  },
  /* 22 */
  { 0x1f,
    NACL_OPCODE_NULL_OFFSET,
    g_OpcodeSeq + 23,
    NULL,
  },
  /* 23 */
  { 0x44,
    NACL_OPCODE_NULL_OFFSET,
    g_OpcodeSeq + 24,
    g_OpcodeSeq + 26,
  },
  /* 24 */
  { 0x00,
    NACL_OPCODE_NULL_OFFSET,
    g_OpcodeSeq + 25,
    NULL,
  },
  /* 25 */
  { 0x00,
    286,
    NULL,
    NULL,
  },
  /* 26 */
  { 0x84,
    NACL_OPCODE_NULL_OFFSET,
    g_OpcodeSeq + 27,
    NULL,
  },
  /* 27 */
  { 0x00,
    NACL_OPCODE_NULL_OFFSET,
    g_OpcodeSeq + 28,
    NULL,
  },
  /* 28 */
  { 0x00,
    NACL_OPCODE_NULL_OFFSET,
    g_OpcodeSeq + 29,
    NULL,
  },
  /* 29 */
  { 0x00,
    NACL_OPCODE_NULL_OFFSET,
    g_OpcodeSeq + 30,
    NULL,
  },
  /* 30 */
  { 0x00,
    NACL_OPCODE_NULL_OFFSET,
    g_OpcodeSeq + 31,
    NULL,
  },
  /* 31 */
  { 0x00,
    286,
    NULL,
    NULL,
  },
  /* 32 */
  { 0x2e,
    NACL_OPCODE_NULL_OFFSET,
    g_OpcodeSeq + 33,
    g_OpcodeSeq + 41,
  },
  /* 33 */
  { 0x0f,
    NACL_OPCODE_NULL_OFFSET,
    g_OpcodeSeq + 34,
    NULL,
  },
  /* 34 */
  { 0x1f,
    NACL_OPCODE_NULL_OFFSET,
    g_OpcodeSeq + 35,
    NULL,
  },
  /* 35 */
  { 0x84,
    NACL_OPCODE_NULL_OFFSET,
    g_OpcodeSeq + 36,
    NULL,
  },
  /* 36 */
  { 0x00,
    NACL_OPCODE_NULL_OFFSET,
    g_OpcodeSeq + 37,
    NULL,
  },
  /* 37 */
  { 0x00,
    NACL_OPCODE_NULL_OFFSET,
    g_OpcodeSeq + 38,
    NULL,
  },
  /* 38 */
  { 0x00,
    NACL_OPCODE_NULL_OFFSET,
    g_OpcodeSeq + 39,
    NULL,
  },
  /* 39 */
  { 0x00,
    NACL_OPCODE_NULL_OFFSET,
    g_OpcodeSeq + 40,
    NULL,
  },
  /* 40 */
  { 0x00,
    286,
    NULL,
    NULL,
  },
  /* 41 */
  { 0x66,
    NACL_OPCODE_NULL_OFFSET,
    g_OpcodeSeq + 42,
    g_OpcodeSeq + 91,
  },
  /* 42 */
  { 0x2e,
    NACL_OPCODE_NULL_OFFSET,
    g_OpcodeSeq + 43,
    g_OpcodeSeq + 51,
  },
  /* 43 */
  { 0x0f,
    NACL_OPCODE_NULL_OFFSET,
    g_OpcodeSeq + 44,
    NULL,
  },
  /* 44 */
  { 0x1f,
    NACL_OPCODE_NULL_OFFSET,
    g_OpcodeSeq + 45,
    NULL,
  },
  /* 45 */
  { 0x84,
    NACL_OPCODE_NULL_OFFSET,
    g_OpcodeSeq + 46,
    NULL,
  },
  /* 46 */
  { 0x00,
    NACL_OPCODE_NULL_OFFSET,
    g_OpcodeSeq + 47,
    NULL,
  },
  /* 47 */
  { 0x00,
    NACL_OPCODE_NULL_OFFSET,
    g_OpcodeSeq + 48,
    NULL,
  },
  /* 48 */
  { 0x00,
    NACL_OPCODE_NULL_OFFSET,
    g_OpcodeSeq + 49,
    NULL,
  },
  /* 49 */
  { 0x00,
    NACL_OPCODE_NULL_OFFSET,
    g_OpcodeSeq + 50,
    NULL,
  },
  /* 50 */
  { 0x00,
    286,
    NULL,
    NULL,
  },
  /* 51 */
  { 0x66,
    NACL_OPCODE_NULL_OFFSET,
    g_OpcodeSeq + 52,
    NULL,
  },
  /* 52 */
  { 0x2e,
    NACL_OPCODE_NULL_OFFSET,
    g_OpcodeSeq + 53,
    g_OpcodeSeq + 61,
  },
  /* 53 */
  { 0x0f,
    NACL_OPCODE_NULL_OFFSET,
    g_OpcodeSeq + 54,
    NULL,
  },
  /* 54 */
  { 0x1f,
    NACL_OPCODE_NULL_OFFSET,
    g_OpcodeSeq + 55,
    NULL,
  },
  /* 55 */
  { 0x84,
    NACL_OPCODE_NULL_OFFSET,
    g_OpcodeSeq + 56,
    NULL,
  },
  /* 56 */
  { 0x00,
    NACL_OPCODE_NULL_OFFSET,
    g_OpcodeSeq + 57,
    NULL,
  },
  /* 57 */
  { 0x00,
    NACL_OPCODE_NULL_OFFSET,
    g_OpcodeSeq + 58,
    NULL,
  },
  /* 58 */
  { 0x00,
    NACL_OPCODE_NULL_OFFSET,
    g_OpcodeSeq + 59,
    NULL,
  },
  /* 59 */
  { 0x00,
    NACL_OPCODE_NULL_OFFSET,
    g_OpcodeSeq + 60,
    NULL,
  },
  /* 60 */
  { 0x00,
    286,
    NULL,
    NULL,
  },
  /* 61 */
  { 0x66,
    NACL_OPCODE_NULL_OFFSET,
    g_OpcodeSeq + 62,
    NULL,
  },
  /* 62 */
  { 0x2e,
    NACL_OPCODE_NULL_OFFSET,
    g_OpcodeSeq + 63,
    g_OpcodeSeq + 71,
  },
  /* 63 */
  { 0x0f,
    NACL_OPCODE_NULL_OFFSET,
    g_OpcodeSeq + 64,
    NULL,
  },
  /* 64 */
  { 0x1f,
    NACL_OPCODE_NULL_OFFSET,
    g_OpcodeSeq + 65,
    NULL,
  },
  /* 65 */
  { 0x84,
    NACL_OPCODE_NULL_OFFSET,
    g_OpcodeSeq + 66,
    NULL,
  },
  /* 66 */
  { 0x00,
    NACL_OPCODE_NULL_OFFSET,
    g_OpcodeSeq + 67,
    NULL,
  },
  /* 67 */
  { 0x00,
    NACL_OPCODE_NULL_OFFSET,
    g_OpcodeSeq + 68,
    NULL,
  },
  /* 68 */
  { 0x00,
    NACL_OPCODE_NULL_OFFSET,
    g_OpcodeSeq + 69,
    NULL,
  },
  /* 69 */
  { 0x00,
    NACL_OPCODE_NULL_OFFSET,
    g_OpcodeSeq + 70,
    NULL,
  },
  /* 70 */
  { 0x00,
    286,
    NULL,
    NULL,
  },
  /* 71 */
  { 0x66,
    NACL_OPCODE_NULL_OFFSET,
    g_OpcodeSeq + 72,
    NULL,
  },
  /* 72 */
  { 0x2e,
    NACL_OPCODE_NULL_OFFSET,
    g_OpcodeSeq + 73,
    g_OpcodeSeq + 81,
  },
  /* 73 */
  { 0x0f,
    NACL_OPCODE_NULL_OFFSET,
    g_OpcodeSeq + 74,
    NULL,
  },
  /* 74 */
  { 0x1f,
    NACL_OPCODE_NULL_OFFSET,
    g_OpcodeSeq + 75,
    NULL,
  },
  /* 75 */
  { 0x84,
    NACL_OPCODE_NULL_OFFSET,
    g_OpcodeSeq + 76,
    NULL,
  },
  /* 76 */
  { 0x00,
    NACL_OPCODE_NULL_OFFSET,
    g_OpcodeSeq + 77,
    NULL,
  },
  /* 77 */
  { 0x00,
    NACL_OPCODE_NULL_OFFSET,
    g_OpcodeSeq + 78,
    NULL,
  },
  /* 78 */
  { 0x00,
    NACL_OPCODE_NULL_OFFSET,
    g_OpcodeSeq + 79,
    NULL,
  },
  /* 79 */
  { 0x00,
    NACL_OPCODE_NULL_OFFSET,
    g_OpcodeSeq + 80,
    NULL,
  },
  /* 80 */
  { 0x00,
    286,
    NULL,
    NULL,
  },
  /* 81 */
  { 0x66,
    NACL_OPCODE_NULL_OFFSET,
    g_OpcodeSeq + 82,
    NULL,
  },
  /* 82 */
  { 0x2e,
    NACL_OPCODE_NULL_OFFSET,
    g_OpcodeSeq + 83,
    NULL,
  },
  /* 83 */
  { 0x0f,
    NACL_OPCODE_NULL_OFFSET,
    g_OpcodeSeq + 84,
    NULL,
  },
  /* 84 */
  { 0x1f,
    NACL_OPCODE_NULL_OFFSET,
    g_OpcodeSeq + 85,
    NULL,
  },
  /* 85 */
  { 0x84,
    NACL_OPCODE_NULL_OFFSET,
    g_OpcodeSeq + 86,
    NULL,
  },
  /* 86 */
  { 0x00,
    NACL_OPCODE_NULL_OFFSET,
    g_OpcodeSeq + 87,
    NULL,
  },
  /* 87 */
  { 0x00,
    NACL_OPCODE_NULL_OFFSET,
    g_OpcodeSeq + 88,
    NULL,
  },
  /* 88 */
  { 0x00,
    NACL_OPCODE_NULL_OFFSET,
    g_OpcodeSeq + 89,
    NULL,
  },
  /* 89 */
  { 0x00,
    NACL_OPCODE_NULL_OFFSET,
    g_OpcodeSeq + 90,
    NULL,
  },
  /* 90 */
  { 0x00,
    286,
    NULL,
    NULL,
  },
  /* 91 */
  { 0x90,
    286,
    NULL,
    NULL,
  },
  /* 92 */
  { 0x90,
    286,
    NULL,
    g_OpcodeSeq + 93,
  },
  /* 93 */
  { 0xf3,
    NACL_OPCODE_NULL_OFFSET,
    g_OpcodeSeq + 94,
    NULL,
  },
  /* 94 */
  { 0x90,
    286,
    NULL,
    NULL,
  },
};
