//=====================================================================================
//
//	模块:	反汇编 MCS-51系列代码
//
//	版本:	1.00
//
//	日期:	2007-07-10
//
//	作者:	施探宇
//
//	说明:	d51 -n rom.bin src.asm
//			d51 -Annn:nnn rom.hex src.asm
//			d51 -Annn,nnn rom.hex src.asm
//=====================================================================================
#include "stdafx.h"

//	保存标号个数
#define MAX_LABELS			(1024)

//	代码地址上限
#define MAX_ADDRESS			(0x10000)

//	最长代码字节(字节对齐)
#define CODE_MAX_BYTES		4

//  普通代码长度(字节对齐)
#define CODE_BYTES			1

//	代码位数(实际指令位数 8 - 24)
#define CODE_BITS			8

//	默认的数据格式
#define FMT_BIT				_T("0x%02x")
#define FMT_DAT				_T("#%03xH")
#define FMT_PTR				_T("0x%02x")
#define FMT_RN				_T("R%d")
#define FMT_RI				_T("@R%d")
#define FMT_LAB				_T("lab_%04x")
#define FMT_LABCR			FMT_LAB _T(":\n")

//	函数结尾处理
#define FMT_RET				_T("\n;--------------------------------------------------------------------------------------") \
							_T("\n; Function:") \
							_T("\n;--------------------------------------------------------------------------------------")

//-----------------------------------------------------------------------------------
//	长整型,所有指令都要改长,这样改成别语言就方便;
//-----------------------------------------------------------------------------------
typedef unsigned long		UINT64;
typedef unsigned int		UINT32;
typedef unsigned short		UINT16;
typedef unsigned char		UINT8;
typedef unsigned char		BYTE;

//-----------------------------------------------------------------------------------
//	反汇编转换表
//-----------------------------------------------------------------------------------
typedef struct tagDASM
{
	//	代码值
	UINT64	CodeData;
	//	代码位(比较匹配)
	UINT64	CodeMask;
	//	寄存器(专用寄存器)
	UINT64	RegsMask;
	//	比特位(内部地址)
	UINT64	BitsMask;
	//	数据位(立即数)
	UINT64	DataMask;
	//	指针位(数据指针)
	UINT64	PtrsMask;
	//	标号位(代码地址)
	UINT64	LabsMask;
	//	字节数(多字节指令)
	UINT8	nBytes;
	//	输出格式
	_TCHAR *pCodeFmt;
}DASM_TBL;

//-----------------------------------------------------------------------------------
//	寄存器名称表
//-----------------------------------------------------------------------------------
typedef struct tagREGS
{
	UINT8 uAddr;
	_TCHAR *pName;

}REGS_TBL;

