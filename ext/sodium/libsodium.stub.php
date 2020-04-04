<?php

function sodium_crypto_aead_aes256gcm_is_available(): bool {}

#ifdef HAVE_AESGCM
function sodium_crypto_aead_aes256gcm_decrypt(string $string, string $ad, string $nonce, string $key): string|false {}

function sodium_crypto_aead_aes256gcm_encrypt(string $string, string $ad, string $nonce, string $key): string {}

function sodium_crypto_aead_aes256gcm_keygen(): string {}
#endif

function sodium_crypto_aead_chacha20poly1305_decrypt(string $string, string $ad, string $nonce, string $key): string|false {}

function sodium_crypto_aead_chacha20poly1305_encrypt(string $string, string $ad, string $nonce, string $key): string {}

function sodium_crypto_aead_chacha20poly1305_keygen(): string {}

function sodium_crypto_aead_chacha20poly1305_ietf_decrypt(string $string, string $ad, string $nonce, string $key): string|false {}

function sodium_crypto_aead_chacha20poly1305_ietf_encrypt(string $string, string $ad, string $nonce, string $key): string {}

function sodium_crypto_aead_chacha20poly1305_ietf_keygen(): string {}

#ifdef crypto_aead_xchacha20poly1305_IETF_NPUBBYTES
function sodium_crypto_aead_xchacha20poly1305_ietf_decrypt(string $string, string $ad, string $nonce, string $key): string|false {}

function sodium_crypto_aead_xchacha20poly1305_ietf_keygen(): string {}

function sodium_crypto_aead_xchacha20poly1305_ietf_encrypt(string $string, string $ad, string $nonce, string $key): string {}
#endif

function sodium_crypto_auth(string $string, string $key): string {}

function sodium_crypto_auth_keygen(): string {}

function sodium_crypto_auth_verify(string $signature, string $string, string $key): bool {}

function sodium_crypto_box(string $string, string $nonce, string $key): string {}

function sodium_crypto_box_keypair(): string {}

function sodium_crypto_box_seed_keypair(string $key): string {}

function sodium_crypto_box_keypair_from_secretkey_and_publickey(string $secret_key, string $public_key): string {}

function sodium_crypto_box_open(string $string, string $nonce, string $key): string|false {}

function sodium_crypto_box_publickey(string $key): string {}

function sodium_crypto_box_publickey_from_secretkey(string $key): string {}

function sodium_crypto_box_seal(string $string, string $key): string {}

function sodium_crypto_box_seal_open(string $string, string $key): string|false {}

function sodium_crypto_box_secretkey(string $key): string {}

function sodium_crypto_kx_keypair(): string {}

function sodium_crypto_kx_publickey(string $key): string {}

function sodium_crypto_kx_secretkey(string $key): string {}

function sodium_crypto_kx_seed_keypair(string $string): string {}

function sodium_crypto_kx_client_session_keys(string $client_keypair, string $server_key): array {}

function sodium_crypto_kx_server_session_keys(string $server_keypair, string $client_key): array {}

function sodium_crypto_generichash(string $string, string $key = "", int $length = SODIUM_CRYPTO_GENERICHASH_BYTES): string {}

function sodium_crypto_generichash_keygen(): string {}

function sodium_crypto_generichash_init(string $key = "", int $length = SODIUM_CRYPTO_GENERICHASH_BYTES): string {}

function sodium_crypto_generichash_update(string &$state, string $string): bool {}

function sodium_crypto_generichash_final(string &$state, $length = SODIUM_CRYPTO_GENERICHASH_BYTES): string {}

function sodium_crypto_kdf_derive_from_key(int $subkey_len, int $subkey_id, string $context, string $key): string {}

function sodium_crypto_kdf_keygen(): string {}

#ifdef crypto_pwhash_SALTBYTES
function sodium_crypto_pwhash(int $length, string $password, string $salt, int $opslimit, int $memlimit, int $alg = SODIUM_CRYPTO_PWHASH_ALG_DEFAULT): string {}

function sodium_crypto_pwhash_str(string $password, int $opslimit, int $memlimit): string {}

