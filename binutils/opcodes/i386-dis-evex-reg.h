  /* REG_EVEX_0F71 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "%XEvpsrlw",	{ Vex, EXx, Ib }, PREFIX_DATA },
    { Bad_Opcode },
    { "%XEvpsraw",	{ Vex, EXx, Ib }, PREFIX_DATA },
    { Bad_Opcode },
    { "%XEvpsllw",	{ Vex, EXx, Ib }, PREFIX_DATA },
  },
  /* REG_EVEX_0F72 */
  {
    { "vpror%DQ",	{ Vex, EXx, Ib }, PREFIX_DATA },
    { "vprol%DQ",	{ Vex, EXx, Ib }, PREFIX_DATA },
    { VEX_W_TABLE (EVEX_W_0F72_R_2) },
    { Bad_Opcode },
    { "%XEvpsra%DQ",	{ Vex, EXx, Ib }, PREFIX_DATA },
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F72_R_6) },
  },
  /* REG_EVEX_0F73 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F73_R_2) },
    { "%XEvpsrldqY",	{ Vex, EXx, Ib }, PREFIX_DATA },
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F73_R_6) },
    { "%XEvpslldqY",	{ Vex, EXx, Ib }, PREFIX_DATA },
  },
  /* REG_EVEX_0F38C6_L_2 */
  {
    { Bad_Opcode },
    { "vgatherpf0dp%XW",  { MVexVSIBDWpX }, PREFIX_DATA },
    { "vgatherpf1dp%XW",  { MVexVSIBDWpX }, PREFIX_DATA },
    { Bad_Opcode },
    { Bad_Opcode },
    { "vscatterpf0dp%XW",  { MVexVSIBDWpX }, PREFIX_DATA },
    { "vscatterpf1dp%XW",  { MVexVSIBDWpX }, PREFIX_DATA },
  },
  /* REG_EVEX_0F38C7_L_2 */
  {
    { Bad_Opcode },
    { "vgatherpf0qp%XW",  { MVexVSIBQWpX }, PREFIX_DATA },
    { "vgatherpf1qp%XW",  { MVexVSIBQWpX }, PREFIX_DATA },
    { Bad_Opcode },
    { Bad_Opcode },
    { "vscatterpf0qp%XW",  { MVexVSIBQWpX }, PREFIX_DATA },
    { "vscatterpf1qp%XW",  { MVexVSIBQWpX }, PREFIX_DATA },
  },
  /* REG_EVEX_MAP4_80 */
  {
    { "addA",	{ VexGb, Eb, Ib }, NO_PREFIX },
    { "orA",	{ VexGb, Eb, Ib }, NO_PREFIX },
    { "adcA",	{ VexGb, Eb, Ib }, NO_PREFIX },
    { "sbbA",	{ VexGb, Eb, Ib }, NO_PREFIX },
    { "andA",	{ VexGb, Eb, Ib }, NO_PREFIX },
    { "subA",	{ VexGb, Eb, Ib }, NO_PREFIX },
    { "xorA",	{ VexGb, Eb, Ib }, NO_PREFIX },
  },
  /* REG_EVEX_MAP4_81 */
  {
    { "addQ",	{ VexGv, Ev, Iv }, PREFIX_NP_OR_DATA },
    { "orQ",	{ VexGv, Ev, Iv }, PREFIX_NP_OR_DATA },
    { "adcQ",	{ VexGv, Ev, Iv }, PREFIX_NP_OR_DATA },
    { "sbbQ",	{ VexGv, Ev, Iv }, PREFIX_NP_OR_DATA },
    { "andQ",	{ VexGv, Ev, Iv }, PREFIX_NP_OR_DATA },
    { "subQ",	{ VexGv, Ev, Iv }, PREFIX_NP_OR_DATA },
    { "xorQ",	{ VexGv, Ev, Iv }, PREFIX_NP_OR_DATA },
  },
  /* REG_EVEX_MAP4_83 */
  {
    { "addQ",	{ VexGv, Ev, sIb }, PREFIX_NP_OR_DATA },
    { "orQ",	{ VexGv, Ev, sIb }, PREFIX_NP_OR_DATA },
    { "adcQ",	{ VexGv, Ev, sIb }, PREFIX_NP_OR_DATA },
    { "sbbQ",	{ VexGv, Ev, sIb }, PREFIX_NP_OR_DATA },
    { "andQ",	{ VexGv, Ev, sIb }, PREFIX_NP_OR_DATA },
    { "subQ",	{ VexGv, Ev, sIb }, PREFIX_NP_OR_DATA },
    { "xorQ",	{ VexGv, Ev, sIb }, PREFIX_NP_OR_DATA },
  },
  /* REG_EVEX_MAP4_8F */
  {
    { VEX_W_TABLE (EVEX_W_MAP4_8F_R_0) },
  },
  /* REG_EVEX_MAP4_F6 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "notA",	{ VexGb, Eb }, NO_PREFIX },
    { "negA",	{ VexGb, Eb }, NO_PREFIX },
  },
  /* REG_EVEX_MAP4_F7 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "notQ",	{ VexGv, Ev }, PREFIX_NP_OR_DATA },
    { "negQ",	{ VexGv, Ev }, PREFIX_NP_OR_DATA },
  },
  /* REG_EVEX_MAP4_FE */
  {
    { "incA",	{ VexGb, Eb }, NO_PREFIX },
    { "decA",	{ VexGb, Eb }, NO_PREFIX },
  },
  /* REG_EVEX_MAP4_FF */
  {
    { "incQ",	{ VexGv, Ev }, PREFIX_NP_OR_DATA },
    { "decQ",	{ VexGv, Ev }, PREFIX_NP_OR_DATA },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_MAP4_FF_R_6) },
  },
