#include "stdafx.h"
#include "pce2png.h"

/*	
 *	pcemon - P/ECE monitor
 *	Copyright (C) 2002 Naoyuki Sawa
 *	sprite.cppより改変
 */

/****************************************************************************
 *	定数・型
 ****************************************************************************/

/* PIECE KERNEL : Ver 1.18 - piece.h より */
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

/* PIECE KERNEL : Ver 1.18 - piece.h より */
typedef struct _pceAPPHEAD {
	unsigned long signature;	/* 'pCeA'固定。ただし実行開始時にAppInit()内で0クリアされます */
	unsigned short sysver;		/* システムバージョン（非ゼロ、詳細な値は仮定できない）       */
	unsigned short resv1;		/* 現在のスタートアップライブラリでは0固定（仮定できない）    */
	unsigned long initialize;	/* 初期化関数アドレス（非ゼロ必須、ハーフワードアライメント） */
	unsigned long periodic_proc;	/* 処理関数アドレス（非ゼロ必須、ハーフワードアライメント） */
	unsigned long pre_terminate;	/* 終了関数アドレス（非ゼロ必須、ハーフワードアライメント）   */
	unsigned long notify_proc;	/* 通知関数アドレス（非ゼロ必須、ハーフワードアライメント）   */
	unsigned long stack_size;	/* 現在のスタートアップライブラリでは0固定（仮定できない）    */
	unsigned long bss_end;		/* BSSの終了アドレス＝pceHeapAlloc()用ヒープ開始アドレス      */
					/*（非ゼロ。BIOS1.18はワードアライメントを想定しているが、    */
					/* リンカの-alignオプションで変更可能。仮定しない方が安全）   */
} pceAPPHEAD;
#define APPSTARTPOS1	0x100000 /* 通常のアプリケーションのヘッダ位置   */
#define APPSTARTPOS2	0x138000 /* 昔のスタートアップシェルのヘッダ位置 */

/* PIECE sprite library : Ver 1.00 - pclsprite.h より */
#define FR_CX		(16)				/* 全体のフレームバッファ   1プレーン当たりのラインバイト数 */
#define FR_Y		(88)				/* 全体のフレームバッファ   ライン数                        */
#define BG_CX		(32)				/* 背景キャラクタバッファ   横キャラクタ数                  */
#define BG_CY		(32)				/* 背景キャラクタバッファ   縦キャラクタ数                  */
#define BG_CBUFF_SIZE	((BG_CX * BG_CY) * 2)		/* 背景キャラクタバッファ   バイト数                        */
#define BG_FBUFF_SIZE	((BG_CX * BG_CY) * 8 * 3)	/* 背景フレームバッファ     バイト数                        */
#define FRAME_SIZE	(FR_CX * FR_Y * 2)		/* 全体のフレームバッファ   バイト数                        */
#define BG_LINE_SIZE	(FR_Y * 2)			/* ラインスクロールレジスタ バイト数                        */
#define SPRITE_WORK_MIN	(BG_CBUFF_SIZE * 4 + BG_FBUFF_SIZE * 2 + FRAME_SIZE + BG_LINE_SIZE * 2)	/* スプライトワーク バイト数 (スプライトレジスタは含まない) */
/* PIECE sprite library : Ver 1.00 - pclsprite.c より */
/* ※readyとsprmaxは初期値が設定されているので、dataセクションに配置されます。それ以外はbssセクションに配置されます */
typedef struct _SPRITEINFO {
//	short ready;			/* ライブラリ使用可能フラグ		初期化済みなら-1固定。初期化前は0ですが、未初期化構造体を見つけても無意味なので考慮しません。           */
	unsigned long pclsprite_pat;	/* ソースパターン			SRAM上のワード境界アドレス。内蔵RAMやフラッシュ上に直接ソースパターンを置いたケースは考慮しません。     */
	int disp;			/* 表示コントロールレジスタ		上位22ビットは未使用ですが、マスクしていないので0とは限りません。ここの値はチェックしません。           */
	unsigned long fram;		/* 全体のフレームバッファ		計算順11: line1xy + BG_LINE_SIZE                                                                        */
	unsigned long sreg;		/* スプライトレジスタ			       4: bg1a + BG_CBUFF_SIZE / 4      ※ 元はint*なので"/4"していますが、数値計算時は省いてください。 */
//	int sprmax;			/* スプライト総数			       1: (ssize - SPRITE_WORK_MIN) / 4    以下同様です。                                               */
	unsigned long bg0f;		/* 奥　背景 フレームバッファ		       7: bg1b + BG_CBUFF_SIZE / 4                                                                      */
	unsigned long bg0a;		/* 奥　背景 キャラクタバッファ		       2: sbuff                                                                                         */
	unsigned long bg0b;		/* 奥　背景 キャラクタバッファ(内部用)	       5: sreg + sprmax                 ※ 元はint*なので"+sprmax"ですが、数値計算時は*4が必要です。    */
	unsigned long bg1f;		/* 手前背景 フレームバッファ		       8: bg0f + BG_FBUFF_SIZE / 4                                                                      */
	unsigned long bg1a;		/* 手前背景 キャラクタバッファ		       3: bg0a + BG_CBUFF_SIZE / 4                                                                      */
	unsigned long bg1b;		/* 手前背景 キャラクタバッファ(内部用)	       6: bg0b + BG_CBUFF_SIZE / 4                                                                      */
	unsigned long line0xy;		/* ラインスクロールレジスタ		       9: bg1f + BG_FBUFF_SIZE / 4                                                                      */
	unsigned long line1xy;		/* 					      10: line0xy + BG_LINE_SIZE                                                                        */
	unsigned char bg0x;		/* BGスクロールレジスタ (0～255) */
	unsigned char bg0y;		/*                               */
	unsigned char bg1x;		/*                               */
	unsigned char bg1y;		/*                               */
	unsigned char cursor_x;		/* カーソル位置横       ( 0～15) */
	unsigned char cursor_y;		/* カーソル位置縦       ( 0～10) */
	unsigned char cur_lx;		/* カーソル位置左限界   ( 0固定) */
	unsigned char cur_rx;		/* カーソル位置右限界+1 (16固定) */
	unsigned char cur_uy;		/* カーソル位置上限界   ( 0固定) */
	unsigned char cur_dy;		/* カーソル位置下限界+1 (11固定) */
} SPRITEINFO;
#define CUR_LX		 0		/* cur_lxの固定値、cursor_xの最小値   */
#define CUR_RX		16		/* cur_rxの固定値、cursor_xの最大値+1 */
#define CUR_UY		 0		/* cur_uyの固定値、cursor_yの最小値   */
#define CUR_DY		11		/* cur_dyの固定値、cursor_yの最大値+1 */

