__attribute__ ((section(".rodata"), unused))
const unsigned char last_crypto_rodata = 0x20;

__attribute__ ((section(".text"), unused))
void last_crypto_text(void){}

__attribute__ ((section(".init.text"), unused))
void last_crypto_init(void){}
