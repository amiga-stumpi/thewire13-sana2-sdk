# TheWire13 SANA-II Driver SDK

A compact AmigaOS 1.3 development kit for writing classic SANA-II network devices compatible with TheWire13 and other SANA-II consumers.

## Included

- OS1.3-safe SANA-II public header
- loadable `sana2skeleton.device` source with resident/autoinit glue
- classic `S2_CopyToBuff` / `S2_CopyFromBuff` callback adapter
- asynchronous `CMD_READ` queue and synchronous transmit path
- device query, station address, configure, online and offline handling
- isolated hardware backend API and inert example backend
- `test_sana_open` compatibility utility
- driver implementation and test checklist

## Requirements and build

Install bebbo amiga-gcc below `/opt/amiga`, then run:

```sh
make clean && make
```

Outputs are written to `build/`. The skeleton is intentionally not a working NIC driver. Copy `src/hardware_stub.c`, implement the hardware operations, and arrange for received Ethernet payloads to call `sdk_sana2_receive()` from safe deferred processing, never directly from a hard interrupt.

## TheWire13 configuration

Install the finished device in `Devs:networks/` and use either:

```ini
device=myethernet.device
unit=0
```

or an explicit AmigaDOS path. Bare names are resolved below `Devs:networks/`.

See [docs/DRIVER_GUIDE.md](docs/DRIVER_GUIDE.md) and [docs/TESTING.md](docs/TESTING.md).
