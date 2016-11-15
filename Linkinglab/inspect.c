#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <elf.h>
#include <memory.h>

/* Given the in-memory ELF header pointer as `ehdr` and a section
   header pointer as `shdr`, returns a pointer to the memory that
   contains the in-memory content of the section */
#define AT_SEC(ehdr, shdr) ((void *)(ehdr) + (shdr)->sh_offset)
#define CHECK_HIGH_BIT(var) (var >> 2 & 0xF0)
#define CHECK_LOW_BIT(var) (var << 1 & 0x0F)

// static void check_for_shared_object(Elf64_Ehdr *ehdr);
// static void fail(char *reason, int err_code);
// static Elf64_Shdr* section_by_name(Elf64_Ehdr* ehdr, char* name);
// static Elf64_Shdr* section_by_index(Elf64_Ehdr* ehdr, int index);
// static void inspecting(Elf64_Ehdr* ehdr, unsigned char* strs);
// static int inspect_helper(unsigned char* strs);
static void fail(char *reason, int err_code) {
    fprintf(stderr, "%s (%d)\n", reason, err_code);
    exit(1);
}

static void check_for_shared_object(Elf64_Ehdr *ehdr) {
    if ((ehdr->e_ident[EI_MAG0] != ELFMAG0)
        || (ehdr->e_ident[EI_MAG1] != ELFMAG1)
        || (ehdr->e_ident[EI_MAG2] != ELFMAG2)
        || (ehdr->e_ident[EI_MAG3] != ELFMAG3))
        fail("not an ELF file", 0);

    if (ehdr->e_ident[EI_CLASS] != ELFCLASS64)
        fail("not a 64-bit ELF file", 0);

    if (ehdr->e_type != ET_DYN)
        fail("not a shared-object file", 0);
}

// static int check_low_bit(char var)
// {
//     return var >> 2 & 0xF0;
// }
// static int check_high_bit(char var)
// {
//     return var << 1 & 0x0F;
// }



static Elf64_Shdr* section_by_name(Elf64_Ehdr* ehdr, char* name)
{
    Elf64_Shdr* shdrs = (void*)ehdr+ehdr->e_shoff;
    Elf64_Shdr* for_return = NULL;
    char *strs = (void*)ehdr+shdrs[ehdr->e_shstrndx].sh_offset;
    int i;
    for (i = 0; i < ehdr->e_shnum; i++) {
      //  printf("%s\n", strs + shdrs[i].sh_name);
       if(strcmp(name, strs + shdrs[i].sh_name) == 0)
       {
           for_return = &shdrs[i];
           break;
       }
    }
    return for_return;
}

static Elf64_Shdr* section_by_index(Elf64_Ehdr* ehdr, int index)
{
    Elf64_Shdr* shdrs = (void*)ehdr+ehdr->e_shoff;
    //for (i = 0; i < ehdr->e_shnum; i++) {
    return &shdrs[index];
}

static int inspect_helper(unsigned char* strs)
{
    if(*strs == 0x48) {
         //printf("  %s\n", "4 TEST !!!!!!");
         if(*(strs+1) == 0x8b)
        {
             //printf("  %s\n", "4 TEST !!!!!!");
            if(CHECK_HIGH_BIT(*(strs+2)) == 0 && CHECK_LOW_BIT(*(strs+2))==10)
            {

              //  printf("  %s\n", "4 TEST !!!!!!");
                return 4;
            }
        }
        if (*(strs+1) == 0x63 || *(strs+1) == 0x89 || *(strs+1) == 0x8b) {
            if (CHECK_HIGH_BIT(*(strs+2))== 3)
            {
//printf("  %s\n", "6");
                return 6;
            }
            if(CHECK_HIGH_BIT(*(strs+2)) == 0 && CHECK_LOW_BIT(*(strs+2))==10)
            {

             //   printf("  %s\n", "8");
                return 8;
            }
            if(CHECK_HIGH_BIT(*(strs+2)) == 0 && CHECK_LOW_BIT(*(strs+2))==8)
            {
                return 10;
            }
            if(!(CHECK_HIGH_BIT(*(strs+2)) == 0 && CHECK_LOW_BIT(*(strs+2))==8) ||!(CHECK_HIGH_BIT(*(strs+2)) == 0 && CHECK_LOW_BIT(*(strs+2))==10))
            {
                return 12;
            }
        }
        

    }
    else if(*strs == 0x63 || *strs == 0x89 || *strs == 0x8b)
    {
        if(CHECK_HIGH_BIT(*(strs+1)) == 0 && CHECK_LOW_BIT(*(strs+1))==10)
        {
            return 7;
        }

        if(CHECK_HIGH_BIT(*(strs+1)) == 3)
        {

             //   printf("  %s\n", "5");
            return 5;
        }

        if(CHECK_HIGH_BIT(*(strs+1)) == 0 && CHECK_LOW_BIT(*(strs+1))==8)
        {
            return 9;
        }
        if(!(CHECK_HIGH_BIT(*(strs+1)) == 0 && CHECK_LOW_BIT(*(strs+1))==8) ||!(CHECK_HIGH_BIT(*(strs+1)) == 0 && CHECK_LOW_BIT(*(strs+1))==10))
        {
            return 11;
        }
    }

        return EXIT_SUCCESS;

}

