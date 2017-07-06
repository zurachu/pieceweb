#pragma once

/*
 *	PIECE_Bmp.hより抜粋
 */

typedef struct {
	DWORD	head;		//	HEADER   'PBMP'
	DWORD	fsize;		//	ファイル長 （BYTE単位）
	BYTE	bpp;		//	bit深度  （2）
	BYTE	mask;		//	マスクのbit深度  （1）
	short	w;			//	X幅		４ピクセル単位厳守
	short	h;			//	Y高さ		
	DWORD	buf_size;	//	BMPサイズ	（BYTE単位）
}PBMP_FILEHEADER;

typedef struct{	//2BIT BMP + 1BIT MASK
	PBMP_FILEHEADER	header;
	BYTE			*buf;	//2BIT １ピクセル
	BYTE			*mask;	//1BIT １ピクセル
}PIECE_BMP;

/*	
 *	pcemon - P/ECE monitor
 *	Copyright (C) 2002 Naoyuki Sawa
 *	app.hより改変
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
