#include "stdafx.h"
#include "pce2png.h"

/*	
 *	pcemon - P/ECE monitor
 *	Copyright (C) 2002 Naoyuki Sawa
 *	sprite.cpp������
 */

/****************************************************************************
 *	�萔�E�^
 ****************************************************************************/

/* PIECE KERNEL : Ver 1.18 - piece.h ��� */
typedef struct tagSYSTEMINFO {
	unsigned short size;
	unsigned short hard_ver;
	unsigned short bios_ver;
	unsigned short bios_date;
	unsigned long sys_clock;
	unsigned short vdde_voltage;
	unsigned short resv1;
	unsigned long sram_top;
	unsigned long sram_end;
	unsigned long pffs_top;
	unsigned long pffs_end;
} SYSTEMINFO;

/* PIECE KERNEL : Ver 1.18 - piece.h ��� */
typedef struct _pceAPPHEAD {
	unsigned long signature;	/* 'pCeA'�Œ�B���������s�J�n����AppInit()����0�N���A����܂� */
	unsigned short sysver;		/* �V�X�e���o�[�W�����i��[���A�ڍׂȒl�͉���ł��Ȃ��j       */
	unsigned short resv1;		/* ���݂̃X�^�[�g�A�b�v���C�u�����ł�0�Œ�i����ł��Ȃ��j    */
	unsigned long initialize;	/* �������֐��A�h���X�i��[���K�{�A�n�[�t���[�h�A���C�����g�j */
	unsigned long periodic_proc;	/* �����֐��A�h���X�i��[���K�{�A�n�[�t���[�h�A���C�����g�j */
	unsigned long pre_terminate;	/* �I���֐��A�h���X�i��[���K�{�A�n�[�t���[�h�A���C�����g�j   */
	unsigned long notify_proc;	/* �ʒm�֐��A�h���X�i��[���K�{�A�n�[�t���[�h�A���C�����g�j   */
	unsigned long stack_size;	/* ���݂̃X�^�[�g�A�b�v���C�u�����ł�0�Œ�i����ł��Ȃ��j    */
	unsigned long bss_end;		/* BSS�̏I���A�h���X��pceHeapAlloc()�p�q�[�v�J�n�A�h���X      */
					/*�i��[���BBIOS1.18�̓��[�h�A���C�����g��z�肵�Ă��邪�A    */
					/* �����J��-align�I�v�V�����ŕύX�\�B���肵�Ȃ��������S�j   */
} pceAPPHEAD;
#define APPSTARTPOS1	0x100000 /* �ʏ�̃A�v���P�[�V�����̃w�b�_�ʒu   */
#define APPSTARTPOS2	0x138000 /* �̂̃X�^�[�g�A�b�v�V�F���̃w�b�_�ʒu */

