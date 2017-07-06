#include "stdafx.h"
#include "pceweb.h"
#include "ws.h"

#define BUILD_DATE	20050202

/*
 *	�ҁ`�������� Ver.20050202
 *	(c)2005 �d��Chu���ĂƂ灚�ۂ���
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
	if(!pw_init(dev)) {											// P/ECE����������
		if(!ws_init()) {										// WinSock����������
			if((s = ws_server_init(port)) != INVALID_SOCKET) {	// �T�[�o�\�z����
				_beginthread(ws_server_thread, 0, (void*)s);	// �҂��󂯃X���b�h�쐬
				CreateDirectory("cache", NULL);					// �L���b�V���t�H���_�쐬
				printf("�ҁ`�������� Ver.%d\n", BUILD_DATE);
				printf("(c)�ĂƂ灚�ۂ��� http://zurachu.net/\n");
				printf("P/ECE #%d Port %d Running...\n", dev, port);
				printf("�����L�[�������ƏI�����܂��B\n\n");
				while(!kbhit())	Sleep(100);
			} else {
				printf("�T�[�o�̍\�z�Ɏ��s���܂����B\n");
			}
			ws_close();
		} else {
			printf("WinSock2 �̏������Ɏ��s���܂����B\n");
			printf("�|�[�g %d �����p�\�ȃ|�[�g�ԍ����m�F���ĉ������B\n", port);
		}
		pw_exit();
	} else {	// P/ECE���������s
		printf("P/ECE #%d �̏������Ɏ��s���܂����B\n", dev);
		printf("���̃A�v���P�[�V�����ƒʐM���Ă��Ȃ����m�F���邩�A\n");
		printf("P/ECE��ڑ��������ĉ������B\n");
	}

	
	return 0;
}




