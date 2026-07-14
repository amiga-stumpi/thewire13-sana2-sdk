#include <exec/types.h>
#include <exec/libraries.h>
#include <exec/devices.h>
#include <exec/errors.h>
#include <exec/ports.h>
#include <devices/sana2.h>
#include <proto/exec.h>

#include "thewire13_sana2/hardware.h"

#define DEVICE_NAME "sana2skeleton.device"
#define DEVICE_MTU 1500UL
#define DEVICE_BPS 10000000UL
#ifndef S2ERR_NO_ERROR
#define S2ERR_NO_ERROR 0
#define S2ERR_BAD_ARGUMENT 3
#define S2ERR_OUTOFSERVICE 5
#endif
#ifndef S2WERR_GENERIC_ERROR
#define S2WERR_GENERIC_ERROR 0
#define S2WERR_UNIT_OFFLINE 2
#define S2WERR_BUFF_ERROR 4
#endif

typedef APTR BufferFunc;
struct BufferManagement {
    BufferFunc copy_to;
    BufferFunc copy_from;
};

extern BOOL sdk_call_buffer(BufferFunc func, APTR dst, APTR src, LONG size);

const char sdk_device_name[] = DEVICE_NAME;
const char sdk_device_id[] = DEVICE_NAME " 1.0 (14.7.2026) OS1.3 SANA-II SDK";
struct ExecBase *SysBase;

static BPTR g_seglist;
static struct List g_reads;
static struct BufferManagement g_buffers;
static UBYTE g_mac[6] = { 0x02, 0x54, 0x57, 0x13, 0x00, 0x01 };
static UBYTE g_tx[DEVICE_MTU];
static int g_open;
static int g_online;

static void copy_bytes(APTR dst, const APTR src, ULONG length)
{
    UBYTE *d = (UBYTE *)dst;
    const UBYTE *s = (const UBYTE *)src;
    while (length--)
        *d++ = *s++;
}

static void init_list(struct List *list)
{
    list->lh_Head = (struct Node *)&list->lh_Tail;
    list->lh_Tail = 0;
    list->lh_TailPred = (struct Node *)&list->lh_Head;
}

static ULONG find_tag(ULONG wanted, struct TagItem *tags)
{
    while (tags) {
        if (tags->ti_Tag == TAG_DONE)
            break;
        if (tags->ti_Tag == TAG_IGNORE) {
            ++tags;
            continue;
        }
        if (tags->ti_Tag == TAG_SKIP) {
            tags += tags->ti_Data + 1;
            continue;
        }
        if (tags->ti_Tag == TAG_MORE) {
            tags = (struct TagItem *)tags->ti_Data;
            continue;
        }
        if (tags->ti_Tag == wanted)
            return tags->ti_Data;
        ++tags;
    }
    return 0;
}

static int request_is_queued(struct IOSana2Req *req)
{
    struct Node *node;
    for (node = g_reads.lh_Head; node && node->ln_Succ; node = node->ln_Succ) {
        if (node == &req->ios2_Req.io_Message.mn_Node)
            return 1;
    }
    return 0;
}

static void complete(struct IOSana2Req *req)
{
    if (req->ios2_Req.io_Flags & IOF_QUICK)
        req->ios2_Req.io_Message.mn_Node.ln_Type = NT_REPLYMSG;
    else
        ReplyMsg(&req->ios2_Req.io_Message);
}

static void abort_reads(void)
{
    struct Node *node;
    struct IOSana2Req *req;

    Forbid();
    while ((node = RemHead(&g_reads)) != 0) {
        req = (struct IOSana2Req *)node;
        req->ios2_Req.io_Error = IOERR_ABORTED;
        req->ios2_WireError = S2WERR_GENERIC_ERROR;
        ReplyMsg(&req->ios2_Req.io_Message);
    }
    Permit();
}