/* PIECE sprite library : Ver 1.00 - pclsprite.h ��� */
#define FR_CX		(16)				/* �S�̂̃t���[���o�b�t�@   1�v���[��������̃��C���o�C�g�� */
#define FR_Y		(88)				/* �S�̂̃t���[���o�b�t�@   ���C����                        */
#define BG_CX		(32)				/* �w�i�L�����N�^�o�b�t�@   ���L�����N�^��                  */
#define BG_CY		(32)				/* �w�i�L�����N�^�o�b�t�@   �c�L�����N�^��                  */
#define BG_CBUFF_SIZE	((BG_CX * BG_CY) * 2)		/* �w�i�L�����N�^�o�b�t�@   �o�C�g��                        */
#define BG_FBUFF_SIZE	((BG_CX * BG_CY) * 8 * 3)	/* �w�i�t���[���o�b�t�@     �o�C�g��                        */
#define FRAME_SIZE	(FR_CX * FR_Y * 2)		/* �S�̂̃t���[���o�b�t�@   �o�C�g��                        */
#define BG_LINE_SIZE	(FR_Y * 2)			/* ���C���X�N���[�����W�X�^ �o�C�g��                        */
#define SPRITE_WORK_MIN	(BG_CBUFF_SIZE * 4 + BG_FBUFF_SIZE * 2 + FRAME_SIZE + BG_LINE_SIZE * 2)	/* �X�v���C�g���[�N �o�C�g�� (�X�v���C�g���W�X�^�͊܂܂Ȃ�) */
/* PIECE sprite library : Ver 1.00 - pclsprite.c ��� */
/* ��ready��sprmax�͏����l���ݒ肳��Ă���̂ŁAdata�Z�N�V�����ɔz�u����܂��B����ȊO��bss�Z�N�V�����ɔz�u����܂� */
typedef struct _SPRITEINFO {
//	short ready;			/* ���C�u�����g�p�\�t���O		�������ς݂Ȃ�-1�Œ�B�������O��0�ł����A���������\���̂������Ă����Ӗ��Ȃ̂ōl�����܂���B           */
	unsigned long pclsprite_pat;	/* �\�[�X�p�^�[��			SRAM��̃��[�h���E�A�h���X�B����RAM��t���b�V����ɒ��ڃ\�[�X�p�^�[����u�����P�[�X�͍l�����܂���B     */
	int disp;			/* �\���R���g���[�����W�X�^		���22�r�b�g�͖��g�p�ł����A�}�X�N���Ă��Ȃ��̂�0�Ƃ͌���܂���B�����̒l�̓`�F�b�N���܂���B           */
	unsigned long fram;		/* �S�̂̃t���[���o�b�t�@		�v�Z��11: line1xy + BG_LINE_SIZE                                                                        */
	unsigned long sreg;		/* �X�v���C�g���W�X�^			       4: bg1a + BG_CBUFF_SIZE / 4      �� ����int*�Ȃ̂�"/4"���Ă��܂����A���l�v�Z���͏Ȃ��Ă��������B */
//	int sprmax;			/* �X�v���C�g����			       1: (ssize - SPRITE_WORK_MIN) / 4    �ȉ����l�ł��B                                               */
	unsigned long bg0f;		/* ���@�w�i �t���[���o�b�t�@		       7: bg1b + BG_CBUFF_SIZE / 4                                                                      */
	unsigned long bg0a;		/* ���@�w�i �L�����N�^�o�b�t�@		       2: sbuff                                                                                         */
	unsigned long bg0b;		/* ���@�w�i �L�����N�^�o�b�t�@(�����p)	       5: sreg + sprmax                 �� ����int*�Ȃ̂�"+sprmax"�ł����A���l�v�Z����*4���K�v�ł��B    */
	unsigned long bg1f;		/* ��O�w�i �t���[���o�b�t�@		       8: bg0f + BG_FBUFF_SIZE / 4                                                                      */
	unsigned long bg1a;		/* ��O�w�i �L�����N�^�o�b�t�@		       3: bg0a + BG_CBUFF_SIZE / 4                                                                      */
	unsigned long bg1b;		/* ��O�w�i �L�����N�^�o�b�t�@(�����p)	       6: bg0b + BG_CBUFF_SIZE / 4                                                                      */
	unsigned long line0xy;		/* ���C���X�N���[�����W�X�^		       9: bg1f + BG_FBUFF_SIZE / 4                                                                      */
	unsigned long line1xy;		/* 					      10: line0xy + BG_LINE_SIZE                                                                        */
	unsigned char bg0x;		/* BG�X�N���[�����W�X�^ (0�`255) */
	unsigned char bg0y;		/*                               */
	unsigned char bg1x;		/*                               */
	unsigned char bg1y;		/*                               */
	unsigned char cursor_x;		/* �J�[�\���ʒu��       ( 0�`15) */
	unsigned char cursor_y;		/* �J�[�\���ʒu�c       ( 0�`10) */
	unsigned char cur_lx;		/* �J�[�\���ʒu�����E   ( 0�Œ�) */
	unsigned char cur_rx;		/* �J�[�\���ʒu�E���E+1 (16�Œ�) */
	unsigned char cur_uy;		/* �J�[�\���ʒu����E   ( 0�Œ�) */
	unsigned char cur_dy;		/* �J�[�\���ʒu�����E+1 (11�Œ�) */
} SPRITEINFO;
#define CUR_LX		 0		/* cur_lx�̌Œ�l�Acursor_x�̍ŏ��l   */
#define CUR_RX		16		/* cur_rx�̌Œ�l�Acursor_x�̍ő�l+1 */
#define CUR_UY		 0		/* cur_uy�̌Œ�l�Acursor_y�̍ŏ��l   */
#define CUR_DY		11		/* cur_dy�̌Œ�l�Acursor_y�̍ő�l+1 */

/****************************************************************************
 *	�֐��錾
 ****************************************************************************/

static unsigned char* sram_read(SYSTEMINFO* sysinfo);
static unsigned long apphead_find(SYSTEMINFO* sysinfo, unsigned char* sram);

/****************************************************************************
 *	fram_find
 ****************************************************************************/

