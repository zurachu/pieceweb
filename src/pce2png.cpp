#include "stdafx.h"
#include "pce2png.h"
#include "dib2png.h"

/*	
 *	pcemon - P/ECE monitor
 *	Copyright (C) 2002 Naoyuki Sawa
 *	main.c������
 */

unsigned long fram_addr;

void pce2png(char* png_filename, BYTE* img, int w, int h)
{
	FILE* fp;
	
	// DIB���̍쐬
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

	// PGD�t�@�C���ǂݍ���
	fseek(fp, 0, SEEK_END);
	len = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	pgd = (BYTE*)malloc(len);
	fread(pgd, 1, len, fp);
	fclose(fp);

	// PIECE_BMP�\���̂̐ݒ�i�w�b�_�̓`�F�b�N���ĂȂ���I�j
	memcpy(&pbmp.header, pgd, sizeof(PBMP_FILEHEADER));
	pbmp.buf	= pgd + sizeof(PBMP_FILEHEADER);
	pbmp.mask	= pbmp.buf + (pbmp.header.w * pbmp.header.h) / 4;
	
	img = (BYTE*)calloc(pbmp.header.w * pbmp.header.h, 1);
	
	// ��f
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
	
	// �}�X�N
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

	// PNG�o��
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

	/* * ismLCDCaptureScreen()�́A�|�[�Y�����҂����[�v����20ms�̃E�F�C�g������Ă��܂��B
	 *   �L���v�`�����ׂ������̂́A�ǂ���炱�ꂪ�����̂悤�ł��B
	 * *  ismAppPause(1)���s��A��x�ڂ�ismLCDGetInfo()�ł͂܂��|�[�Y�������Ă��܂��񂪁A
	 *   20ms�������Ȃ葁���^�C�~���O�Ń|�[�Y��������ꍇ�ɂ��A
	 *   ismLCDCaptureScreen()�͖�������20ms�҂��Ă��܂��܂��B
	 *   ���̂��߁AP/ECE���|�[�Y��Ԃ̂܂�20ms��҂�����Ă��܂��܂��B
	 * * USB�ʐM���̂�P/ECE�ɂ��قǕ��ׂ������Ȃ��͂��Ȃ̂ŁA
	 *   �E�F�C�g���O�������O�̃��[�v���񂷂��Ƃɂ��܂����B
	 */

	/* �񓯊��L���v�`���łȂ���΃|�[�Y�������܂��B */
//	if(!g_async) {
		retval = ismAppPause(1);
		if(retval != 0) goto ERR;
		/* �A�v���P�[�V�������|�[�Y��ԂɂȂ�܂ő҂��܂��B */
		T0 = GetTickCount();
		for(;;) {
			retval = ismLCDGetInfo(&stat, &pbuff); /* ��Ŏg���̂ŉ��zVRAM�A�h���X���擾 */
			if(retval != 0) goto ERR;
			/* * �ȑO�̃J�[�l���́A
			 *	stat��bit0�Ƀ|�[�Y�v���t���O
			 *	stat��bit1��pceAppProc���s���t���O
			 *   ��Ԃ��Ă����̂� (stat & 3) == 1 �ɂȂ�܂ő҂K�v������܂����B
			 *   (�ٍ�miniISD�͂��̍��ɍ�����̂ŁA�����Ȃ��Ă��܂�)
			 * * BIOS1.18���݂̃J�[�l���́A
			 *	stat=0:�|�[�Y�v�������s����Ă��Ȃ��A�܂��͗v�������s���ꂽ���܂�pceAppProc�����s��
			 *	     1:�|�[�Y��Ԃɓ�����
			 *   ��Ԃ��悤�ɂȂ����̂ŁA�P��stat��0���ۂ��𔻒f���邾���ł悭�Ȃ�܂����B
			 * * ���݂̃J�[�l���ł��A�ȑO�̔��莮 (stat & 3) == 1 �͐����������܂����A
			 *   ���͂�bit2��0���ǂ����𒲂ׂ�͖̂��Ӗ��ɂȂ����̂�(���0������)�A
			 *   ����͐V�����J�[�l����O��Ƃ������f�����g�����Ƃɂ��܂����B
			 */
			if(stat) break;
			/* * �����ɃE�F�C�g�����Ă��������A���܂̂Ƃ���S���ŉ񂷂��Ƃɂ��܂��B
			 *   ���̊֐��̐擪������̃R�����g�Q�ƁB
			 */
			T = GetTickCount();
			dT = T - T0;
			if(dT >= 1000) { /* 1�b�Ń^�C���A�E�g */
				retval = ismAppPause(0); /* �|�[�Y���� */
				goto ERR;
			}
		}
//	}

	/* �X�v���C�g���[�h */
	if(fram_addr != 0) {
		/* �X�v���C�g���C�u�������̃t���[���o�b�t�@��ǂݍ��݂܂��B */
		retval = ismReadMem((unsigned char*)fram, fram_addr, sizeof fram);
		if(retval != 0) {
			/*if(!g_async)*/ ismAppPause(0); /* �|�[�Y���� */
			goto ERR;
		}

	/* �O���t�B�b�N���[�h */
	} else {
		/* ���zVRAM��ǂݍ��݂܂��B
		 * * ���zVRAM�A�h���X�̓|�[�Y�����҂��̂��߂�ismLCDGetInfo()�ł�������Ɏ擾�ς݁I
		 *   �悭�l�����Ă���Ȃ��B
		 */
		retval = ismReadMem(vbuff, pbuff, len);
		if(retval != 0) {
			/*if(!g_async)*/ ismAppPause(0); /* �|�[�Y���� */
			goto ERR;
		}
	}

	/* �񓯊��L���v�`���łȂ���΃|�[�Y�������܂��B */
	/*if(!g_async)*/ ismAppPause(0);

	/* �X�v���C�g���[�h�Ȃ�A�t���[���o�b�t�@�����zVRAM�`���ɓW�J���܂��B
	 * P/ECE�̃|�[�Y���Ԃ������ł��Z�����邽�߁A�|�[�Y������ɍs���܂��B
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
	/* �G���[���b�Z�[�W�\���͌Ăяo�����ōs���Ă��������B */
}
