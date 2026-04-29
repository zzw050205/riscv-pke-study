/*
 * routines that scan and load a (host) Executable and Linkable Format (ELF)
 * file into the (emulated) memory.
 */

#include "elf.h"
#include "riscv.h"
#include "spike_interface/spike_file.h"
#include "spike_interface/spike_utils.h"
#include "string.h"

//
// the implementation of allocater. allocates memory space for later segment
// loading
//
static void *elf_alloc_mb(elf_ctx *ctx, uint64 elf_pa, uint64 elf_va,
                          uint64 size) {
  // directly returns the virtual address as we are in the Bare mode in lab1_x
  return (void *)elf_va;
}

//
// actual file reading, using the spike file interface.
//
static uint64 elf_fpread(elf_ctx *ctx, void *dest, uint64 nb, uint64 offset) {
  elf_info *msg = (elf_info *)ctx->info;
  // call spike file utility to load the content of elf file into memory.
  // spike_file_pread will read the elf file (msg->f) from offset to memory
  // (indicated by *dest) for nb bytes.
  return spike_file_pread(msg->f, dest, nb, offset);
}

//
// init elf_ctx, a data structure that loads the elf.
//
elf_status elf_init(elf_ctx *ctx, void *info) {
  ctx->info = info;

  // load the elf header
  if (elf_fpread(ctx, &ctx->ehdr, sizeof(ctx->ehdr), 0) != sizeof(ctx->ehdr))
    return EL_EIO;

  // check the signature (magic value) of the elf
  if (ctx->ehdr.magic != ELF_MAGIC)
    return EL_NOTELF;

  return EL_OK;
}

// leb128 (little-endian base 128) is a variable-length
// compression algoritm in DWARF
void read_uleb128(uint64 *out, char **off) {
  uint64 value = 0;
  int shift = 0;
  uint8 b;
  for (;;) {
    b = *(uint8 *)(*off);
    (*off)++;
    value |= ((uint64)b & 0x7F) << shift;
    shift += 7;
    if ((b & 0x80) == 0)
      break;
  }
  if (out)
    *out = value;
}
void read_sleb128(int64 *out, char **off) {
  int64 value = 0;
  int shift = 0;
  uint8 b;
  for (;;) {
    b = *(uint8 *)(*off);
    (*off)++;
    value |= ((uint64_t)b & 0x7F) << shift;
    shift += 7;
    if ((b & 0x80) == 0)
      break;
  }
  if (shift < 64 && (b & 0x40))
    value |= -(1 << shift);
  if (out)
    *out = value;
}
// Since reading below types through pointer cast requires aligned address,
// so we can only read them byte by byte
void read_uint64(uint64 *out, char **off) {
  *out = 0;
  for (int i = 0; i < 8; i++) {
    *out |= (uint64)(**off) << (i << 3);
    (*off)++;
  }
}
void read_uint32(uint32 *out, char **off) {
  *out = 0;
  for (int i = 0; i < 4; i++) {
    *out |= (uint32)(**off) << (i << 3);
    (*off)++;
  }
}
void read_uint16(uint16 *out, char **off) {
  *out = 0;
  for (int i = 0; i < 2; i++) {
    *out |= (uint16)(**off) << (i << 3);
    (*off)++;
  }
}

/*
 * analyzis the data in the debug_line section
 *
 * the function needs 3 parameters: elf context, data in the debug_line section
 * and length of debug_line section
 *
 * make 3 arrays:
 * "process->dir" stores all directory paths of code files
 * "process->file" stores all code file names of code files and their directory
 * path index of array "dir" "process->line" stores all relationships map
 * instruction addresses to code line numbers and their code file name index of
 * array "file"
 */
