#include "stdafx.h"
#include "pceweb.h"
#include "pce2png.h"

#define SEMAPHORE_NAME	"pieceweb_isd_semaphore"
#define TIMEOUT		(10*1000)
#define BUF_SIZE	1024

/*
 *	ぴ～すうぇぶ Ver.20050202
 *	(c)2005 ヅラChu＠てとら★ぽっと
 *	http://zurachu.net/
 */

HANDLE smp;
int dev;

int pw_init(int n)
{
	dev = n;
	smp = CreateSemaphore(NULL, 1, 1, SEMAPHORE_NAME);
	return(ismInitEx(dev, PIECE_DEF_WAITN));
}

int pw_exit()
{
	CloseHandle(smp);
	return(ismExitEx(dev));
}

void pw_correspond(SOCKET s)
{
	char	method[BUF_SIZE];	// HTTPリクエストメソッド
	char	uri[BUF_SIZE];		// HTTPリクエストURI
	int		code;				// HTTPレスポンスコード
	char	ext[BUF_SIZE];		// ファイルの拡張子
	int		len = 0;			// ファイル長
	FILE*	fp;					// ファイルポインタ
	char*	body = NULL;		// HTTPレスポンスボディ

	code = pw_recv_request(s, method, uri);
	shutdown(s, 0);	// ソケットの受信を無効にする

	if(WaitForSingleObject(smp, TIMEOUT) == WAIT_OBJECT_0) {	// セマフォ獲得
		SetCurrentDirectory("cache");
		fp = pw_get_file(&code, uri, ext);
		SetCurrentDirectory("..");
		ReleaseSemaphore(smp, 1, NULL);	// セマフォ解放
	} else {
		code = 503;	// Service Unavailable
	}

	if(code != 200) {
		if(WaitForSingleObject(smp, TIMEOUT) == WAIT_OBJECT_0) {	// セマフォ獲得
			SetCurrentDirectory("error");
			strcpy(ext, ".htm");
			fp = pw_get_error(code);
			SetCurrentDirectory("..");
			ReleaseSemaphore(smp, 1, NULL);	// セマフォ解放
		}
	}

	if(fp != NULL) {
		fseek(fp, 0, SEEK_END);
		len = ftell(fp);			// ファイル長取得
		fseek(fp, 0, SEEK_SET);
		body = (char*)malloc(len);	// メモリ確保
		fread(body, 1, len, fp);	// ファイルの読み込み
		fclose(fp);					// ファイルを閉じる
	}

	pw_send_response_header(s, code, ext, len);
	if(!strcmp(method, "GET") && len > 0) {	// GETメソッドの場合レスポンスボディも送る
		send(s, body, len, 0);
	}
	if(body != NULL)	free(body);
}

int pw_recv_request(SOCKET s, char* method, char* uri)
{
	char	req[BUF_SIZE];	// HTTPリクエスト受け取り
	char	http_ver[9];	// HTTPのバージョン
	
	recv(s, req, 3, 0);
	for(int i = 3; recv(s, req + i, 1, 0) == 1 && i < BUF_SIZE; i++) {
		if(!strncmp(req + i - 3, "\r\n\r\n", 4)) {
			*(req + i + 1) = '\0';
			break;
		}
	}

	if(sscanf(req, "%s %s %s\r\n", method, uri, http_ver) != 3)	return 400;
	printf("%s %s %s\n", method, uri, http_ver);	// ログ表示
	if(strncmp(http_ver, "HTTP/", 5))	return 400;					// 400 Bad Request
//	if(atof(http_ver + 5) > 1.0)		return 505;					// 505 HTTP Version Not Supported
	if(strcmp(method, "GET") && strcmp(method, "HEAD"))	return 501;	// 501 Not Implemented
	
	return 200;
}

#define TYPE_NUM	6

void pw_send_response_header(SOCKET s, int code, char* ext, int len)
{
	char	res[BUF_SIZE];
	char*	p = res;
	char*	cext[TYPE_NUM]  = {".txt", ".htm", ".css", ".gif", ".png", ".jpg"};
	char*	ctype[TYPE_NUM] =	{"text/plain", "text/html", "text/css", "image/gif", "image/png", "image/jpeg"};

	p += sprintf(p, "HTTP/1.0 %d ", code);
	switch(code) {	// HTTPレスポンスメッセージ
		case 200: p += sprintf(p, "OK");							break;
		case 400: p += sprintf(p, "Bad Request");					break;
		case 404: p += sprintf(p, "Not Found");						break;
		case 500: p += sprintf(p, "Internal Server Error");			break;
		case 501: p += sprintf(p, "Not Implemented");				break;
		case 503: p += sprintf(p, "Service Unavailable");			break;
		case 505: p += sprintf(p, "HTTP Version Not Supported");	break;
	}
	
	p += sprintf(p, "\r\nContent-Type: ");	// Content-Type
	int i;
	for(i = 0; i < TYPE_NUM; i++) {
		if(!strcmp(ext, cext[i])) {
			p += sprintf(p, ctype[i]);
			break;
		}
	}
	if(i >= TYPE_NUM)	p += sprintf(p, "application/octet-stream");
	
	p += sprintf(p, "\r\nContent-Length: %d\r\n\r\n", len);	// Content-Length
	send(s, res, (int)strlen(res), 0);
}

