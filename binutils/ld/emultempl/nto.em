# This shell script emits a C file. -*- C -*-
#   Copyright (C) 2023-2024 Free Software Foundation, Inc.
#
# This file is part of GLD, the Gnu Linker.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston, MA 02110-1301, USA.
#

# This file is sourced from elf.em, and defines extra Neutrino
# specific routines.

# NTO templates aims to refine the default ${ARCH}elf.em template.
. "${srcdir}/emultempl/${ARCH}elf.em"

cat >>e${EMULATION_NAME}.c <<EOF

#include "elf/internal.h"
#include "elf/common.h"
#include "elf-bfd.h"
#include "../bfd/libbfd.h"

bool nto_lazy_stack = false;
struct nto_stack_note
{
  unsigned char stacksize[4];
  unsigned char stackalloc[4];
  unsigned char execstack[4];
};

static asection*
nto_create_QNX_note_section(int type)
{
  asection *note_sec;
  flagword flags;
  Elf_External_Note *e_note;
  bfd_size_type size;

  /* As ${ARCH}elf.em is imported and ${ARCH}_elf_create_output_section_statements
     is called before this function, stub_file should already be defined.  */
  if (!stub_file)
    {
      einfo (_("%F%P: cannot create .note section in stub BFD.\n"));
      return NULL;
    }

  flags = (SEC_ALLOC | SEC_LOAD | SEC_READONLY | SEC_HAS_CONTENTS
	   | SEC_IN_MEMORY | SEC_LINKER_CREATED);
  note_sec = bfd_make_section_anyway_with_flags (stub_file->the_bfd, ".note", flags);
  if (! note_sec)
    {
      einfo (_("%F%P: failed to create .note section\n"));
      return NULL;
    }

  size = offsetof (Elf_External_Note, name[sizeof "QNX"]);
  size = (size + 3) & -(bfd_size_type) 4;
  size += sizeof (struct nto_stack_note);
  note_sec->size = size;

  elf_section_type (note_sec) = SHT_NOTE;
  note_sec->contents = xmalloc (note_sec->size);
  e_note = (Elf_External_Note *) note_sec->contents;
  bfd_h_put_32 (stub_file->the_bfd, sizeof "QNX", &e_note->namesz);
  bfd_h_put_32 (stub_file->the_bfd, sizeof (struct nto_stack_note), &e_note->descsz);
  bfd_h_put_32 (stub_file->the_bfd, type, &e_note->type);
  memcpy (e_note->name, "QNX", sizeof "QNX");

  return note_sec;
}

/* Lookup for a section holding a QNX note or create a new section.  */
static asection*
nto_lookup_QNX_note_section(int type)
{
  asection *stack_note_sec = NULL;
  bfd *abfd;
  bool duplicated_notes_detected = false;
  for (abfd = link_info.input_bfds; abfd != NULL; abfd = abfd->link.next)
    {
      Elf_External_Note *e_note;
      asection *sec;

      /* QNX notes are held under a note section simply named ".note".  */
      sec = bfd_get_section_by_name (abfd, ".note");
      if (!sec)
	continue;

      /* Verify that this is a QNX note of the expected type.  */
      sec->contents = xmalloc(sec->size);
      if (!bfd_get_section_contents (sec->owner, sec, sec->contents, (file_ptr) 0,
				     sec->size))
	einfo (_("%F%P: %pB: can't read contents of section .note: %E\n"),
	       sec->owner);

      e_note = (Elf_External_Note *) sec->contents;
      if (! strcmp("QNX", e_note->name) && *e_note->type == type)
	{
	  if (stack_note_sec)
	    {
	      if (!duplicated_notes_detected)
		{
		  einfo (_("%P: %pB: warning: duplicated QNX stack .note detected\n"),
			 stack_note_sec->owner);
		  duplicated_notes_detected = true;
		}
	      einfo (_("%P: %pB: warning: duplicated QNX stack .note detected\n"),
		     sec->owner);
	    }
	  else
	    {
	      stack_note_sec = sec;
	      /* Allow modification of this .note content.  */
	      stack_note_sec->flags |= SEC_IN_MEMORY;
	    }
	}
    }

  if (stack_note_sec)
    return stack_note_sec;
  else
    return nto_create_QNX_note_section(type);

}

