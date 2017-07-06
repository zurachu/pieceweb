#pragma once

int		pw_init(int n);
int		pw_exit();
void	pw_correspond(SOCKET s);
int		pw_recv_request(SOCKET s, char* method, char* uri);
void	pw_send_response_header(SOCKET s, int code, char* ext, int len);
FILE*	pw_get_file(int* code, char* uri, char* ext);
FILE*	pw_get_error(int code);
FILE*	pw_get_pffslist();