void make_addr_line(elf_ctx *ctx, char *debug_line, uint64 length) {
  process *p = ((elf_info *)ctx->info)->p;
  p->debugline = debug_line;
  // directory name char pointer array
  p->dir = (char **)((((uint64)debug_line + length + 7) >> 3) << 3);
  int dir_ind = 0, dir_base;
  // file name char pointer array
  p->file = (code_file *)(p->dir + 64);
  int file_ind = 0, file_base;
  // table array
  p->line = (addr_line *)(p->file + 64);
  p->line_ind = 0;
  char *off = debug_line;
  while (off < debug_line + length) { // iterate each compilation unit(CU)
    debug_header *dh = (debug_header *)off;
    off += sizeof(debug_header);
    dir_base = dir_ind;
    file_base = file_ind;
    // get directory name char pointer in this CU
    while (*off != 0) {
      p->dir[dir_ind++] = off;
      while (*off != 0)
        off++;
      off++;
    }
    off++;
    // get file name char pointer in this CU
    while (*off != 0) {
      p->file[file_ind].file = off;
      while (*off != 0)
        off++;
      off++;
      uint64 dir;
      read_uleb128(&dir, &off);
      p->file[file_ind++].dir = dir - 1 + dir_base;
      read_uleb128(NULL, &off);
      read_uleb128(NULL, &off);
    }
    off++;
    addr_line regs;
    regs.addr = 0;
    regs.file = 1;
    regs.line = 1;
    // simulate the state machine op code
    for (;;) {
      uint8 op = *(off++);
      switch (op) {
      case 0: // Extended Opcodes
        read_uleb128(NULL, &off);
        op = *(off++);
        switch (op) {
        case 1: // DW_LNE_end_sequence
          if (p->line_ind > 0 && p->line[p->line_ind - 1].addr == regs.addr)
            p->line_ind--;
          p->line[p->line_ind] = regs;
          p->line[p->line_ind].file += file_base - 1;
          p->line_ind++;
          goto endop;
        case 2: // DW_LNE_set_address
          read_uint64(&regs.addr, &off);
          break;
        // ignore DW_LNE_define_file
        case 4: // DW_LNE_set_discriminator
          read_uleb128(NULL, &off);
          break;
        }
        break;
      case 1: // DW_LNS_copy
        if (p->line_ind > 0 && p->line[p->line_ind - 1].addr == regs.addr)
          p->line_ind--;
        p->line[p->line_ind] = regs;
        p->line[p->line_ind].file += file_base - 1;
        p->line_ind++;
        break;
      case 2: { // DW_LNS_advance_pc
        uint64 delta;
        read_uleb128(&delta, &off);
        regs.addr += delta * dh->min_instruction_length;
        break;
      }
      case 3: { // DW_LNS_advance_line
        int64 delta;
        read_sleb128(&delta, &off);
        regs.line += delta;
        break;
      }
      case 4: // DW_LNS_set_file
        read_uleb128(&regs.file, &off);
        break;
      case 5: // DW_LNS_set_column
        read_uleb128(NULL, &off);
        break;
      case 6: // DW_LNS_negate_stmt
      case 7: // DW_LNS_set_basic_block
        break;
      case 8: { // DW_LNS_const_add_pc
        int adjust = 255 - dh->opcode_base;
        int delta = (adjust / dh->line_range) * dh->min_instruction_length;
        regs.addr += delta;
        break;
      }
      case 9: { // DW_LNS_fixed_advanced_pc
        uint16 delta;
        read_uint16(&delta, &off);
        regs.addr += delta;
        break;
      }
        // ignore 10, 11 and 12
      default: { // Special Opcodes
        int adjust = op - dh->opcode_base;
        int addr_delta = (adjust / dh->line_range) * dh->min_instruction_length;
        int line_delta = dh->line_base + (adjust % dh->line_range);
        regs.addr += addr_delta;
        regs.line += line_delta;
        if (p->line_ind > 0 && p->line[p->line_ind - 1].addr == regs.addr)
          p->line_ind--;
        p->line[p->line_ind] = regs;
        p->line[p->line_ind].file += file_base - 1;
        p->line_ind++;
        break;
      }
      }
    }
  endop:;
  }
  // for (int i = 0; i < p->line_ind; i++)
  //     sprint("%p %d %d\n", p->line[i].addr, p->line[i].line,
  //     p->line[i].file);
}

