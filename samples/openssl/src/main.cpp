#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>

void generate_rsa(BIO* pub, BIO* priv)
{
    // generate key
    EVP_PKEY* pkey = EVP_RSA_gen(1024);
    if (pkey == NULL)
    {
        fprintf(stderr, "error: rsa gen\n");
        ERR_print_errors_fp(stderr);
        return;
    }

    // write public key
    if (PEM_write_bio_PUBKEY(pub, pkey) != 1)
    {
        fprintf(stderr, "error: write public key\n");
        ERR_print_errors_fp(stderr);
        EVP_PKEY_free(pkey);
        return;
    }

    // write private key
    if (PEM_write_bio_PrivateKey(priv, pkey, NULL, NULL, 0, NULL, NULL) != 1)
    {
        fprintf(stderr, "error: write private key\n");
        ERR_print_errors_fp(stderr);
        EVP_PKEY_free(pkey);
        return;
    }

    // release key
    EVP_PKEY_free(pkey);
}

int main()
{
    // create BIO
    BIO* pub_bio = BIO_new(BIO_s_mem());
    BIO* priv_bio = BIO_new(BIO_s_mem());
    if (pub_bio == NULL || priv_bio == NULL)
    {
        fprintf(stderr, "error: create BIO\n");
        return 1;
    }

    // generate RSA keys
    generate_rsa(pub_bio, priv_bio);

    // print public key
    BIO_flush(pub_bio);
    BUF_MEM* pub_buf;
    BIO_get_mem_ptr(pub_bio, &pub_buf);
    BIO_set_close(pub_bio, BIO_NOCLOSE); // prevent BIO_free from freeing the buffer
    printf("Public Key:\n%s\n", pub_buf->data);

    // print private key
    BIO_flush(priv_bio);
    BUF_MEM* priv_buf;
    BIO_get_mem_ptr(priv_bio, &priv_buf);
    BIO_set_close(priv_bio, BIO_NOCLOSE); // prevent BIO_free from freeing the buffer
    printf("Private Key:\n%s\n", priv_buf->data);

    // free BIO
    BIO_free_all(pub_bio);
    BIO_free_all(priv_bio);

    return 0;
}