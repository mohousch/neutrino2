#ifndef __DH_RSA_MISC_H_
#define __DH_RSA_MISC_H_

int dh_gen_exp(uint8_t *dest, int dest_len, uint8_t *dh_g, int dh_g_len, uint8_t *dh_p, int dh_p_len);
int dh_mod_exp(uint8_t *dest, int dest_len, uint8_t *base, int base_len, uint8_t *mod, int mod_len, uint8_t *exp, int exp_len);
int dh_dhph_signature(uint8_t *out, uint8_t *nonce, uint8_t *dhph, RSA *r);

#endif