unsigned long
fram_find()
{
	int retval, sprmax;
	unsigned char* sram;
	unsigned long apphead_addr, sprinfo_addr, fram;
	SYSTEMINFO sysinfo;
	SPRITEINFO* sprinfo;
	pceAPPHEAD* apphead;

	/* �V�X�e�������擾���܂��B */
	retval = ismGetVersion(&sysinfo, 1);
	if(retval == PIECE_INVALID_VERSION) {
		fprintf(stderr, "�V�X�e����񂪎擾�ł��܂���B\n");
//		ismExit(); /* �ؒf */
		return 0;
	}

	/* SRAM�S�̂�ǂݍ��񂾃��������擾���܂��B */
	sram = sram_read(&sysinfo);
	if(sram == NULL) {
//		ismExit(); /* �ؒf */
		return 0;
	}

	/* �A�v���P�[�V�����w�b�_��T���܂��B */
	apphead_addr = apphead_find(&sysinfo, sram);
	if(apphead_addr == 0) {
		free(sram); /* SRAM�J�� */
//		ismExit(); /* �ؒf */
		return 0;
	}
	apphead = (pceAPPHEAD*)&sram[apphead_addr - sysinfo.sram_top];

	/* �A�v���P�[�V�����w�b�_�̒��ォ��BSS�̈�I�[(-�\���̃T�C�Y)�܂ő�������... */
	for(sprinfo_addr = apphead_addr + sizeof(pceAPPHEAD);
	    sprinfo_addr < apphead->bss_end - sizeof(SPRITEINFO);
	    sprinfo_addr += 4) {
		sprinfo = (SPRITEINFO*)&sram[sprinfo_addr - sysinfo.sram_top];

		/* �ȒP�Ȍ���... */

		/* �e���[�h�|�C���^��SRAM��̃��[�h���E�A�h���X���w���Ă��邩�H
		 * line0xy,line1xy�̓o�C�g�|�C���^�ł����K�����[�h���E�ɔz�u�����̂ŁA���l�Ɍ������܂��B
		 */
		if(sprinfo->pclsprite_pat & 3 || sprinfo->pclsprite_pat < sysinfo.sram_top || sysinfo.sram_end <= sprinfo->pclsprite_pat) continue;
		if(sprinfo->fram          & 3 || sprinfo->fram          < sysinfo.sram_top || sysinfo.sram_end <= sprinfo->fram         ) continue;
		if(sprinfo->sreg          & 3 || sprinfo->sreg          < sysinfo.sram_top || sysinfo.sram_end <= sprinfo->sreg         ) continue;
		if(sprinfo->bg0f          & 3 || sprinfo->bg0f          < sysinfo.sram_top || sysinfo.sram_end <= sprinfo->bg0f         ) continue;
		if(sprinfo->bg0a          & 3 || sprinfo->bg0a          < sysinfo.sram_top || sysinfo.sram_end <= sprinfo->bg0a         ) continue;
		if(sprinfo->bg0b          & 3 || sprinfo->bg0b          < sysinfo.sram_top || sysinfo.sram_end <= sprinfo->bg0b         ) continue;
		if(sprinfo->bg1f          & 3 || sprinfo->bg1f          < sysinfo.sram_top || sysinfo.sram_end <= sprinfo->bg1f         ) continue;
		if(sprinfo->bg1a          & 3 || sprinfo->bg1a          < sysinfo.sram_top || sysinfo.sram_end <= sprinfo->bg1a         ) continue;
		if(sprinfo->bg1b          & 3 || sprinfo->bg1b          < sysinfo.sram_top || sysinfo.sram_end <= sprinfo->bg1b         ) continue;
		if(sprinfo->line0xy       & 3 || sprinfo->line0xy       < sysinfo.sram_top || sysinfo.sram_end <= sprinfo->line0xy      ) continue;
		if(sprinfo->line1xy       & 3 || sprinfo->line1xy       < sysinfo.sram_top || sysinfo.sram_end <= sprinfo->line1xy      ) continue;

		/* �J�[�\���ʒu�͈̔͂ƌŒ�l�������B
		 * 2002/08/10�C��: cursor_x,y�̉�ʉE�[�E���[�̔���ɂ���
		 * * �҂������ʉE�[�ŕ����񂪏I���悤�ɕ`�悷��ƁAcursor_x=cur_rx�ɂȂ邱�Ƃ�����܂��B
		 *   �Ⴆ�΁Alocate(9, 0); printstr("HISCORE"); �Ƃ�����ƁAcursor_x=16�ɂȂ��Ă��܂��B
		 *   ���̏����Ɉ�v�����Ƃ��A�X�v���C�g���̌��o�Ɏ��s���Ă����̂ŁA�͂����������u<=�v�łȂ��u<�v�ɏC�����܂����B
		 * * cursor_y�̕��̂͂��������́u<=�v�̂܂܂ł����Ǝv���̂ł����A���ƂȂ��|���̂ł�������u<�v�ɕύX���邱�Ƃɂ��܂����B
		 */
		if(sprinfo->cursor_x < sprinfo->cur_lx || sprinfo->cur_rx < /*�u<=�v�ł̓_���I*/ sprinfo->cursor_x) continue;
		if(sprinfo->cursor_y < sprinfo->cur_uy || sprinfo->cur_dy < /*�u<=�v�ł��n�j�H*/ sprinfo->cursor_y) continue;
		if(sprinfo->cur_lx != CUR_LX) continue;
		if(sprinfo->cur_rx != CUR_RX) continue;
		if(sprinfo->cur_uy != CUR_UY) continue;
		if(sprinfo->cur_dy != CUR_DY) continue;

		/* �ȒP�Ȍ��������ł����Ă���ӂɌ��܂�܂����A�O�̂��ߏڍׂȌ������s���܂�... */

		/* sreg[sprmax],bg0b �̕��тȂ̂ŁAsreg��bg0b�̍�����sprmax�����߂��܂��B */
		sprmax = (sprinfo->bg0b - sprinfo->sreg) / 4;
		if(sprmax < 0) continue;

		/* �e�o�b�t�@�̈ʒu�֌W���m�F���܂��B */
		if(sprinfo->bg1a    != sprinfo->bg0a    + BG_CBUFF_SIZE) continue;
		if(sprinfo->sreg    != sprinfo->bg1a    + BG_CBUFF_SIZE) continue;
		if(sprinfo->bg0b    != sprinfo->sreg    + sprmax * 4   ) continue;
		if(sprinfo->bg1b    != sprinfo->bg0b    + BG_CBUFF_SIZE) continue;
		if(sprinfo->bg0f    != sprinfo->bg1b    + BG_CBUFF_SIZE) continue;
		if(sprinfo->bg1f    != sprinfo->bg0f    + BG_FBUFF_SIZE) continue;
		if(sprinfo->line0xy != sprinfo->bg1f    + BG_FBUFF_SIZE) continue;
		if(sprinfo->line1xy != sprinfo->line0xy + BG_LINE_SIZE ) continue;
		if(sprinfo->fram    != sprinfo->line1xy + BG_LINE_SIZE ) continue;

		/* ���ׂĂ̌����Ƀp�X�����̂ŁA���ʂ��i�[���A�������[�v�𔲂��܂��B */
		fram = sprinfo->fram;
		break;
	}
	if(sprinfo_addr >= apphead->bss_end - sizeof(SPRITEINFO)) {
//		fprintf(stderr, "�X�v���C�g��񂪌�����܂���B\n");
		free(sram); /* SRAM�J�� */
//		ismExit(); /* �ؒf */
		return 0;
	}

	/* SRAM�S�̂�ǂݍ��񂾃��������J�����܂��B */
	free(sram);

	/* P/ECE����ؒf���܂��B */
//	ismExit();

	return fram;
}

