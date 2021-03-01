# bbconsole
Linux console for BlueBasic (BASIC interpreter for CC2540 and CC2541 Bluetooth LE chips).
See [BlueBasic](https://github.com/aanon4/BlueBasic) for more information. Also have a look
at [BlueBasic-loader](https://github.com/0xFACE/BlueBasic-loader).

## Usage:
```bash
./bbconsole btaddress
```
`btaddress` is the MAC address of the device (e.g. `B4:99:4C:21:5A:97`).

## Needs [BlueZ](https://git.kernel.org/pub/scm/bluetooth/bluez.git/) source:

### Prerequisite (Debian / Raspbian)
```bash
sudo apt install automake libtool libdbus-1-dev udev libudev-dev libical-dev libreadline-dev glib2.0
```

### Build

#### BlueZ
```bash
git clone git://git.kernel.org/pub/scm/bluetooth/bluez.git
cd bluez
./bootstrap
./configure
make
```

#### bbconsole
```bash
make
```


### Note
**All sources need to be at the same level:**
```bash
$ ls -la
drwxr-xr-x  3 pi   pi     4096 Mar  1 18:38 bbconsole
drwxr-xr-x 25 pi   pi     4096 Mar  1 18:47 bluez
drwxr-xr-x 25 pi   pi     4096 Mar  1 18:49 BlueBasic-loader
...
```

# Disclaimer
This is my dirty hack program. I'm not C hacker at all :wink:
Please feel free to send me PR's to make this program better.
