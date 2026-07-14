#ifndef THEWIRE13_SANA2_HARDWARE_H
#define THEWIRE13_SANA2_HARDWARE_H

#include <exec/types.h>

int sdk_hw_init(const UBYTE *mac);
void sdk_hw_shutdown(void);
int sdk_hw_send(const UBYTE *dst, const UBYTE *src, UWORD packet_type,
    const UBYTE *payload, ULONG length);

/* Call this from the driver RX task or deferred interrupt processing. */
int sdk_sana2_receive(const UBYTE *src, const UBYTE *dst, UWORD packet_type,
    const UBYTE *payload, ULONG length);

#endif