static void inspecting(Elf64_Ehdr* ehdr, unsigned char* strs, Elf64_Addr st_value)
{
    unsigned char* current_addr = strs;
    while(1)
    {
        //printf("  %s\n", "entered");
        //1
        if(*current_addr == 0xc3) break;
            //2
        else if(*current_addr == 0xe9) {
			current_addr += 5;
			st_value += 5;
		}
            //3
        else if(*current_addr == 0xeb){
			 current_addr += 2;
			st_value += 2;
		}	
            //4
    
        else if(inspect_helper(current_addr) == 4)
        {
			
           // printf("  %s\n", "variable test!");
//            Elf64_Shdr *dynsym_shdr = section_by_name(ehdr, ".dynsym");
//            Elf64_Sym *syms = AT_SEC(ehdr, dynsym_shdr);
//            char *strs = AT_SEC(ehdr, section_by_name(ehdr, ".dynstr"));
//            int i, count = dynsym_shdr->sh_size / sizeof(Elf64_Sym);
			
			//Elf64_Addr temp_addr = (st_value + 7) + (int)(*(current_addr + 3));
			
			Elf64_Addr temp_addr = st_value + 7;
			int offset = *((int*)(current_addr + 3));
            Elf64_Shdr *dynsym_shdr = section_by_name(ehdr, ".dynsym");
            Elf64_Sym *syms = AT_SEC(ehdr, dynsym_shdr);
            char *strs = AT_SEC(ehdr, section_by_name(ehdr, ".dynstr"));


            Elf64_Shdr *rela_dyn_shdr = section_by_name(ehdr, ".rela.dyn");
            Elf64_Rela *relas = AT_SEC(ehdr, rela_dyn_shdr);
            int i, count = rela_dyn_shdr->sh_size / sizeof(Elf64_Rela);


            for (i = 0; i < count; i++)
            {
                int info = ELF64_R_SYM(relas[i].r_info);
                char* temp_str = strs + syms[info].st_name;

				
          // printf("  %s\n", temp_str);
              //  if(strcmp(temp_str, "_ITM_deregisterTMCloneTable") != 0 && strcmp(temp_str, "__gmon_start__") != 0 && strcmp(temp_str, "_Jv_RegisterClasses") != 0 && strcmp(temp_str, "_ITM_registerTMCloneTable") != 0
                // && strcmp(temp_str, "__cxa_finalize") != 0 && strcmp(temp_str, "") != 0)

				if(temp_addr + offset == relas[i].r_offset)
                {
                    printf("  %s\n", temp_str);
                }

               // printf("%d\n", ELF64_R_SYM(relas[i].r_info));
            }
            current_addr += 7;
			st_value += 7;
        }
        //5
        else if(inspect_helper(current_addr) == 5)
        {
            current_addr += 2;
			st_value += 2;
        }
        else if(inspect_helper(current_addr) == 6)
        {
            current_addr += 3;
			st_value += 3;
        }
        else if(inspect_helper(current_addr) == 7)
        {
            current_addr += 6;
			st_value += 6;
        }
        else if(inspect_helper(current_addr) == 8)
        {
            current_addr += 7;
			st_value += 7;
        }
        else if(inspect_helper(current_addr) == 9)
        {
            current_addr += 3;
			st_value += 3;
        }
        else if(inspect_helper(current_addr) == 10)
        {
            current_addr += 4;
			st_value += 4;
        }
        else if(inspect_helper(current_addr) == 11)
        {
            current_addr += 2;
			st_value += 2;
        }
        else if(inspect_helper(current_addr) == 12)
        {
            current_addr += 3;
			st_value += 3;
        }
        else if(*current_addr == 0xff && *(current_addr + 1) == 0x25)
        {
            current_addr += 6;
			st_value += 6;
        }else{
            break;
        }


//        switch (*current_addr)
//        {
//            //The 0xc3 opcode represents a ret instruction.
//            case 0xc3:
//                goto exit_loop;
//            //The 0xe9 opcode represents a jmp instruction
//            case 0xe9:
//                current_addr += 5;
//                break;
//           //The 0xeb opcode also represents a jmp instruction, but with a signed char displacement
//            case 0xeb:
//                current_addr += 2;
//                break;
//            case :
//
//
//
//        }
    }
//    exit_loop:;


}


int main(int argc, char **argv) {
    int fd;
    size_t len;
    void *p;
    Elf64_Ehdr *ehdr;

    if (argc != 2)
        fail("expected one file on the command line", 0);

    /* Open the shared-library file */
    fd = open(argv[1], O_RDONLY);
    if (fd == -1)
        fail("could not open file", errno);

    /* Find out how big the file is: */
    len = lseek(fd, 0, SEEK_END);

    /* Map the whole file into memory: */
    p = mmap(NULL, len, PROT_READ, MAP_PRIVATE, fd, 0);
    if (p == (void*)-1)
        fail("mmap failed", errno);

    /* Since the ELF file starts with an ELF header, the in-memory image
       can be cast to a `Elf64_Ehdr *` to inspect it: */
    ehdr = (Elf64_Ehdr *)p;

    /* Check that we have the right kind of file: */
    check_for_shared_object(ehdr);

    /* Add a call to your work here */
    Elf64_Shdr *dynsym_shdr = section_by_name(ehdr, ".dynsym");
    Elf64_Sym *syms = AT_SEC(ehdr, dynsym_shdr);
    char *strs = AT_SEC(ehdr, section_by_name(ehdr, ".dynstr"));
    int i, count = dynsym_shdr->sh_size / sizeof(Elf64_Sym);
    for(i = 0; i < count; i++)
    {
        if(ELF64_ST_TYPE(syms[i].st_info) == STT_FUNC)
        {
            if(strlen(strs + syms[i].st_name) == 1){
                printf("%s\n", strs + syms[i].st_name);
            Elf64_Shdr* shdr = section_by_index(ehdr, syms[i].st_shndx);
            unsigned char *address = AT_SEC(ehdr,shdr) + (syms[i].st_value - shdr->sh_addr);
            inspecting(ehdr, address, syms[i].st_value);
            }
        }
    }
    return 0;
}





//
// Created by qixiangc on 10/11/16.
//

