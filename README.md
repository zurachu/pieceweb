# pieceweb
PCを介してP/ECEを擬似的なWebサーバにする『ぴ～すうぇぶ』

##	概要
Windows PC上で動作し、PCを介してP/ECEを擬似的なWebサーバにするためのソフトです。
[メガストア杯](http://aquaplus.jp/piece/contest/ms_cup.html#C)応募バージョンからソースコードを構築しなおし、
安定性の向上および機能追加を行っています。

## アーカイブについて
アーカイブを展開すると以下のファイルが展開されます。ご確認下さい。
```
pieceweb.exe  プログラムファイル
pieceif.dll   P/ECE-PC通信ライブラリ
readme.txt    当説明ファイル
+ error       エラーコードHTML
+ src         ソースコード
```

## 起動方法
PCにP/ECEを接続している状態で、pieceweb.exeのアイコンをダブルクリックするなどしてぴ～すうぇぶを実行して下さい。
コンソール上でぴ～すうぇぶが起動されます。
デフォルトでは、ポート番号は10810（東鳩ｗ）になり、またP/ECEを複数台接続している時は1台目が参照されます。
複数起動も可能ですが、1つのぴ～すうぇぶにつき1台のP/ECE、1ポートが割り当てられると考えて下さい。

※WindowsXP SP2でご利用の場合等、ファイアウォールにブロックされる可能性があるので、予めぴ～すうぇぶはブロックしないよう設定して下さい。

※ぴ～すうぇぶ起動後は他のP/ECEに関しても接続を変更すべきではありません。

```
［コマンドラインオプション］
-d		デバイス番号（0～21）
-p		ポート番号（0～65536）

［例：デバイス番号#1（2台目）のP/ECEを8080番ポートでぴ～すうぇぶにする］
>	pieceweb -d1 -p8080
```

## 閲覧方法
ぴ～すうぇぶが起動している状態で、Internet ExplorerなどのWebブラウザで
```
http://localhost:ポート番号/
```
にアクセスすると、P/ECEのフラッシュメモリ上のファイル一覧を見ることができます。

ご使用のPCがLAN接続などによるプライベートIP、およびインターネット接続によるグローバルIP、ドメインをもっていれば、
```
http://IPおよびドメイン:ポート番号/
```
でネットワーク上にあなたのP/ECEの中身を公開することができます。

Webブラウザに表示されているファイル一覧からファイルをクリック、もしくはURIでファイル名を指定すると、
ファイルを閲覧したりダウンロードしたりできます。
現在閲覧が可能なファイルは以下の拡張子だけで，他のファイルはクリックするとダウンロード確認画面に移行します。
* *.txt
* *.htm
* *.css
* *.gif
* *.png
* *.jpg

PGDファイル（P/ECEの標準ビットマップ形式）に対しては、?viewというクエリを付加（ファイル一覧の[VIEW]アンカーも同様）することで、
PNG形式に変換してブラウザに表示させることができます。
また、ドメイントップに?viewクエリを付加（ファイル一覧のLCD Caputureアンカーも同様）すると、
現在のP/ECEの液晶画面をキャプチャしてPNG形式でブラウザに表示します。

全てのファイルは一旦cacheフォルダにキャッシュされてからクライアントに転送されます。
終了後はcacheフォルダを削除していただいてもかまいません。

404 Not Foundなどのエラーページについては、errorフォルダ以下のHTML文書を編集することで、自由に変更することができます。
