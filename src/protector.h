#ifndef PROTECTOR_H_
#define PROTECTOR_H_

#ifdef _WIN32
#define _WIN32_WINNT 0x0501
#define VC_EXTRALEAN
#include "Windows.h"
#else
#include <sys/ptrace.h>
#endif

bool isDebuggerAttached()
{
    #ifdef _WIN32
    return IsDebuggerPresent();
    #else
    return ptrace(PTRACE_TRACEME, 0, NULL, 0) == -1;
    #endif
}

#endif
