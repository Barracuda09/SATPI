| Build Status | Coverity Scan | PayPal |
|-------|-------|-------|
| [![Build Status](https://travis-ci.org/Barracuda09/SATPI.svg)](https://travis-ci.org/Barracuda09/SATPI) | [![Coverity Scan](https://scan.coverity.com/projects/4842/badge.svg)](https://scan.coverity.com/projects/4842) | [![PayPal](https://img.shields.io/badge/donate-PayPal-blue.svg)](https://www.paypal.com/cgi-bin/webscr?cmd=_donations&business=H9AX9N7HWSWXE&item_name=SatPI&item_number=SatPI&currency_code=EUR&bn=PP%2dDonationsBF%3abtn_donateCC_LG%2egif%3aNonHosted) |

# SatPI

An SAT>IP server for linux, suitable for running on an Raspberry Pi, VU+, BeagleBone or any other linux box.

<a href="https://github.com/Barracuda09/SATPI/wiki/02.-Build-SatPI">See wiki on how to build SatPI</a>

Currently supporting:

- DVB-S(2), DVB-T(2) and DVB-C
- Web Interface for monitoring and configuring various things (http port 8875)
	- http://ip.of.your.box:8875
- Transform for example DVB-S(2) requests to DVB-C
- RTP/AVP over UDP and TCP
- Decrypting of channels via DVB-API protocol implemented by OSCam, therefore you need the dvbcsa library and an official subscription
- FILE input, reading from an TS File
- STREAMER input, reading from an multicast/unicast input
- CHILDPIPE input, reading from an PIPE input for example wget
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
- donate

Donate
------

If you like and use SatPI then please consider making a donation, to support my effort in
developing SatPI.<br>
Many thanks to all who donated already.<br>
<br>
Please find the Sponsor button here:
<a href="https://github.com/Barracuda09/SATPI"><img src="https://www.paypalobjects.com/en_US/NL/i/btn/btn_donateCC_LG.gif"/></a>

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
- <a href="https://shop.sundtek.de">Sundtek DVB-C/T/T2</a>
- <a href="https://shop.sundtek.de">Sundtek SkyTV Ultimate Dual (2x DVB-S/S2/S2X)</a>
- <a href="http://www.satip.info/sites/satip/files/files/DSR41IP_GB.pdf">Schwaiger DSR41IP</a> - Thanks to Axel Hartmann for kindly suppling this to me
- DIGITALBOX IMPERIAL HD 6i - Thanks to <a href="https://www.apfutura.com">APfutura</a> for kindly suppling this to me
- Anysee-S2(LP) STV090x Multistandard
- HMP-Combo DVB-T2/C
- <a href="http://www.vuplus.de">VU+ Zero 4K with DVB-S2X Tuner</a>
- <a href="http://www.vuplus.de">VU+ Uno 4K with DVB-C FBC Tuner</a>
- <a href="https://gigablue.de/produkte">GigaBlue UHD Quad 4K with DVB-S2X FBC Tuner</a> - Thanks to <a href="https://store.impex-sat.de">Impex-Sat GmbH & Co. KG</a> for kindly suppling this to me
- <a href="https://gigablue.de/produkte">GigaBlue Ultra SCR-LNB / 24 SCR - 2 Legacy UHD 4K LNB</a> - Thanks to <a href="https://store.impex-sat.de">Impex-Sat GmbH & Co. KG</a> for kindly suppling this to me
- <a href="https://www.durasat.de/LNB/DUR-line/DUR-line-UK-124-Unicable-LNB.html">DUR-line UK 124 - Unicable LNB</a>

Build
-----
- Always Update the Web folder as well, as it may contain new features

- To build SatPI just run these commands:

    `git clone git://github.com/Barracuda09/satpi.git`<br/>
    `cd satpi/`<br/>
    `git branch -f devtmp 9c4b71d` -> _will make a branch devtmp of commit 9c4b71d_<br/>
    `git checkout devtmp` -> _this will checkout devtmp_<br/>
    <br/>
    `git branch -a` -> see all available branches<br/>
    `git branch` -> see on which branch you are working/building<br/>
    `git checkout V1.6.2` -> to checkout (switch to) branch 'V1.6.2'<br/>
    `make`<br/>

- See some new commits/changes you need, rebuild with:

    `cd satpi`<br/>
    `git pull`<br/>
    `make`<br/>

- If you need to make a debug version to help with testing, use:

    `make debug`<br/>

- If you need to clean the project (because there was something wrong), use:

    `make clean`<br/>

- If you like to try OSCam with DVBAPI, use:

    `make debug LIBDVBCSA=yes`<br/>

- If you like to run it on an Enigma2 box **_(With the correct toolchain)_**, use:

    `make debug ENIGMA=yes`<br/>

- Here is an toolchain I use for Vu+ Receivers (Broadcom CPU) it has MIPS and ARM cross-compiler:

    `https://github.com/Broadcom/stbgcc-6.3/releases`<br/>

- For Cross Compiling, here are some tips you can try _**(I did not try this myself)**_:

    - get the sys root like headers and libraries for your device and copy it to dir<br/>
    `export INCLUDES=--sysroot=dir`<br/>
    - for pointing to a different compiler for your device<br/>
    `export CXXPREFIX=arm-linux-gnueabihf-`<br/>
    `export CXXSUFFIX=`<br/>
    - To buils just run (Or some other build you like)<br/>
    `make debug`<br/>

- If you see building errors, then perhaps your toolchain is not C++17 compatible. In this case try this before compiling:

    `make non-c++17`<br/>

- If you like to build the documentation, use:

    `make docu   (!! you need Doxygen and Graphviz/dot !!)`<br/>

Usage
-----
For help on options:

    ./satpi --help

For normal use just run:

    ./satpi   (!!Note you should have the appropriate privilege to open tcp/udp port 554!!)