int sdk_sana2_receive(const UBYTE *src, const UBYTE *dst, UWORD packet_type,
    const UBYTE *payload, ULONG length)
{
    struct Node *node;
    struct IOSana2Req *req = 0;

    if (!g_online || !payload || length > DEVICE_MTU)
        return 0;

    Forbid();
    for (node = g_reads.lh_Head; node && node->ln_Succ; node = node->ln_Succ) {
        struct IOSana2Req *candidate = (struct IOSana2Req *)node;
        if ((UWORD)candidate->ios2_PacketType == packet_type) {
            Remove(node);
            req = candidate;
            break;
        }
    }
    Permit();

    if (!req)
        return 0;

    if (!sdk_call_buffer(g_buffers.copy_to, req->ios2_Data,
        (APTR)payload, (LONG)length)) {
        req->ios2_Req.io_Error = S2ERR_BAD_ARGUMENT;
        req->ios2_WireError = S2WERR_BUFF_ERROR;
    } else {
        req->ios2_DataLength = length;
        req->ios2_Req.io_Error = S2ERR_NO_ERROR;
        req->ios2_WireError = 0;
        copy_bytes(req->ios2_SrcAddr, (APTR)src, 6);
        copy_bytes(req->ios2_DstAddr, (APTR)dst, 6);
    }
    ReplyMsg(&req->ios2_Req.io_Message);
    return 1;
}

static void query(struct IOSana2Req *req)
{
    struct Sana2DeviceQuery *q = (struct Sana2DeviceQuery *)req->ios2_StatData;
    if (!q) {
        req->ios2_Req.io_Error = S2ERR_BAD_ARGUMENT;
        return;
    }
    q->DevQueryFormat = 0;
    q->DeviceLevel = 0;
    if (q->SizeAvailable >= 18) q->AddrFieldSize = 48;
    if (q->SizeAvailable >= 22) q->MTU = DEVICE_MTU;
    if (q->SizeAvailable >= 26) q->BPS = DEVICE_BPS;
    if (q->SizeAvailable >= 30) q->HardwareType = S2WireType_Ethernet;
    q->SizeSupplied = q->SizeAvailable < 30 ? q->SizeAvailable : 30;
}

static struct Library *device_init(register APTR base __asm("d0"),
    register BPTR seglist __asm("a0"),
    register struct ExecBase *sysbase __asm("a6"))
{
    struct Library *lib = (struct Library *)base;
    SysBase = sysbase;
    g_seglist = seglist;
    init_list(&g_reads);
    lib->lib_Node.ln_Type = NT_DEVICE;
    lib->lib_Node.ln_Name = (char *)sdk_device_name;
    lib->lib_Flags = LIBF_SUMUSED | LIBF_CHANGED;
    lib->lib_Version = 1;
    lib->lib_Revision = 0;
    lib->lib_IdString = (APTR)sdk_device_id;
    return lib;
}

static ULONG device_expunge(register struct Library *lib __asm("a6"))
{
    if (lib->lib_OpenCnt) {
        lib->lib_Flags |= LIBF_DELEXP;
        return 0;
    }
    Remove((struct Node *)lib);
    return (ULONG)g_seglist;
}

static void device_open(register struct Library *lib __asm("a6"),
    register struct IOSana2Req *req __asm("a1"),
    register ULONG unit __asm("d0"), register ULONG flags __asm("d1"))
{
    struct TagItem *tags;
    (void)flags;
    req->ios2_Req.io_Error = IOERR_OPENFAIL;
    req->ios2_WireError = S2WERR_GENERIC_ERROR;
    if (unit != 0 || g_open)
        return;
    tags = (struct TagItem *)req->ios2_BufferManagement;
    g_buffers.copy_to = (BufferFunc)find_tag(S2_CopyToBuff, tags);
    g_buffers.copy_from = (BufferFunc)find_tag(S2_CopyFromBuff, tags);
    if (!g_buffers.copy_to || !g_buffers.copy_from) {
        req->ios2_Req.io_Error = S2ERR_BAD_ARGUMENT;
        return;
    }
    req->ios2_BufferManagement = &g_buffers;
    req->ios2_Req.io_Device = (struct Device *)lib;
    req->ios2_Req.io_Unit = 0;
    req->ios2_Req.io_Error = 0;
    req->ios2_WireError = 0;
    ++lib->lib_OpenCnt;
    g_open = 1;
}