//-----------------------------------------------------------------------------------
//	代码转换有优先顺序:格式为 printf("CODE %S,%S,%S",REG,BIT,DAT)
//-----------------------------------------------------------------------------------
//	MCS-51 指令
//	共256指令.
//	所有指令宽度需要转换为64位宽.
//
// Lab11 ->code_addr(11位地址)
// Lab   ->code_addr(16位地址)
// Deret ->code_addr(08位地址)
// Ptr   ->data_addr(16位指针)
// Bit   ->data_addr(8位指针)
//
//-----------------------------------------------------------------------------------
DASM_TBL tblCode[]=
{
	//	操作码		有效位		寄存器		比特位		数据位		指针位		标号位		字节	格式
	//------------------------------------------------------------------------------------------------------------------------------------------------
	//  CODE-ID		CODE-MASK	REGS-MASK	BITS-MASK	DATS-MASK	PTRS-MASK	LABS-MASK	BYTES,	FORMAT
	//------------------------------------------------------------------------------------------------------------------------------------------------
	{	0x000000,	0xff0000,	0x000000,	0x000000,	0x000000,	0x000000,	0x000000,	0x1,	_T("NOP")},				// NOP
	{	0x010000,	0x1f0000,	0x000000,	0x000000,	0x000000,	0x000000,	0xe0ff00,	0x2,	_T("AJMP    %s")},		// AJMP Lab11
	{	0x020000,	0xff0000,	0x000000,	0x000000,	0x000000,	0x000000,	0x00ffff,	0x3,	_T("LJMP    %s")},		// LJMP Lab16
	{	0x030000,	0xff0000,	0x000000,	0x000000,	0x000000,	0x000000,	0x000000,	0x1,	_T("RR      A")},		// RR	A
	{	0x040000,	0xff0000,	0x000000,	0x000000,	0x000000,	0x000000,	0x000000,	0x1,	_T("INC     A")},		// INC	A
	{	0x050000,	0xff0000,	0x000000,	0x000000,	0x000000,	0x00ff00,	0x000000,	0x2,	_T("INC     %s")},		// INC	Ptr
	{	0x060000,	0xfe0000,	0x010000,	0x000000,	0x000000,	0x000000,	0x000000,	0x1,	_T("INC     %s")},		// INC	@Ri
	{	0x080000,	0xf80000,	0x070000,	0x000000,	0x000000,	0x000000,	0x000000,	0x1,	_T("INC     %s")},		// INC	Rn
//----------------------------------------------------------------------------------------------------------------------------------
	{	0x100000,	0xff0000,	0x000000,	0x00ff00,	0x000000,	0x000000,	0x0000ff,	0x3,	_T("JBC     %s,%s")},	// JBC	Bit,Lab8
	{	0x110000,	0x1f0000,	0x000000,	0x000000,	0x000000,	0x000000,	0xe0ff00,	0x2,	_T("ACALL   %s")},		// ACALL Lab11
	{	0x120000,	0xff0000,	0x000000,	0x000000,	0x000000,	0x000000,	0x00ffff,	0x3,	_T("LCALL   %s")},		// LCALL Lab
	{	0x130000,	0xff0000,	0x000000,	0x000000,	0x000000,	0x000000,	0x000000,	0x1,	_T("RRC     A")},		// RRC	A
	{	0x140000,	0xff0000,	0x000000,	0x000000,	0x000000,	0x000000,	0x000000,	0x1,	_T("DEC     A")},		// DEC	A
	{	0x150000,	0xff0000,	0x000000,	0x000000,	0x000000,	0x00ff00,	0x000000,	0x2,	_T("DEC     %s")},		// DEC	Ptr
	{	0x160000,	0xfe0000,	0x010000,	0x000000,	0x000000,	0x000000,	0x000000,	0x1,	_T("DEC     %s")},		// DEC	@Ri
	{	0x180000,	0xf80000,	0x070000,	0x000000,	0x000000,	0x000000,	0x000000,	0x1,	_T("DEC     %s")},		// DEC	Rn
//----------------------------------------------------------------------------------------------------------------------------------
	{	0x200000,	0xff0000,	0x000000,	0x00ff00,	0x000000,	0x000000,	0x0000ff,	0x3,	_T("JB      %s,%s")},	// JB	Bit,Lab8
	{	0x220000,	0xff0000,	0x000000,	0x000000,	0x000000,	0x000000,	0x000000,	0x1,	_T("RET") FMT_RET},		// RET
	{	0x230000,	0xff0000,	0x000000,	0x000000,	0x000000,	0x000000,	0x000000,	0x1,	_T("RL      A")},		// RL	A
	{	0x240000,	0xff0000,	0x000000,	0x000000,	0x00ff00,	0x000000,	0x000000,	0x2,	_T("ADD     A,%s")},	// ADD	A,#data
	{	0x250000,	0xff0000,	0x000000,	0x000000,	0x000000,	0x00ff00,	0x000000,	0x2,	_T("ADD     A,%s")},	// ADD	A,Ptr
	{	0x260000,	0xfe0000,	0x010000,	0x000000,	0x000000,	0x000000,	0x000000,	0x1,	_T("ADD     A,%s")},	// ADD	A,@Ri
	{	0x280000,	0xf80000,	0x070000,	0x000000,	0x000000,	0x000000,	0x000000,	0x1,	_T("ADD     A,%s")},	// ADD	A,Rn
//----------------------------------------------------------------------------------------------------------------------------------
	{	0x300000,	0xff0000,	0x000000,	0x00ff00,	0x000000,	0x000000,	0x0000ff,	0x3,	_T("JNB     %s,%s")},	// JNB	bit,Lab8
	{	0x320000,	0xff0000,	0x000000,	0x000000,	0x000000,	0x000000,	0x000000,	0x1,	_T("RETI") FMT_RET},	// RETI
	{	0x330000,	0xff0000,	0x000000,	0x000000,	0x000000,	0x000000,	0x000000,	0x1,	_T("RLC     A")},		// RLC	A
	{	0x340000,	0xff0000,	0x000000,	0x000000,	0x00ff00,	0x000000,	0x000000,	0x2,	_T("ADC     A,%s")},	// ADC	A,#data
	{	0x350000,	0xff0000,	0x000000,	0x000000,	0x000000,	0x00ff00,	0x000000,	0x2,	_T("ADC     A,%s")},	// ADC	A,Ptr
	{	0x360000,	0xfe0000,	0x010000,	0x000000,	0x000000,	0x000000,	0x000000,	0x1,	_T("ADC     A,%s")},	// ADC	A,@Ri
	{	0x380000,	0xf80000,	0x070000,	0x000000,	0x000000,	0x000000,	0x000000,	0x1,	_T("ADC     A,%s")},	// ADC	A,Rn
//----------------------------------------------------------------------------------------------------------------------------------
	{	0x400000,	0xff0000,	0x000000,	0x000000,	0x000000,	0x000000,	0x00ff00,	0x2,	_T("JC      %s")},		// JC	Lab
	{	0x420000,	0xff0000,	0x000000,	0x000000,	0x000000,	0x00ff00,	0x000000,	0x2,	_T("ORL     %s,A")},	// ORL	Ptr,A
	{	0x430000,	0xff0000,	0x000000,	0x000000,	0x0000ff,	0x00ff00,	0x000000,	0x3,	_T("ORL     %s,%s")},	// ORL	Ptr,#data
	{	0x440000,	0xff0000,	0x000000,	0x000000,	0x00ff00,	0x000000,	0x000000,	0x2,	_T("ORL     A,%s")},	// ORL	A,#data
	{	0x450000,	0xff0000,	0x000000,	0x000000,	0x000000,	0x00ff00,	0x000000,	0x2,	_T("ORL     A,%s")},	// ORL	A,Ptr
	{	0x460000,	0xfe0000,	0x010000,	0x000000,	0x000000,	0x000000,	0x000000,	0x1,	_T("ORL     A,%s")},	// ORL	A,@Ri
	{	0x480000,	0xf80000,	0x070000,	0x000000,	0x000000,	0x000000,	0x000000,	0x1,	_T("ORL     A,%s")},	// ORL	A,Rn
//----------------------------------------------------------------------------------------------------------------------------------
	{	0x500000,	0xff0000,	0x000000,	0x000000,	0x000000,	0x000000,	0x00ff00,	0x2,	_T("JNC     %s")},		// JNC	Lab
	{	0x520000,	0xff0000,	0x000000,	0x000000,	0x000000,	0x00ff00,	0x000000,	0x2,	_T("ANL     %s,A")},	// ANL	Ptr,A
	{	0x530000,	0xff0000,	0x000000,	0x000000,	0x0000ff,	0x00ff00,	0x000000,	0x3,	_T("ANL     %s,%s")},	// ANL	Ptr,#data
	{	0x540000,	0xff0000,	0x000000,	0x000000,	0x00ff00,	0x000000,	0x000000,	0x2,	_T("ANL     A,%s")},	// ANL	A,#data
	{	0x550000,	0xff0000,	0x000000,	0x000000,	0x000000,	0x00ff00,	0x000000,	0x2,	_T("ANL     A,%s")},	// ANL	A,Ptr
	{	0x560000,	0xfe0000,	0x010000,	0x000000,	0x000000,	0x000000,	0x000000,	0x1,	_T("ANL     A,%s")},	// ANL	A,@Ri
	{	0x580000,	0xf80000,	0x070000,	0x000000,	0x000000,	0x000000,	0x000000,	0x1,	_T("ANL     A,%s")},	// ANL	A,Rn
//----------------------------------------------------------------------------------------------------------------------------------
	{	0x600000,	0xff0000,	0x000000,	0x000000,	0x000000,	0x000000,	0x00ff00,	0x2,	_T("JZ      %s")},		// JZ	Lab
	{	0x620000,	0xff0000,	0x000000,	0x000000,	0x000000,	0x00ff00,	0x000000,	0x2,	_T("XRL     %s,A")},	// XRL	Ptr,A
	{	0x630000,	0xff0000,	0x000000,	0x000000,	0x0000ff,	0x00ff00,	0x000000,	0x3,	_T("XRL     %s,%s")},	// XRL	Ptr,#data
	{	0x640000,	0xff0000,	0x000000,	0x000000,	0x00ff00,	0x000000,	0x000000,	0x2,	_T("XRL     A,%s")},	// XRL	A,#data
	{	0x650000,	0xff0000,	0x000000,	0x000000,	0x000000,	0x00ff00,	0x000000,	0x2,	_T("XRL     A,%s")},	// XRL	A,Ptr
	{	0x660000,	0xfe0000,	0x010000,	0x000000,	0x000000,	0x000000,	0x000000,	0x1,	_T("XRL     A,%s")},	// XRL	A,@Ri
	{	0x680000,	0xf80000,	0x070000,	0x000000,	0x000000,	0x000000,	0x000000,	0x1,	_T("XRL     A,%s")},	// XRL	A,Rn
//----------------------------------------------------------------------------------------------------------------------------------
	{	0x700000,	0xff0000,	0x000000,	0x000000,	0x000000,	0x000000,	0x00ff00,	0x2,	_T("JNZ     %s")},		// JNZ	Lab
	{	0x720000,	0xff0000,	0x000000,	0x00ff00,	0x000000,	0x000000,	0x000000,	0x2,	_T("ORL     C,%s")},	// ORL	C,Bit
	{	0x730000,	0xff0000,	0x000000,	0x000000,	0x000000,	0x000000,	0x000000,	0x1,	_T("JMP     @A + DPTR")},// JMP	@A+DPTR
	{	0x740000,	0xff0000,	0x000000,	0x000000,	0x00ff00,	0x000000,	0x000000,	0x2,	_T("MOV     A,%s")},	// MOV	A,#data
	{	0x750000,	0xff0000,	0x000000,	0x000000,	0x0000ff,	0x00ff00,	0x000000,	0x3,	_T("MOV     %s,%s")},	// MOV	Ptr,#data
	{	0x760000,	0xfe0000,	0x010000,	0x000000,	0x00ff00,	0x000000,	0x000000,	0x2,	_T("MOV     %s,A")},	// MOV	@Ri,#data
	{	0x780000,	0xf80000,	0x070000,	0x000000,	0x00ff00,	0x000000,	0x000000,	0x2,	_T("MOV     %s,%s")},	// MOV	Rn,#data
//----------------------------------------------------------------------------------------------------------------------------------	
	{	0x800000,	0xff0000,	0x000000,	0x000000,	0x000000,	0x000000,	0x00ff00,	0x2,	_T("SJMP    %s")},		// SJMP Lab
	{	0x820000,	0xff0000,	0x000000,	0x00ff00,	0x000000,	0x000000,	0x000000,	0x2,	_T("ANL     C,%s")},	// ANL	C,bit
	{	0x830000,	0xff0000,	0x000000,	0x000000,	0x000000,	0x000000,	0x000000,	0x1,	_T("MOVC    A,@A + PC")},// MOVC A,@A+PC
	{	0x840000,	0xff0000,	0x000000,	0x000000,	0x000000,	0x000000,	0x000000,	0x1,	_T("DIV     AB")},		// DIV	AB
	{	0x850000,	0xff0000,	0x000000,	0x000000,	0x0000ff,	0x00ff00,	0x000000,	0x3,	_T("MOV     %s,%s")},	// MOV	Ptr,Ptr // 两个指针
	{	0x860000,	0xfe0000,	0x010000,	0x000000,	0x000000,	0x00ff00,	0x000000,	0x2,	_T("MOV     %s,%s")},	// MOV	Ptr,@Ri //
	{	0x880000,	0xf80000,	0x070000,	0x000000,	0x000000,	0x00ff00,	0x000000,	0x2,	_T("MOV     %s,%s")},	// MOV	Ptr,Rn  // 
//----------------------------------------------------------------------------------------------------------------------------------
	{	0x900000,	0xff0000,	0x000000,	0x000000,	0x00ffff,	0x000000,	0x000000,	0x3,	_T("MOV     DPTR,%s")},	// MOV	DPTR,#data
	{	0x920000,	0xff0000,	0x000000,	0x00ff00,	0x000000,	0x000000,	0x000000,	0x2,	_T("MOV     %s,C")},	// MOV	Bit,C
	{	0x930000,	0xff0000,	0x000000,	0x000000,	0x000000,	0x000000,	0x000000,	0x1,	_T("MOVC    A,@A + DPTR")},//MOVC	A,@A+DPTR
	{	0x940000,	0xff0000,	0x000000,	0x000000,	0x00ff00,	0x000000,	0x000000,	0x2,	_T("SUBB    A,%s")},	// SUBB A,#data
	{	0x950000,	0xff0000,	0x000000,	0x000000,	0x000000,	0x00ff00,	0x000000,	0x2,	_T("SUBB    A,%s")},	// SUBB A,Ptr
	{	0x960000,	0xfe0000,	0x010000,	0x000000,	0x000000,	0x000000,	0x000000,	0x1,	_T("SUBB    A,%s")},	// SUBB A,@Ri
	{	0x980000,	0xf80000,	0x070000,	0x000000,	0x000000,	0x000000,	0x000000,	0x1,	_T("SUBB    A,%s")},	// SUBB A,Rn
//----------------------------------------------------------------------------------------------------------------------------------
	{	0xa00000,	0xff0000,	0x00ff00,	0x000000,	0x000000,	0x000000,	0x000000,	0x2,	_T("ORL     C,%s")},	// ORL	C,bit
	{	0xa20000,	0xff0000,	0x00ff00,	0x000000,	0x000000,	0x000000,	0x000000,	0x2,	_T("MOV     C,%s")},	// MOV	C,bit
	{	0xa30000,	0xff0000,	0x000000,	0x000000,	0x000000,	0x000000,	0x000000,	0x1,	_T("INC     DPTR")},	// INC	DPTR
	{	0xa40000,	0xff0000,	0x000000,	0x000000,	0x000000,	0x000000,	0x000000,	0x1,	_T("MUL     AB")},		// INC	DPTR
	//  0xa50000 -- 保留指令
	{	0xa60000,	0xfe0000,	0x010000,	0x000000,	0x000000,	0x00ff00,	0x000000,	0x2,	_T("MOV     %s,%s")},	// MOV	@Ri,Ptr	// 注意跟86反向
	{	0xa80000,	0xf80000,	0x070000,	0x000000,	0x000000,	0x00ff00,	0x000000,	0x2,	_T("MOV     %s,%s")},	// MOV	Rn,Ptr	// 注意跟88反向
//----------------------------------------------------------------------------------------------------------------------------------
	{	0xb00000,	0xff0000,	0x00ff00,	0x000000,	0x000000,	0x000000,	0x000000,	0x2,	_T("ANL     C,%s")},	// MOV	C,bit
	{	0xb20000,	0xff0000,	0x000000,	0x00ff00,	0x000000,	0x000000,	0x000000,	0x2,	_T("CPL     %s")},		// CPL	bit
	{	0xb30000,	0xff0000,	0x000000,	0x000000,	0x000000,	0x000000,	0x000000,	0x1,	_T("CPL     C")},		// CPL	C
	{	0xb40000,	0xff0000,	0x000000,	0x000000,	0x00ff00,	0x000000,	0x0000ff,	0x3,	_T("CJNE    A,%s,%s")}, // CJNE A,#data,Lab
	{	0xb50000,	0xff0000,	0x000000,	0x000000,	0x000000,	0x00ff00,	0x0000ff,	0x3,	_T("CJNE    A,%s,%s")},	// CJNE A,Ptr,Lab
	{	0xb60000,	0xfe0000,	0x010000,	0x000000,	0x00ff00,	0x000000,	0x0000ff,	0x3,	_T("CJNE    %s,%s,%s")},// CJNE @Ri,#data,Lab
	{	0xb80000,	0xf80000,	0x070000,	0x000000,	0x00ff00,	0x000000,	0x0000ff,	0x3,	_T("CJNE    %s,%s,%s")},// CJNE Rn,#data,Lab
//----------------------------------------------------------------------------------------------------------------------------------
	{	0xc00000,	0xff0000,	0x000000,	0x000000,	0x000000,	0x00ff00,	0x000000,	0x2,	_T("PUSH    %s")},		// PUSH	Ptr
	{	0xc20000,	0xff0000,	0x000000,	0x000000,	0x000000,	0x00ff00,	0x000000,	0x2,	_T("CLR     %s")},		// CLR	Ptr
	{	0xc30000,	0xff0000,	0x000000,	0x000000,	0x000000,	0x000000,	0x000000,	0x1,	_T("CLR     C")},		// CLR	C
	{	0xc40000,	0xff0000,	0x000000,	0x000000,	0x000000,	0x000000,	0x000000,	0x1,	_T("SWAP    A")},		// SWAP	A
	{	0xc50000,	0xff0000,	0x000000,	0x000000,	0x000000,	0x00ff00,	0x000000,	0x2,	_T("XCH     A,%s")},	// XCH	A,Ptr
	{	0xc60000,	0xfe0000,	0x010000,	0x000000,	0x000000,	0x000000,	0x000000,	0x1,	_T("XCH     A,%s")},	// XCH	A,@Ri
	{	0xc80000,	0xf80000,	0x070000,	0x000000,	0x000000,	0x000000,	0x000000,	0x1,	_T("XCH     A,%s")},	// XCH	A,Rn
//----------------------------------------------------------------------------------------------------------------------------------
	{	0xd00000,	0xff0000,	0x000000,	0x000000,	0x000000,	0x00ff00,	0x000000,	0x2,	_T("POP     %s")},		// POP	Ptr
	{	0xd20000,	0xff0000,	0x000000,	0x00ff00,	0x000000,	0x000000,	0x000000,	0x2,	_T("SETB    %s")},		// SETB	Bi
	{	0xd30000,	0xff0000,	0x000000,	0x000000,	0x000000,	0x000000,	0x000000,	0x1,	_T("SETB    C")},		// SETB	C
	{	0xd40000,	0xff0000,	0x000000,	0x000000,	0x000000,	0x000000,	0x000000,	0x1,	_T("DA      A")},		// DA	A
	{	0xd50000,	0xff0000,	0x000000,	0x000000,	0x000000,	0x00ff00,	0x0000ff,	0x3,	_T("DJNZ    %s,%s")},	// DJNZ	Ptr,Lab
	{	0xd60000,	0xfe0000,	0x010000,	0x000000,	0x000000,	0x000000,	0x000000,	0x1,	_T("XCHD    A,%s")},	// XCHD	A,@Ri
	{	0xd80000,	0xf80000,	0x070000,	0x000000,	0x000000,	0x000000,	0x00ff00,	0x2,	_T("DJNZ    %s,%s")},	// DJNZ	Rn,Lab
//----------------------------------------------------------------------------------------------------------------------------------
	{	0xe00000,	0xff0000,	0x000000,	0x000000,	0x000000,	0x000000,	0x000000,	0x1,	_T("MOVX    A,@DPTR")},	// MOVX	A,@DPTR
	{	0xe20000,	0xfe0000,	0x010000,	0x000000,	0x000000,	0x000000,	0x000000,	0x1,	_T("MOVX    A,%s")},	// MOVX	A,@Ri
	{	0xe40000,	0xff0000,	0x000000,	0x000000,	0x000000,	0x000000,	0x000000,	0x1,	_T("CLR     A")},		// CLR	A
	{	0xe50000,	0xff0000,	0x000000,	0x000000,	0x000000,	0x00ff00,	0x000000,	0x2,	_T("MOV     A,%s")},	// MOV	A,Ptr
	{	0xe60000,	0xfe0000,	0x010000,	0x000000,	0x000000,	0x000000,	0x000000,	0x1,	_T("MOV     A,%s")},	// MOV	A,@Ri
	{	0xe80000,	0xf80000,	0x070000,	0x000000,	0x000000,	0x000000,	0x000000,	0x1,	_T("MOV     A,%s")},	// MOV	A,Rn
//----------------------------------------------------------------------------------------------------------------------------------
	{	0xf00000,	0xff0000,	0x000000,	0x000000,	0x000000,	0x000000,	0x000000,	0x1,	_T("MOVX    @DPTR,A")},	// MOV	A,Rn
	{	0xf20000,	0xff0000,	0x010000,	0x000000,	0x000000,	0x000000,	0x000000,	0x1,	_T("MOVX    %s,A")},	// MOV	@Ri,A
	{	0xf40000,	0xff0000,	0x000000,	0x000000,	0x000000,	0x000000,	0x000000,	0x1,	_T("CPL     A")},		// CPL	A
	{	0xf50000,	0xff0000,	0x000000,	0x000000,	0x000000,	0x00ff00,	0x000000,	0x2,	_T("MOV     %s,A")},	// MOV	Ptr,A
	{	0xf60000,	0xfe0000,	0x010000,	0x000000,	0x000000,	0x000000,	0x000000,	0x1,	_T("MOV     %s,A")},	// MOV	@Ri,A
	{	0xf80000,	0xf80000,	0x070000,	0x000000,	0x000000,	0x000000,	0x000000,	0x1,	_T("MOV     %s,A")},	// MOV	Rn,A
};

