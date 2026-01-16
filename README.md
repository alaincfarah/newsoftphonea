# newsoftphonea

Windows softphone sample built on a C backend (PJSIP) for FreePBX.

Features
- SIP registration and calling
- Call hold, blind transfer, warm transfer
- Call recording (WAV)
- Open a URL on incoming call using caller CNAME
- Codec priority for G.729, u-law (PCMU), a-law (PCMA)

Prerequisites
- Windows 10/11 x64
- Visual Studio 2022 (MSVC)
- CMake 3.20+
- PJSIP built for Windows with the codecs you need

Notes on G.729
- PJSIP does not ship G.729 due to licensing. You must build and license a
  G.729 plugin (for example, a commercial G.729 implementation) and enable it
  in your PJSIP build. The app will still run if G.729 is missing, but it will
  log that the codec is unavailable.

Build
1. Build PJSIP and set `PJSIP_DIR` to the install root that contains `include`
   and `lib`.
2. Run `scripts\build_windows.bat`.

Run
- `build\Release\softphone.exe config\softphone.ini`

Configuration
Edit `config/softphone.ini`:
- `sip_domain`, `sip_user`, `sip_password`: FreePBX credentials
- `sip_proxy`: SIP proxy or registrar
- `transport`: `udp`, `tcp`, or `tls`
- `local_bind`: bind to a specific local IP (useful for OpenVPN)
- `url_template`: URL to open on inbound calls
  - `{CNAME}` is the remote display name (fallback to caller)
  - `{CALLER}` is the user part of the SIP URI
- `codec_order`: priority order, for example `G729,PCMU,PCMA`

OpenVPN usage
1. Connect to your OpenVPN profile.
2. Set `local_bind` to the VPN IP assigned to the tunnel.
3. Ensure your FreePBX host is reachable from the VPN and set `sip_domain` and
   `sip_proxy` accordingly.

CLI commands
- `help`
- `list`
- `call <sip-uri|extension>`
- `answer [call-id]`
- `hangup [call-id]`
- `hold <call-id>`
- `unhold <call-id>`
- `blindxfer <call-id> <sip-uri>`
- `warmxfer <call-id> <sip-uri>`
- `warmcomplete <call-id> <consult-call-id>`
- `record start <call-id>`
- `record stop <call-id>`
- `quit`