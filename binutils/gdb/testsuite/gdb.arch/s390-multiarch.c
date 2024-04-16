/* Copyright 2013-2024 Free Software Foundation, Inc.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include <errno.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct
{
  unsigned char	e_ident[16];
  uint16_t	e_type;
  uint16_t	e_machine;
  uint32_t	e_version;
  uint32_t	e_entry;
  uint32_t	e_phoff;
  uint32_t	e_shoff;
  uint32_t	e_flags;
  uint16_t	e_ehsize;
  uint16_t	e_phentsize;
  uint16_t	e_phnum;
  uint16_t	e_shentsize;
  uint16_t	e_shnum;
  uint16_t	e_shstrndx;
} Elf32_Ehdr;

typedef struct
{
  unsigned char	e_ident[16];
  uint16_t	e_type;
  uint16_t	e_machine;
  uint32_t	e_version;
  uint64_t	e_entry;
  uint64_t	e_phoff;
  uint64_t	e_shoff;
  uint32_t	e_flags;
  uint16_t	e_ehsize;
  uint16_t	e_phentsize;
  uint16_t	e_phnum;
  uint16_t	e_shentsize;
  uint16_t	e_shnum;
  uint16_t	e_shstrndx;
} Elf64_Ehdr;

typedef struct
{
  uint32_t	p_type;
  uint32_t	p_offset;
  uint32_t	p_vaddr;
  uint32_t	p_paddr;
  uint32_t	p_filesz;
  uint32_t	p_memsz;
  uint32_t	p_flags;
  uint32_t	p_align;
} Elf32_Phdr;

typedef struct
{
  uint32_t	p_type;
  uint32_t	p_flags;
  uint64_t	p_offset;
  uint64_t	p_vaddr;
  uint64_t	p_paddr;
  uint64_t	p_filesz;
  uint64_t	p_memsz;
  uint64_t	p_align;
} Elf64_Phdr;

struct elfbuf
{
  const char *path;
  unsigned char *buf;
  size_t len;
  enum { ELFCLASS32 = 1,
	 ELFCLASS64 = 2 } ei_class;
};

#define ELFBUF_EHDR_LEN(elf)					\
  ((elf)->ei_class == ELFCLASS32 ? sizeof (Elf32_Ehdr) :	\
   sizeof (Elf64_Ehdr))

#define ELFBUF_EHDR(elf, memb)			\
  ((elf)->ei_class == ELFCLASS32 ?		\
   ((Elf32_Ehdr *) (elf)->buf)->memb		\
   : ((Elf64_Ehdr *) (elf)->buf)->memb)

#define ELFBUF_PHDR_LEN(elf)					\
  ((elf)->ei_class == ELFCLASS32 ? sizeof (Elf32_Phdr) :	\
   sizeof (Elf64_Phdr))

#define ELFBUF_PHDR(elf, idx, memb)				\
  ((elf)->ei_class == ELFCLASS32 ?				\
   ((Elf32_Phdr *) &(elf)->buf[((Elf32_Ehdr *)(elf)->buf)	\
			       ->e_phoff])[idx].memb		\
   : ((Elf64_Phdr *) &(elf)->buf[((Elf64_Ehdr *)(elf)->buf)	\
				 ->e_phoff])[idx].memb)

static void
exit_with_msg(const char *fmt, ...)
{
  va_list ap;

  fflush (stdout);
  va_start (ap, fmt);
  vfprintf (stderr, fmt, ap);
  va_end (ap);

  if (errno)
    {
      fputs (": ", stderr);
      perror (NULL);
    }
  else
    fputc ('\n', stderr);
  exit (1);
}

static void
read_file (unsigned char **buf_ptr, size_t *len_ptr, FILE *fp)
{
  size_t len = 0;
  size_t size = 1024;
  size_t chunk;
  unsigned char *buf = malloc (size);

  while ((chunk = fread (buf + len, 1, size - len, fp)) == size - len)
    {
      len = size;
      size *= 2;
      buf = realloc (buf, size);
    }
  len += chunk;
  *buf_ptr = buf;
  *len_ptr = len;
}

static void
write_file (unsigned char *buf, size_t len, FILE *fp)
{
  fwrite (buf, 1, len, fp);
}

static void
elfbuf_init_from_file (struct elfbuf *elf, const char *path)
{
  FILE *fp = fopen (path, "rb");
  unsigned char *buf;
  size_t len;

  if (fp == NULL)
    exit_with_msg ("%s", path);

  read_file (&buf, &len, fp);
  fclose (fp);

  /* Validate ELF identification. */
  if (len < 16
      || buf[0] != 0x7f || buf[1] != 0x45 || buf[2] != 0x4c || buf[3] != 0x46
      || buf[4] < 1 || buf[4] > 2 || buf[5] < 1 || buf[5] > 2)
    exit_with_msg ("%s: unsupported or invalid ELF file", path);

  elf->path = path;
  elf->buf = buf;
  elf->len = len;
  elf->ei_class = buf[4];

  if (ELFBUF_EHDR_LEN (elf) > len
      || ELFBUF_EHDR (elf, e_phoff) > len
      || ELFBUF_EHDR (elf, e_phnum) > ((len - ELFBUF_EHDR (elf, e_phoff))
				       / ELFBUF_PHDR_LEN (elf)) )
    exit_with_msg ("%s: unexpected end of data", path);

  if (ELFBUF_EHDR (elf, e_phentsize) != ELFBUF_PHDR_LEN (elf))
    exit_with_msg ("%s: inconsistent ELF header", path);
}