/****************************************************************************
 *	ismAppPause
 ****************************************************************************/

int
ismAppPause(int pausef)
{
	unsigned char tmp[2];
	tmp[0] = 16;
	tmp[1] = (unsigned char)pausef;
	return ismCmdW(tmp, 2, 0, 0);
}

/****************************************************************************
 *	ismLCDGetInfo
 ****************************************************************************/

int
ismLCDGetInfo(int* pstat, unsigned long* pbuff)
{
	int retval;
	unsigned char tmp[12];
	tmp[0] = 17;
	retval = ismCmdR(tmp, 1, tmp, 12);
	if(retval == 0) {
		if(pstat != NULL) *pstat = tmp[0];
		if(pbuff != NULL) *pbuff = *(unsigned long*)(tmp + 8);
	}
	return retval;
}

/****************************************************************************
 *	sram_read
 ****************************************************************************/

static unsigned char*
sram_read(SYSTEMINFO* sysinfo)
{
	int retval, sram_len, ofs, len;
	unsigned char* sram;

	/* SRAM�S�̂�ǂݍ��ރ��������m�ۂ��܂��B */
	sram_len = sysinfo->sram_end - sysinfo->sram_top;
	sram = (unsigned char*)malloc(sram_len);
	if(sram == NULL) {
		fprintf(stderr, "�������s���ł��B\n");
		return NULL;
	}

	/* �������ǂݍ��ݒ��ɓ��e���ς��Ȃ��悤�A�|�[�Y�������܂��B */
	retval = ismAppPause(1);
	if(retval != 0) {
		fprintf(stderr, "�A�v���P�[�V�������������܂���B\n");
		free(sram); /* �������J�� */
		return NULL;
	}

	/* SRAM�S�̂�ǂݍ��݂܂��B
	 * ����x�̎�M�T�C�Y��64KB�����������肩�瓮��s����ɂȂ�X��������A
	 *   P/ECE���n���O�A�b�v���邱�Ƃ�����̂ŁA������M���邱�Ƃɂ��܂����B
	 *   �o���N�]���̈��S�T�C�Y��4KB�ƕ��������Ƃ�����̂ŁA4KB�ɂ��܂����B
	 */
	ofs = 0;
	while(ofs < sram_len) {
		len = sram_len - ofs;
		if(len > 0x1000) len = 0x1000; /* 4KB���� */
		retval = ismReadMem(sram + ofs, sysinfo->sram_top + ofs, len);
		if(retval != 0) {
			fprintf(stderr, "SRAM�̓ǂݍ��݂Ɏ��s���܂����B\n");
			ismAppPause(0); /* �|�[�Y���� */
			free(sram); /* �������J�� */
			return NULL;
		}
		ofs += len;
	}

	/* �|�[�Y���������܂��B */
	ismAppPause(0);

	return sram;
}

