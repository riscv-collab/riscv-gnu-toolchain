/* Copyright (C) 1986-2024 Free Software Foundation, Inc.

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

#ifndef YY_REMAP_H
#define YY_REMAP_H

/* Remap normal yacc parser interface names (yyparse, yylex, yyerror,
   etc), as well as gratuitiously global symbol names, so we can have
   multiple yacc generated parsers in gdb.  Note that these are only
   the variables produced by yacc.  If other parser generators (bison,
   byacc, etc) produce additional global names that conflict at link
   time, then those parser generators need to be fixed instead of
   adding those names to this list.  */

/* NOTE: This is clumsy since BISON and FLEX provide --prefix options.
   We are maintaining it to accommodate systems without BISON.  */

/* Define GDB_YY_REMAP_PREFIX to the desired remapping prefix before
   including this file.  */
#ifndef GDB_YY_REMAP_PREFIX
# error "GDB_YY_REMAP_PREFIX not defined"
#endif

#define GDB_YY_REMAP_2(PREFIX, YYSYM) PREFIX ## YYSYM
#define GDB_YY_REMAP_1(PREFIX, YYSYM) GDB_YY_REMAP_2 (PREFIX, YYSYM)
#define GDB_YY_REMAP(YYSYM) GDB_YY_REMAP_1 (GDB_YY_REMAP_PREFIX, YYSYM)

#define yymaxdepth	GDB_YY_REMAP (yymaxdepth)
#define yyparse		GDB_YY_REMAP (yyparse)
#define yylex		GDB_YY_REMAP (yylex)
#define yyerror		GDB_YY_REMAP (yyerror)
#define yylval		GDB_YY_REMAP (yylval)
#define yychar		GDB_YY_REMAP (yychar)
#define yydebug		GDB_YY_REMAP (yydebug)
#define yypact		GDB_YY_REMAP (yypact)
#define yyr1		GDB_YY_REMAP (yyr1)
#define yyr2		GDB_YY_REMAP (yyr2)
#define yydef		GDB_YY_REMAP (yydef)
#define yychk		GDB_YY_REMAP (yychk)
#define yypgo		GDB_YY_REMAP (yypgo)
#define yyact		GDB_YY_REMAP (yyact)
#define yyexca		GDB_YY_REMAP (yyexca)
#define yyerrflag	GDB_YY_REMAP (yyerrflag)
#define yynerrs		GDB_YY_REMAP (yynerrs)
#define yyps		GDB_YY_REMAP (yyps)
#define yypv		GDB_YY_REMAP (yypv)
#define yys		GDB_YY_REMAP (yys)
#define yy_yys		GDB_YY_REMAP (yy_yys)
#define yystate		GDB_YY_REMAP (yystate)
#define yytmp		GDB_YY_REMAP (yytmp)
#define yyv		GDB_YY_REMAP (yyv)
#define yy_yyv		GDB_YY_REMAP (yy_yyv)
#define yyval		GDB_YY_REMAP (yyval)
#define yylloc		GDB_YY_REMAP (yylloc)
#define yyreds		GDB_YY_REMAP (yyreds)  /* With YYDEBUG defined */
#define yytoks		GDB_YY_REMAP (yytoks)  /* With YYDEBUG defined */
#define yyname		GDB_YY_REMAP (yyname)  /* With YYDEBUG defined */
#define yyrule		GDB_YY_REMAP (yyrule)  /* With YYDEBUG defined */
#define yylhs		GDB_YY_REMAP (yylhs)
#define yylen		GDB_YY_REMAP (yylen)
#define yydefred	GDB_YY_REMAP (yydefred)
#define yydgoto		GDB_YY_REMAP (yydgoto)
#define yysindex	GDB_YY_REMAP (yysindex)
#define yyrindex	GDB_YY_REMAP (yyrindex)
#define yygindex	GDB_YY_REMAP (yygindex)
#define yytable		GDB_YY_REMAP (yytable)
#define yycheck		GDB_YY_REMAP (yycheck)
#define yyss		GDB_YY_REMAP (yyss)
#define yysslim		GDB_YY_REMAP (yysslim)
#define yyssp		GDB_YY_REMAP (yyssp)
#define yystacksize	GDB_YY_REMAP (yystacksize)
#define yyvs		GDB_YY_REMAP (yyvs)
#define yyvsp		GDB_YY_REMAP (yyvsp)
#define YYSTACKDATA	GDB_YY_REMAP (YYSTACKDATA)

/* The following are common to all parsers.  */

#ifndef YYDEBUG
# define YYDEBUG 1  /* Default to yydebug support */
#endif

#ifndef TEST_CPNAMES
# define YYFPRINTF parser_fprintf
#endif

#endif /* YY_REMAP_H */
