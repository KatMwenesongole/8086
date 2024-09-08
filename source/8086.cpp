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

// (1) 8086_single_register_mov - COMPLETE
// (2) 8086_many_register_mov   - COMPLETE
// (3) 8086_more_movs           - COMPLETE

const char* REG_NAME[] =
{
    "al", "cl", "dl", "bl", "ah", "ch", "dh", "bh",
    "ax", "cx", "dx", "bx", "sp", "bp", "si", "di"
};
const char* EA[] =
{
    "bx + si", "bx + di", "bp + si", "bp + di", "si", "di", "bp", "bx"
};

#define WORD_OFFSET 8

#define MOV_REGMEM_REG 34
#define MOV_IMM_REG    11

#define MM   0
#define MM8  1
#define MM16 2
#define RM   3

#define DA   6

int WinMain(HINSTANCE instance,
	    HINSTANCE prev_instance,
	    LPSTR     cmd_line,
	    int       cmd_show)
{
    windows_file file = windows_readfile("p:/8086/source/8086_more_movs");
    if(file.data)
    {
	int size = 0;
	while(size < file.size)
	{
	    unsigned char* instruction = ((unsigned char*)file.data) + size;
	    unsigned int   instruction_size = 2;

	    unsigned char opcode = ((unsigned char)(*instruction) >> 2);
	    
	    unsigned char d = 0;
	    unsigned char w = 0;
	    unsigned char reg = 0;

	    char destination[32] = {};
	    char source     [32] = {};

	    char* dest = destination;
	    char* src  = source;
	    
	    if((unsigned char)((*instruction) >> 4) == MOV_IMM_REG) // immediate-to-register.
	    {
		// [ 0 0 0 0 ] opcode [ 0 ] w [ 0 0 0 ] reg 
		
		w = (unsigned char)(((unsigned char)((*instruction) << 4)) >> 7); // 8 / 16.
		reg = (unsigned char)(((unsigned char)((*instruction) << 5)) >> 5); // register.

		int offset = (w) ? WORD_OFFSET : 0;
		if(w)
		{
		    short data0 = (*(instruction + 1) << 8) >> 8;
		    short data1 = (*(instruction + 2) << 8);

		    sprintf(src, "%i", (data1 | data0));

		    instruction_size = 3;
		}
		else
		{
		    char data = (char)(*(instruction + 1));
		    sprintf(src, "%i", data);
		}

		sprintf(dest, "%s", REG_NAME[reg + offset]);
	    }
	    else if(opcode == MOV_REGMEM_REG)                       // register-to-register.
	    {
		// [ 0 0 0 0 0 0 ] opcode [ 0 ] d [ 0 ] w
		// [ 0 0 ] mod [ 0 0 0 ] reg [ 0 0 0 ] rm
		
		// [ 0 0 0 0 0 0 0 0 ] disp - lo
		// [ 0 0 0 0 0 0 0 0 ] disp - hi
		
		d = (unsigned char)(((unsigned char)((*instruction) << 6)) >> 7); // source is in REG (0)? destination is in REG (1)?                   
		w = (unsigned char)(((unsigned char)((*instruction) << 7)) >> 7); // 8 / 16.

		unsigned char mod = *(instruction + 1) >> 6; // memory mode.
		reg = (unsigned char)(((unsigned char)((*(instruction + 1)) << 2)) >> 5);                // source or destination?
		unsigned char rm  = (unsigned char)(((unsigned char)((*(instruction + 1)) << 5)) >> 5);  // source or destination?

		if     (mod == MM) // eac (no displacement). 
		{
		    // special case*
		    if(rm == DA) // direct address (16 bit).
		    {
			short data0 = (*(instruction + 2) << 8) >> 8;
			short data1 = (*(instruction + 3) << 8);
			sprintf(src, "[%i]", (data1 | data0));
			instruction_size = 4;
		    }
		    else
		    {
			sprintf(src, "[%s]", EA[rm]);
			instruction_size = 2;
		    }
		}
		else if(mod == MM8) // eac (8 bit displacement).
		{
		    char data = (char)(*(instruction + 2));
		    sprintf(src, "[%s + %i]", EA[rm], data);

		    instruction_size = 3; 
		}
		else if(mod == MM16) // eac (16 bit displacement).
		{
		    short data0 = (*(instruction + 2) << 8) >> 8;
		    short data1 = (*(instruction + 3) << 8);
		    
		    sprintf(src, "[%s + %i]", EA[rm], (data1 | data0));
		    
		    instruction_size = 4;
		}
		else if(mod == RM) 
		{    
		    sprintf(src, "%s", (w) ? REG_NAME[rm  + WORD_OFFSET] : REG_NAME[rm]);
		    instruction_size = 2;
		}

		sprintf(dest, "%s", (w) ? REG_NAME[reg + WORD_OFFSET] : REG_NAME[reg]);

		if(!d)
		{
		    SWAP(char*, dest, src);
		}
	    }
	    
	    printf("mov %s, %s\n", dest, src);
	    
	    size += instruction_size;
	}
    }
    return(0);
}

