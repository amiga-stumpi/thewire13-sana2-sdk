# Driver Guide

## Contract

TheWire13 opens a conventional SANA-II device with an `IOSana2Req`. Before `OpenDevice()`, `ios2_BufferManagement` points to a terminated TagItem list containing `S2_CopyToBuff` and `S2_CopyFromBuff`. On success the device may replace it with its private opener cookie. Callback registers are A0=destination, A1=source and D0=length.

Implement the standard Exec device vectors: Open, Close, Expunge, BeginIO and AbortIO. Unit 0 is sufficient for a single-interface driver. Reject invalid units and unsupported open modes cleanly.

## Required commands

The practical minimum is `S2_DEVICEQUERY`, `S2_GETSTATIONADDRESS`, `S2_CONFIGINTERFACE`, `S2_ONLINE`, `S2_OFFLINE`, `CMD_READ`, `CMD_WRITE`, and `S2_BROADCAST`. Report Ethernet, 48 address bits and the real MTU/BPS.

`CMD_READ` must be asynchronous: clear `IOF_QUICK`, queue the request and reply only after a matching packet arrives or the request is aborted. Match `ios2_PacketType`, copy the payload through the client's callback, fill source/destination addresses and `ios2_DataLength`, then `ReplyMsg()` exactly once.

For writes, copy client data through `S2_CopyFromBuff`, transmit it, and complete the request with meaningful `io_Error` and `ios2_WireError` values. Broadcasting uses ff:ff:ff:ff:ff:ff.

## Hardware integration

Replace `src/hardware_stub.c` with backend code implementing `sdk_hw_init`, `sdk_hw_shutdown`, and `sdk_hw_send`. Deliver received Ethernet payloads through `sdk_sana2_receive(src,dst,type,payload,length)`. Keep hard interrupt handlers minimal; acknowledge hardware and defer queue/callback/ReplyMsg work to a task or soft interrupt context appropriate for the hardware.

## OS1.3 rules

Use Exec 1.3 APIs only. Do not depend on timer.device VBlank extensions, utility.library tag helpers, DOS 2.x APIs, threads, POSIX calls, or large automatic buffers. Pair every request, port, signal and interrupt allocation on all error and close paths. Protect queues with short Forbid/Permit sections, not long hardware operations.

The provided skeleton supports one opener for clarity. Production drivers may support multiple openers, but buffer-management cookies and pending reads must then be maintained per opener.
