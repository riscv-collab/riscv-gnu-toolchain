SECTIONS
{
  .text : { *(REVERSE(EXCLUDE_FILE(foo) .text*)) }
  /DISCARD/ : { *(.*) }
}