//-----------------------------------------------------------------------------------
//	寄存器名称,和地址
//-----------------------------------------------------------------------------------
REGS_TBL arrRegs[]=
{
	{0xE0,_T("ACC")},
	{0xF0,_T("B")},
	{0xD0,_T("PSW")},
	{0xF0,_T("SP")},
	{0x82,_T("DPL")},
	{0x83,_T("DPH")},
	{0x80,_T("P0")},
	{0x90,_T("P1")},
	{0xA0,_T("P2")},
	{0xB0,_T("P3")},
	{0xB8,_T("IP")},
	{0xA8,_T("IE")},
	{0x89,_T("TMOD")},
	{0x88,_T("TCON")},
	{0xA8,_T("T2CON")},
	{0x8C,_T("TH0")},
	{0x8A,_T("TL0")},
	{0x8D,_T("TH1")},
	{0x8B,_T("TL1")},
	{0xCD,_T("TH2")},
	{0xCC,_T("TL2")},
	{0xCB,_T("RCAP2H")},
	{0xCA,_T("RCAP2L")},
	{0x98,_T("SCON")},
	{0x99,_T("SBUF")},
	{0x87,_T("TR1")},
	{0x8e,_T("PCON")},
};

//-----------------------------------------------------------------------------------
//	默认文件名称
//-----------------------------------------------------------------------------------
_TCHAR *pStart = _T("");
_TCHAR *pBinFile = _T("rom.bin");
_TCHAR *pSrcFile = _T("src.asm");
_TCHAR buff[]=_T("0x0000");

