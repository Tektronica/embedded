# Develop on Raspberry Pi

Use VS Code on another machine with the [Remote - SSH](https://code.visualstudio.com/docs/remote/ssh) extension to edit files, run a terminal, and read logs on the Pi. VS Code installs a small server on the Pi the first time it connects. The Pi needs no desktop, so Raspberry Pi OS Lite works.

## Requirements

- Raspberry Pi OS Buster (10) or later, 32-bit ARMv7l or 64-bit AArch64. ARMv6 boards (Pi 1, Pi Zero, Pi Zero W) aren't supported.
- 1 GB RAM at minimum. VS Code recommends 2 GB and two cores, since the server shares memory with the weather station services.
- SSH enabled on the Pi with public-key authentication, as set in Raspberry Pi Imager.
- VS Code with the Remote - SSH extension, and an OpenSSH client, on the local machine.

## Connect

1. Check that SSH works from a local terminal:

   ```bash
   ssh <user>@<hostname>.local
   ```

2. In VS Code, open the Command Palette and run **Remote-SSH: Connect to Host**. Enter the same `<user>@<hostname>.local`. If asked to select an SSH configuration file, choose `~/.ssh/config`, not the system-wide `/etc/ssh/ssh_config`. Choose Linux if VS Code asks for the platform.
3. Open the folder `~/embedded/projects/weather-station/python`.
4. Run **Python: Select Interpreter** and choose the environment for the package being edited, such as `.venv-sensors/bin/python`.

To list the Pi by name in VS Code, add a host entry to `~/.ssh/config` on the local machine:

```
Host weather-station
  HostName <hostname>.local
  User <user>
  IdentityFile ~/.ssh/<key>
  IdentitiesOnly yes
  AddressFamily inet
```

Then connect to `weather-station`.

## Troubleshooting

### Connection closed before authentication

`<hostname>.local` can resolve to an IPv6 link-local address that drops the connection. Force IPv4 in the host entry:

```
  AddressFamily inet
```

### Permission denied (publickey)

SSH offers only the default key files unless the host entry names one. Add the key to the host entry:

```
  IdentityFile ~/.ssh/<key>
  IdentitiesOnly yes
```

Confirm key login:

```bash
ssh <user>@<hostname>.local 'hostname; uname -m'
```

Imager's public-key-only option turns off password logins, so `ssh-copy-id` can't add a missing key. If the Pi still rejects the key, check that the key named in `IdentityFile` is the one pasted into Imager. If the Pi never received it, add the key to `~/.ssh/authorized_keys` from a local console or reflash with Imager.

## Limits

- VS Code treats ARM hosts as preview. Extensions that ship x86 native code may not run on the Pi.
- The Pi's deploy key is read-only, so changes made on the Pi can't be pushed. Make code changes on the local machine and deploy them with `./deploy.sh`. Use the remote session for logs, `config.json` and `.env` edits, and debugging.
- A local edit to a tracked file stops the next `./deploy.sh` at the pull. See [Install on Raspberry Pi](../installing-on-rpi/README.md#update).
