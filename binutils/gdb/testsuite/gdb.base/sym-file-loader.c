/* Copyright 2013-2024 Free Software Foundation, Inc.
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <assert.h>

#include "sym-file-loader.h"

#include <inttypes.h>
#include <ansidecl.h>
#include <elf/common.h>
#include <elf/external.h>

#ifdef TARGET_LP64

typedef Elf64_External_Phdr Elf_External_Phdr;
typedef Elf64_External_Ehdr Elf_External_Ehdr;
typedef Elf64_External_Shdr Elf_External_Shdr;
typedef Elf64_External_Sym Elf_External_Sym;
typedef uint64_t Elf_Addr;

#elif defined TARGET_ILP32

typedef Elf32_External_Phdr Elf_External_Phdr;
typedef Elf32_External_Ehdr Elf_External_Ehdr;
typedef Elf32_External_Shdr Elf_External_Shdr;
typedef Elf32_External_Sym Elf_External_Sym;
typedef uint32_t Elf_Addr;

#endif

#define GET(hdr, field) (\
sizeof ((hdr)->field) == 1 ? (uint64_t) (hdr)->field[0] : \
sizeof ((hdr)->field) == 2 ? (uint64_t) *(uint16_t *) (hdr)->field : \
sizeof ((hdr)->field) == 4 ? (uint64_t) *(uint32_t *) (hdr)->field : \
sizeof ((hdr)->field) == 8 ? *(uint64_t *) (hdr)->field : \
*(uint64_t *) NULL)

#define GETADDR(hdr, field) (\
sizeof ((hdr)->field) == sizeof (Elf_Addr) ? *(Elf_Addr *) (hdr)->field : \
*(Elf_Addr *) NULL)

struct segment
{
  uint8_t *mapped_addr;
  size_t mapped_size;
  Elf_External_Phdr *phdr;
  struct segment *next;
};

struct library
{
  int fd;
  Elf_External_Ehdr *ehdr;
  struct segment *segments;
};

static Elf_External_Shdr *find_shdr (Elf_External_Ehdr *ehdr,
				     const char *section);
static int translate_offset (uint64_t file_offset, struct segment *seg,
			     void **addr);

#ifdef TARGET_LP64

uint8_t
elf_st_type (uint8_t st_info)
{
  return ELF64_ST_TYPE (st_info);
}

#elif defined TARGET_ILP32

uint8_t
elf_st_type (uint8_t st_info)
{
  return ELF32_ST_TYPE (st_info);
}

#endif

/* Load a program segment.  */