//	文件指针
FILE *fpBin;
FILE *fpSrc;

//	默认起始地址:
int iCodeStart = 0;

//	参数序号
int idArgv = 1;

//	指令缓冲区
BYTE pCodeBuff[CODE_MAX_BYTES];

//	存储标号和函数地址
UINT64 useLabels[MAX_LABELS];
UINT64 posLabels = 0;

//	显示帮助
void ShowHelp();

//	寄存器名
_TCHAR *GetRegname(UINT64 nReg);

int GetCodeSize(UINT64 lCode);

//	登记标号
int RegistLabel(UINT64 lCode,UINT64 lLine);

//	代码分析
int ParseCode(UINT64 lLine,UINT64 lCode,FILE *fpOut);

//	查找已登记标号
int FindLabel(UINT64 lLine);

//	文件分析
int ParseFile(int iStart, FILE *pIn,FILE *pOut);

//=====================================================================================
//	开始函数
//=====================================================================================
int main(int argc, _TCHAR* argv[])
{
	//	判断参数
	if(argc < 2 || argc > 4)
	{
		ShowHelp();
#ifdef _DEBUG
		getchar();
#endif
		return -1;
	}

	//	默认指向第一个参数
	idArgv = 1;
	pStart = argv[idArgv];

	//	是否有地址参数?
	if(pStart[0] == '-')
	{
		iCodeStart = atoi(&pStart[1]);
		idArgv ++;
	}

	if(argc > idArgv )
	{
		pBinFile = argv[idArgv ];
	}

	if(argc > idArgv + 1)
	{
		pSrcFile = argv[idArgv +1];
	}
	
	fpBin = fopen(pBinFile,"rb");
	if(!fpBin)
	{
		printf("Can't open bin file: %s!\n",pBinFile);
#ifdef _DEBUG
		getchar();
#endif
		return -1;
	}

	fpSrc = fopen(pSrcFile,"w+a");
	if(!fpSrc)
	{
		fclose(fpBin);
		printf("Can open src file: %s!\n",pSrcFile);
#ifdef _DEBUG
		getchar();
#endif
		return -1;
	}

	//	显示抬头
	printf(_T(" MCS-51 Disasm tools(C) Version 1.00\n"));
	printf(_T("---------------------------------------------------------------\n"));
	printf(_T(" Code start:  %x.\n"),iCodeStart);
	printf(_T(" Input file:  %s.\n"),pBinFile);
	printf(_T(" Output file: %s.\n"),pSrcFile);
	printf(_T("---------------------------------------------------------------\n"));
	printf(_T(" QQ:190376601,TEL:13751152175              --Aleck.Shi\n"));

	//	分析文件
	ParseFile(iCodeStart,fpBin,fpSrc);

	fclose(fpBin);
	fclose(fpSrc);

	//	处理完毕!
	printf(_T(" Processed done!\n"));

#ifdef _DEBUG
	getchar();
#endif
	return 0;
}

