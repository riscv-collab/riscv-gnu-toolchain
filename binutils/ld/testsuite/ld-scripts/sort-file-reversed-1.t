SECTIONS
{
  .text : { SORT_BY_NAME(REVERSE(*))(.text*) }
  .data : { *(.data*) }
  /DISCARD/ : { *(.*) }
}