/****************************************************************************
 *	apphead_find
 ****************************************************************************/

static unsigned long
apphead_find(SYSTEMINFO* sysinfo, unsigned char* sram)
{
	int i;
	unsigned long addrs[] = { APPSTARTPOS1, APPSTARTPOS2 };
	unsigned long addr;
	pceAPPHEAD* apphead;

	for(i = 0; i < sizeof addrs / sizeof *addrs; i++) {
		addr = addrs[i];
		if(addr < sysinfo->sram_top || sysinfo->sram_end < addr + sizeof(pceAPPHEAD)) continue; /* �w�b�_��SRAM���͂ݏo���Ă� */
		apphead = (pceAPPHEAD*)&sram[addr - sysinfo->sram_top];

		if(apphead->signature != 0) continue; /* �V�O�l�`�����N���A����Ă��Ȃ� */
		if(apphead->sysver == 0) continue; /* �V�X�e���o�[�W�������ݒ肳��Ă��Ȃ� */
		if(apphead->bss_end < sysinfo->sram_top || sysinfo->sram_end < apphead->bss_end) continue; /* BSS�I���A�h���X��SRAM�O */

		/* �e�G���g���|�C���g�́A���߂̂P�o�C�g�ڂ��w���Ă���̂ŁA�n�[�t���[�h�A���C�����g����Ă���͂��B
		 * �܂��Asrf�t�@�C���̃Z�N�V�����z�u�́A{ pceAPPHEAD�`code�Z�N�V�����`data�Z�N�V�����`bss�Z�N�V�����`bss_end } �Ȃ̂ŁA
		 * �e�G���g���|�C���g�́A���Ȃ��Ƃ��ApceAPPHEAD�̒��ォ��bss_end�̊Ԃɂ���͂��ł��B
		 * �ȏ�A��̏������A�l�̃G���g���|�C���g�ɑ΂��Č������܂��B
		 */
		if(apphead->initialize    & 1 || apphead->initialize    < addr + sizeof(pceAPPHEAD) || apphead->bss_end <= apphead->initialize   ) continue;
		if(apphead->periodic_proc & 1 || apphead->periodic_proc < addr + sizeof(pceAPPHEAD) || apphead->bss_end <= apphead->periodic_proc) continue;
		if(apphead->pre_terminate & 1 || apphead->pre_terminate < addr + sizeof(pceAPPHEAD) || apphead->bss_end <= apphead->pre_terminate) continue;
		if(apphead->notify_proc   & 1 || apphead->notify_proc   < addr + sizeof(pceAPPHEAD) || apphead->bss_end <= apphead->notify_proc  ) continue;

		/* APPSTARTPOS1���L���Ȃ�AAPPSTARTPOS2�͒��ׂ܂���B�ʏ�̃A�v���P�[�V�����́AAPPSTARTPOS1�Ńq�b�g����͂��ł��B
		 * APPSTARTPOS2���g���Ă���̂͂��������̃X�^�[�g�A�b�v�V�F�������ŁA�����������malloc()���g���Ă��܂���B
		 * �ʏ�̃A�v���P�[�V������APPSTARTPOS2�𒲂ׂĂ��A�Ԉ���ăS�~�Ƀq�b�g���邾�����Ǝv���܂��B
		 */
		return addr;
	}

	fprintf(stderr, "�A�v���P�[�V�����w�b�_��������܂���B\n");
	return 0;
}
