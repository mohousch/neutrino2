/* DVB CI CC Manager */
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>

#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/sha.h>
#include <openssl/aes.h>
#include <openssl/opensslv.h>

#include "misc.h"
#include "_dh_params.h"
#include "dh_rsa_misc.h"
#include "descrambler.h"
#include "aes_xcbc_mac.h"

#include "dvbci_ccmgr.h"

/* for some debug > set to 1 */
#define x_debug 0
#define y_debug 0

static const char *FILENAME = "[dvbci_ccmgr]";

/* storage & load of authenticated data (HostID & DHSK & AKH) */

static void CheckFile(char *file)
{
	if (access(file, F_OK) != 0)
	{
		printf("No File: %s\n", file);
		FILE *fd;
		fd = fopen(file, "w");
		fclose(fd);
	}
}

static void get_authdata_filename(char *dest, size_t len, unsigned int slot)
{
	snprintf(dest, len, "/etc/ci_auth_slot_%u.bin", slot);
	CheckFile(dest);
}

static bool get_authdata(uint8_t *host_id, uint8_t *dhsk, uint8_t *akh, unsigned int slot, unsigned int index)
{
	char filename[FILENAME_MAX];
	int fd;
	uint8_t chunk[8 + 256 + 32];
	unsigned int i;

	printf("%s -> %s\n", FILENAME, __FUNCTION__);

	/* 5 pairs of data only */
	if (index > 5)
		return false;

	get_authdata_filename(filename, sizeof(filename), slot);

	fd = open(filename, O_RDONLY);
	if (fd <= 0)
	{
		fprintf(stderr, "cannot open %s\n", filename);
		return false;
	}

	for (i = 0; i < 5; i++)
	{
		if (read(fd, chunk, sizeof(chunk)) != sizeof(chunk))
		{
			fprintf(stderr, "cannot read auth_data\n");
			close(fd);
			return false;
		}

		if (i == index)
		{
			memcpy(host_id, chunk, 8);
			memcpy(dhsk, &chunk[8], 256);
			memcpy(akh, &chunk[8 + 256], 32);
			close(fd);
			return true;
		}
	}

	close(fd);
	return false;
}