static void
elfbuf_write_to_file (struct elfbuf *elf, const char *path)
{
  FILE *fp = fopen (path, "wb");

  if (fp == NULL)
    exit_with_msg ("%s", path);

  write_file (elf->buf, elf->len, fp);
  fclose (fp);
}

/* In the auxv note starting at OFFSET with size LEN, mask the hwcap
   field using the HWCAP_MASK. */

static void
elfbuf_handle_auxv (struct elfbuf *elf, size_t offset, size_t len,
		    unsigned long hwcap_mask)
{
  size_t i;
  uint32_t *auxv32 = (uint32_t *) (elf->buf + offset);
  uint64_t *auxv64 = (uint64_t *) auxv32;
  size_t entry_size = elf->ei_class == ELFCLASS32 ?
    sizeof (auxv32[0]) : sizeof (auxv64[0]);

  for (i = 0; i < len / entry_size; i++)
    {
      uint64_t auxv_type = elf->ei_class == ELFCLASS32 ?
	auxv32[2 * i] : auxv64[2 * i];

      if (auxv_type == 0)
	break;
      if (auxv_type != 16)
	continue;

      if (elf->ei_class == ELFCLASS32)
	auxv32[2 * i + 1] &= (uint32_t) hwcap_mask;
      else
	auxv64[2 * i + 1] &= (uint64_t) hwcap_mask;
    }
}

/* In the note segment starting at OFFSET with size LEN, make notes
   with type NOTE_TYPE unrecognizable by GDB.  Also, mask the hwcap
   field of any auxv notes using the HWCAP_MASK. */

static void
elfbuf_handle_note_segment (struct elfbuf *elf, size_t offset, size_t len,
			    unsigned note_type, unsigned long hwcap_mask)
{
  size_t pos = 0;

  while (pos + 12 < len)
    {
      uint32_t *note = (uint32_t *) (elf->buf + offset + pos);
      size_t desc_pos = pos + 12 + ((note[0] + 3) & ~3);
      size_t next_pos = desc_pos + ((note[1] + 3) & ~3);

      if (desc_pos > len || next_pos > len)
	exit_with_msg ("%s: corrupt notes data", elf->path);

      if (note[2] == note_type)
	note[2] |= 0xff000000;
      else if (note[2] == 6 && hwcap_mask != 0)
	elfbuf_handle_auxv (elf, offset + desc_pos, note[1],
			    hwcap_mask);
      pos = next_pos;
    }
}

static void
elfbuf_handle_core_notes (struct elfbuf *elf, unsigned note_type,
			  unsigned long hwcap_mask)
{
  unsigned ph_idx;

  if (ELFBUF_EHDR (elf, e_type) != 4)
    exit_with_msg ("%s: not a core file", elf->path);

  /* Iterate over program headers. */
  for (ph_idx = 0; ph_idx != ELFBUF_EHDR (elf, e_phnum); ph_idx++)
    {
      size_t offset = ELFBUF_PHDR (elf, ph_idx, p_offset);
      size_t filesz = ELFBUF_PHDR (elf, ph_idx, p_filesz);

      if (offset > elf->len || filesz > elf->len - offset)
	exit_with_msg ("%s: unexpected end of data", elf->path);

      /* Deal with NOTE segments only. */
      if (ELFBUF_PHDR (elf, ph_idx, p_type) != 4)
	continue;
      elfbuf_handle_note_segment (elf, offset, filesz, note_type,
				  hwcap_mask);
    }
}

int
main (int argc, char *argv[])
{
  unsigned note_type;
  unsigned long hwcap_mask = 0;
  struct elfbuf elf;

  if (argc < 4)
    {
      abort ();
    }

  if (sscanf (argv[3], "%u", &note_type) != 1)
    exit_with_msg ("%s: bad command line arguments\n", argv[0]);

  if (argc >= 5)
    {
      if (sscanf (argv[4], "%lu", &hwcap_mask) != 1)
	exit_with_msg ("%s: bad command line arguments\n", argv[0]);
    }

  elfbuf_init_from_file (&elf, argv[1]);
  elfbuf_handle_core_notes (&elf, note_type, hwcap_mask);
  elfbuf_write_to_file (&elf, argv[2]);

  return 0;
}
