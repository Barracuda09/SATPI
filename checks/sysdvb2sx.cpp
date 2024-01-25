#include <stdio.h>
#include <linux/dvb/frontend.h>
#include <linux/dvb/version.h>
#include <linux/dvb/dmx.h>
int main(void) {
	fe_delivery_system delSys = SYS_DVBS2X;
  return delSys != 21;
}