//=====================================================================================
//	显示帮助
//=====================================================================================
void ShowHelp()
{
	printf(_T(" MCS-51 Disasm tools(C) Version:1.00\n"));
	printf(_T("---------------------------------------------------------------\n"));
	printf(_T(" usage:d51 [-code address] <bin file> [src file]\n"));
	printf(_T("   exp:d51 rom.bin\n"));
	printf(_T("       d51 -2 rom.bin\n"));
	printf(_T("       d51 -10 rom.bin src.asm\n"));
	printf(_T("---------------------------------------------------------------\n"));
	printf(_T(" QQ:190376601,TEL:13751152175                     ----Aleck.Shi\n"));
}

//=====================================================================================
//	文件分析
//=====================================================================================
int ParseFile(int iStart, FILE *fpIn,FILE *fpOut)
{
	UINT64 lCodePos = 0;
	UINT64 lCode = 0;
	int iSize = 0;
	int i = 0;

	fprintf(fpOut,_T(";//=====================================================================================\n"));
	fprintf(fpOut,_T(";//\n"));
	fprintf(fpOut,_T(";//	模块:	<模块名称>\n"));
	fprintf(fpOut,_T(";//\n"));
	fprintf(fpOut,_T(";//	版本:	<版本号>\n"));
	fprintf(fpOut,_T(";//\n"));
	fprintf(fpOut,_T(";//	日期:	<日期>\n"));
	fprintf(fpOut,_T(";//\n"));
	fprintf(fpOut,_T(";//	作者:	<作者>\n"));
	fprintf(fpOut,_T(";//\n"));
	fprintf(fpOut,_T(";//	说明:	<模块说明>\n"));
	fprintf(fpOut,_T(";//\n"));
	fprintf(fpOut,_T(";//=====================================================================================\n"));
	fprintf(fpOut,_T("\n"));
	fprintf(fpOut,_T(";//-------------------------------------------------------------------------------------\n"));
	fprintf(fpOut,_T(";// 常量定义:\n"));
	fprintf(fpOut,_T(";//-------------------------------------------------------------------------------------\n"));
	fprintf(fpOut,_T("\n"));

	for(i = 0; i < (sizeof(arrRegs) / sizeof(arrRegs[0])); i++)
	{
		fprintf(fpOut,_T(";%s\t\tEQU\t0x%02x\n"),arrRegs[i].pName,arrRegs[i].uAddr);
	}

	fprintf(fpOut,_T(";//-------------------------------------------------------------------------------------\n"));
	fprintf(fpOut,_T(";// 变量定义:\n"));
	fprintf(fpOut,_T(";//-------------------------------------------------------------------------------------\n"));
	fprintf(fpOut,_T("\n"));
	fprintf(fpOut,_T(";//-------------------------------------------------------------------------------------\n"));
	fprintf(fpOut,_T(";// 代码开始:\n"));
	fprintf(fpOut,_T(";//-------------------------------------------------------------------------------------\n"));
	fprintf(fpOut,_T("\t\tORG     0x00\n"));

	//	查找起始地址
	fseek(fpIn,iStart,SEEK_SET);

	//----------------------------------------------------
	//	登记标号!
	//----------------------------------------------------
	posLabels = 0;
	lCodePos = 0;
	while(!feof(fpIn))
	{
		fread(pCodeBuff,1,CODE_BYTES,fpIn);
		lCode = pCodeBuff[0] << 16;
		iSize = GetCodeSize(lCode);
		if(iSize > 1)
		{
			for(i = 0;i < CODE_MAX_BYTES; i ++ )
				pCodeBuff[i] = 0;

			fread(pCodeBuff,(iSize - 1),CODE_BYTES,fpIn);
			lCode |= pCodeBuff[0] << 8;
			lCode |= pCodeBuff[1] << 0;
		}

		//lCodePos += iSize;
		//	登记标号
		RegistLabel(lCode,lCodePos);
		lCodePos += iSize;

		//	地址上限检查
		if(lCodePos >= MAX_ADDRESS)
			break;
	}

	//----------------------------------------------------
	//	查找起始地址
	//----------------------------------------------------
	fseek(fpIn,iStart,SEEK_SET);
	lCodePos = 0;
	while(!feof(fpIn))
	{
		//	读取数据
		fread(pCodeBuff,1,CODE_BYTES,fpIn);
		lCode = pCodeBuff[0] << 16;
		iSize = GetCodeSize(lCode);

		if(iSize > 1)
		{
			for(i = 0;i < CODE_MAX_BYTES; i++)
				pCodeBuff[i] = 0;

			fread(pCodeBuff,(iSize - 1),CODE_BYTES,fpIn);
			lCode |= pCodeBuff[0] << 8;
			lCode |= pCodeBuff[1] << 0;
		}
	
		//lCodePos += iSize;

		//	代码生成
		ParseCode(lCodePos,lCode,fpOut);
		lCodePos += iSize;

		//	地址上限检查
		if(lCodePos >= MAX_ADDRESS)
			break;
	}

	fprintf(fpOut,_T("\t\tEND\n"));
	fprintf(fpOut,_T(";//=====================================================================================\n"));
	fprintf(fpOut,_T(";// 文件结束. <END OF FILE> \n"));
	fprintf(fpOut,_T(";//=====================================================================================\n"));
	return 0;
}

