#include "stdafx.h"
#include "ws.h"
#include "pceweb.h"

int		s_mode = 0;

/* WinSock 2.0 �̏������i����=0�C���s=�G���[�R�[�h�j */
int ws_init(void)
{
	WSADATA	wsa;
	return(WSAStartup(MAKEWORD(2, 0), &wsa));
}

/* WinSock����� */
void ws_close(void)
{
	// �T�[�o�\�P�b�g�����
	s_mode = 0;
	WSACleanup();
}

/* �T�[�o�̍\�z�i����=�T�[�o�\�P�b�g�A���s=INVALID_SOCKET�j */
SOCKET ws_server_init(unsigned short port)
{
	SOCKET		s_socket = INVALID_SOCKET;
	SOCKADDR_IN s_sockaddr;

	// �\�P�b�g�̍쐬
	if((s_socket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
		return(INVALID_SOCKET);

	memset(&s_sockaddr, 0, sizeof(s_sockaddr));
	s_sockaddr.sin_port			= htons(port);			// �|�[�g�ԍ�
	s_sockaddr.sin_family		= AF_INET;				// �C���^�[�l�b�g
	s_sockaddr.sin_addr.s_addr	= htonl(INADDR_ANY);	// �S�A�h���X�̃A�N�Z�X������

	// �|�[�g�̃o�C���h
	if(bind(s_socket, (LPSOCKADDR)&s_sockaddr, sizeof(SOCKADDR_IN)) == SOCKET_ERROR)
		return(INVALID_SOCKET);

	// �ڑ��v���̑҂���
	if(listen(s_socket, 5) == SOCKET_ERROR)
		return(INVALID_SOCKET);

	return(s_socket);
}


/* �҂��󂯃X���b�h���� */
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

/* �ʐM�X���b�h���� */
void ws_client_thread(void* c)
{
	SOCKET	c_socket = (SOCKET)c;
	
	pw_correspond(c_socket);

	shutdown(c_socket, 2);					// �\�P�b�g�̉��
	closesocket(c_socket);
}
