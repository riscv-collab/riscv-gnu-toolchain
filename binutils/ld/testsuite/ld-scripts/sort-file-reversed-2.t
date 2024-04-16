SECTIONS
{
  .text : { REVERSE(SORT_BY_NAME(*))(.text*) }
  .data : { REVERSE(*)(.data*) }
  /DISCARD/ : { *(.*) }
}
