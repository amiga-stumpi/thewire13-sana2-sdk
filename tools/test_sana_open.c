#include <exec/types.h>
#include <exec/ports.h>
#include <devices/sana2.h>
#include <proto/exec.h>
#include <clib/alib_protos.h>
#include <stdio.h>
#include <stdlib.h>

static BOOL copy_buffer(register APTR dst __asm("a0"), register APTR src __asm("a1"), register LONG len __asm("d0"))
{
    UBYTE *d = dst, *s = src;
    while (len-- > 0) *d++ = *s++;
    return TRUE;
}

int main(int argc, char **argv)
{
    static struct Sana2DeviceQuery q;
    static struct TagItem tags[3];
    struct MsgPort *port;
    struct IOSana2Req *req;
    LONG rc;
    ULONG unit = argc > 2 ? (ULONG)atoi(argv[2]) : 0;
    if (argc < 2) { printf("Usage: test_sana_open <device> [unit]\n"); return 5; }
    port = CreatePort(0, 0);
    if (!port) return 20;
    req = (struct IOSana2Req *)CreateExtIO(port, sizeof(*req));
    if (!req) { DeletePort(port); return 20; }
    tags[0].ti_Tag = S2_CopyToBuff; tags[0].ti_Data = (ULONG)copy_buffer;
    tags[1].ti_Tag = S2_CopyFromBuff; tags[1].ti_Data = (ULONG)copy_buffer;
    tags[2].ti_Tag = TAG_END;
    req->ios2_BufferManagement = tags;
    rc = OpenDevice((STRPTR)argv[1], unit, (struct IORequest *)req, 0);
    printf("OpenDevice rc=%ld io_Error=%d wire=%lu\n", rc, req->ios2_Req.io_Error, req->ios2_WireError);
    if (!rc) {
        q.SizeAvailable = sizeof(q); req->ios2_StatData = &q;
        req->ios2_Req.io_Command = S2_DEVICEQUERY; DoIO((struct IORequest *)req);
        printf("Query error=%d address=%u bits MTU=%lu BPS=%lu type=%lu\n", req->ios2_Req.io_Error, q.AddrFieldSize, q.MTU, q.BPS, q.HardwareType);
        req->ios2_Req.io_Command = S2_GETSTATIONADDRESS; DoIO((struct IORequest *)req);
        printf("MAC %02lx:%02lx:%02lx:%02lx:%02lx:%02lx\n", (ULONG)req->ios2_SrcAddr[0], (ULONG)req->ios2_SrcAddr[1], (ULONG)req->ios2_SrcAddr[2], (ULONG)req->ios2_SrcAddr[3], (ULONG)req->ios2_SrcAddr[4], (ULONG)req->ios2_SrcAddr[5]);
        CloseDevice((struct IORequest *)req);
    }
    DeleteExtIO((struct IORequest *)req); DeletePort(port);
    return rc ? 10 : 0;
}
