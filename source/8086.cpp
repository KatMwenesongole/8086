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
//
// ...


// (1) 8086_single_register_mov - COMPLETE
// (2) 8086_many_register_mov   - COMPLETE
//


const char* REG_NAME[] = { "al", "cl", "dl", "bl", "ah", "ch", "dh", "bh",
                           "ax", "cx", "dx", "bx", "sp", "bp", "si", "di" };
#define WORD_OFFSET 8

#define MOV_REGMEM_REG 34

#define MM   0
#define MM8  1
#define MM16 2
#define RM   3

int WinMain(HINSTANCE instance,
	    HINSTANCE prev_instance,
	    LPSTR     cmd_line,
	    int       cmd_show)
{
    windows_file file = windows_readfile("p:/8086/source/8086_many_register_mov");
    if(file.data)
    {
	int size = 0;
	while(size < file.size)
	{
	    unsigned char* instruction = ((unsigned char*)file.data) + size;
	    unsigned int instruction_size = 2;
	    
	    // [ 0 0 0 0 0 0 ] opcode [ 0 ] d [ 0 ] w
	    
	    unsigned char opcode = (*instruction) >> 2;   
	    unsigned char d = (unsigned char)(((unsigned char)((*instruction) << 6)) >> 7); // is src / dest in REG                   
	    unsigned char w = (unsigned char)(((unsigned char)((*instruction) << 7)) >> 7); // is it byte / word

	    // [ 0 0 ] mod [ 0 0 0 ] reg [ 0 0 0 ] rm

	    unsigned char mod = *(instruction + 1) >> 6; // memory mode.
	    unsigned char reg = (unsigned char)(((unsigned char)((*(instruction + 1)) << 2)) >> 5); // source or destination?
	    unsigned char rm  = (unsigned char)(((unsigned char)((*(instruction + 1)) << 5)) >> 5);  // source or destination?
	    

	    if(opcode == MOV_REGMEM_REG)
	    {
		int offset = (w) ? WORD_OFFSET : 0;

		const char* dest = (d) ? REG_NAME[reg + offset] : REG_NAME[rm  + offset];
		const char* src  = (d) ? REG_NAME[rm  + offset] : REG_NAME[reg + offset];

		printf("mov %s, %s\n", dest, src);
	    }
	    
	    size += instruction_size;
	}
    }
    return(0);
}

