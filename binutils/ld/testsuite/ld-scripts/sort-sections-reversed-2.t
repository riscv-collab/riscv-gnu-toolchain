SECTIONS
{
  .text : { *(REVERSE(SORT_BY_INIT_PRIORITY(.text*))) }
  /DISCARD/ : { *(REVERSE(.*)) }
}
