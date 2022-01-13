/*
 * ntp_iocplmem.h - separate memory pool for IOCPL related objects
 *
 * Written by Juergen Perlinger (perlinger@ntp.org) for the NTP project.
 * The contents of 'html/copyright.html' apply.
 *
 * --------------------------------------------------------------------
 * Notes on the implementation:
 *
 * Implements a thin layer over Windows Memory pools
 */
#ifndef NTP_IOCPLMEM_H
#define NTP_IOCPLMEM_H

#include <stdlib.h>

extern void IOCPLPoolInit(size_t initSize);
extern void IOCPLPoolDone(void);

extern void* __fastcall	IOCPLPoolAlloc(size_t size, const char*	desc);
extern void* __fastcall	IOCPLPoolMemDup(const void* psrc, size_t size, const char* desc);
extern void  __fastcall	IOCPLPoolFree(void* ptr, const char* desc);

#endif /*!defined(NTP_IOCPLMEM_H)*/

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/tags/7.0.0/GA/lib/io-pkt/dist/ntp/ports/winnt/include/ntp_iocplmem.h $ $Rev: 802718 $")
#endif
