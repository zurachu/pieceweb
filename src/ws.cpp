#include "stdafx.h"
#include "ws.h"
#include "pceweb.h"

int		s_mode = 0;

/* WinSock 2.0 の初期化（成功=0，失敗=エラーコード） */
int ws_init(void)
{
	WSADATA	wsa;
	return(WSAStartup(MAKEWORD(2, 0), &wsa));
}

/* WinSockを閉じる */
void ws_close(void)
{
	// サーバソケットを閉じる
	s_mode = 0;
	WSACleanup();
}

/* サーバの構築（成功=サーバソケット、失敗=INVALID_SOCKET） */
SOCKET ws_server_init(unsigned short port)
{
	SOCKET		s_socket = INVALID_SOCKET;
	SOCKADDR_IN s_sockaddr;

	// ソケットの作成
	if((s_socket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
		return(INVALID_SOCKET);

	memset(&s_sockaddr, 0, sizeof(s_sockaddr));
	s_sockaddr.sin_port			= htons(port);			// ポート番号
	s_sockaddr.sin_family		= AF_INET;				// インターネット
	s_sockaddr.sin_addr.s_addr	= htonl(INADDR_ANY);	// 全アドレスのアクセスを許可

	// ポートのバインド
	if(bind(s_socket, (LPSOCKADDR)&s_sockaddr, sizeof(SOCKADDR_IN)) == SOCKET_ERROR)
		return(INVALID_SOCKET);

	// 接続要求の待ち受け
	if(listen(s_socket, 5) == SOCKET_ERROR)
		return(INVALID_SOCKET);

	return(s_socket);
}


/* 待ち受けスレッド処理 */
void ws_server_thread(void* s)
{
	SOCKET		s_socket = (SOCKET)s;
	SOCKET		c_socket;
	SOCKADDR_IN	c_sockaddr;
	int			len = sizeof(SOCKADDR_IN);
	
	s_mode = 1;
	while(s_mode) {
		if((c_socket = accept(s_socket, (LPSOCKADDR)&c_sockaddr, &len)) != INVALID_SOCKET)
			_beginthread(ws_client_thread, 0, (void*)c_socket);
	}

	shutdown(s_socket, 2);
	closesocket(s_socket);
}

/* 通信スレッド処理 */
void ws_client_thread(void* c)
{
	SOCKET	c_socket = (SOCKET)c;
	
	pw_correspond(c_socket);

	shutdown(c_socket, 2);					// ソケットの解放
	closesocket(c_socket);
}
