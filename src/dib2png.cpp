#include "stdafx.h"

/*
 *	DIB -> PNG �ϊ�
 *	dib2png.c	2000.02 J.Baba
 *	������
 */

/* 1/4/8bit(�񈳏k) / 24bit DIB �Ή� */

int dib2png(FILE* fp, BITMAPINFO* bmi, BYTE* bmp)
{
	png_struct*	png;	// PNG����p�\����
	png_info*	info;	// PNG���\����
	png_color*	pal;	// PNG�p�p���b�g
	BYTE**		img;	// PNG�}���p�̍s�z��
	int			line_bytes;
	int			use_pal;
	int			pal_clr;

	BITMAPINFOHEADER*	bi	= &bmi->bmiHeader;
	RGBQUAD*			rgb	= &bmi->bmiColors[0];

	int w = bi->biWidth, h = bi->biHeight;
	int topdown = FALSE;	// DIB���g�b�v�_�E�����{�g���A�b�v���̃t���O
	if(h < 0) {
		topdown = TRUE;
		h = -h;
	}

	// �T�|�[�g�F��
	switch(bi->biBitCount) {
		case 1:	case 4:	case 8:	
			use_pal = TRUE;
			pal_clr = bi->biClrUsed;	// �p���b�g�̎g�p�F��
			if(pal_clr == 0)	pal_clr = 1 << bi->biBitCount;	// �O�̈�
			rgb	= &bmi->bmiColors[0];	// �p���b�g�f�[�^
			break;
		case 24:	case 0:
			break;
		default:
			return FALSE;
    }

	BYTE* src;	BYTE* dst;
    if(!use_pal) {
		// 24 bit �t���J���[
        line_bytes = ((w * 3 + 3) / 4) * 4;
		img = (BYTE**)malloc(h * sizeof(BYTE*));
		for(int i = 0; i < h; i++) {
			dst = img[i] = (BYTE*)malloc(w * 3);
			if(topdown) {
				src = &bmp[i * line_bytes];
			} else {
				src = &bmp[(h - 1 - i) * line_bytes];
			}
            for(int j = 0; j < w; j++) {	// �G���f�B�A�����t��
				*dst++ = src[2];
				*dst++ = src[1];
				*dst++ = src[0];
				src += 3;
            }
        }
    }  else {
        // 1,4,8 �p���b�g�J���[
		pal = (png_color*)malloc(pal_clr * sizeof(png_color));
		RGBQUAD*	src_pal = rgb;
		png_color*	dst_pal = pal;
		for(int i = 0; i < pal_clr; i++) {
			dst_pal->red	= src_pal->rgbRed;
			dst_pal->green	= src_pal->rgbGreen;
			dst_pal->blue	= src_pal->rgbBlue;
			dst_pal++;	src_pal++;
        }
	switch( bi->biBitCount ) {
		case 8:
			line_bytes = ((w + 3) / 4) * 4;
			break;
		case 4:
			line_bytes = (((w + 1) / 2 + 3) / 4) * 4;
			break;
		case 1:
			line_bytes = (((w + 3) / 8 + 3) / 4) * 4;
			break;
		}
		img = (BYTE**)malloc(h * sizeof(BYTE*));
		for(int i = 0; i < h; i++ ) {
			dst = img[i] = (BYTE*)malloc(line_bytes);
 			if(topdown) {
				src = &bmp[i * line_bytes];
			} else {
				src = &bmp[(h - 1 - i) * line_bytes];
			}
            for(int j = 0; j < line_bytes; j++ ) {
                *dst++ = *src++;
            }
        }
    }

	if(!(png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL)))	return FALSE;
	if(!(info = png_create_info_struct(png))) {
	    png_destroy_write_struct(&png, png_infopp_NULL);
		return FALSE;
	}
	png_init_io(png, fp);
	// PNG �w�b�_�[�̐ݒ�
	if(!use_pal) {
		png_set_IHDR(png, info, w, h, 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
	} else {
		png_set_IHDR(png, info, w, h, bi->biBitCount, PNG_COLOR_TYPE_PALETTE, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
		png_set_packing(png);
		// PNG �p���b�g�G���g���[�̐ݒ�
		png_set_PLTE(png, info, pal, pal_clr);
    }
	png_write_info(png, info);
	png_write_image(png, img);
	png_write_end(png, info);
	png_destroy_write_struct(&png, &info);

	// ��n��
	if(use_pal)	free(pal);
	for(int i = 0; i < h; i++ )	free(img[i]);
    free(img);
    return TRUE;
}
