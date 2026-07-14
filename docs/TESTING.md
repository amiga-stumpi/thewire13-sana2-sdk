# Compatibility Testing

1. Build with `make clean && make`; fix new warnings.
2. Copy the device and `test_sana_open` to the Amiga.
3. Run `stack 16000` and `test_sana_open Devs:networks/my.device 0`.
4. Verify query values and MAC address.
5. Start TheWire13 with DHCP disabled; verify ARP, IPv4, UDP and DNS.
6. Test DHCP, TCP HTTP GET, sustained traffic and repeated start/stop cycles.
7. Test aborting pending reads, offline/online, invalid units and missing callbacks.
8. Validate first on AmigaOS 1.3, then repeat on OS3.x.

A successful open alone does not prove RX lifecycle safety. Use serial logging and packet capture and verify every queued request is replied exactly once.