static ULONG device_close(register struct Library *lib __asm("a6"),
    register struct IOSana2Req *req __asm("a1"))
{
    g_online = 0;
    sdk_hw_shutdown();
    abort_reads();
    g_open = 0;
    if (lib->lib_OpenCnt) --lib->lib_OpenCnt;
    req->ios2_Req.io_Device = 0;
    req->ios2_Req.io_Unit = 0;
    if (!lib->lib_OpenCnt && (lib->lib_Flags & LIBF_DELEXP))
        return device_expunge(lib);
    return 0;
}

static void device_begin_io(register struct Library *lib __asm("a6"),
    register struct IOSana2Req *req __asm("a1"))
{
    (void)lib;
    req->ios2_Req.io_Error = 0;
    req->ios2_WireError = 0;

    switch (req->ios2_Req.io_Command) {
    case CMD_READ:
        if (!g_online || req->ios2_BufferManagement != &g_buffers) {
            req->ios2_Req.io_Error = S2ERR_OUTOFSERVICE;
            req->ios2_WireError = S2WERR_UNIT_OFFLINE;
            break;
        }
        req->ios2_Req.io_Flags &= (UBYTE)~IOF_QUICK;
        Forbid();
        AddTail(&g_reads, &req->ios2_Req.io_Message.mn_Node);
        Permit();
        return;

    case CMD_WRITE:
    case S2_BROADCAST:
        if (!g_online || req->ios2_DataLength > DEVICE_MTU ||
            req->ios2_BufferManagement != &g_buffers) {
            req->ios2_Req.io_Error = S2ERR_BAD_ARGUMENT;
            break;
        }
        if (!sdk_call_buffer(g_buffers.copy_from, g_tx, req->ios2_Data,
            (LONG)req->ios2_DataLength) ||
            !sdk_hw_send(req->ios2_DstAddr, g_mac,
                (UWORD)req->ios2_PacketType, g_tx, req->ios2_DataLength))
            req->ios2_Req.io_Error = IOERR_ABORTED;
        break;

    case S2_DEVICEQUERY:
        query(req);
        break;
    case S2_GETSTATIONADDRESS:
        copy_bytes(req->ios2_SrcAddr, g_mac, 6);
        copy_bytes(req->ios2_DstAddr, g_mac, 6);
        break;
    case S2_CONFIGINTERFACE:
        copy_bytes(g_mac, req->ios2_SrcAddr, 6);
        if (!sdk_hw_init(g_mac)) {
            req->ios2_Req.io_Error = S2ERR_OUTOFSERVICE;
            req->ios2_WireError = S2WERR_UNIT_OFFLINE;
        } else g_online = 1;
        break;
    case S2_ONLINE:
        if (sdk_hw_init(g_mac)) g_online = 1;
        else req->ios2_Req.io_Error = S2ERR_OUTOFSERVICE;
        break;
    case S2_OFFLINE:
        g_online = 0;
        sdk_hw_shutdown();
        abort_reads();
        break;
    default:
        req->ios2_Req.io_Error = IOERR_NOCMD;
        break;
    }
    complete(req);
}

static ULONG device_abort_io(register struct Library *lib __asm("a6"),
    register struct IOSana2Req *req __asm("a1"))
{
    int queued;
    (void)lib;
    Forbid();
    queued = request_is_queued(req);
    if (queued) Remove(&req->ios2_Req.io_Message.mn_Node);
    Permit();
    if (queued) {
        req->ios2_Req.io_Error = IOERR_ABORTED;
        req->ios2_WireError = S2WERR_GENERIC_ERROR;
        ReplyMsg(&req->ios2_Req.io_Message);
    }
    return 0;
}

static ULONG device_vectors[] = {
    (ULONG)device_open, (ULONG)device_close, (ULONG)device_expunge, 0,
    (ULONG)device_begin_io, (ULONG)device_abort_io, (ULONG)-1
};

ULONG sdk_auto_init_tables[] = {
    sizeof(struct Library), (ULONG)device_vectors, 0, (ULONG)device_init
};
