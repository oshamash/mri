#ifndef _MRI_GENERAL_H_
#define _MRI_GENERAL_H_

/* TODO(omer): Fix includes */
#include "mri_logging.h"

#ifndef likely
	#define likely(x)	__builtin_expect((x),1)
#endif
#ifndef unlikely
	#define unlikely(x)	__builtin_expect((x),0)
#endif

#include <unistd.h>
#include <sys/syscall.h>
extern char *__progname;
#define MRI_LOG(level, fmt, ...) \
	mri_log(level, "[%s:%d:%s] [%s:%d] " fmt, __FILE__, __LINE__, __PRETTY_FUNCTION__, __progname, (int) syscall(SYS_gettid),  ##__VA_ARGS__)
#endif