/****************************************************************************
 *	関数宣言
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

	/* システム情報を取得します。 */
	retval = ismGetVersion(&sysinfo, 1);
	if(retval == PIECE_INVALID_VERSION) {
		fprintf(stderr, "システム情報が取得できません。\n");
//		ismExit(); /* 切断 */
		return 0;
	}

	/* SRAM全体を読み込んだメモリを取得します。 */
	sram = sram_read(&sysinfo);
	if(sram == NULL) {
//		ismExit(); /* 切断 */
		return 0;
	}

	/* アプリケーションヘッダを探します。 */
	apphead_addr = apphead_find(&sysinfo, sram);
	if(apphead_addr == 0) {
		free(sram); /* SRAM開放 */
//		ismExit(); /* 切断 */
		return 0;
	}
	apphead = (pceAPPHEAD*)&sram[apphead_addr - sysinfo.sram_top];

	/* アプリケーションヘッダの直後からBSS領域終端(-構造体サイズ)まで走査して... */
	for(sprinfo_addr = apphead_addr + sizeof(pceAPPHEAD);
	    sprinfo_addr < apphead->bss_end - sizeof(SPRITEINFO);
	    sprinfo_addr += 4) {
		sprinfo = (SPRITEINFO*)&sram[sprinfo_addr - sysinfo.sram_top];

		/* 簡単な検査... */

		/* 各ワードポインタがSRAM上のワード境界アドレスを指しているか？
		 * line0xy,line1xyはバイトポインタですが必ずワード境界に配置されるので、同様に検査します。
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

		/* カーソル位置の範囲と固定値を検査。
		 * 2002/08/10修正: cursor_x,yの画面右端・下端の判定について
		 * * ぴったり画面右端で文字列が終わるように描画すると、cursor_x=cur_rxになることもあります。
		 *   例えば、locate(9, 0); printstr("HISCORE"); とかすると、cursor_x=16になっています。
		 *   この条件に一致したとき、スプライト情報の検出に失敗していたので、はじく条件を「<=」でなく「<」に修正しました。
		 * * cursor_yの方のはじく条件は「<=」のままでいいと思うのですが、何となく怖いのでこちらも「<」に変更することにしました。
		 */
		if(sprinfo->cursor_x < sprinfo->cur_lx || sprinfo->cur_rx < /*「<=」ではダメ！*/ sprinfo->cursor_x) continue;
		if(sprinfo->cursor_y < sprinfo->cur_uy || sprinfo->cur_dy < /*「<=」でもＯＫ？*/ sprinfo->cursor_y) continue;
		if(sprinfo->cur_lx != CUR_LX) continue;
		if(sprinfo->cur_rx != CUR_RX) continue;
		if(sprinfo->cur_uy != CUR_UY) continue;
		if(sprinfo->cur_dy != CUR_DY) continue;

		/* 簡単な検査だけでたいてい一意に決まりますが、念のため詳細な検査を行います... */

		/* sreg[sprmax],bg0b の並びなので、sregとbg0bの差からsprmaxが求められます。 */
		sprmax = (sprinfo->bg0b - sprinfo->sreg) / 4;
		if(sprmax < 0) continue;

		/* 各バッファの位置関係を確認します。 */
		if(sprinfo->bg1a    != sprinfo->bg0a    + BG_CBUFF_SIZE) continue;
		if(sprinfo->sreg    != sprinfo->bg1a    + BG_CBUFF_SIZE) continue;
		if(sprinfo->bg0b    != sprinfo->sreg    + sprmax * 4   ) continue;
		if(sprinfo->bg1b    != sprinfo->bg0b    + BG_CBUFF_SIZE) continue;
		if(sprinfo->bg0f    != sprinfo->bg1b    + BG_CBUFF_SIZE) continue;
		if(sprinfo->bg1f    != sprinfo->bg0f    + BG_FBUFF_SIZE) continue;
		if(sprinfo->line0xy != sprinfo->bg1f    + BG_FBUFF_SIZE) continue;
		if(sprinfo->line1xy != sprinfo->line0xy + BG_LINE_SIZE ) continue;
		if(sprinfo->fram    != sprinfo->line1xy + BG_LINE_SIZE ) continue;

		/* すべての検査にパスしたので、結果を格納し、走査ループを抜けます。 */
		fram = sprinfo->fram;
		break;
	}
	if(sprinfo_addr >= apphead->bss_end - sizeof(SPRITEINFO)) {
//		fprintf(stderr, "スプライト情報が見つかりません。\n");
		free(sram); /* SRAM開放 */
//		ismExit(); /* 切断 */
		return 0;
	}

	/* SRAM全体を読み込んだメモリを開放します。 */
	free(sram);

	/* P/ECEから切断します。 */
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

	/* SRAM全体を読み込むメモリを確保します。 */
	sram_len = sysinfo->sram_end - sysinfo->sram_top;
	sram = (unsigned char*)malloc(sram_len);
	if(sram == NULL) {
		fprintf(stderr, "メモリ不足です。\n");
		return NULL;
	}

	/* メモリ読み込み中に内容が変わらないよう、ポーズをかけます。 */
	retval = ismAppPause(1);
	if(retval != 0) {
		fprintf(stderr, "アプリケーションが応答しません。\n");
		free(sram); /* メモリ開放 */
		return NULL;
	}

	/* SRAM全体を読み込みます。
	 * ※一度の受信サイズが64KB超えたあたりから動作不安定になる傾向があり、
	 *   P/ECEがハングアップすることがあるので、分割受信することにしました。
	 *   バルク転送の安全サイズは4KBと聞いたことがあるので、4KBにしました。
	 */
	ofs = 0;
	while(ofs < sram_len) {
		len = sram_len - ofs;
		if(len > 0x1000) len = 0x1000; /* 4KB制限 */
		retval = ismReadMem(sram + ofs, sysinfo->sram_top + ofs, len);
		if(retval != 0) {
			fprintf(stderr, "SRAMの読み込みに失敗しました。\n");
			ismAppPause(0); /* ポーズ解除 */
			free(sram); /* メモリ開放 */
			return NULL;
		}
		ofs += len;
	}

	/* ポーズを解除します。 */
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
		if(addr < sysinfo->sram_top || sysinfo->sram_end < addr + sizeof(pceAPPHEAD)) continue; /* ヘッダがSRAMをはみ出してる */
		apphead = (pceAPPHEAD*)&sram[addr - sysinfo->sram_top];

		if(apphead->signature != 0) continue; /* シグネチャがクリアされていない */
		if(apphead->sysver == 0) continue; /* システムバージョンが設定されていない */
		if(apphead->bss_end < sysinfo->sram_top || sysinfo->sram_end < apphead->bss_end) continue; /* BSS終了アドレスがSRAM外 */

		/* 各エントリポイントは、命令の１バイト目を指しているので、ハーフワードアライメントされているはず。
		 * また、srfファイルのセクション配置は、{ pceAPPHEAD～codeセクション～dataセクション～bssセクション～bss_end } なので、
		 * 各エントリポイントは、少なくとも、pceAPPHEADの直後からbss_endの間にあるはずです。
		 * 以上、二つの条件を、四つのエントリポイントに対して検査します。
		 */
		if(apphead->initialize    & 1 || apphead->initialize    < addr + sizeof(pceAPPHEAD) || apphead->bss_end <= apphead->initialize   ) continue;
		if(apphead->periodic_proc & 1 || apphead->periodic_proc < addr + sizeof(pceAPPHEAD) || apphead->bss_end <= apphead->periodic_proc) continue;
		if(apphead->pre_terminate & 1 || apphead->pre_terminate < addr + sizeof(pceAPPHEAD) || apphead->bss_end <= apphead->pre_terminate) continue;
		if(apphead->notify_proc   & 1 || apphead->notify_proc   < addr + sizeof(pceAPPHEAD) || apphead->bss_end <= apphead->notify_proc  ) continue;

		/* APPSTARTPOS1が有効なら、APPSTARTPOS2は調べません。通常のアプリケーションは、APPSTARTPOS1でヒットするはずです。
		 * APPSTARTPOS2を使っているのはごく初期のスタートアップシェルだけで、しかもそれはmalloc()を使っていません。
		 * 通常のアプリケーションでAPPSTARTPOS2を調べても、間違ってゴミにヒットするだけだと思います。
		 */
		return addr;
	}

	fprintf(stderr, "アプリケーションヘッダが見つかりません。\n");
	return 0;
}
