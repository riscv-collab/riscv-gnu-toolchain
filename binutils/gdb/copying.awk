BEGIN	{
	  FS="\"";
	  print "/* ==> Do not modify this file!!  " \
		"-*- buffer-read-only: t -*- vi" \
		":set ro:";
	  print "   It is created automatically by copying.awk.";
	  print "   Modify copying.awk instead.  <== */";
	  print ""
	  print "#include \"defs.h\""
	  print "#include \"command.h\""
	  print "#include \"gdbcmd.h\""
	  print ""
	  print "static void show_copying_command (const char *, int);"
	  print ""
	  print "static void show_warranty_command (const char *, int);"
	  print ""
	  print "static void";
	  print "show_copying_command (const char *ignore, int from_tty)";
	  print "{";
	}
NR == 1,/^[ 	]*15\. Disclaimer of Warranty\.[ 	]*$/	{
	  if ($0 ~ //)
	    {
	      printf "  gdb_printf (\"\\n\");\n";
	    }
	  else if ($0 !~ /^[ 	]*15\. Disclaimer of Warranty\.[ 	]*$/) 
	    {
	      printf "  gdb_printf (\"";
	      for (i = 1; i < NF; i++)
		printf "%s\\\"", $i;
	      printf "%s\\n\");\n", $NF;
	    }
	}
/^[	 ]*15\. Disclaimer of Warranty\.[ 	]*$/	{
	  print "}";
	  print "";
	  print "static void";
	  print "show_warranty_command (const char *ignore, int from_tty)";
	  print "{";
	}
/^[ 	]*15\. Disclaimer of Warranty\.[ 	]*$/, /^[ 	]*END OF TERMS AND CONDITIONS[ 	]*$/{  
	  if (! ($0 ~ /^[ 	]*END OF TERMS AND CONDITIONS[ 	]*$/)) 
	    {
	      printf "  gdb_printf (\"";
	      for (i = 1; i < NF; i++)
		printf "%s\\\"", $i;
	      printf "%s\\n\");\n", $NF;
	    }
	}
END	{
	  print "}";
	  print "";
	  print "void _initialize_copying ();"
	  print "void"
	  print "_initialize_copying ()";
	  print "{";
	  print "  add_cmd (\"copying\", no_set_class, show_copying_command,";
	  print "	   _(\"Conditions for redistributing copies of GDB.\"),";
	  print "	   &showlist);";
	  print "  add_cmd (\"warranty\", no_set_class, show_warranty_command,";
	  print "	   _(\"Various kinds of warranty you do not have.\"),";
	  print "	   &showlist);";
	  print "";
	  print "  /* For old-timers, allow \"info copying\", etc.  */";
	  print "  add_info (\"copying\", show_copying_command,";
	  print "	    _(\"Conditions for redistributing copies of GDB.\"));";
	  print "  add_info (\"warranty\", show_warranty_command,";
	  print "	    _(\"Various kinds of warranty you do not have.\"));";
	  print "}";
	}
