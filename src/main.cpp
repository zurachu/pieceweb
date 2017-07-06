#include "stdafx.h"
#include "pceweb.h"
#include "ws.h"

#define BUILD_DATE	20050202

/*
 *	ぴ～すうぇぶ Ver.20050202
 *	(c)2005 ヅラChu＠てとら★ぽっと
 *	http://zurachu.net/
 *	zurachu@zurachu.net
*/


int main(int argc, char* argv[])
{
	unsigned short	port = 10810;
	int				dev = 0;
	for(int i = 1; i < argc; i++) {
		if(!strncmp(argv[i], "-p", 2))	port = atoi(argv[i] + 2);
		if(!strncmp(argv[i], "-d", 2))	dev  = atoi(argv[i] + 2);
	}

	SOCKET s;
	if(!pw_init(dev)) {											// P/ECE初期化成功
		if(!ws_init()) {										// WinSock初期化成功
			if((s = ws_server_init(port)) != INVALID_SOCKET) {	// サーバ構築成功
				_beginthread(ws_server_thread, 0, (void*)s);	// 待ち受けスレッド作成
				CreateDirectory("cache", NULL);					// キャッシュフォルダ作成
				printf("ぴ～すうぇぶ Ver.%d\n", BUILD_DATE);
				printf("(c)てとら★ぽっと http://zurachu.net/\n");
				printf("P/ECE #%d Port %d Running...\n", dev, port);
				printf("何かキーを押すと終了します。\n\n");
				while(!kbhit())	Sleep(100);
			} else {
				printf("サーバの構築に失敗しました。\n");
			}
			ws_close();
		} else {
			printf("WinSock2 の初期化に失敗しました。\n");
			printf("ポート %d が利用可能なポート番号か確認して下さい。\n", port);
		}
		pw_exit();
	} else {	// P/ECE初期化失敗
		printf("P/ECE #%d の初期化に失敗しました。\n", dev);
		printf("他のアプリケーションと通信していないか確認するか、\n");
		printf("P/ECEを接続し直して下さい。\n");
	}

	
	return 0;
}




