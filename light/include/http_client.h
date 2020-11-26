/*
 * http_client.h
 */

#ifndef _HTTP_CLIENT_H_
#define _HTTP_CLIENT_H_

#include "ets_sys.h"
#include "os_type.h"

extern void http_get_lightstate_init(u8* ip, u16 port);
extern void http_get_lightstate_connect(void);
extern void user_upgrade_init(void);
extern void user_upgrade_finish(void);

extern void http_get_version_init(char* ip, u16 port);
extern void http_get_version_connect(void);

extern void http_get_lightstate_send(void);



#endif /* _HTTP_CLIENT_H_ */
