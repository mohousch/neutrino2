#ifndef __DESCR_H_
#define __DESCR_H_

int descrambler_init(void);
void descrambler_deinit(void);
bool descrambler_open(void);
void descrambler_close(void);
int descrambler_set_key(int index, int parity, unsigned char *data);
/* we don't use this for sh4 ci cam ! */
int descrambler_set_pid(int index, int enable, int pid);

#endif
