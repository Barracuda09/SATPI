# SATPI
SATIP server for example the Raspberry Pi or BeagleBone. Currently only supports DVB-S and DVB-S2

Initial working vesion, but a work in progress.

- SATPI provides an web interface for monitoring various things (http port 8875)

Tested
------
- Tvheadend: this is a TV streaming server see: https://tvheadend.org/
- DVBviewer Lite Edition. see http://www.satip.info/products

Build
-----
To build SATPI just run these commands::

    git clone git://github.com/Barracuda09/satpi.git
    cd satpi/
    make

Usage
-----

for help on options:  satpi --help

For normal use run:  satpi --yes-ssdp --yes-rtcp
