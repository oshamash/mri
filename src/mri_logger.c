/* TODO(omer): Fix include paths */
#include "../include/mri_logging.h"

#include <time.h>
#include <stdio.h>

const char *sev2string(mri_severity_t sev) {
	switch (sev) {
	case MRI_DEBUG		: return "[DEBUG ]";
	case MRI_NOTICE		: return "[NOTICE]";
	case MRI_INFO		: return "[ INFO ]";
	case MRI_WARNING	: return "[ WARN ]";
	case MRI_ERROR		: return "[ERROR ]";
	case MRI_CRITICAL	: return "[ CRIT ]";
	case MRI_ALERT		: return "[ALERT ]";
	case MRI_EMERG		: return "[EMERG ]";
	default				: return "[BUGGED]";
	}
}

/* Weak-ref logger functions */

void __attribute__((weak))
mri_log(mri_severity_t level, const char *fmt, ...) {
	va_list va;
	va_start(va, fmt);
	mri_vlog(level, fmt, va);
	va_end(va);
}

void __attribute__((weak))
mri_vlog(mri_severity_t level, const char *fmt, va_list args) {
	time_t time_now;
	time(&time_now);
	printf("%s %s ", ctime(&time_now), sev2string(level));
	vprintf(fmt, args);
	printf("\n");
}