FILE* pw_get_file(int* code, char* uri, char* ext)
{
	FILE*	fp;					// ファイポインタ
	char	fname[BUF_SIZE];	// ファイル名
	char*	query;				// クエリ文字列へのポインタ
	BOOL	view = FALSE;		// 画面およびPGD画像を表示形式で返すかどうか

	// '?'があれば前後で切り分け（前：uri／後：query）
	if((query = strchr(uri, '?')) != NULL) {
		*query = '\0';
		query++;
		if(!strcmp(query, "view"))	view = TRUE;	// とりあえずクエリは"view"だけ認証するか…。
	}

	if(*code == 200) {
		if(!strcmp(uri, "/") || !strlen(uri)) {	// ドメイントップへのアクセス
			if(view) {	// viewフラグONなら画面キャプチャしてpng画像生成する
				if(!ismSelect(dev)) {
					strcpy(ext, ".png");
					if((fp = lcd2png()) != NULL)	return fp;
				}
			} else {	// viewフラグOFFならファイルリストを返す
				if(!ismSelect(dev)) {
					strcpy(ext, ".htm");
					if((fp = pw_get_pffslist()) != NULL)	return fp;
				}
			}
			*code = 500;
		} else {				// ファイルへのアクセス
			strcpy(fname, uri + 1);
			if(!ismSelect(dev)) {
				if(!ismPFFSRead(fname, fname)) {
					strcpy(ext, strrchr(fname, '.'));	// 拡張子セット
					fp = fopen(fname, "rb");			// ファイルハンドル取得
					if(!strcmp(ext, ".pgd") && view) {	// pgd画像かつviewフラグONならpng画像生成する
						strcpy(ext, ".png");
						fp = pgd2png(fp, fname);
					}
					return fp;
				} else {
					*code = 404;	// 404 Not Found
				}
			} else {
				*code = 500;	// 500 Internal Server Error
			}
		}
	}

	// エラーの場合ここまで来る
	return NULL;
}

#define CACHE_FILENAME	"pffscache.htm"

FILE* pw_get_pffslist()
{
	FILE*	fp;
	char	buf[32 * 256];
	char	fname[32];
	int		sct, size;
	char*	p;

	if(!ismPFFSDir(buf, sizeof(buf), 0)) {
		fp = fopen(CACHE_FILENAME, "w");
		fprintf(fp, "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0 Transitional//EN\" \"http://www.w3.org/TR/REC-html40/loose.dtd\">\n");
		fprintf(fp, "<HTML>\n<HEAD><TITLE>Index of P/ECE #%d</TITLE></HEAD>\n<BODY>\n", dev);
		fprintf(fp, "<H2>Index of P/ECE #%d</H2></FONT>\n",dev);
		fprintf(fp, "<ADDRESS><A href=\"?view\">LCD Capture</A></ADDRESS>\n<HR>\n");
		fprintf(fp, "<PRE><TABLE border=\"0\">\n<TR><TD align=\"center\">Sector</TD><TD align=\"center\">Size</TD><TD align=\"center\" colspan=\"2\">Name</TD></TR>\n");

		p = strtok(buf, "\n");
		while(1) {
			/* 合計サイズ行を発見したら抜けます。 */
			if(*p != '+') {
				fprintf(fp, "</TABLE></PRE>\n<HR>\n<ADDRESS>%s</ADDRESS>\n</BODY>\n</HTML>\n", p);
				break;
			}

			/* (番号)、サイズ、ファイル名に分割します。 */
			sscanf(p + 1, "%d:%d %s\n", &sct, &size, fname);
			fprintf(fp, "<TR><TD align=\"center\">%d</TD><TD align=\"right\">%d</TD><TD align=\"left\">", sct, size);
			if(strstr(fname, ".pgd") != NULL) {
				fprintf(fp, "[<A href=\"%s?view\">view</A>]", fname);
			}
			fprintf(fp, " </TD><TD align=\"left\"><A href=\"%s\">%s</A></TD></TR>\n", fname, fname);

			/* 次のファイルへ。
			 * 合計サイズ行なしで終了することはないはず。
			 */
			p = strtok(NULL, "\n");
		}

		fclose(fp);
		fp = fopen(CACHE_FILENAME, "rb");
		return fp;
	}

	return NULL;
}

FILE* pw_get_error(int code)
{
	char	fname[32];

	sprintf(fname, "%d.htm", code);
	return(fopen(fname, "rb"));
}
