__attribute__ ((section(".rodata"), unused))
const unsigned char first_crypto_rodata = 0x10;

__attribute__ ((section(".text"), unused))
void first_crypto_text(void){}

__attribute__ ((section(".init.text"), unused))
void first_crypto_init(void){}
