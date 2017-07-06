#pragma once

/*
 *	PIECE_Bmp.h��蔲��
 */

typedef struct {
	DWORD	head;		//	HEADER   'PBMP'
	DWORD	fsize;		//	�t�@�C���� �iBYTE�P�ʁj
	BYTE	bpp;		//	bit�[�x  �i2�j
	BYTE	mask;		//	�}�X�N��bit�[�x  �i1�j
	short	w;			//	X��		�S�s�N�Z���P�ʌ���
	short	h;			//	Y����		
	DWORD	buf_size;	//	BMP�T�C�Y	�iBYTE�P�ʁj
}PBMP_FILEHEADER;

typedef struct{	//2BIT BMP + 1BIT MASK
	PBMP_FILEHEADER	header;
	BYTE			*buf;	//2BIT �P�s�N�Z��
	BYTE			*mask;	//1BIT �P�s�N�Z��
}PIECE_BMP;

/*	
 *	pcemon - P/ECE monitor
 *	Copyright (C) 2002 Naoyuki Sawa
 *	app.h������
 */

#define DISP_X	128
#define DISP_Y	 88

void	pce2png(char* png_filename, BYTE* img, int w, int h);
FILE*	lcd2png();
FILE*	pgd2png(FILE* fp, char* fname);

/* main.cpp */
//int PASCAL WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nCmdShow);
//LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
//void popup_menu();
//void show(int visible);
//void move_resize();
//void update(HDC hdc, const RECT* rect);
int capture(unsigned char* vbuff, int len);
//void change_scale(int scale);
//void change_interval(int interval);
//void change_vscreen(int vscreen);
void graphic_mode();
void sprite_mode();

/* sprite.cpp */
unsigned long fram_find();
int ismAppPause(int pausef);
int ismLCDGetInfo(int* pstat, unsigned long* pbuff);
