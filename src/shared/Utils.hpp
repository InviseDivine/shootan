#ifdef _WIN32
    #include <external/fix_win32_compatibility.h>
    #define sleepMs(ms) Sleep(ms)
#else
    #include <unistd.h>
    #define sleepMs(ms) usleep((ms) * 1000)
#endif

#define PACK_INDEX(x, y, width) (y * width + x)
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))