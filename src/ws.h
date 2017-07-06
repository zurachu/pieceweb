#pragma once

int		ws_init(void);
void	ws_close(void);
SOCKET	ws_server_init(unsigned short port);
void	ws_server_thread(void* s);
void	ws_client_thread(void* c);

