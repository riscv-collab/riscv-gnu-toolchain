#include <stdio.h>
#include <stdint.h>

static void print_ruyisdk_logo(void) {
	//  _____             _  _____ _____  _  __
	// |  __ \           (_)/ ____|  __ \| |/ /
	// | |__) |   _ _   _ _| (___ | |  | | ' / 
	// |  _  / | | | | | | |\___ \| |  | |  <  
	// | | \ \ |_| | |_| | |____) | |__| | . \ 
	// |_|  \_\__,_|\__, |_|_____/|_____/|_|\_\
	//               __/ |                     
	//              |___/                      

	printf("-----------------------------------------\n");
	printf("  _____             _  _____ _____  _  __\n");
	printf(" |  __ \\           (_)/ ____|  __ \\| |/ /\n");
	printf(" | |__) |   _ _   _ _| (___ | |  | | ' / \n");
	printf(" |  _  / | | | | | | |\\___ \\| |  | |  <  \n");
	printf(" | | \\ \\ |_| | |_| | |____) | |__| | . \\ \n");
	printf(" |_|  \\_\\__,_|\\__, |_|_____/|_____/|_|\\_\\\n");
	printf("               __/ |                     \n");
	printf("              |___/                      \n");
	printf("                    -- Presented by ISCAS\n");
	printf("-----------------------------------------\n");
}

static void print_rv64ilp32(void) {
    sizeof(long);
    printf("-----------------------------------------\n");
    printf("       Hello 64ilp32 ABI world           \n");
    printf("                                         \n");
    printf("       sizeof(long): %lu                   \n", sizeof(long));
    printf("       sizeof(int): %lu                    \n", sizeof(int));
    printf("       sizeof(void*): %lu                  \n", sizeof(void*));
    printf("       sizeof(long long): %lu              \n", sizeof(long long));
    printf("       sizeof(long double): %lu            \n", sizeof(long double));
    printf("       sizeof(size_t): %lu                 \n", sizeof(size_t));
    printf("       sizeof(intptr_t): %lu               \n", sizeof(intptr_t));
    printf("       sizeof(uintptr_t): %lu              \n", sizeof(uintptr_t));
    printf("-----------------------------------------\n");
}

int main(void) {
    print_ruyisdk_logo();
    print_rv64ilp32();
    return 0;
}
