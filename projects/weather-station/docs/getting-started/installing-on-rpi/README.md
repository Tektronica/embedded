# Install on Raspberry Pi

Use this guide for a first install on a Pi prepared as described in [Set up Raspberry Pi](../setting-up-rpi/README.md). For a Pi that already has data or services, use [Upgrade an existing installation](../upgrading-existing-installation/README.md).

## 1. Authenticate with GitHub

The repository is private. The Pi reads it over SSH with a read-only deploy key scoped to this repository. Don't add the Pi's key to a GitHub account, which grants write access to every repository the account can reach.

On the Pi, generate a key without a passphrase so `deploy.sh` runs without a prompt:

```bash
ssh-keygen -t ed25519 -f ~/.ssh/id_ed25519 -N ""
cat ~/.ssh/id_ed25519.pub
```

On GitHub, open the repository's Settings > Deploy keys > Add deploy key, paste the public key, and leave "Allow write access" off. Then check the connection from the Pi:

```bash
ssh -T git@github.com
```

GitHub replies with a greeting naming the repository. A deploy key works for one repository only.

## 2. Clone the project

Clone only the weather station folder. The rest of the repository isn't needed on the Pi.

```bash
git clone --filter=blob:none --sparse git@github.com:Tektronica/embedded.git ~/embedded
git -C ~/embedded sparse-checkout set projects/weather-station
```

## 3. Configure

```bash
cd ~/embedded/projects/weather-station/python
cp -n .env.example .env
cp -n config.example.json config.json
```

Edit `.env` to add the SMTP credentials. Edit `config.json` to set `email.recipients` and review `sensors.devices` against the connected hardware. `./init.sh` creates both files when they are missing, so this step also happens on the first deploy. See [Configure sensors](../../hardware-configuration/sensor-configuration/README.md).

## 4. Deploy

From `projects/weather-station/python`, as the user the services run as:

```bash
./deploy.sh
```

`deploy.sh` runs these steps in order:

1. Pulls the current branch with `git pull --ff-only`.
2. Stops any running weather station units.
3. Rebuilds `.venv-sensors`, `.venv-web`, and `.venv-notifications` with `./init.sh services`.
4. Copies the units from `deploy/` into `/etc/systemd/system/`, with `User` set to the current user and paths set to this checkout.
5. Installs a 2 minute timeout for `systemd-time-wait-sync.service` and enables it, so the sensors and boot units wait for the clock to sync before they start.
6. Enables and starts the sensors service, then the web service, the digest timer, and the alarm timer. Enables the boot email unit without starting it.

The units in `deploy/` use `pi` and `/home/pi/embedded` as placeholders. `deploy.sh` replaces both, so any username and checkout location work.

The services start at boot without a login session.

## Update

Run `./deploy.sh` again. The pull stops the deploy before any service is touched if a local edit to a tracked file conflicts with an incoming change. `.env` and `config.json` are ignored by Git and never conflict. Commit or stash the local edit, then rerun.

## Check

```bash
systemctl status weather-sensors.service weather-web.service weather-digest.timer weather-alarm.timer
journalctl -u weather-sensors.service -f
```

See [Troubleshooting](../../operations/troubleshooting/README.md) when a service doesn't start.
