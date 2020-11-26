/*
 * http_client.h
 */

#ifndef _HTTP_CLIENT_H_
#define _HTTP_CLIENT_H_

#include "ets_sys.h"
#include "os_type.h"

extern void http_client_init(u8* ip, u16 port);
extern void http_client_connect(void);

extern void http_get_version_init(u8* ip, u16 port);
extern void http_get_version_connect(void);

extern void http_location_init(u16 port);
extern void http_location_connect(void);
extern void dns_test(void);
extern struct ip_addr ipaddress;//AP的ip地址，用来解析api.newayz.com的ip地址
extern void wifiscan(void);







#endif /* _HTTP_CLIENT_H_ */
