/* Basic argc/argv tests.
# mach: bfin
# cc: -msim
# progopts: a bb ccc dddd
*/

int streq(const char *s1, const char *s2)
{
	int i = 0;

	while (s1[i] && s2[i] && s1[i] == s2[i])
		++i;

	return s1[i] == '\0' && s2[i] == '\0';
}

int main(int argc, char *argv[])
{
	if (argc != 5)
		return 1;
	if (!streq(argv[1], "a"))
		return 2;
	if (!streq(argv[2], "bb"))
		return 2;
	if (!streq(argv[3], "ccc"))
		return 2;
	if (!streq(argv[4], "dddd"))
		return 2;
	puts("pass");
	return 0;
}
