#ifndef DEVICES_SANA2_H
#define DEVICES_SANA2_H 1

#include <exec/types.h>
#include <exec/ports.h>
#include <exec/io.h>
#include <exec/errors.h>

#ifndef TAG_USER
#define TAG_USER (0x80000000UL)
#endif

#ifndef TAG_END
#define TAG_END 0UL
#define TAG_DONE TAG_END
#define TAG_IGNORE 1UL
#define TAG_MORE 2UL
#define TAG_SKIP 3UL
#endif

#define SANA2_MAX_ADDR_BITS   128
#define SANA2_MAX_ADDR_BYTES  ((SANA2_MAX_ADDR_BITS + 7) / 8)

struct TagItem
{
    ULONG ti_Tag;
    ULONG ti_Data;
};

struct IOSana2Req
{
    struct IORequest ios2_Req;
    ULONG ios2_WireError;
    ULONG ios2_PacketType;
    UBYTE ios2_SrcAddr[SANA2_MAX_ADDR_BYTES];
    UBYTE ios2_DstAddr[SANA2_MAX_ADDR_BYTES];
    ULONG ios2_DataLength;
    APTR  ios2_Data;
    APTR  ios2_StatData;
    APTR  ios2_BufferManagement;
};

struct Sana2DeviceQuery
{
    ULONG SizeAvailable;
    ULONG SizeSupplied;
    ULONG DevQueryFormat;
    ULONG DeviceLevel;
    UWORD AddrFieldSize;
    ULONG MTU;
    ULONG BPS;
    ULONG HardwareType;
};

#define SANA2IOB_QUICK  IOB_QUICK
#define SANA2IOF_QUICK  IOF_QUICK

#define SANA2OPB_MINE   0
#define SANA2OPF_MINE   (1UL << SANA2OPB_MINE)

#define S2_Dummy        (TAG_USER + 0xB0000)
#define S2_CopyToBuff     (S2_Dummy + 1)
#define S2_CopyFromBuff   (S2_Dummy + 2)
#define S2_PacketFilter   (S2_Dummy + 3)
#define S2_CopyToBuff16   (S2_Dummy + 4)
#define S2_CopyFromBuff16 (S2_Dummy + 5)

#define S2_START              CMD_NONSTD
#define S2_DEVICEQUERY        (S2_START + 0)
#define S2_GETSTATIONADDRESS  (S2_START + 1)
#define S2_CONFIGINTERFACE    (S2_START + 2)
#define S2_BROADCAST          (S2_START + 8)
#define S2_ONLINE             (S2_START + 16)
#define S2_OFFLINE            (S2_START + 17)
#define S2ERR_NO_ERROR 0
#define S2ERR_BAD_ARGUMENT 3
#define S2ERR_OUTOFSERVICE 10
#define S2WERR_GENERIC_ERROR 0
#define S2WERR_UNIT_OFFLINE 3
#define S2WERR_BUFF_ERROR 6

#define S2WireType_Ethernet   1

#endif
