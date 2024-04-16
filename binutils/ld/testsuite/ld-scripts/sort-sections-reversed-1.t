SECTIONS
{
  .text : { *(SORT_BY_NAME(REVERSE(.text*))) }
  /DISCARD/ : { *(.*) }
}
