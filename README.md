# Wireless Plant Monitoring

### Installation and Building for Re-motes
1. Download and extract the [ARM Toolchain](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/downloads) to `~/.arm`.

2. Clone the Contiki OS to `~/.contiki` and install submodules\
```
$ git clone https://gitlab.lrz.de/wsnlab/contiki-remote.git ~/.contiki
$ cd ~/.contiki
$ git submodule update --init
```
3. Install Python Serial\
```
$ pip install pyserial --user
```

4. Building the source for Zolertia Re-mote and upload it\
```
$ cd src && ./build
```