//=====================================================================================
//	查询指令字节数
//=====================================================================================
int GetCodeSize(UINT64 lCode)
{
	UINT64 i = 0;	
	UINT64 lCount =  sizeof(tblCode) / sizeof(tblCode[0]);
	UINT64 lLine = 0;
	
	//	初始化
	for( i = 0; i < lCount; i++ )
	{
		//	匹配指令集,查找字节数
		if(((tblCode[i].CodeMask & lCode) == tblCode[i].CodeData))
		{
			return tblCode[i].nBytes;
		}
	}
	printf("Unknow Code's Size!\n");
	return 1;
}

//=====================================================================================
//	换算行号
//=====================================================================================
UINT64 GetLabel(UINT64 lCode,UINT64 lLine,UINT64 lMask)
{
	UINT64 lCurLine = 0;

	//	不同的标号,处理不同,计算出绝对地址,这样好登记!
	switch(lMask)
	{
	//	绝对长地址
	case 0x00ffff:
		lCurLine = lCode & lMask;
		break;

	//	页内绝对地址
	case 0xe0ff00:
		lCurLine  = (lCode & 0xe00000) >> 21;
		lCurLine |= (lCode & 0x00ff00) >> 8;
		lCurLine |= (lLine & 0x00f800);
		break;

	//	相对短地址
	case 0x00ff00:
		lCurLine = (lCode & lMask) >> 8;
		lCurLine = lLine + (char)lCurLine + GetCodeSize(lCode);
		break;

	//	相对短地址
	case 0x0000ff:
		lCurLine = lCode & lMask;
		lCurLine = lLine + (char)lCurLine + GetCodeSize(lCode);
		break;

	//	其他格式报错
	default:
		lCurLine = 0;
		lCurLine += lLine;
		printf("Label Format Error!\n");
		break;
	}
	return lCurLine;
}

