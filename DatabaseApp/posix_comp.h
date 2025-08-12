#ifndef POSIX_COMP_H
#define POSIX_COMP_H

#include <stddef.h>

//Welcome to hell.
//So, why is this file here, what does it do?
//Welp, ssize_t isn't a standard type for C on windows, so we have to define it ourselves
//Oh why oh why POSIX?
#ifdef _WIN32
	#include <basetsd.h>
	typedef SSIZE_T ssize_t;
#else
	//Do nothing because we have ssize_t already
#endif
#endif