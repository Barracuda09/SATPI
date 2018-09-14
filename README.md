| Build Status | Coverity Scan | PayPal |
|-------|-------|-------|
| [![Build Status](https://travis-ci.org/Barracuda09/SATPI.svg)](https://travis-ci.org/Barracuda09/SATPI) | [![Coverity Scan](https://scan.coverity.com/projects/4842/badge.svg)](https://scan.coverity.com/projects/4842) | [![PayPal](https://img.shields.io/badge/donate-PayPal-blue.svg)](https://www.paypal.com/cgi-bin/webscr?cmd=_donations&business=H9AX9N7HWSWXE&item_name=SatPI&item_number=SatPI&currency_code=EUR&bn=PP%2dDonationsBF%3abtn_donateCC_LG%2egif%3aNonHosted) |

# SatPI

An SAT>IP server for linux, suitable for running on an Raspberry Pi, VU+, BeagleBone or any other linux box.

<a href="https://github.com/Barracuda09/SATPI/wiki/02.-Build-SatPI">See wiki on how to build SatPI</a>

Currently supporting:
- DVB-S(2), DVB-T(2) and DVB-C
- Transform for example DVB-S(2) requests to DVB-C
- RTP/AVP over UDP and TCP
- Decrypting of channels via DVB-API protocol implemented by OSCam, therefore you need the dvbcsa library and an official subscription
- Web Interface for monitoring and configuring various things (http port 8875)
	- http://ip.of.your.box:8875
-------
- The Description xml can be found like:
	- http://ip.of.your.box:8875/desc.xml

- The SatPI wiki can be found here:
	- https://github.com/Barracuda09/SATPI/wiki

Help
-------
Help in any way is appreciated, just send me an email with anything you can
contribute to the project, like:
- coding
- web design
- ideas / feature requests
- test reports
- spread the word!

Donate
------

If you like and use SatPI then please consider making a donation, to support my effort in
developing SatPI.<br>
<br>
Thank You.

<a href="https://www.paypal.com/cgi-bin/webscr?cmd=_donations&business=H9AX9N7HWSWXE&item_name=SatPI&item_number=SatPI&currency_code=EUR&bn=PP%2dDonationsBF%3abtn_donateCC_LG%2egif%3aNonHosted"><img src="https://www.paypalobjects.com/en_US/NL/i/btn/btn_donateCC_LG.gif"/></a>

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
- <a href="http://www.raspberrypi.org">Raspberry Pi</a>
- <a href="http://www.beagleboard.org">BeagleBone Black</a>
- <a href="http://www.orangepi.org">Orange Pi One with armbian</a>
- <a href="http://www.sundtek.co">Sundtek DVB-C/T/T2</a>
- <a href="http://www.satip.info/sites/satip/files/files/DSR41IP_GB.pdf">Schwaiger DSR41IP</a> - Thanks to Axel Hartmann for kindly suppling this to me
- <a href="https://www.digitalbox.de/de_DE/HD-6i/490-1460-10080/">DIGITALBOX IMPERIAL HD 6i</a> - Thanks to <a href="http://www.apfutura.net/en">APfutura</a> for kindly suppling this to me
- Anysee-S2(LP) STV090x Multistandard
- HMP-Combo DVB-T2/C
- <a href="http://www.vuplus.de/produkt_uno4k.php">VU+ Uno 4K with DVB-C FBC Tuner</a>

Build
-----
Build Status: <a href="https://travis-ci.org/Barracuda09/SATPI"><img src="https://travis-ci.org/Barracuda09/SATPI.svg"/></a>

Coverity Scan Build Status <a href="https://scan.coverity.com/projects/4842">
  <img alt="Coverity Scan Build Status"
       src="https://scan.coverity.com/projects/4842/badge.svg"/></a>

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

If you like to try DVBAPI (OSCam), use:

    make debug LIBDVBCSA=yes

If you like to run it on an Enigma2 box, use:

    make debug ENIGMA=yes

For Cross Compiling, here are some tips you can try (I did not try this myself):

    export INCLUDES=--sysroot=dir         (get the sys root like headers and libraries for your device and copy it to dir)
    export CXXPREFIX=arm-linux-gnueabihf- (for pointing to a different compiler for your device)
    export CXXSUFFIX=                     (for pointing to a different compiler for your device, if it needs it!)
    make debug                            (Or some other build you like)

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