function sodium_crypto_pwhash_str_verify(string $hash, string $password): bool {}
#endif

#if SODIUM_LIBRARY_VERSION_MAJOR > 9 || (SODIUM_LIBRARY_VERSION_MAJOR == 9 && SODIUM_LIBRARY_VERSION_MINOR >= 6)
function sodium_crypto_pwhash_str_needs_rehash(string $password, int $opslimit, int $memlimit): bool {}
#endif

#ifdef crypto_pwhash_scryptsalsa208sha256_SALTBYTES
function sodium_crypto_pwhash_scryptsalsa208sha256(int $length, string $password, string $salt, int $opslimit, int $memlimit): string {}

function sodium_crypto_pwhash_scryptsalsa208sha256_str(string $password, int $opslimit, int $memlimit): string {}

function sodium_crypto_pwhash_scryptsalsa208sha256_str_verify(string $hash, string $password): bool {}
#endif

function sodium_crypto_scalarmult(string $string_1, string $string_2): string {}

function sodium_crypto_secretbox(string $string, string $nonce, string $key): string {}

function sodium_crypto_secretbox_keygen(): string {}

function sodium_crypto_secretbox_open(string $string, string $nonce, string $key): string|false {}

#ifdef crypto_secretstream_xchacha20poly1305_ABYTES
function sodium_crypto_secretstream_xchacha20poly1305_keygen(): string {}

function sodium_crypto_secretstream_xchacha20poly1305_init_push(string $key): array {}

function sodium_crypto_secretstream_xchacha20poly1305_push(string &$state, string $msg, string $ad  = "", int $tag  = SODIUM_CRYPTO_SECRETSTREAM_XCHACHA20POLY1305_TAG_MESSAGE): string {}

function sodium_crypto_secretstream_xchacha20poly1305_init_pull(string $string, string $key): string {}

function sodium_crypto_secretstream_xchacha20poly1305_pull(string &$state, string $c, string $ad  = ""): array|false {}

function sodium_crypto_secretstream_xchacha20poly1305_rekey(string &$state): void {}
#endif

function sodium_crypto_shorthash(string $string, string $key): string {}

function sodium_crypto_shorthash_keygen(): string {}

function sodium_crypto_sign(string $string, string $keypair): string {}

function sodium_crypto_sign_detached(string $string, string $keypair): string {}

function sodium_crypto_sign_ed25519_pk_to_curve25519(string $key): string {}

function sodium_crypto_sign_ed25519_sk_to_curve25519(string $key): string {}

function sodium_crypto_sign_keypair(): string {}

function sodium_crypto_sign_keypair_from_secretkey_and_publickey(string $secret_key, string $public_key): string {}

function sodium_crypto_sign_open(string $string, string $keypair): string|false {}

function sodium_crypto_sign_publickey(string $key): string {}

function sodium_crypto_sign_secretkey(string $key): string {}

function sodium_crypto_sign_publickey_from_secretkey(string $key): string {}

function sodium_crypto_sign_seed_keypair(string $key): string {}

function sodium_crypto_sign_verify_detached(string $signature, string $string, string $key): bool {}

function sodium_crypto_stream(int $length, string $nonce, string $key): string {}

function sodium_crypto_stream_keygen(): string {}

function sodium_crypto_stream_xor(string $string, string $nonce, string $key): string {}

function sodium_add(string &$string_1, string $string_2): void {}

function sodium_compare(string $string_1, string $string_2): int {}

function sodium_increment(string &$string): void {}

function sodium_memcmp(string $string_1, string $string_2): int {}

function sodium_memzero(string &$reference): void {}

function sodium_pad(string $string, int $length): string {}

function sodium_unpad(string $string, int $block_size): string {}

function sodium_bin2hex(string $string): string {}

function sodium_hex2bin(string $string, $ignore  = ""): string {}

#ifdef sodium_base64_VARIANT_ORIGINAL
function sodium_bin2base64(string $string, int $id): string {}

function sodium_base642bin(string $string, int $id, string $ignore  = ""): string {}
#endif

/** @alias sodium_crypto_box_publickey_from_secretkey */
function sodium_crypto_scalarmult_base(string $key): string {}
