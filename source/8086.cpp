#include <windows.h>
#include <stdio.h>

#define SWAP(t, a, b)				\
    t c = a;					\
    a   = b;					\
    b   = c;

#define SWAP_ARRAY(t, a, b)				\
    t c[128] = {};					\
    MoveMemory((PVOID)c, (VOID*)a, (SIZE_T)128);	\
    MoveMemory((PVOID)a, (VOID*)b, (SIZE_T)128);	\
    MoveMemory((PVOID)b, (VOID*)c, (SIZE_T)128);     	

struct windows_file
{
    void* data;
    int   size;
};
static windows_file
windows_readfile (char* filename)
{
    windows_file file = {};
    
    HANDLE handle = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if(handle)
    {
	LARGE_INTEGER file_size = {};
	if(GetFileSizeEx(handle, &file_size))
	{
	    file.data = VirtualAlloc(0, file_size.QuadPart, MEM_COMMIT, PAGE_READWRITE);

	    if(file.data)
	    {
		DWORD bytes_read = 0;
		if(ReadFile(handle, file.data, file_size.QuadPart, &bytes_read, 0))
		{
		    if(bytes_read == file_size.QuadPart)
		    {
			file.size = file_size.QuadPart;
		    }
		    else
		    {
			VirtualFree(file.data, 0, MEM_RELEASE);
			file.data = 0;

			OutputDebugStringA("file size retrieved from 'GetFileSizeEx' the same as number of bytes read, returned from 'ReadFile'!\n");
		    }
		}
		else
		{
		    OutputDebugStringA("'ReadFile' failed!\n");
		}
	    }
	    else
	    {
		OutputDebugStringA("'VirtualAlloc' failed!\n");
	    }
	}
	else
	{
	    OutputDebugStringA("'GetFileSizeEx' failed!\n");
	}

	CloseHandle(handle);
    }
    else
    {
	OutputDebugStringA("'CreateFileA' failed!\n");
    }

    return(file);
}

// OPCODES
#define MOV_REGMEM_REG  34 // 1 0 0 0 1 0  - Register/Memory to/from Register
#define ADD_REGMEM_REG   0 // 0 0 0 0 0 0  - Register/Memory with Register to either

#define MOV_IMMED_REGMEM  49 // 1 1 0 0 0 1  - Immediate to Register/Memory
#define ADD_IMMED_REGMEM  32 // 1 0 0 0 0 0  - Immediate to Register/Memory (0 0 0)

#define MOV_IMMED_REG     11 //     1 0 1 1  - Immediate to Register

#define MOV_MEM_ACCUM     40 // 1 0 1 0 0 0  - Memory to Accumulator

#define MOV (152 + 0)
#define ADD (152 + 1)

#define IMMED_REG     (152 + 2)
#define IMMED_REGMEM  (152 + 3)

#define MM   0 // 0 0 (memory mode no disp*)
#define MM8  1 // 0 1 (memory mode disp:  8)
#define MM16 2 // 1 0 (memory mode disp: 16)
#define RM   3 // 1 1 (register mode)

#define DIRECT_ADDR 6 // 1 1 0 (direct address)

char* REG_BYTE[] =
{
    "al", "cl", "dl", "bl", "ah", "ch", "dh", "bh"
};
char* REG_WORD[] =
{
    "ax", "cx", "dx", "bx", "sp", "bp", "si", "di"
};

char* EAC[] = // Effective Address Calculation
{
    "bx + si", "bx + di", "bp + si", "bp + di", "si", "di", "bp", "bx"  
};

// TWO's Complement (-128 -> +127)
//
// when interpreting a binary number as negative:
//
// .highest bit is always one 
//
// .the seven other bits are looked at for the value
//     .subtract one
//     .invert bits
//
// e.g
// 1 0 0 0 1 0 0 1 
// 
// .highest bit is always one
//  |1| 0001001
// .subtract one
//  |1| 0001000
// .invert bits
//  |1| 1110111
//
//  = 64+32+16+4+2+1
//  = (-)119
//