//
// load the elf segments to memory regions as we are in Bare mode in lab1
//
elf_status elf_load(elf_ctx *ctx) {
  // elf_prog_header structure is defined in kernel/elf.h
  elf_prog_header ph_addr;
  int i, off;
  uint64 max_vaddr = 0;

  // traverse the elf program segment headers
  for (i = 0, off = ctx->ehdr.phoff; i < ctx->ehdr.phnum;
       i++, off += sizeof(ph_addr)) {
    // read segment headers
    if (elf_fpread(ctx, (void *)&ph_addr, sizeof(ph_addr), off) !=
        sizeof(ph_addr))
      return EL_EIO;

    if (ph_addr.type != ELF_PROG_LOAD)
      continue;
    if (ph_addr.memsz < ph_addr.filesz)
      return EL_ERR;
    if (ph_addr.vaddr + ph_addr.memsz < ph_addr.vaddr)
      return EL_ERR;

    // allocate memory block before elf loading
    void *dest = elf_alloc_mb(ctx, ph_addr.vaddr, ph_addr.vaddr, ph_addr.memsz);

    // actual loading
    if (elf_fpread(ctx, dest, ph_addr.memsz, ph_addr.off) != ph_addr.memsz)
      return EL_EIO;

    // record the max vaddr
    if (ph_addr.vaddr + ph_addr.memsz > max_vaddr)
      max_vaddr = ph_addr.vaddr + ph_addr.memsz;
  }

  // load the debug_line section
  // 1. load the string table section
  elf_sect_header shdr;
  if (elf_fpread(ctx, &shdr, sizeof(shdr),
                 ctx->ehdr.shoff + ctx->ehdr.shstrndx * sizeof(shdr)) !=
      sizeof(shdr))
    return EL_EIO;

  // align max_vaddr
  max_vaddr = (max_vaddr + 7) & ~7;
  char *shstrtab = (char *)max_vaddr;
  if (elf_fpread(ctx, shstrtab, shdr.size, shdr.offset) != shdr.size)
    return EL_EIO;
  max_vaddr += shdr.size;

  // 2. traverse the section headers to find .debug_line
  for (i = 0, off = ctx->ehdr.shoff; i < ctx->ehdr.shnum;
       i++, off += sizeof(shdr)) {
    if (elf_fpread(ctx, &shdr, sizeof(shdr), off) != sizeof(shdr))
      return EL_EIO;

    if (strcmp(shstrtab + shdr.name, ".debug_line") == 0) {
      max_vaddr = (max_vaddr + 7) & ~7;
      char *debug_line = (char *)max_vaddr;
      if (elf_fpread(ctx, debug_line, shdr.size, shdr.offset) != shdr.size)
        return EL_EIO;

      make_addr_line(ctx, debug_line, shdr.size);
      break;
    }
  }

  return EL_OK;
}

typedef union {
  uint64 buf[MAX_CMDLINE_ARGS];
  char *argv[MAX_CMDLINE_ARGS];
} arg_buf;

//
// returns the number (should be 1) of string(s) after PKE kernel in command
// line. and store the string(s) in arg_bug_msg.
//
static size_t parse_args(arg_buf *arg_bug_msg) {
  // HTIFSYS_getmainvars frontend call reads command arguments to (input)
  // *arg_bug_msg
  long r = frontend_syscall(HTIFSYS_getmainvars, (uint64)arg_bug_msg,
                            sizeof(*arg_bug_msg), 0, 0, 0, 0, 0);
  kassert(r == 0);

  size_t pk_argc = arg_bug_msg->buf[0];
  uint64 *pk_argv = &arg_bug_msg->buf[1];

  int arg = 1; // skip the PKE OS kernel string, leave behind only the
               // application name
  for (size_t i = 0; arg + i < pk_argc; i++)
    arg_bug_msg->argv[i] = (char *)(uintptr_t)pk_argv[arg + i];

  // returns the number of strings after PKE kernel in command line
  return pk_argc - arg;
}

//
// load the elf of user application, by using the spike file interface.
//
void load_bincode_from_host_elf(process *p) {
  arg_buf arg_bug_msg;

  uint64 hartid = read_tp();

  // retrieve command line arguements
  size_t argc = parse_args(&arg_bug_msg);
  if (!argc)
    panic("You need to specify the application program!\n");

  sprint("hartid = %ld: Application: %s\n", hartid, arg_bug_msg.argv[hartid]);
  strcpy(p->name, arg_bug_msg.argv[hartid]);

  // elf loading. elf_ctx is defined in kernel/elf.h, used to track the loading
  // process.
  elf_ctx elfloader;
  // elf_info is defined above, used to tie the elf file and its corresponding
  // process.
  elf_info info;

  info.f = spike_file_open(arg_bug_msg.argv[hartid], O_RDONLY, 0);
  info.p = p;
  // IS_ERR_VALUE is a macro defined in spike_interface/spike_htif.h
  if (IS_ERR_VALUE(info.f))
    panic("Fail on openning the input application program.\n");

  // init elfloader context. elf_init() is defined above.
  if (elf_init(&elfloader, &info) != EL_OK)
    panic("fail to init elfloader.\n");

  // load elf. elf_load() is defined above.
  if (elf_load(&elfloader) != EL_OK)
    panic("Fail on loading elf.\n");

  // entry (virtual, also physical in lab1_x) address
  p->trapframe->epc = elfloader.ehdr.entry;

  // close the host spike file
  spike_file_close(info.f);

  sprint("hartid = %ld: Application program entry point (virtual address): "
         "0x%lx\n",
         hartid, p->trapframe->epc);
}

