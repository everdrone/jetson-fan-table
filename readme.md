# Installation

## Requirements

- Autotools
- Autoconf Archive (`apt install autoconf-archive`)

## Build

```sh
./autogen.sh && cd build

../configure

# installation requires privileges
sudo make install
```

# Usage

Once installed just start the service

```sh
sudo systemctl start fantable

# And to have it run at startup
sudo systemctl enable fantable
```

The configuration files are located in `/etc/fantable`

The table **must** be formatted correctly for the program to work.
Each line should be formatted as follows:

```
<temperature> <speed>
```

The default configuration is:

```
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
