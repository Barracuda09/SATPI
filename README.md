# SatPI
SatPI server for example the Raspberry Pi, BeagleBone or any other linux box.
Currently supporting DVB-S, DVB-S2 and DVB-T

Initial working vesion, but a work in progress.

- SatPI provides an web interface for monitoring various things (http port 8875)
	- http://ip.of.your.box:8875

- The Description xml can be found like:
	- http://ip.of.your.box:8875/desc.xml

Contact
-------
If you feel the need to contact me, you can do so by sending an email to:

    m.a.postema -at- alice.nl

Tested Programs
---------------
- Tvheadend: this is a TV streaming server see: https://tvheadend.org/
- DVBviewer Lite Edition. see http://www.satip.info/products
- Elgato Sat>IP App for Android

Tested Hardware
---------------
- Raspberry Pi see: http://www.raspberrypi.org/
- BeagleBone Black see: http://www.beagleboard.org/
- Sundtek DVB-C/T/T2 see: http://www.sundtek.com/
- Anysee-S2(LP) STV090x Multistandard

Build
-----
To build SatPI just run these commands:

    git clone git://github.com/Barracuda09/satpi.git
    cd satpi/
    make

See some new commits/changes you need, rebuild with:

    cd satpi
    git pull
    make

If you need to make a debug version to help with testing, use:

    make debug


Usage
-----
For help on options:

    ./satpi --help

For normal use just run:

    ./satpi   (!!Note you should have the appropriate privilege to open tcp/udp port 554!!)
