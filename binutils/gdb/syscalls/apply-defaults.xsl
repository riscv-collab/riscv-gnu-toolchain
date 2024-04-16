<!-- Generate syscall XML files based on defaults template.
     Copyright (C) 2016-2024 Free Software Foundation, Inc.

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
     along with this program.  If not, see <http://www.gnu.org/licenses/>. -->

<xsl:stylesheet version="1.0"
		xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
  <xsl:output method="xml" doctype-system="gdb-syscalls.dtd"/>

  <xsl:template match="node()|@*" name="identity">
    <xsl:copy>
      <xsl:apply-templates select="node()|@*"/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="/syscalls_info/syscall">
    <xsl:copy>
      <xsl:apply-templates select="@*|node()"/>
      <xsl:variable name="syscall"><xsl:value-of select="@name"/></xsl:variable>
      <xsl:variable name="tgroups"><xsl:value-of select="@groups"/></xsl:variable>
      <xsl:for-each select="document('linux-defaults.xml.in')/syscalls_defaults/child::*[@name=$syscall]">
	<xsl:attribute name="groups">
	  <xsl:value-of select="@groups"/>
	  <xsl:if test="$tgroups != '' ">,<xsl:value-of select="$tgroups"/></xsl:if>
	</xsl:attribute>
      </xsl:for-each>
    </xsl:copy>
  </xsl:template>

</xsl:stylesheet>
