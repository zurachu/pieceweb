#include "stdafx.h"
#include "pce2png.h"
#include "dib2png.h"

/*	
 *	pcemon - P/ECE monitor
 *	Copyright (C) 2002 Naoyuki Sawa
 *	main.cより改変
 */

unsigned long fram_addr;

void pce2png(char* png_filename, BYTE* img, int w, int h)
{
	FILE* fp;
	
	// DIB情報の作成
	BITMAPINFO*	bmi = (BITMAPINFO*)calloc(sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * 256, 1);

	BITMAPINFOHEADER* bi = &bmi->bmiHeader;
	bi->biBitCount	= 8;
	bi->biSize		= sizeof(BITMAPINFOHEADER);
	bi->biWidth		= w;
	bi->biHeight	= -h;
	bi->biPlanes	= 1;

	RGBQUAD* rgb = &bmi->bmiColors[0];
	for(int i = 0; i < 256; i++) {
		if(i < 4) {
			rgb[i].rgbRed = rgb[i].rgbGreen = rgb[i].rgbBlue = 255 - 85 * i;
		} else {
			rgb[i].rgbGreen = 255;
			rgb[i].rgbRed = rgb[i].rgbBlue = 0;
		}
	}

	fp = fopen(png_filename, "wb");
	dib2png(fp, bmi, img);
	fclose(fp);
	free(bmi);
}

#define CACHE_FILENAME	"viewcache.png"

FILE* lcd2png()
{
	BYTE vbuff[DISP_X * DISP_Y];

	fram_addr = 0;
	sprite_mode();
	if(fram_addr == 0)	graphic_mode();

	if(capture(vbuff, sizeof(vbuff)) != 0)	memset(vbuff, -1, sizeof(vbuff));
	pce2png(CACHE_FILENAME, vbuff, DISP_X, DISP_Y);
	return(fopen(CACHE_FILENAME, "rb"));
}

FILE* pgd2png(FILE* fp, char* fname)
{
	BYTE*		pgd;
	BYTE*		img;
	BYTE*		p;
	PIECE_BMP	pbmp;
	int			len;

	// PGDファイル読み込み
	fseek(fp, 0, SEEK_END);
	len = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	pgd = (BYTE*)malloc(len);
	fread(pgd, 1, len, fp);
	fclose(fp);

	// PIECE_BMP構造体の設定（ヘッダはチェックしてないよ！）
	memcpy(&pbmp.header, pgd, sizeof(PBMP_FILEHEADER));
	pbmp.buf	= pgd + sizeof(PBMP_FILEHEADER);
	pbmp.mask	= pbmp.buf + (pbmp.header.w * pbmp.header.h) / 4;
	
	img = (BYTE*)calloc(pbmp.header.w * pbmp.header.h, 1);
	
	// 画素
	p = img;
	for(int y = 0; y < pbmp.header.h; y++) {
		for(int x = 0; x < pbmp.header.w; x += 4) {
			switch(pbmp.header.bpp) {
				case 1:
					for(int i = 6; i >= 0; i -= 2)
						*p++ = ((*(pbmp.buf + (y * pbmp.header.w + x) / 4) >> i) & 3) * 3;
					break;
				case 2:
					for(int i = 6; i >= 0; i -= 2) {
						*p++ = (*(pbmp.buf + (y * pbmp.header.w + x) / 4) >> i) & 3;
					}
					break;
			}
		}
	}
	
	// マスク
	if(pbmp.header.mask) {
		p = img;
		for(int y = 0; y < pbmp.header.h; y++) {
			for(int x = 0; x < pbmp.header.w; x += 8) {
				for(int i = 7; i >= 0; i--) {
					if(!((*(pbmp.mask + (y * pbmp.header.w + x) / 8) >> i) & 1))	*p = -1;
					p++;
				}
			}
		}
	}

	// PNG出力
	strcat(fname, ".png");
	pce2png(fname, img, pbmp.header.w, pbmp.header.h);
	if(img != NULL)	free(img);
	return(fopen(fname, "rb"));
}


/****************************************************************************
 *	capture
 ****************************************************************************/

