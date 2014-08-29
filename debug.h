#ifdef LOG
    // wrapping fprintf(..) allows us to put a ; after LOG, so it looks like a function
    #define LOG(..) do { printf(__VA_ARGS__); } while (0)
#else
    #define LOG(..)
#endif