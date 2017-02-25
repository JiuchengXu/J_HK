#ifndef __ESP8266_H__
#define __ESP8266_H__

s8 set_mode(u8 mode);
s8 set_show_ip(int mode);
s8 connect_ap(char *id, char *passwd, s8 channel);
s8 set_auto_conn(u8 i);
s8 set_ap(char *sid, char *passwd);
s8 set_echo(s8 on);
s8 set_mux(s8 mode);
s8 close_conn(void);
s8 udp_setup(u32 ip, u16 remote_port, u16 local_port);
s8 send_data(u32 ip, u16 src_port, u16 dst_port, char *data, u16 len);
s8 set_ip(char *ip);
void recv_data(u32 *ip, u16 *port, char *buf, u16 *buf_len);
s8 udp_close(u8 id);

void esp8266_init(void);

#endif