static struct segment *
load (uint8_t *addr, Elf_External_Phdr *phdr, struct segment *tail_seg)
{
  struct segment *seg = NULL;
  uint8_t *mapped_addr = NULL;
  size_t mapped_size = 0;
  void *from = NULL;
  void *to = NULL;

  /* For the sake of simplicity all operations are permitted.  */
  unsigned perm = PROT_READ | PROT_WRITE | PROT_EXEC;

  mapped_addr = (uint8_t *) mmap ((void *) GETADDR (phdr, p_vaddr),
				  GET (phdr, p_memsz), perm,
				  MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
  assert (mapped_addr != MAP_FAILED);

  mapped_size = GET (phdr, p_memsz);

  from = (void *) (addr + GET (phdr, p_offset));
  to = (void *) mapped_addr;

  memcpy (to, from, GET (phdr, p_filesz));

  seg = (struct segment *) malloc (sizeof (struct segment));

  if (seg == 0)
    return 0;

  seg->mapped_addr = mapped_addr;
  seg->mapped_size = mapped_size;
  seg->phdr = phdr;
  seg->next = 0;

  if (tail_seg != 0)
    tail_seg->next = seg;

  return seg;
}

#ifdef __linux__
# define SELF_LINK "/proc/self/exe"
#elif defined NETBSD
# define SELF_LINK "/proc/curproc/exe"
#elif defined __OpenBSD__ || defined __FreeBSD__ || defined __DragonFly__
# define SELF_LINK "/proc/curproc/file"
#elif defined SunOS
# define SELF_LINK "/proc/self/path/a.out"
#endif

/* Like RPATH=$ORIGIN, return the dirname of the current
   executable.  */

static const char *
get_origin (void)
{
  static char self_path[PATH_MAX];
  static ssize_t self_path_len;

  if (self_path_len == 0)
    {
#ifdef SELF_LINK
      self_path_len = readlink (SELF_LINK, self_path, PATH_MAX - 1);
      if (self_path_len != -1)
	{
	  char *dirsep;

	  self_path[self_path_len] = '\0';
	  dirsep = strrchr (self_path, '/');
	  *dirsep = '\0';
	}
#else
      self_path_len = -1;
#endif
    }

  if (self_path_len == -1)
    return NULL;
  else
    return self_path;
}

/* Unload/unmap a segment.  */

static void
unload (struct segment *seg)
{
  munmap (seg->mapped_addr, seg->mapped_size);
  free (seg);
}

void
unload_shlib (struct library *lib)
{
  struct segment *seg, *next_seg;

  for (seg = lib->segments; seg != NULL; seg = next_seg)
    {
      next_seg = seg->next;
      unload (seg);
    }

  close (lib->fd);
  free (lib);
}

/* Mini shared library loader.  No reallocation
   is performed for the sake of simplicity.  */

struct library *
load_shlib (const char *file)
{
  struct library *lib;
  uint64_t i;
  int fd = -1;
  off_t fsize;
  uint8_t *addr;
  Elf_External_Ehdr *ehdr;
  Elf_External_Phdr *phdr;
  struct segment *head_seg = NULL;
  struct segment *tail_seg = NULL;
  const char *origin;
  char *path;

  /* Map the lib in memory for reading.

     If the file name is relative, try looking it up relative to the
     main executable's path.  I.e., emulate RPATH=$ORIGIN.  */
  if (file[0] != '/')
    {
      origin = get_origin ();
      if (origin == NULL)
	{
	  fprintf (stderr, "get_origin not implemented.");
	  return NULL;
	}

      path = alloca (strlen (origin) + 1 + strlen (file) + 1);
      sprintf (path, "%s/%s", origin, file);
      fd = open (path, O_RDONLY);
    }

  if (fd < 0)
    fd = open (file, O_RDONLY);

  if (fd < 0)
    {
      perror ("fopen failed.");
      return NULL;
    }

  fsize = lseek (fd, 0, SEEK_END);

  if (fsize < 0)
    {
      perror ("lseek failed.");
      return NULL;
    }

  addr = (uint8_t *) mmap (NULL, fsize, PROT_READ, MAP_PRIVATE, fd, 0);
  if (addr == MAP_FAILED)
    {
      perror ("mmap failed.");
      return NULL;
    }

  /* Check if the lib is an ELF file.  */
  ehdr = (Elf_External_Ehdr *) addr;
  if (ehdr->e_ident[EI_MAG0] != ELFMAG0
      || ehdr->e_ident[EI_MAG1] != ELFMAG1
      || ehdr->e_ident[EI_MAG2] != ELFMAG2
      || ehdr->e_ident[EI_MAG3] != ELFMAG3)
    {
      printf ("Not an ELF file: %x\n", ehdr->e_ident[EI_MAG0]);
      return NULL;
    }

  if (ehdr->e_ident[EI_CLASS] == ELFCLASS32)
    {
      if (sizeof (void *) != 4)
	{
	  printf ("Architecture mismatch.");
	  return NULL;
	}
    }
  else if (ehdr->e_ident[EI_CLASS] == ELFCLASS64)
    {
      if (sizeof (void *) != 8)
	{
	  printf ("Architecture mismatch.");
	  return NULL;
	}
    }

  lib = malloc (sizeof (struct library));
  if (lib == NULL)
    {
      printf ("malloc failed.");
      return NULL;
    }

  lib->fd = fd;

  /* Load the program segments.  For the sake of simplicity
     assume that no reallocation is needed.  */
  phdr = (Elf_External_Phdr *) (addr + GET (ehdr, e_phoff));
  for (i = 0; i < GET (ehdr, e_phnum); i++, phdr++)
    {
      if (GET (phdr, p_type) == PT_LOAD)
	{
	  struct segment *next_seg = load (addr, phdr, tail_seg);
	  if (next_seg == 0)
	    continue;
	  tail_seg = next_seg;
	  if (head_seg == 0)
	    head_seg = next_seg;
	}
    }
  lib->ehdr = ehdr;
  lib->segments = head_seg;
  return lib;
}

int
get_text_addr (struct library *lib, void **text_addr)
{
  Elf_External_Shdr *text;

  /* Get the text section.  */
  text = find_shdr (lib->ehdr, ".text");
  if (text == NULL)
    return -1;

  if (translate_offset (GET (text, sh_offset), lib->segments, text_addr)
      != 0)
    return -1;

  return 0;
}

/* Return the section-header table.  */

Elf_External_Shdr *
find_shdrtab (Elf_External_Ehdr *ehdr)
{
  return (Elf_External_Shdr *) (((uint8_t *) ehdr) + GET (ehdr, e_shoff));
}

/* Return the string table of the section headers.  */

const char *
find_shstrtab (Elf_External_Ehdr *ehdr, uint64_t *size)
{
  const Elf_External_Shdr *shdr;
  const Elf_External_Shdr *shstr;

  if (GET (ehdr, e_shnum) <= GET (ehdr, e_shstrndx))
    {
      printf ("The index of the string table is corrupt.");
      return NULL;
    }

  shdr = find_shdrtab (ehdr);

  shstr = &shdr[GET (ehdr, e_shstrndx)];
  *size = GET (shstr, sh_size);
  return ((const char *) ehdr) + GET (shstr, sh_offset);
}

/* Return the string table named SECTION.  */

const char *
find_strtab (Elf_External_Ehdr *ehdr,
	     const char *section, uint64_t *strtab_size)
{
  uint64_t shstrtab_size = 0;
  const char *shstrtab;
  uint64_t i;
  const Elf_External_Shdr *shdr = find_shdrtab (ehdr);

  /* Get the string table of the section headers.  */
  shstrtab = find_shstrtab (ehdr, &shstrtab_size);
  if (shstrtab == NULL)
    return NULL;

  for (i = 0; i < GET (ehdr, e_shnum); i++)
    {
      uint64_t name = GET (shdr + i, sh_name);
      if (GET (shdr + i, sh_type) == SHT_STRTAB && name <= shstrtab_size
	  && strcmp ((const char *) &shstrtab[name], section) == 0)
	{
	  *strtab_size = GET (shdr + i, sh_size);
	  return ((const char *) ehdr) + GET (shdr + i, sh_offset);
	}

    }
  return NULL;
}

/* Return the section header named SECTION.  */

static Elf_External_Shdr *
find_shdr (Elf_External_Ehdr *ehdr, const char *section)
{
  uint64_t shstrtab_size = 0;
  const char *shstrtab;
  uint64_t i;

  /* Get the string table of the section headers.  */
  shstrtab = find_shstrtab (ehdr, &shstrtab_size);
  if (shstrtab == NULL)
    return NULL;

  Elf_External_Shdr *shdr = find_shdrtab (ehdr);
  for (i = 0; i < GET (ehdr, e_shnum); i++)
    {
      uint64_t name = GET (shdr + i, sh_name);
      if (name <= shstrtab_size)
	{
	  if (strcmp ((const char *) &shstrtab[name], section) == 0)
	    return &shdr[i];
	}

    }
  return NULL;
}

/* Return the symbol table.  */

static Elf_External_Sym *
find_symtab (Elf_External_Ehdr *ehdr, uint64_t *symtab_size)
{
  uint64_t i;
  const Elf_External_Shdr *shdr = find_shdrtab (ehdr);

  for (i = 0; i < GET (ehdr, e_shnum); i++)
    {
      if (GET (shdr + i, sh_type) == SHT_SYMTAB)
	{
	  *symtab_size = GET (shdr + i, sh_size) / sizeof (Elf_External_Sym);
	  return (Elf_External_Sym *) (((const char *) ehdr) +
				       GET (shdr + i, sh_offset));
	}
    }
  return NULL;
}

/* Translate a file offset to an address in a loaded segment.   */

static int
translate_offset (uint64_t file_offset, struct segment *seg, void **addr)
{
  while (seg)
    {
      uint64_t p_from, p_to;

      Elf_External_Phdr *phdr = seg->phdr;

      if (phdr == NULL)
	{
	  seg = seg->next;
	  continue;
	}

      p_from = GET (phdr, p_offset);
      p_to = p_from + GET (phdr, p_filesz);

      if (p_from <= file_offset && file_offset < p_to)
	{
	  *addr = (void *) (seg->mapped_addr + (file_offset - p_from));
	  return 0;
	}
      seg = seg->next;
    }

  return -1;
}

/* Lookup the address of FUNC.  */

int
lookup_function (struct library *lib, const char *func, void **addr)
{
  const char *strtab;
  uint64_t strtab_size = 0;
  Elf_External_Sym *symtab;
  uint64_t symtab_size = 0;
  uint64_t i;
  Elf_External_Ehdr *ehdr = lib->ehdr;
  struct segment *seg = lib->segments;

  /* Get the string table for the symbols.  */
  strtab = find_strtab (ehdr, ".strtab", &strtab_size);
  if (strtab == NULL)
    {
      printf (".strtab not found.");
      return -1;
    }

  /* Get the symbol table.  */
  symtab = find_symtab (ehdr, &symtab_size);
  if (symtab == NULL)
    {
      printf ("symbol table not found.");
      return -1;
    }

  for (i = 0; i < symtab_size; i++)
    {
      Elf_External_Sym *sym = &symtab[i];

      if (elf_st_type (GET (sym, st_info)) != STT_FUNC)
	continue;

      if (GET (sym, st_name) < strtab_size)
	{
	  const char *name = &strtab[GET (sym, st_name)];
	  if (strcmp (name, func) == 0)
	    {

	      uint64_t offset = GET (sym, st_value);
	      return translate_offset (offset, seg, addr);
	    }
	}
    }

  return -1;
}
