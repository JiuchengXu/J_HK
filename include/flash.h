#ifndef __FLASH__
#define __FlASH__

void flash_bytes_read(u32 addr, u8 *buf, u16 len);
void flash_page_write(uint32_t page, uint8_t *data);


#endif