/* Generate the QNX stack .note section.  */
static void
nto_add_note_section (void) {
  asection *note_sec;
  struct nto_stack_note *n_note;
  bfd_size_type h_size;
  bool is_update = false;

  if (nto_lazy_stack && !link_info.stacksize)
    {
      einfo (_("%F%P: error: --lazy-stack must follow -zstack-size=<size>\n"));
      return;
    }

  /* Don't create a note if none of the stack parameter have to be modified.  */
  if (link_info.stacksize <= 0 && (link_info.execstack == link_info.noexecstack))
    return;

  note_sec = nto_lookup_QNX_note_section(QNT_STACK);
  if (! note_sec)
    return;

  /* Update QNX stack note content.  */
  h_size = note_sec->size - sizeof(struct nto_stack_note);
  n_note = (struct nto_stack_note *) (note_sec->contents + h_size);
  is_update = note_sec->owner != stub_file->the_bfd;

  if (link_info.stacksize > 0)
    bfd_h_put_32 (note_sec->owner, link_info.stacksize, &n_note->stacksize);
  else if (!is_update)
    bfd_h_put_32 (note_sec->owner, 0, &n_note->stacksize);

  if (nto_lazy_stack || (!is_update && link_info.stacksize <= 0))
    bfd_h_put_32 (note_sec->owner, 4096, &n_note->stackalloc);
  else if (link_info.stacksize > 0)
    bfd_h_put_32 (note_sec->owner, link_info.stacksize, &n_note->stackalloc);

  if (link_info.execstack)
    bfd_h_put_32 (note_sec->owner, 0, &n_note->execstack);
  else if (!is_update || link_info.noexecstack)
    bfd_h_put_32 (note_sec->owner, 1, &n_note->execstack);
}

static void
nto_after_open (void)
{
  nto_add_note_section();
  gld${EMULATION_NAME}_after_open ();
}

EOF

# Define some shell vars to insert bits of code into the standard elf
# parse_args and list_options functions.
#

PARSE_AND_LIST_PROLOGUE=${PARSE_AND_LIST_PROLOGUE}'
enum nto_options
{
  OPTION_STACK = 500,
  OPTION_LAZY_STACK,
};
'

PARSE_AND_LIST_LONGOPTS=${PARSE_AND_LIST_LONGOPTS}'
  { "stack", required_argument, NULL, OPTION_STACK },
  { "lazy-stack", no_argument, NULL, OPTION_LAZY_STACK },
'

PARSE_AND_LIST_OPTIONS=${PARSE_AND_LIST_OPTIONS}'
  fprintf (file, _("\
  --stack <size>              Set size of the initial stack\n\
  --lazy-stack		      Set lazy allocation of stack\n\
"));
'

PARSE_AND_LIST_ARGS_CASES=${PARSE_AND_LIST_ARGS_CASES}'
    case OPTION_STACK:
      {
        char *end;
        link_info.stacksize = strtoul (optarg, &end, 0);
        if (*end || link_info.stacksize < 0)
          einfo (_("%F%P: invalid stack size `%s'\''\n"), optarg + 11);
        if (!link_info.stacksize)
          /* Use -1 for explicit no-stack, because zero means
             'default'.   */
          link_info.stacksize = -1;
        break;
      }
    case OPTION_LAZY_STACK:
      nto_lazy_stack = true;
      break;
'

# Put these extra Neutrino routines in ld_${EMULATION_NAME}_emulation
#

LDEMUL_AFTER_OPEN=nto_after_open