int WinMain(HINSTANCE instance,
	    HINSTANCE prev_instance,
	    LPSTR     cmd_line,
	    int       cmd_show)
{
    windows_file file = windows_readfile("d:/source/8086_add_sub_cmp_jnz");
    if(file.data)
    {
	int size = 0;
	while(size < file.size)
	{
	    unsigned char* instruction = ((unsigned char*)file.data) + size;
	    unsigned int   instruction_size = 0; // bytes
	    
	    unsigned char  byte0 = *instruction;
	    unsigned char  byte1 = *(instruction + 1);
	    unsigned char  byte2 = *(instruction + 2); // disp-lo
	    unsigned char  byte3 = *(instruction + 3); // disp-hi
	    unsigned char  byte4 = *(instruction + 4); // data-lo
	    unsigned char  byte5 = *(instruction + 5); // data-hi
	    
	    unsigned char opcode = byte0 >> 2; // six bits

	    char dest[128] = { };
	    char src [128] = { };

	    // d = 1
	    // destination is in REG (source in R/M)
	    // d = 0
	    // source is in REG (destination in R/M)

	    // w = 0 (byte)
	    // w = 1 (word)

	    unsigned char d = ((unsigned char)(byte0 >> 1)) << 7; // 1 bit;
	    unsigned char w = byte0 << 7;                         // 1 bit;

	    unsigned char mod = byte1 >> 6;                                                 // 2 bits
	    unsigned char reg = ((unsigned char)(((unsigned char)(byte1 >> 3)) << 5)) >> 5; // 3 bits
	    unsigned char rm  = ((unsigned char)(byte1 << 5)) >> 5;                         // 3 bits

	    unsigned char instruction_type = 0;
	    unsigned char      opcode_type = 0;
	    
	    if(opcode == MOV_IMMED_REGMEM || opcode == ADD_IMMED_REGMEM)
	    {
		instruction_type = IMMED_REGMEM;
	    }

	    if(mod == RM) 
	    {
		instruction_size = 2;

		if(w)
		{
		    sprintf(dest, "%s", REG_WORD[reg]); // dest = REG_WORD[reg]
		    sprintf(src , "%s", REG_WORD[rm]);  // src  = REG_WORD[rm]
		}
		else
		{
		    sprintf(dest, "%s", REG_BYTE[reg]); // dest = REG_BYTE[reg]
		    sprintf(src , "%s", REG_BYTE[rm]);  // src  = REG_BYTE[rm]
		}
	    }
	    else
	    {
		// note: use EA calculation. (assuming d = 1)
		//       must be word as EA calculation is word

		sprintf(dest, "%s", REG_WORD[reg]); // dest = REG_WORD[reg]
		sprintf(src , "%s", EAC[rm]);       // src  = EAC[rm]
		    
		if     (mod == MM)
		{
		    instruction_size = 2; 
		    
		    if(rm == DIRECT_ADDR) 
		    {
			instruction_size = 4;
			
			unsigned short direct_address = byte2 | ((unsigned short)(byte3 << 8));
			sprintf(src, "[%i]", direct_address); // src = [direct_address]
		    }
		    else
		    {
			sprintf(src, "[%s]", EAC[rm]); // src = [EAC[rm]]
		    }
		}
		else if(mod == MM8 || mod == MM16) // note: displacement
		{	
		    instruction_size = 3;

		    unsigned short disp = byte2;
		    if(mod == MM16)
		    {
			instruction_size += 1;
			disp |= ((unsigned short)(byte3 << 8));
		    }
		    sprintf(src, "[%s + %i]", EAC[rm], disp); // src = [EAC[rm] + disp]
		}
	    }
	    
	    if     (instruction_type == IMMED_REGMEM)
	    {
		// d = s (ADD)
		
		instruction_size += 1;
			
		unsigned char* immediate_byte = instruction + 2;
		if     (mod == MM8)  immediate_byte += 1;
		else if(mod == MM16) immediate_byte += 2;

		unsigned short immediate = *immediate_byte;
		if(!d && w)
		{
		    instruction_size += 1;
		    immediate |= (*(immediate_byte + 1)) << 8;
		}
		sprintf(dest, "%i", immediate); // dest = immediate

		SWAP_ARRAY(char, src, dest);
	    }

	    // 
	    // DETERMINE OPCODE
	    //

	    if                          (opcode == MOV_MEM_ACCUM) 
	    {
		opcode_type = MOV;
		
		instruction_size = 2;
		
		unsigned short address = byte1;
		sprintf(src, "%s", REG_BYTE[0]); // src = REG_BYTE[0]
		if(w)
		{
		    instruction_size += 1;
		    address |= byte2 << 8;
		    sprintf(src, "%s", REG_WORD[0]); // src = REG_WORD[0]
		}
		sprintf(dest, "[%i]", address); // dest = [address]
	    }
	    else if((unsigned char)(byte0 >> 4) == MOV_IMMED_REG)
	    {
		opcode_type = MOV;
		
		instruction_size = 2;
		
		w   = ((unsigned char)(byte0 >> 3)) << 7; // 1 bit
		reg = ((unsigned char)(byte0 << 5)) >> 5; // 3 bits

		sprintf(dest, "%s", REG_BYTE[reg]); // dest = REG_BYTE[reg]
		unsigned short immediate = byte1;
		if(w)
		{
		    instruction_size += 1;
		    immediate |= byte2 << 8;
		    sprintf(dest, "%s", REG_WORD[reg]); // dest = REG_WORD[reg]
		}
		sprintf(src, "%i", immediate); // src = immediate
	    }
	    else if(opcode == MOV_IMMED_REGMEM || opcode == MOV_REGMEM_REG)
	    {
		opcode_type = MOV;	
	    }

	    else if(opcode == ADD_REGMEM_REG || opcode == ADD_IMMED_REGMEM)
	    {
		opcode_type = ADD;	
	    }

	    if(!d)
	    {
		SWAP_ARRAY(char, dest, src);
	    }

	    if     (opcode_type == MOV)
	    {
		printf("mov %s, %s\n", dest, src);
	    }
	    else if(opcode_type == ADD)
	    {
		printf("add %s, %s\n", dest, src);
	    }
	    
	    size += instruction_size;
	}
    }
    return(0);
}
