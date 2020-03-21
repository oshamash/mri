#ifndef _MRI_LOGGING_H
#define _MRI_LOGGING_H

#include <stdarg.h>

/* Severity similar to syslog */
typedef enum mri_severity {
	MRI_DEBUG = 7,
	MRI_NOTICE = 6,
	MRI_INFO = 5,
	MRI_WARNING = 4,
	#define MRI_WARN MRI_WARNING
	MRI_ERROR = 3,
	#define MRI_ERR MRI_ERROR
	MRI_CRITICAL = 2,
	#define MRI_CRIT MRI_CRITICAL
	MRI_ALERT = 1,
	MRI_EMERG = 0
	#define MRI_PANIC MRI_EMERG
} mri_severity_t;

/* Note: it is sufficient to override just one func */
/* Weak-ref logging functions defaulted to printf() */
extern void mri_log(mri_severity_t, const char *, ...);
extern void mri_vlog(mri_severity_t, const char *, va_list);

#endif 
