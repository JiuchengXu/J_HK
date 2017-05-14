#ifndef ESP_H__
#define ESP_H__

s8 set_mode(u8 mode);

s8 set_show_ip(int mode);

s8 connect_ap(char *id, char *passwd, s8 channel);
s8 set_mac_addr(void);

s8 set_auto_conn(u8 i);
s8 close_conn(void);

s8 set_RF_power(s8 v);

s8 set_ap(char *sid, char *passwd);

s8 set_dhcp(void);

s8 set_echo(s8 on);

s8 set_mux(s8 mode);

s8 udp_setup(u32 ip, u16 remote_port, u16 local_port);

void ping(u32 ip);

s8 send_data(u32 ip, u16 src_port, u16 dst_port, char *data, u16 len);

void recv_data(u32 *ip, u16 *port, char *buf, u16 *buf_len);

s8 udp_close(u8 id);

s8 set_ip(char *ip);

s8 set_bound(void);

s8 esp_reset(void);

void esp8266_init(void);

void esp8266_update_firmware(void);

s8 get_ip(void);

#endif