//
// print the code line info by instruction address.
// format: "Runtime error at <path>/<filename>:<line_number>  <source_line>
// <exception_type>!" e.g., "Runtime error at user/app_errorline.c:13   asm
// volatile(\"csrw sscratch, 0\"); Illegal instruction!"
//
void print_error_line(uint64 addr) {
  uint64 hartid = read_tp();
  process *p = current[hartid];
  if (!p || !p->line)
    return;

  int i;
  for (i = 0; i < p->line_ind; i++) {
    if (p->line[i].addr > addr)
      break;
  }
  if (i > 0)
    i--;

  // Read the source code line from file
  char line_buf[256];
  line_buf[0] = '\0';

  // Build the full source file path: dir/file
  char src_path[128];
  int si = 0;
  char *d = p->dir[p->file[p->line[i].file].dir];
  while (*d && si < 122)
    src_path[si++] = *(d++);
  src_path[si++] = '/';
  char *f = p->file[p->line[i].file].file;
  while (*f && si < 127)
    src_path[si++] = *(f++);
  src_path[si] = '\0';

  // Try to open source file and read the specific line
  // Use the host file path directly
  spike_file_t *sf = spike_file_open(src_path, O_RDONLY, 0);
  if (IS_ERR_VALUE(sf)) {
    // Try with "./" prefix
    char alt_path[128];
    alt_path[0] = '.';
    alt_path[1] = '/';
    int ai = 2;
    for (int ci = 0; src_path[ci] && ai < 125; ci++)
      alt_path[ai++] = src_path[ci];
    alt_path[ai] = '\0';
    sf = spike_file_open(alt_path, O_RDONLY, 0);
    if (!IS_ERR_VALUE(sf)) {
      // Copy alt_path to src_path for reference
      for (ai = 0; alt_path[ai]; ai++)
        src_path[ai] = alt_path[ai];
      src_path[ai] = '\0';
    }
  }

  if (!IS_ERR_VALUE(sf)) {
    int cur_line = 1;
    int pos = 0;
    char ch;
    while (cur_line < p->line[i].line && spike_file_read(sf, &ch, 1) > 0) {
      if (ch == '\n')
        cur_line++;
    }
    // Read the target line content
    while (pos < 254 && spike_file_read(sf, &ch, 1) > 0 && ch != '\n') {
      line_buf[pos++] = ch;
    }
    line_buf[pos] = '\0';
    spike_file_close(sf);
  }

  // Single line output: Runtime error at <path>/<filename>:<line_number>
  // <source_line> (exception type is appended by the caller via panic())
  sprint("Runtime error at %s/%s:%d  %s ", p->dir[p->file[p->line[i].file].dir],
         p->file[p->line[i].file].file, p->line[i].line, line_buf);
}

//
// find the symbol name by address. added @lab1_challenge1
//
elf_status elf_find_symbol_by_addr(elf_ctx *ctx, uint64 addr, char *name) {
  elf_section_header sh;
  int i;

  // 1. Find the symbol table section and string table section
  elf_section_header symtab_sh, strtab_sh;
  int found_symtab = 0, found_strtab = 0;

  // We need to read the section names to find .symtab and .strtab
  // First, read the section header string table
  elf_section_header shstrtab_sh;
  uint64 shstrtab_off = ctx->ehdr.shoff + ctx->ehdr.shstrndx * sizeof(sh);
  if (elf_fpread(ctx, &shstrtab_sh, sizeof(shstrtab_sh), shstrtab_off) !=
      sizeof(shstrtab_sh))
    return EL_EIO;

  for (i = 0; i < ctx->ehdr.shnum; i++) {
    if (elf_fpread(ctx, &sh, sizeof(sh), ctx->ehdr.shoff + i * sizeof(sh)) !=
        sizeof(sh))
      return EL_EIO;

    char sh_name[32];
    elf_fpread(ctx, sh_name, sizeof(sh_name), shstrtab_sh.off + sh.name);

    if (strcmp(sh_name, ".symtab") == 0) {
      symtab_sh = sh;
      found_symtab = 1;
    } else if (strcmp(sh_name, ".strtab") == 0) {
      strtab_sh = sh;
      found_strtab = 1;
    }
  }

  if (!found_symtab || !found_strtab)
    return EL_ERR;

  // 2. Traverse the symbol table
  elf_symbol sym;
  for (i = 0; i < symtab_sh.size / sizeof(sym); i++) {
    if (elf_fpread(ctx, &sym, sizeof(sym), symtab_sh.off + i * sizeof(sym)) !=
        sizeof(sym))
      return EL_EIO;

    if (addr >= sym.value && addr < sym.value + sym.size) {
      // Found the symbol!
      if (elf_fpread(ctx, name, 64, strtab_sh.off + sym.name) > 0) {
        return EL_OK;
      }
    }
  }

  return EL_ERR;
}
