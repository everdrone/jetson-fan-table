<h1 align="center">Fan Control Daemon</h1>

<p align="center">Configurable, fast and lightweight</p>

## Requirements

- A 5V PWM fan
- Default L4T Linux image for the Jetson Nano
- Autotools
- Autoconf Archive (`apt install autoconf-archive`)

## Build & Install

Clone the submodules after cloning

```sh
git submodule update --init --recursive
```

Build with autotools

```sh
# set up autotools
./autogen.sh && cd build && ../configure

# installation requires privileges
sudo make install
```

## Usage

Once installed just start the service

```sh
sudo systemctl start fantable

# And to have it run at startup
sudo systemctl enable fantable
```

To check the status of the daemon use the `--check` and `--status` options

```sh
$ sudo fantable -s
process pid: 3157
temperature: 37 C
current pwm: 86
current rpm: 1370
```

## Configuration

The configuration files are located in `/etc/fantable`

The `config` file contains daemon options that can be overridden by command line arguments.

```ini
# /etc/fantable/config
interval = 3
average = yes
```

can be overridden by calling fantable with new arguments

```
fantable -i 2 -A
```

The `table` file **must** be formatted correctly for the program to work.
Each line should be formatted as follows:

```
<temperature> <speed>
```

The default configuration is:

```ini
# /etc/fantable/table
27 0
38 20
45 50
50 80
59 90
60 100
```

After a configuration change, the service must to be restarted to see the changes.

```sh
sudo systemctl restart fantable
```

## Credits

Similar projects:

- [kooscode/fan-daemon](https://github.com/kooscode/fan-daemon)
- [Pyrestone/jetson-fan-ctl](https://github.com/Pyrestone/jetson-fan-ctl)
