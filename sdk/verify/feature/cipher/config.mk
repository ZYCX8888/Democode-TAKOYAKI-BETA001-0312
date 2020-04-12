include clear-config.mk
CFLAGS:=-O0
SRCS:=test_main.c test_cipher.c test_hash.c test_rsa_enc.c test_rsa_sign.c test_cipher_common.c
LIBS:=mi_sys mi_cipher
include add-config.mk