static bool write_authdata(unsigned int slot, const uint8_t *host_id, const uint8_t *dhsk, const uint8_t *akh)
{
	printf("%s -> %s\n", FILENAME, __FUNCTION__);

	char filename[FILENAME_MAX];
	int fd;
	uint8_t buf[(8 + 256 + 32) * 5];
	unsigned int entries;
	unsigned int i;
	bool ret = false;

	for (entries = 0; entries < 5; entries++)
	{
		int offset = (8 + 256 + 32) * entries;
		if (!get_authdata(&buf[offset], &buf[offset + 8], &buf[offset + 8 + 256], slot, entries))
			break;

		/* check if we got this pair already */
		if (!memcmp(&buf[offset + 8 + 256], akh, 32))
		{
			printf("data already stored\n");
			return true;
		}
	}

	//printf("got %d entries\n", entries);

	get_authdata_filename(filename, sizeof(filename), slot);

	fd = open(filename, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
	if (fd <= 0)
	{
		printf("cannot open %s for writing - authdata not stored\n", filename);
		return false;
	}

	/* store new entry first */
	if (write(fd, host_id, 8) != 8)
	{
		fprintf(stderr, "error in write\n");
		goto end;
	}

	if (write(fd, dhsk, 256) != 256)
	{
		fprintf(stderr, "error in write\n");
		goto end;
	}

	if (write(fd, akh, 32) != 32)
	{
		fprintf(stderr, "error in write\n");
		goto end;
	}

	/* skip the last one if exists */
	if (entries > 3)
		entries = 3;

	for (i = 0; i < entries; i++)
	{
		int offset = (8 + 256 + 32) * i;
		if (write(fd, &buf[offset], (8 + 256 + 32)) != (8 + 256 + 32))
		{
			fprintf(stderr, "error in write\n");
			goto end;
		}
	}

	ret = true;
end:
	close(fd);

	return ret;
}

/* CI+ certificates */

struct cert_ctx
{
	X509_STORE *store;

	/* Host */
	X509 *cust_cert;
	X509 *device_cert;

	/* Module */
	X509 *ci_cust_cert;
	X509 *ci_device_cert;
};

static int verify_cb(int /*ok*/, X509_STORE_CTX *ctx)
{
	if (X509_STORE_CTX_get_error(ctx) == X509_V_ERR_CERT_NOT_YET_VALID)
	{
		time_t now = time(NULL);
		struct tm *t = localtime(&now);
		if (t->tm_year < 2015)
			//printf("seems our rtc is wrong - ignore!\n");
			return 1;
		//printf("wrong date!\n");
	}

	if (X509_STORE_CTX_get_error(ctx) == X509_V_ERR_CERT_HAS_EXPIRED)
		return 1;
	return 0;
}

static RSA *rsa_privatekey_open(const char *filename)
{
	FILE *fp;
	RSA *r = NULL;

	fp = fopen(filename, "r");
	if (!fp)
	{
		fprintf(stderr, "cannot open %s\n", filename);
		return NULL;
	}

	PEM_read_RSAPrivateKey(fp, &r, NULL, NULL);
	if (!r)
		fprintf(stderr, "read error\n");

	fclose(fp);

	return r;
}

static X509 *certificate_open(const char *filename)
{
	FILE *fp;
	X509 *cert;

	fp = fopen(filename, "r");
	if (!fp)
	{
		fprintf(stderr, "cannot open %s\n", filename);
		return NULL;
	}

	cert = PEM_read_X509(fp, NULL, NULL, NULL);
	if (!cert)
		fprintf(stderr, "cannot read cert\n");

	fclose(fp);

	return cert;
}

static bool certificate_validate(struct cert_ctx *ctx, X509 *cert)
{
	X509_STORE_CTX *store_ctx;
	int ret;

	store_ctx = X509_STORE_CTX_new();

	X509_STORE_CTX_init(store_ctx, ctx->store, cert, NULL);
	X509_STORE_CTX_set_verify_cb(store_ctx, verify_cb);
	X509_STORE_CTX_set_flags(store_ctx, X509_V_FLAG_IGNORE_CRITICAL);

	ret = X509_verify_cert(store_ctx);

	if (ret != 1)
#if OPENSSL_VERSION_NUMBER < 0x10100000L
		fprintf(stderr, "%s\n", X509_verify_cert_error_string(store_ctx->error));
#else
		fprintf(stderr, "%s\n", X509_verify_cert_error_string(ret));
#endif

	X509_STORE_CTX_free(store_ctx);

	return ret == 1;
}

static X509 *certificate_load_and_check(struct cert_ctx *ctx, const char *filename)
{
	X509 *cert;

	if (!ctx->store)
	{
		/* we assume this is the first certificate added - so its root-ca */
		ctx->store = X509_STORE_new();
		if (!ctx->store)
		{
			fprintf(stderr, "cannot create cert_store\n");
			exit(-1);
		}

		if (X509_STORE_load_locations(ctx->store, filename, NULL) != 1)
		{
			fprintf(stderr, "load of first certificate (root_ca) failed\n");
			exit(-1);
		}

		return NULL;
	}

	cert = certificate_open(filename);
	if (!cert)
	{
		fprintf(stderr, "cannot open certificate %s\n", filename);
		return NULL;
	}

	if (!certificate_validate(ctx, cert))
	{
		fprintf(stderr, "cannot vaildate certificate\n");
		X509_free(cert);
		return NULL;
	}

	/* push into store - create a chain */
	if (X509_STORE_load_locations(ctx->store, filename, NULL) != 1)
	{
		fprintf(stderr, "load of certificate failed\n");
		X509_free(cert);
		return NULL;
	}

	return cert;
}

static X509 *certificate_import_and_check(struct cert_ctx *ctx, const uint8_t *data, int len)
{
	X509 *cert;

	cert = d2i_X509(NULL, &data, len);
	if (!cert)
	{
		fprintf(stderr, "cannot read certificate\n");
		return NULL;
	}

	if (!certificate_validate(ctx, cert))
	{
		fprintf(stderr, "cannot vaildate certificate\n");
		X509_free(cert);
		return NULL;
	}

	X509_STORE_add_cert(ctx->store, cert);

	return cert;
}


/* CI+ credentials */

#define MAX_ELEMENTS    33

uint32_t datatype_sizes[MAX_ELEMENTS] =
{
	0, 50, 0, 0, 0, 8, 8, 0,
	0, 0, 0, 0, 32, 256, 256, 0,
	0, 256, 256, 32, 8, 8, 32, 32,
	0, 8, 2, 32, 1, 32, 1, 0,
	32
};

struct element
{
	uint8_t *data;
	uint32_t size;
	/* buffer valid */
	bool valid;
};

struct cc_ctrl_data
{
	/* parent */
	//struct ci_session *session;
	eDVBCISlot *slot;

	/* ci+ credentials */
	struct element elements[MAX_ELEMENTS];

	/* DHSK */
	uint8_t dhsk[256];

	/* KS_host */
	uint8_t ks_host[32];

	/* derived keys */
	uint8_t sek[16];
	uint8_t sak[16];

	/* AKH checks - module performs 5 tries to get correct AKH */
	unsigned int akh_index;

	/* authentication data */
	uint8_t dh_exp[256];

	/* certificates */
	struct cert_ctx *cert_ctx;

	/* private key of device-cert */
	RSA *rsa_device_key;
};


static struct element *element_get(struct cc_ctrl_data *cc_data, unsigned int id)
{
	/* array index */
	if ((id < 1) || (id >= MAX_ELEMENTS))
	{
		fprintf(stderr, "element_get: invalid id\n");
		return NULL;
	}

	return &cc_data->elements[id];
}

static void element_invalidate(struct cc_ctrl_data *cc_data, unsigned int id)
{
	struct element *e;

	e = element_get(cc_data, id);
	if (e)
	{
		free(e->data);
		memset(e, 0, sizeof(struct element));
	}
}

static void element_init(struct cc_ctrl_data *cc_data)
{
	unsigned int i;

	for (i = 1; i < MAX_ELEMENTS; i++)
		element_invalidate(cc_data, i);
}

static bool element_set(struct cc_ctrl_data *cc_data, unsigned int id, const uint8_t *data, uint32_t size)
{
	struct element *e;

	e = element_get(cc_data, id);
	if (e == NULL)
		return false;

	/* check size */
	if ((datatype_sizes[id] != 0) && (datatype_sizes[id] != size))
	{
		fprintf(stderr, "size %d of datatype_id %d doesn't match\n", size, id);
		return false;
	}

	free(e->data);
	e->data = (uint8_t *)malloc(size);
	memcpy(e->data, data, size);
	e->size = size;
	e->valid = true;

	//printf("stored %d with len %d\n", id, size);

	return true;
}

static bool element_set_certificate(struct cc_ctrl_data *cc_data, unsigned int id, X509 *cert)
{
	unsigned char *cert_der = NULL;
	int cert_len;

	cert_len = i2d_X509(cert, &cert_der);
	if (cert_len <= 0)
	{
		printf("cannot get data in DER format\n");
		return false;
	}

	if (!element_set(cc_data, id, cert_der, cert_len))
	{
		printf("cannot store element (%d)\n", id);
		return false;
	}

	return true;
}

static bool element_set_hostid_from_certificate(struct cc_ctrl_data *cc_data, unsigned int id, X509 *cert)
{
	X509_NAME *subject;
	int nid_cn = OBJ_txt2nid("CN");
	char hostid[20];
	uint8_t bin_hostid[8];

	if ((id != 5) && (id != 6))
	{
		printf("wrong datatype_id for hostid\n");
		return false;
	}

	subject = X509_get_subject_name(cert);
	X509_NAME_get_text_by_NID(subject, nid_cn, hostid, sizeof(hostid));

	if (strlen(hostid) != 16)
	{
		printf("malformed hostid\n");
		return false;
	}

	str2bin(bin_hostid, hostid, 16);

	if (!element_set(cc_data, id, bin_hostid, sizeof(bin_hostid)))
	{
		printf("cannot set hostid\n");
		return false;
	}

	return true;
}

static bool element_valid(struct cc_ctrl_data *cc_data, unsigned int id)
{
	struct element *e;

	e = element_get(cc_data, id);

	return e && e->valid;
}

static unsigned int element_get_buf(struct cc_ctrl_data *cc_data, uint8_t *dest, unsigned int id)
{
	struct element *e;

	e = element_get(cc_data, id);
	if (e == NULL)
		return 0;

	if (!e->valid)
	{
		fprintf(stderr, "element_get_buf: datatype %d not valid\n", id);
		return 0;
	}

	if (!e->data)
	{
		fprintf(stderr, "element_get_buf: datatype %d doesn't exist\n", id);
		return 0;
	}

	if (dest)
		memcpy(dest, e->data, e->size);

	return e->size;
}

static unsigned int element_get_req(struct cc_ctrl_data *cc_data, uint8_t *dest, unsigned int id)
{
	unsigned int len = element_get_buf(cc_data, &dest[3], id);

	if (len == 0)
	{
		fprintf(stderr, "cannot get element %d\n", id);
		return 0;
	}

	dest[0] = id;
	dest[1] = len >> 8;
	dest[2] = len;

	return 3 + len;
}

static uint8_t *element_get_ptr(struct cc_ctrl_data *cc_data, unsigned int id)
{
	struct element *e;

	e = element_get(cc_data, id);
	if (e == NULL)
		return NULL;

	if (!e->valid)
	{
		fprintf(stderr, "element_get_ptr: datatype %u not valid\n", id);
		return NULL;
	}

	if (!e->data)
	{
		fprintf(stderr, "element_get_ptr: datatype %u doesn't exist\n", id);
		return NULL;
	}

	return e->data;
}


/* content_control commands */

static bool sac_check_auth(const uint8_t *data, unsigned int len, uint8_t *sak)
{
	struct aes_xcbc_mac_ctx ctx;
	uint8_t calced_signature[16];

	if (len < 16)
		return false;

	aes_xcbc_mac_init(&ctx, sak);
	aes_xcbc_mac_process(&ctx, (uint8_t *)"\x04", 1);        /* header len */
	aes_xcbc_mac_process(&ctx, data, len - 16);
	aes_xcbc_mac_done(&ctx, calced_signature);

	if (memcmp(&data[len - 16], calced_signature, 16))
	{
		fprintf(stderr, "signature wrong\n");
		return false;
	}

	//printf("auth ok!\n");
	return true;
}

static int sac_gen_auth(uint8_t *out, uint8_t *in, unsigned int len, uint8_t *sak)
{
	struct aes_xcbc_mac_ctx ctx;
	aes_xcbc_mac_init(&ctx, sak);
	aes_xcbc_mac_process(&ctx, (uint8_t *)"\x04", 1);        /* header len */
	aes_xcbc_mac_process(&ctx, in, len);
	aes_xcbc_mac_done(&ctx, out);

	return 16;
}

static void generate_key_seed(struct cc_ctrl_data *cc_data)
{
	/* this is triggered by new ns_module */

	/* generate new key_seed -> SEK/SAK key derivation */
	SHA256_CTX sha;

	SHA256_Init(&sha);
	SHA256_Update(&sha, &cc_data->dhsk[240], 16);
	SHA256_Update(&sha, element_get_ptr(cc_data, 22), element_get_buf(cc_data, NULL, 22));
	SHA256_Update(&sha, element_get_ptr(cc_data, 20), element_get_buf(cc_data, NULL, 20));
	SHA256_Update(&sha, element_get_ptr(cc_data, 21), element_get_buf(cc_data, NULL, 21));
	SHA256_Final(cc_data->ks_host, &sha);
}

static void generate_ns_host(struct cc_ctrl_data *cc_data)
{
	uint8_t buf[8];

	get_random(buf, sizeof(buf));
	element_set(cc_data, 20, buf, sizeof(buf));
}

static int generate_SAK_SEK(uint8_t *sak, uint8_t *sek, const uint8_t *ks_host)
{
	AES_KEY key;
	const uint8_t key_data[16] = { 0xea, 0x74, 0xf4, 0x71, 0x99, 0xd7, 0x6f, 0x35, 0x89, 0xf0, 0xd1, 0xdf, 0x0f, 0xee, 0xe3, 0x00 };
	uint8_t dec[32];
	int i;

	/* key derivation of sak & sek */

	AES_set_encrypt_key(key_data, 128, &key);

	for (i = 0; i < 2; i++)
		AES_ecb_encrypt(&ks_host[16 * i], &dec[16 * i], &key, 1);

	for (i = 0; i < 16; i++)
		sek[i] = ks_host[i] ^ dec[i];

	for (i = 0; i < 16; i++)
		sak[i] = ks_host[16 + i] ^ dec[16 + i];

	return 0;
}

static int sac_crypt(uint8_t *dst, const uint8_t *src, unsigned int len, const uint8_t *key_data, int encrypt)
{
	uint8_t iv[16] = { 0xf7, 0x70, 0xb0, 0x36, 0x03, 0x61, 0xf7, 0x96, 0x65, 0x74, 0x8a, 0x26, 0xea, 0x4e, 0x85, 0x41 };
	AES_KEY key;

	/* AES_ENCRYPT is '1' */
#if x_debug
	printf("%s -> %s\n", FILENAME, __FUNCTION__);
#endif
	if (encrypt)
		AES_set_encrypt_key(key_data, 128, &key);
	else
		AES_set_decrypt_key(key_data, 128, &key);

	AES_cbc_encrypt(src, dst, len, &key, iv, encrypt);

	return 0;
}

static X509 *import_ci_certificates(struct cc_ctrl_data *cc_data, unsigned int id)
{
	struct cert_ctx *ctx = cc_data->cert_ctx;
	X509 *cert;
	uint8_t buf[2048];
	unsigned int len;

	printf("%s -> %s\n", FILENAME, __FUNCTION__);

	len = element_get_buf(cc_data, buf, id);

	cert = certificate_import_and_check(ctx, buf, len);
	if (!cert)
	{
		printf("cannot read/verify DER cert\n");
		return NULL;
	}

	return cert;
}

static int check_ci_certificates(struct cc_ctrl_data *cc_data)
{
	struct cert_ctx *ctx = cc_data->cert_ctx;

	printf("%s -> %s\n", FILENAME, __FUNCTION__);

	/* check if both certificates are available before we push and verify them */

	/* check for CICAM_BrandCert */
	if (!element_valid(cc_data, 8))
		return -1;

	/* check for CICAM_DevCert */
	if (!element_valid(cc_data, 16))
		return -1;

#if 0
	{
		/* write ci device cert to disk */
		int fd = open("ci_cert.der", O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
		write(fd, element_get_ptr(cc_data, 16), element_get_buf(cc_data, NULL, 16));
		close(fd);
	}
#endif

	/* import CICAM_BrandCert */
	if ((ctx->ci_cust_cert = import_ci_certificates(cc_data, 8)) == NULL)
	{
		printf("cannot import cert\n");
		return -1;
	}

	/* import CICAM_DevCert */
	if ((ctx->ci_device_cert = import_ci_certificates(cc_data, 16)) == NULL)
	{
		printf("cannot import cert\n");
		return -1;
	}

	/* everything seems to be fine here - so extract the CICAM_id from cert */
	if (!element_set_hostid_from_certificate(cc_data, 6, ctx->ci_device_cert))
	{
		printf("cannot set cicam_id in elements\n");
		return -1;
	}

	return 0;
}

static int generate_akh(struct cc_ctrl_data *cc_data)
{
	uint8_t akh[32];
	SHA256_CTX sha;

	printf("%s -> %s\n", FILENAME, __FUNCTION__);

	SHA256_Init(&sha);
	SHA256_Update(&sha, element_get_ptr(cc_data, 6), element_get_buf(cc_data, NULL, 6));
	SHA256_Update(&sha, element_get_ptr(cc_data, 5), element_get_buf(cc_data, NULL, 5));
	SHA256_Update(&sha, cc_data->dhsk, 256);
	SHA256_Final(akh, &sha);

	element_set(cc_data, 22, akh, sizeof(akh));

	return 0;
}

static bool check_dh_challenge(struct cc_ctrl_data *cc_data)
{
	printf("%s -> %s\n", FILENAME, __FUNCTION__);

	/* check if every element for calculation of DHSK & AKH is available */

	/* check for auth_nonce */
	if (!element_valid(cc_data, 19))
		return false;

	/* check for CICAM_id */
	if (!element_valid(cc_data, 6))
		return false;

	/* check for DHPM */
	if (!element_valid(cc_data, 14))
		return false;

	/* check for Signature_B */
	if (!element_valid(cc_data, 18))
		return false;

	/* calculate DHSK - DHSK = DHPM ^ dh_exp % dh_p */
	dh_mod_exp(cc_data->dhsk, 256, element_get_ptr(cc_data, 14), 256, dh_p, sizeof(dh_p), cc_data->dh_exp, 256);

	/* gen AKH */
	generate_akh(cc_data);

	/* disable 5 tries of startup -> use new calculated one */
	cc_data->akh_index = 5;

	/* write to disk */
	write_authdata(cc_data->slot->slot, element_get_ptr(cc_data, 5), cc_data->dhsk, element_get_ptr(cc_data, 22));

	return true;
}

static int restart_dh_challenge(struct cc_ctrl_data *cc_data)
{
	uint8_t dhph[256], sign_A[256];
	struct cert_ctx *ctx;

	printf("%s -> %s\n", FILENAME, __FUNCTION__);

	if (!cc_data->cert_ctx)
	{
		ctx = (struct cert_ctx *)calloc(1, sizeof(struct cert_ctx));
		cc_data->cert_ctx = ctx;
	}
	else
	{
		ctx = cc_data->cert_ctx;
	}

	/* load certificates and device key */
	certificate_load_and_check(ctx, ROOT_CERT);
	ctx->cust_cert = certificate_load_and_check(ctx, CUSTOMER_CERT);
	ctx->device_cert = certificate_load_and_check(ctx, DEVICE_CERT);

	if (!ctx->cust_cert || !ctx->device_cert)
	{
		fprintf(stderr, "cannot loader certificates\n");
		return -1;
	}

	/* add data to element store */
	if (!element_set_certificate(cc_data, 7, ctx->cust_cert))
		fprintf(stderr, "cannot store cert in elements\n");

	if (!element_set_certificate(cc_data, 15, ctx->device_cert))
		fprintf(stderr, "cannot store cert in elements\n");

	if (!element_set_hostid_from_certificate(cc_data, 5, ctx->device_cert))
		fprintf(stderr, "cannot set hostid in elements\n");

	cc_data->rsa_device_key = rsa_privatekey_open(DEVICE_CERT);
	if (!cc_data->rsa_device_key)
	{
		fprintf(stderr, "cannot read private key\n");
		return -1;
	}

	/* invalidate elements */
	element_invalidate(cc_data, 6);
	element_invalidate(cc_data, 14);
	element_invalidate(cc_data, 18);
	element_invalidate(cc_data, 22); /* this will refuse a unknown cam */

	/* new dh_exponent */
	dh_gen_exp(cc_data->dh_exp, 256, dh_g, sizeof(dh_g), dh_p, sizeof(dh_p));

	/* new DHPH  - DHPH = dh_g ^ dh_exp % dh_p */
	dh_mod_exp(dhph, sizeof(dhph), dh_g, sizeof(dh_g), dh_p, sizeof(dh_p), cc_data->dh_exp, 256);

	/* store DHPH */
	element_set(cc_data, 13, dhph, sizeof(dhph));

	/* create Signature_A */
	dh_dhph_signature(sign_A, element_get_ptr(cc_data, 19), dhph, cc_data->rsa_device_key);

	/* store Signature_A */
	element_set(cc_data, 17, sign_A, sizeof(sign_A));

	return 0;
}

static int generate_uri_confirm(struct cc_ctrl_data *cc_data, const uint8_t *sak)
{
	SHA256_CTX sha;
	uint8_t uck[32];
	uint8_t uri_confirm[32];

	printf("%s -> %s\n", FILENAME, __FUNCTION__);

	/* calculate UCK (uri confirmation key) */
	SHA256_Init(&sha);
	SHA256_Update(&sha, sak, 16);
	SHA256_Final(uck, &sha);

	/* calculate uri_confirm */
	SHA256_Init(&sha);
	SHA256_Update(&sha, element_get_ptr(cc_data, 25), element_get_buf(cc_data, NULL, 25));
	SHA256_Update(&sha, uck, 32);
	SHA256_Final(uri_confirm, &sha);

	element_set(cc_data, 27, uri_confirm, 32);

	return 0;
}

static void check_new_key(struct cc_ctrl_data *cc_data)
{
	const uint8_t s_key[16] = { 0x3e, 0x20, 0x15, 0x84, 0x2c, 0x37, 0xce, 0xe3, 0xd6, 0x14, 0x57, 0x3e, 0x3a, 0xab, 0x91, 0xb6 };
	AES_KEY aes_ctx;
	uint8_t dec[32];
	uint8_t *kp;
	uint8_t slot;
	unsigned int i;
#if x_debug
	printf("%s -> %s\n", FILENAME, __FUNCTION__);
#endif
	/* check for keyprecursor */
	if (!element_valid(cc_data, 12))
		return;

	/* check for slot */
	if (!element_valid(cc_data, 28))
		return;

	kp = element_get_ptr(cc_data, 12);
	element_get_buf(cc_data, &slot, 28);

	AES_set_encrypt_key(s_key, 128, &aes_ctx);
	for (i = 0; i < 32; i += 16)
		AES_ecb_encrypt(&kp[i], &dec[i], &aes_ctx, 1);

	for (i = 0; i < 32; i++)
	{
		dec[i] ^= kp[i];
		cc_data->slot->lastKey[i] = dec[i];
	}
	cc_data->slot->lastParity = slot;

	if (cc_data->slot->scrambled)
		cc_data->slot->ccmgrSession->resendKey(cc_data->slot);

	/* reset */
	element_invalidate(cc_data, 12);
	element_invalidate(cc_data, 28);
}

static int data_get_handle_new(struct cc_ctrl_data *cc_data, unsigned int id)
{
#if x_debug
	printf("%s -> %s ID = (%d)\n", FILENAME, __FUNCTION__, id);
#endif
	/* handle trigger events */

	/* depends on new received items */

	switch (id)
	{
		case 8:         /* CICAM_BrandCert */
		case 14:        /* DHPM */
		case 16:        /* CICAM_DevCert */
		case 18:        /* Signature_B */
			/* this results in CICAM_ID when cert-chain is verified and ok */
			if (check_ci_certificates(cc_data))
				break;
			/* generate DHSK & AKH */
			check_dh_challenge(cc_data);
			break;

		case 19:        /* auth_nonce - triggers new dh keychallenge - invalidates DHSK & AKH */
			/* generate DHPH & Signature_A */
			restart_dh_challenge(cc_data);
			break;

		case 21:        /* Ns_module - triggers SAC key calculation */
			generate_ns_host(cc_data);
			generate_key_seed(cc_data);
			generate_SAK_SEK(cc_data->sak, cc_data->sek, cc_data->ks_host);
			break;

		/* SAC data messages */

		case 6:                 //CICAM_id
		case 12:                //keyprecursor
			check_new_key(cc_data);
			break;
		case 26:                //programm number
		case 25:                //uri_message
			generate_uri_confirm(cc_data, cc_data->sak);
			break;
		case 28:                //key register
			check_new_key(cc_data);
			break;

		default:
			printf("%s -> %s unhandled ID (%d)\n", FILENAME, __FUNCTION__, id);
			break;
	}

	return 0;
}

static int data_req_handle_new(struct cc_ctrl_data *cc_data, unsigned int id)
{
#if x_debug
	printf("%s -> %s ID = (%d)\n", FILENAME, __FUNCTION__, id);
#endif
	switch (id)
	{
		case 22:                /* AKH */
		{
			uint8_t akh[32], host_id[8];
			memset(akh, 0, sizeof(akh));
			if (cc_data->akh_index != 5)
			{
				if (!get_authdata(host_id, cc_data->dhsk, akh, cc_data->slot->slot, cc_data->akh_index++))
					cc_data->akh_index = 5;
				if (!element_set(cc_data, 22, akh, 32))
					printf("cannot set AKH in elements\n");
				if (!element_set(cc_data, 5, host_id, 8))
					printf("cannot set host_id in elements\n");
			}
		}
		default:
			break;
	}

	return 0;
}

static int data_get_loop(struct cc_ctrl_data *cc_data, const unsigned char *data, unsigned int datalen, unsigned int items)
{
	unsigned int i;
	int dt_id, dt_len;
	unsigned int pos = 0;
#if x_debug
	printf("%s -> %s\n", FILENAME, __FUNCTION__);
#endif
	for (i = 0; i < items; i++)
	{
		if (pos + 3 > datalen)
			return 0;

		dt_id = data[pos++];
		dt_len = data[pos++] << 8;
		dt_len |= data[pos++];

		if (pos + dt_len > datalen)
			return 0;
#if x_debug
		printf("set element (%d) ", dt_id);
#if y_debug
		hexdump(&data[pos], dt_len);
#else
		printf("\n");
#endif
#endif
		element_set(cc_data, dt_id, &data[pos], dt_len);

		data_get_handle_new(cc_data, dt_id);

		pos += dt_len;
	}

	return pos;
}

static int data_req_loop(struct cc_ctrl_data *cc_data, unsigned char *dest, const unsigned char *data, unsigned int datalen, unsigned int items)
{
	int dt_id;
	unsigned int i;
	int pos = 0;
	int len;
#if x_debug
	printf("%s -> %s\n", FILENAME, __FUNCTION__);
#endif
	if (items > datalen)
		return -1;

	for (i = 0; i < items; i++)
	{
		dt_id = *data++;
#if x_debug
		printf("req element %d\n", dt_id);
#endif
		data_req_handle_new(cc_data, dt_id);    /* check if there is any action needed before we answer */
		len = element_get_req(cc_data, dest, dt_id);
		if (len == 0)
		{
			printf("cannot get element %d\n", dt_id);
			return -1;
		}
#if x_debug
		printf("element (%d) > ", dt_id);
#if y_debug
		for (int ic = 0; ic < len; ic++)
			printf("%02x ", dest[ic]);
#endif
		printf("\n");
#endif

		pos += len;
		dest += len;
	}

	return pos;
}

bool eDVBCIContentControlManagerSession::data_initialize(eDVBCISlot *tslot)
{
	struct cc_ctrl_data *data;
	uint8_t buf[32], host_id[8];

	printf("%s -> %s\n", FILENAME, __FUNCTION__);

	if (tslot->private_data)
	{
		fprintf(stderr, "strange private_data not null!\n");
		return false;
	}

	data = (struct cc_ctrl_data *)calloc(1, sizeof(struct cc_ctrl_data));
	if (!data)
	{
		fprintf(stderr, "out of memory\n");
		return false;
	}

	/* parent */
	data->slot = tslot;

	/* clear storage of credentials */
	element_init(data);

	/* set status field - OK */
	memset(buf, 0, 1);
	if (!element_set(data, 30, buf, 1))
		fprintf(stderr, "cannot set status in elements\n");

	/* set uri_versions */
	memset(buf, 0, 32);
	buf[31] = 1;
	if (!element_set(data, 29, buf, 32))
		fprintf(stderr, "cannot set uri_versions in elements\n");

	/* load first AKH */
	data->akh_index = 0;
	if (!get_authdata(host_id, data->dhsk, buf, tslot->slot, data->akh_index))
	{
		/* no AKH available */
		memset(buf, 0, sizeof(buf));
		data->akh_index = 5;    /* last one */
	}

	if (!element_set(data, 22, buf, 32))
		fprintf(stderr, "cannot set AKH in elements\n");

	if (!element_set(data, 5, host_id, 8))
		fprintf(stderr, "cannot set host_id elements\n");

	tslot->private_data = data;

	return true;
}

void eDVBCIContentControlManagerSession::ci_ccmgr_cc_open_cnf(eDVBCISlot *tslot)
{
	const uint8_t tag[3] = { 0x9f, 0x90, 0x02 };
	const uint8_t bitmap = 0x01;
	printf("%s -> %s\n", FILENAME, __FUNCTION__);

	data_initialize(tslot);

	sendAPDU(tag, &bitmap, 1);
}

bool eDVBCIContentControlManagerSession::ci_ccmgr_cc_data_req(eDVBCISlot *tslot, const uint8_t *data, unsigned int len)
{
	struct cc_ctrl_data *cc_data = (struct cc_ctrl_data *)(tslot->private_data);
	uint8_t cc_data_cnf_tag[3] = { 0x9f, 0x90, 0x04 };
	uint8_t dest[2048 * 2];
	int dt_nr;
	int id_bitmask;
	int answ_len;
	unsigned int rp = 0;

	printf("%s -> %s\n", FILENAME, __FUNCTION__);

	if (len < 2)
		return false;

	id_bitmask = data[rp++];

	/* handle data loop */
	dt_nr = data[rp++];
	rp += data_get_loop(cc_data, &data[rp], len - rp, dt_nr);

	if (len < rp + 1)
		return false;

	/* handle req_data loop */
	dt_nr = data[rp++];

	dest[0] = id_bitmask;
	dest[1] = dt_nr;

	answ_len = data_req_loop(cc_data, &dest[2], &data[rp], len - rp, dt_nr);
	if (answ_len <= 0)
	{
		fprintf(stderr, "cannot req data\n");
		return false;
	}

	answ_len += 2;

	sendAPDU(cc_data_cnf_tag, dest, answ_len);

	return true;
}

void eDVBCIContentControlManagerSession::ci_ccmgr_cc_sac_sync_req(eDVBCISlot *tslot, const uint8_t *data, unsigned int
#if y_debug
    len
#endif
)
{
	const uint8_t sync_cnf_tag[3] = { 0x9f, 0x90, 0x10 };
	uint8_t dest[64];
	unsigned int serial;
	int pos = 0;

	printf("%s -> %s\n", FILENAME, __FUNCTION__);
#if y_debug
	hexdump(data, len);
#endif
	serial = UINT32(data, 4);

	pos += BYTE32(&dest[pos], serial);
	pos += BYTE32(&dest[pos], 0x01000000);

	/* status OK */
	dest[pos++] = 0;

	ci_ccmgr_cc_sac_send(tslot, sync_cnf_tag, dest, pos);
	tslot->ccmgr_ready = true;
}

void eDVBCIContentControlManagerSession::ci_ccmgr_cc_sync_req()
{
	const uint8_t tag[3] = { 0x9f, 0x90, 0x06 };
	const uint8_t sync_req_status = 0x00;    /* OK */

	printf("%s -> %s\n", FILENAME, __FUNCTION__);
	sendAPDU(tag, &sync_req_status, 1);
}

bool eDVBCIContentControlManagerSession::ci_ccmgr_cc_sac_send(eDVBCISlot *tslot, const uint8_t *tag, uint8_t *data, unsigned int pos)
{
	struct cc_ctrl_data *cc_data = (struct cc_ctrl_data *)(tslot->private_data);
	printf("%s -> %s (%02X%02X%02X) \n", FILENAME, __FUNCTION__, tag[0], tag[1], tag[2]);

	if (pos < 8)
		return false;

	pos += add_padding(&data[pos], pos - 8, 16);
	BYTE16(&data[6], pos - 8);      /* len in header */

	pos += sac_gen_auth(&data[pos], data, pos, cc_data->sak);
#if y_debug
	printf("Data for encrypt > ");
	for (unsigned int i = 0; i < pos; i++)
		printf("%02x ", data[i]);
	printf("\n");
#endif
	sac_crypt(&data[8], &data[8], pos - 8, cc_data->sek, AES_ENCRYPT);

	sendAPDU(tag, data, pos);
	return true;
}

bool eDVBCIContentControlManagerSession::ci_ccmgr_cc_sac_data_req(eDVBCISlot *tslot, const uint8_t *data, unsigned int len)
{
	struct cc_ctrl_data *cc_data = (struct cc_ctrl_data *)(tslot->private_data);
	const uint8_t data_cnf_tag[3] = { 0x9f, 0x90, 0x08 };
	uint8_t dest[2048];
	uint8_t tmp[len];
	int id_bitmask, dt_nr;
	unsigned int serial;
	int answ_len;
	int pos = 0;
	unsigned int rp = 0;
	printf("%s -> %s\n", FILENAME, __FUNCTION__);

	if (len < 10)
		return false;

	memcpy(tmp, data, 8);
	sac_crypt(&tmp[8], &data[8], len - 8, cc_data->sek, AES_DECRYPT);
	data = tmp;
#if y_debug
	printf("decryted > ");
	for (unsigned int i = 0; i < len; i++)
		printf("%02x ", data[i]);
	printf("\n");
#endif
	if (!sac_check_auth(data, len, cc_data->sak))
	{
		fprintf(stderr, "check_auth of message failed\n");
		return false;
	}

	serial = UINT32(&data[rp], 4);

	/* skip serial & header */
	rp += 8;

	id_bitmask = data[rp++];

	/* handle data loop */
	dt_nr = data[rp++];
	rp += data_get_loop(cc_data, &data[rp], len - rp, dt_nr);

	if (len < rp + 1)
		return false;

	dt_nr = data[rp++];

	/* create answer */
	pos += BYTE32(&dest[pos], serial);
	pos += BYTE32(&dest[pos], 0x01000000);

	dest[pos++] = id_bitmask;
	dest[pos++] = dt_nr;    /* dt_nbr */

	answ_len = data_req_loop(cc_data, &dest[pos], &data[rp], len - rp, dt_nr);
	if (answ_len <= 0)
	{
		fprintf(stderr, "cannot req data\n");
		return false;
	}
	pos += answ_len;

	return ci_ccmgr_cc_sac_send(tslot, data_cnf_tag, dest, pos);
}

eDVBCIContentControlManagerSession::eDVBCIContentControlManagerSession(eDVBCISlot *tslot)
{
	slot = tslot;
	slot->hasCCManager = true;
	slot->ccmgrSession = this;
	descrambler_init();
}

eDVBCIContentControlManagerSession::~eDVBCIContentControlManagerSession()
{
	slot->hasCCManager = false;
	slot->ccmgrSession = NULL;
}

void eDVBCIContentControlManagerSession::ci_ccmgr_doClose(eDVBCISlot *tslot)
{
	struct cc_ctrl_data *data = (struct cc_ctrl_data *)(tslot->private_data);
	printf("%s -> %s\n", FILENAME, __FUNCTION__);

	descrambler_deinit();

	element_init(data);
	free(data);
	tslot->private_data = NULL;
}

int eDVBCIContentControlManagerSession::receivedAPDU(const unsigned char *tag, const void *data, int len)
{
	printf("SESSION(%d)/CC %02x %02x %02x: ", session_nb, tag[0], tag[1], tag[2]);
#if y_debug
	for (int i = 0; i < len; i++)
		printf("%02x ", ((const unsigned char *)data)[i]);
#endif
	printf("\n");

	if ((tag[0] == 0x9f) && (tag[1] == 0x90))
	{
		switch (tag[2])
		{
			case 0x01:
				ci_ccmgr_cc_open_cnf(slot);
				break;
			case 0x03:
				ci_ccmgr_cc_data_req(slot, (const uint8_t *)data, len);
				break;
			case 0x05:
				ci_ccmgr_cc_sync_req();
				break;
			case 0x07:
				ci_ccmgr_cc_sac_data_req(slot, (const uint8_t *)data, len);
				break;
			case 0x09:
				ci_ccmgr_cc_sac_sync_req(slot, (const uint8_t *)data, len);
				break;
			default:
				fprintf(stderr, "unknown apdu tag %02x\n", tag[2]);
				break;
		}
	}

	return 0;
}

int eDVBCIContentControlManagerSession::doAction()
{

	printf("%s > %s\n", FILENAME, __FUNCTION__);
	switch (state)
	{
		case stateStarted:
		{
			state = stateFinal;
			return 0;
		}
		case stateFinal:
			printf("stateFinal und action! kann doch garnicht sein ;)\n");
		// fall through
		default:
			return 0;
	}
}

void eDVBCIContentControlManagerSession::resendKey(eDVBCISlot *tslot)
{
	/* Fix me ! no ci* cam with multi decrypt is known
	 * therefore for now it is OK to use element [0]
	 * in bool arrays
	*/

	if (!tslot->SidBlackListed && (tslot->recordUse[0] || tslot->liveUse[0]))
	{
#if HAVE_ARM_HARDWARE || HAVE_MIPS_HARDWARE
		if (slot->newPids)
		{
			if (slot->pids.size())
			{
				if (descrambler_open())
				{
					for (unsigned int i = 0; i < slot->pids.size(); i++)
						descrambler_set_pid((int)tslot->slot, 1, (int) slot->pids[i]);
				}
			}
			slot->newPids = false;
		}
		descrambler_set_key((int)tslot->slot, tslot->lastParity, tslot->lastKey);
#else
		if (slot->newPids)
		{
			if (slot->pids.size())
			{
				if (descrambler_open())
				{
					for (unsigned int i = 0; i < slot->pids.size(); i++)
						descrambler_set_pid((int)tslot->slot, 1, (int) slot->pids[i]);
				}
			}
			slot->newPids = false;
		}
		descrambler_set_key((int)tslot->source, tslot->lastParity, tslot->lastKey);
#endif
	}
}