//=====================================================================================
//	行号,标号分析(要区分短跳转!和长跳转!)
//=====================================================================================
int RegistLabel(UINT64 lCode,UINT64 lLine)
{
	//
	// 16位为全地址,高5位为页地址
	// lLine->0000,0111,1111,1111
	//
	//	lab16 = abs_address
	//  lab11 = lLine & 0xf800 | lcode.line
	//	lab08 = lLine + lcode.line
	//
	UINT64 i = 0;	
	UINT64 lCount =  (sizeof(tblCode) / sizeof(tblCode[0]));
	UINT64 lCurLine = 0;

	//	初始化
	for( i = 0; i < lCount; i++ )
	{
		//	匹配指令集
		if(((tblCode[i].CodeMask & lCode) == tblCode[i].CodeData))
		{
			//	含有标号
			if(tblCode[i].LabsMask)
			{
				//	取绝对地址
				lCurLine = GetLabel(lCode,lLine,tblCode[i].LabsMask);

				//	查看是否登记!否则登记
				if(!FindLabel(lCurLine))
				{
					useLabels[posLabels] = lCurLine;
#ifdef _DEBUG
					printf("Labels[%d] = %04x\n",posLabels,lCurLine);
#endif
					posLabels++;
					return 1;
				}
			}
		}
	}
	return 0;
}

//=====================================================================================
//	是否登记行号
//=====================================================================================
int FindLabel(UINT64 lLine)
{
	UINT64 i = 0;
	for( i = 0; i < posLabels; i++ )
	{
		//	登记标号
		if(useLabels[i] == lLine)
			return 1;
	}
	return 0;
}

//=====================================================================================
//	取得寄存器名称!
//=====================================================================================
_TCHAR *GetRegName(UINT64 nReg)
{
	int i = 0;
	for(i = 0; i < (sizeof(arrRegs) / sizeof(arrRegs[0])); i++)
	{
		if(arrRegs[i].uAddr == nReg)
		{
			return arrRegs[i].pName;
		}
	}
	sprintf(buff,FMT_PTR,nReg);
	return buff;
}

