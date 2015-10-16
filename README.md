# SatPI

SATIP server for linux, suitable for running on an Raspberry Pi, BeagleBone or any other linux box.
Currently supporting DVB-S, DVB-S2, DVB-T and DVB-C.

- SatPI provides an web interface for monitoring and configuring various things (http port 8875)
	- http://ip.of.your.box:8875

- The Description xml can be found like:
	- http://ip.of.your.box:8875/desc.xml

- The SatPI wiki can be found here:
	- https://github.com/Barracuda09/SATPI/wiki

Help
-------
Help in any way is appreciated, just send me an email with anythink you can
contribute to the project, like:
- coding
- web design
- ideas / feature requests
- test reports
- spread the word!

Contact
-------
If you like to contact me, you can do so by sending an email to:

    mpostema09 -at- gmail.com

Tested Programs
---------------
- Tvheadend: this is a TV streaming server see: https://tvheadend.org/
- DVBviewer Lite Edition. see http://www.satip.info/products
- Elgato Sat>IP App for Android
- VDR

Tested Hardware
---------------
- Raspberry Pi see: http://www.raspberrypi.org/
- BeagleBone Black see: http://www.beagleboard.org/
- Sundtek DVB-C/T/T2 see: http://www.sundtek.com/
- Anysee-S2(LP) STV090x Multistandard
- HMP-Combo DVB-T2/C
- Schwaiger DSR41IP (Thanks to Axel Hartmann for kindly suppling this to me)
  see: http://www.satip.info/sites/satip/files/files/DSR41IP_GB.pdf

Build
-----
Build Status: <a href="https://travis-ci.org/Barracuda09/SATPI"><img src="https://travis-ci.org/Barracuda09/SATPI.svg"/></a>

Coverity Scan Build Status <a href="https://scan.coverity.com/projects/4842">
  <img alt="Coverity Scan Build Status"
       src="https://scan.coverity.com/projects/4842/badge.svg"/>
</a>

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

If you need to clean the project (because there was something wrong), use:

    make clean

If you like to try DVBAPI (but be aware it still buggy), use:

    make debug LIBDVBCSA=yes

If you like to build the documentation, use:

    make docu   (!! you need Doxygen and Graphviz/dot !!)

If you like to build the UML documentation, use:

    make plantuml   (!! you need PlantUML !!)

Usage
-----
For help on options:

    ./satpi --help

For normal use just run:

    ./satpi   (!!Note you should have the appropriate privilege to open tcp/udp port 554!!)