int
capture(unsigned char* vbuff, int len)
{
	int retval, stat, x, y, T0, T, dT;
	unsigned long pbuff;
	unsigned char fram[DISP_Y][2][DISP_X / 8];
	unsigned char* p;

//	retval = ismInitEx(0, 0);
//	if(retval != 0) goto ERR;

	/* * ismLCDCaptureScreen()は、ポーズ完了待ちループ中で20msのウェイトを取っています。
	 *   キャプチャ負荷が高いのは、どうやらこれが原因のようです。
	 * *  ismAppPause(1)発行後、一度目のismLCDGetInfo()ではまだポーズ完了していませんが、
	 *   20msよりもかなり早いタイミングでポーズがかかる場合にも、
	 *   ismLCDCaptureScreen()は無条件で20ms待ってしまいます。
	 *   そのため、P/ECEもポーズ状態のまま20ms弱待たされてしまいます。
	 * * USB通信自体はP/ECEにさほど負荷をかけないはずなので、
	 *   ウェイトを外した自前のループを回すことにしました。
	 */

	/* 非同期キャプチャでなければポーズをかけます。 */
//	if(!g_async) {
		retval = ismAppPause(1);
		if(retval != 0) goto ERR;
		/* アプリケーションがポーズ状態になるまで待ちます。 */
		T0 = GetTickCount();
		for(;;) {
			retval = ismLCDGetInfo(&stat, &pbuff); /* 後で使うので仮想VRAMアドレスも取得 */
			if(retval != 0) goto ERR;
			/* * 以前のカーネルは、
			 *	statのbit0にポーズ要求フラグ
			 *	statのbit1にpceAppProc実行中フラグ
			 *   を返していたので (stat & 3) == 1 になるまで待つ必要がありました。
			 *   (拙作miniISDはその頃に作ったので、そうなっています)
			 * * BIOS1.18現在のカーネルは、
			 *	stat=0:ポーズ要求が発行されていない、または要求が発行されたがまだpceAppProcが実行中
			 *	     1:ポーズ状態に入った
			 *   を返すようになったので、単にstatが0か否かを判断するだけでよくなりました。
			 * * 現在のカーネルでも、以前の判定式 (stat & 3) == 1 は正しく動きますが、
			 *   もはやbit2が0かどうかを調べるのは無意味になったので(常に0だから)、
			 *   今回は新しいカーネルを前提とした判断式を使うことにしました。
			 */
			if(stat) break;
			/* * ここにウェイトを入れてもいいが、いまのところ全速で回すことにします。
			 *   この関数の先頭あたりのコメント参照。
			 */
			T = GetTickCount();
			dT = T - T0;
			if(dT >= 1000) { /* 1秒でタイムアウト */
				retval = ismAppPause(0); /* ポーズ解除 */
				goto ERR;
			}
		}
//	}

	/* スプライトモード */
	if(fram_addr != 0) {
		/* スプライトライブラリ内のフレームバッファを読み込みます。 */
		retval = ismReadMem((unsigned char*)fram, fram_addr, sizeof fram);
		if(retval != 0) {
			/*if(!g_async)*/ ismAppPause(0); /* ポーズ解除 */
			goto ERR;
		}

	/* グラフィックモード */
	} else {
		/* 仮想VRAMを読み込みます。
		 * * 仮想VRAMアドレスはポーズ完了待ちのためのismLCDGetInfo()でいっしょに取得済み！
		 *   よく考えられているなあ。
		 */
		retval = ismReadMem(vbuff, pbuff, len);
		if(retval != 0) {
			/*if(!g_async)*/ ismAppPause(0); /* ポーズ解除 */
			goto ERR;
		}
	}

	/* 非同期キャプチャでなければポーズ解除します。 */
	/*if(!g_async)*/ ismAppPause(0);

	/* スプライトモードなら、フレームバッファを仮想VRAM形式に展開します。
	 * P/ECEのポーズ時間を少しでも短くするため、ポーズ解除後に行います。
	 */
	if(fram_addr != 0) {
		p = vbuff;
		for(y = 0; y < DISP_Y; y++) {
			for(x = 0; x < DISP_X; x++) {
				*p++ = (fram[y][0][x / 8] >> x % 8 & 1) << 1 |
				       (fram[y][1][x / 8] >> x % 8 & 1);
			}
		}
	}

	/* FALL THRU */
ERR:
//	ismExit();

	return retval;
}

/****************************************************************************
 *	graphic_mode
 ****************************************************************************/

void
graphic_mode()
{
	fram_addr = 0;
}

/****************************************************************************
 *	sprite_mode
 ****************************************************************************/

void
sprite_mode()
{
//	HCURSOR old_cursor;
//	old_cursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
	fram_addr = fram_find();
//	SetCursor(old_cursor);
	/* エラーメッセージ表示は呼び出し側で行ってください。 */
}