//=====================================================================================
//	代码分析
//=====================================================================================
int ParseCode(UINT64 lLine,UINT64 lCode,FILE *fpOut)
{
	UINT64 i = 0;	
	UINT64 lCount =  sizeof(tblCode) / sizeof(DASM_TBL);
	UINT64 id = 0;

	_TCHAR buff1[64];
	_TCHAR buff2[64];
	_TCHAR buff3[64];

	UINT64 R,B,D,P,L;
	R = B = D = P = L = 0;

#ifdef _DEBUG
	//printf("Code:%06x\n",lCode);
#endif

	for( i = 0; i < lCount; i++ )
	{
		id = tblCode[i].CodeMask & lCode;

		//	匹配指令集
		if( id == tblCode[i].CodeData)
		{
			//	有需要的才标号,需要查表
			if(FindLabel(lLine))
			{
				//	标号格式
				fprintf(fpOut,FMT_LABCR,lLine);
			}
			//	开始空点位置!
			fprintf(fpOut,_T("\t\t"));
			
			R = tblCode[i].RegsMask;
			B = tblCode[i].BitsMask;
			D = tblCode[i].DataMask;
			P = tblCode[i].PtrsMask;
			L = tblCode[i].LabsMask;

			//-------------------------------------------------------------------------
			// 类似:CJNE Rn,#data,Lab
			//-------------------------------------------------------------------------
			if( R && !B && D && !P && L)
			{
				if(tblCode[i].RegsMask == 0x010000)
					sprintf(buff1,FMT_RI,(tblCode[i].RegsMask & lCode) >> 16);
				else
					sprintf(buff1,FMT_RN,(tblCode[i].RegsMask & lCode) >> 16);
				sprintf(buff2,FMT_DAT, (tblCode[i].DataMask & lCode) >> 8);
				sprintf(buff3,FMT_LAB, GetLabel(lCode,lLine,tblCode[i].LabsMask));
				fprintf(fpOut,(const char*)tblCode[i].pCodeFmt,buff1,buff2,buff3);
			}
			//-------------------------------------------------------------------------
			// 类似:MOV Rn,#data
			//-------------------------------------------------------------------------
			else if( R && !B && D && !P && !L)
			{
				if(tblCode[i].RegsMask == 0x010000)
					sprintf(buff1,FMT_RI,(tblCode[i].RegsMask & lCode) >> 16);
				else
					sprintf(buff1,FMT_RN,(tblCode[i].RegsMask & lCode) >> 16);
				sprintf(buff2,FMT_DAT,(tblCode[i].DataMask & lCode) >> 8);
				fprintf(fpOut,(const char*)tblCode[i].pCodeFmt,buff1,buff2);
			}
			//-------------------------------------------------------------------------
			// 类似:MOV Rn,Ptr(86-A6,88-A8是反的)
			//-------------------------------------------------------------------------
			else if( R && !B && !D &&  P && !L)
			{
				if(tblCode[i].RegsMask == 0x010000)
					sprintf(buff1,FMT_RI,(R & lCode) >> 16);
				else
					sprintf(buff1,FMT_RN,(R & lCode) >> 16);
				strcpy(buff2,GetRegName((tblCode[i].PtrsMask & lCode) >> 8));
				
				if(id == 0x00a60000 || id == 0x00a80000)
				{
					fprintf(fpOut,(const char*)tblCode[i].pCodeFmt,buff2,buff1);
				}
				else
				{
					fprintf(fpOut,(const char*)tblCode[i].pCodeFmt,buff1,buff2);
				}
			}
			//-------------------------------------------------------------------------
			// 类似:MOV Rn,Lab
			//-------------------------------------------------------------------------
			else if( R && !B && !D && !P && L)
			{
				if(tblCode[i].RegsMask == 0x010000)
					sprintf(buff1,FMT_RI,(tblCode[i].RegsMask & lCode) >> 16);
				else
					sprintf(buff1,FMT_RN,(tblCode[i].RegsMask & lCode) >> 16);
				sprintf(buff2,FMT_LAB,GetLabel(lCode,lLine,tblCode[i].LabsMask));
				fprintf(fpOut,(const char*)tblCode[i].pCodeFmt,buff1,buff2);
			}
			//-------------------------------------------------------------------------
			// 类似:INC Rn
			//-------------------------------------------------------------------------
			else if( R && !B && !D && !P && !L)
			{
				if(tblCode[i].RegsMask == 0x010000)
					sprintf(buff1,FMT_RI,(tblCode[i].RegsMask & lCode) >> 16);
				else
					sprintf(buff1,FMT_RN,(tblCode[i].RegsMask & lCode) >> 16);
				fprintf(fpOut,(const char*)tblCode[i].pCodeFmt,buff1);
			}
			//-------------------------------------------------------------------------
			//	类似 JB Bit,Lab
			//-------------------------------------------------------------------------
			else if(!R &&  B && !D && !P && L)
			{
				strcpy(buff1,GetRegName((B & lCode) >> 8));
				sprintf(buff2,FMT_LAB,GetLabel(lCode,lLine,tblCode[i].LabsMask));
				fprintf(fpOut,(const char*)tblCode[i].pCodeFmt,buff1,buff2);
			}
			//-------------------------------------------------------------------------
			//	类似 CJNE A,#data,Lab (3BYTE)
			//-------------------------------------------------------------------------
			else if(!R && !B && D && !P && L)
			{
				sprintf(buff1,FMT_DAT,(lCode & tblCode[i].DataMask) >> 8);
				sprintf(buff2,FMT_LAB,GetLabel(lCode,lLine,tblCode[i].LabsMask));
				fprintf(fpOut,(const char*)tblCode[i].pCodeFmt,buff1,buff2);
			}
			//-------------------------------------------------------------------------
			//	类似 CJNE A,Ptr,Lab (3BYTE)
			//-------------------------------------------------------------------------
			else if(!R && !B && !D && P && L)
			{
				strcpy(buff1,GetRegName((P & lCode) >> 8));
				sprintf(buff2,FMT_LAB,GetLabel(lCode,lLine,tblCode[i].LabsMask));
				fprintf(fpOut,(const char*)tblCode[i].pCodeFmt,buff1,buff2);
			}
			//-------------------------------------------------------------------------
			//	类似 MOV Ptr,#data (3BYTE) //注意!在85,数据是指针,因为该代码为双指针!所以用数据位来存!
			//-------------------------------------------------------------------------
			else if(!R && !B && D && P && !L)
			{
				strcpy(buff1,GetRegName((P & lCode) >> 8));
				
				//	特殊指令
				if(id == 0x00850000)
					strcpy(buff2,GetRegName((D & lCode)));
				else
					sprintf(buff2,FMT_DAT,(D & lCode));
				fprintf(fpOut,(const char*)tblCode[i].pCodeFmt,buff1,buff2);
			}
			//-------------------------------------------------------------------------
			//	类似 STEB Bi ( 2 BYTE )
			//-------------------------------------------------------------------------
			else if(!R && B && !D && !P && !L)
			{
				strcpy(buff1,GetRegName((B & lCode) >> 8));
				//sprintf(buff1,FMT_BIT,(lCode & tblCode[i].BitsMask) >> 8);
				fprintf(fpOut,(const char*)tblCode[i].pCodeFmt,buff1);
			}
			//-------------------------------------------------------------------------
			//	类似 AJMP Lab (2~3BYTE)
			//-------------------------------------------------------------------------
			else if(!R && !B && !D && !P && L)
			{
				sprintf(buff1,FMT_LAB,GetLabel(lCode,lLine,tblCode[i].LabsMask));
				fprintf(fpOut,(const char*)tblCode[i].pCodeFmt,buff1);
			}
			//-------------------------------------------------------------------------
			//	类似 MOV  A,#data (2BYTE)
			//-------------------------------------------------------------------------
			else if(!R && !B && D && !P && !L)
			{
				sprintf(buff1,FMT_DAT,(lCode & tblCode[i].DataMask) >> 8);
				fprintf(fpOut,(const char*)tblCode[i].pCodeFmt,buff1);
			}
			//-------------------------------------------------------------------------
			//	类似 PUSH [Ptr] (2BYTE)
			//-------------------------------------------------------------------------
			else if(!R && !B && !D && P && !L)
			{
				strcpy(buff1,GetRegName((tblCode[i].PtrsMask & lCode) >> 8));
				fprintf(fpOut,(const char*)tblCode[i].pCodeFmt,buff1);
			}
			//-------------------------------------------------------------------------
			//	类似 WDTC,CLRA ( 1 BYTE)
			//-------------------------------------------------------------------------
			else if(!R && !B && !D && !P && !L)
			{
				fprintf(fpOut,(const char*)tblCode[i].pCodeFmt);
			}
			//	错误的匹配!
			else
			{
				fprintf(fpOut,_T(";Error Code:%04x\n"), lCode);
				printf(_T("Error Code:%04x\n"),lCode);
			}
			//	换行
			fprintf(fpOut,_T("\n"));
			break;
		}
	}
	
	//	错误的指令
	if(i >= lCount)
	{
		fprintf(fpOut,_T(";Unknow Code:%04x\n"),lCode);
		printf(_T("Unknow Code:%04x\n"),lCode);
	}

	return 0;
}
