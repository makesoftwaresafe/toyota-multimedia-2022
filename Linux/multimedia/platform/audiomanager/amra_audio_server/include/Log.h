#ifndef __LOG_H__
#define __LOG_H__
#include <dlt/dlt.h>

#define LOG_FN_ENTRY(ctx, ...) do \
{ \
	DLT_LOG((ctx), DLT_LOG_VERBOSE, DLT_STRING(__FILE__), DLT_STRING(" : "), DLT_INT(__LINE__), DLT_STRING(" Entering "), DLT_STRING(__func__), ##__VA_ARGS__); \
} while(0)

#define LOG_FN_EXIT(ctx, ...) do { \
	DLT_LOG((ctx), DLT_LOG_VERBOSE, DLT_STRING(__FILE__), DLT_STRING(" : "), DLT_INT(__LINE__), DLT_STRING(" Exiting "), DLT_STRING(__func__), ##__VA_ARGS__); \
} while(0)

#define LOG_FATAL(ctx, ...) do { \
	DLT_LOG((ctx), DLT_LOG_FATAL, DLT_STRING(__FILE__), DLT_STRING(" : "), DLT_INT(__LINE__), DLT_STRING(" [ "), DLT_STRING(__func__), DLT_STRING(" ] "), ##__VA_ARGS__); \
} while(0)

#define LOG_ERROR(ctx, ...) do { \
	DLT_LOG((ctx), DLT_LOG_ERROR, DLT_STRING(__FILE__), DLT_STRING(" : "), DLT_INT(__LINE__), DLT_STRING(" [ "), DLT_STRING(__func__), DLT_STRING(" ] "), ##__VA_ARGS__); \
} while(0)

#define LOG_WARN(ctx, ...) do { \
	DLT_LOG((ctx), DLT_LOG_WARN, DLT_STRING(__FILE__), DLT_STRING(" : "), DLT_INT(__LINE__), DLT_STRING(" [ "), DLT_STRING(__func__), DLT_STRING(" ] "), ##__VA_ARGS__); \
} while(0)

#define LOG_INFO(ctx, ...) do { \
	DLT_LOG((ctx), DLT_LOG_INFO, DLT_STRING(__FILE__), DLT_STRING(" : "), DLT_INT(__LINE__), DLT_STRING(" [ "), DLT_STRING(__func__), DLT_STRING(" ] "), ##__VA_ARGS__); \
} while(0)

#define LOG_DEBUG(ctx, ...) do { \
	DLT_LOG((ctx), DLT_LOG_DEBUG, DLT_STRING(__FILE__), DLT_STRING(" : "), DLT_INT(__LINE__), DLT_STRING(" [ "), DLT_STRING(__func__), DLT_STRING(" ] "), ##__VA_ARGS__); \
} while(0)

#define LOG_VERBOSE(ctx, ...) do { \
	DLT_LOG((ctx), DLT_LOG_VERBOSE, DLT_STRING(__FILE__), DLT_STRING(" : "), DLT_INT(__LINE__), DLT_STRING(" [ "), DLT_STRING(__func__), DLT_STRING(" ] "), ##__VA_ARGS__); \
} while(0)

#endif
