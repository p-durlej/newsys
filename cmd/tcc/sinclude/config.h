
#include <arch/archdef.h>

#ifdef __ARCH_AMD64__
#define TCC_TARGET_X86_64
#elif defined __ARCH_I386__
#define TCC_TARGET_I386
#endif

#define TCC_VERSION			"0.9.27"
#define TCC_VER1			0
#define TCC_VER2			9
#define TCC_VER3			27
#define CONFIG_TCC_STATIC
#define CONFIG_TCC_SYSINCLUDEPATHS	"/usr/include"
#define CONFIG_TCC_LIBPATHS		"/usr/lib"
#define CONFIG_TCCDIR			"/usr/lib"
#define CONFIG_TCC_ELFINTERP		"/lib/load"
#undef	TCC_IS_NATIVE
