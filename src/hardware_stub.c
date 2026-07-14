#include <exec/types.h>
#include "thewire13_sana2/hardware.h"

int sdk_hw_init(const UBYTE *mac)
{
    (void)mac;
    return 1;
}

void sdk_hw_shutdown(void)
{
}

int sdk_hw_send(const UBYTE *dst, const UBYTE *src, UWORD packet_type,
    const UBYTE *payload, ULONG length)
{
    (void)dst;
    (void)src;
    (void)packet_type;
    (void)payload;
    (void)length;
    return 0; /* Replace with real hardware transmission. */
}
