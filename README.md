# Lattice MachXO 8bit-Arduino Library （Uno, Mega） #

Latticeの[MachXO2](http://www.latticesemi.com/en/Products/FPGAandCPLD/MachXO2)/[（MachXO3:試してない）](http://www.latticesemi.com/en/Products/FPGAandCPLD/MachXO3)に対し、ArduinoUnoかMega（Megaで試した）のSPI（/I2C:試してない）でアクセスして書き込みなどをするためのライブラリと、その使用例です。使用例のmachXOdemo.inoで、書き込みまで完結するはずです。

[元の公式リポジトリ](https://github.com/gsteiert-lscc/MachXO_Library)がAdafruitとかいう32bit環境に依存しているので、8bitの常識的なArduino環境、例えばUnoとかMegaとかで動かせるようにしました。

参考資料:
[MachXO2プログラミングとコンフィグレーション使用ガイド（公式日本語ガイド）](https://www.latticesemi.com/-/media/LatticeSemi/Documents/ApplicationNotes/MO/MachXO2ProgrammingandConfigurationUsageGuideJapaneseLanguageVersion.ashx?document_id=48276)

## ドライバについて ##

### ドライバ部分の依存ライブラリ ###

MachXOとの通信には標準のSPIのみを使います（ソフト/ハード）。ただしコンフィギュレーションファイルは巨大すぎてArduinoに持たせることができないため、コンフィギュレーション書き込み部分だけがSdFatライブラリに依存しています。つまり、Arduinoに別途接続したSDカードからファイルを読みだして書き込むようになっています。

公式ではTinyUSBなどを駆使してArduino上のSDカードがPCからUSBストレージに見えるようにするなどして利便性向上を図っていたようですが、たぶん8bit系でやることではありません。素直にSDカードを挿抜しましょう。

### ドライバの使い方 ###

To use this library, copy this directory and all the contents into the "libraries" directory where Arduino keeps all your installed libraries (typically ~/Documents/Aduino/libraries).

## MachXO2/3 Hex Files ##

.jedそのものではなくて、それをDeployment Tool（Lattice Diamond付属）で.hexに変換したものをカードに入れます。

## 使用例:machXOdemo.ino ##

シリアルコンソールです。CFM書き込みやデバイス状態の確認などができます。

フィーチャ行/フィーチャビットの読み取りは機能していない感じがします。

### 使用法 ###
`h`
ヘルプ

`l hogefile.hex`
CFM書き込み

`x`
デバイスID、ユーザコード、ステータスを表示

ステータスを見ることで書き込みが成功したかがわかります。

例:`Status:  0x00080100`は成功。bit8=DONEが立ってます。

例:`Status:  0x05880000`は失敗。bit26が先の命令の実行失敗を示し、bit[25:23]が`011`=CRCエラーを示しています。

ステータスの詳細な見方は[公式Q&A（英語）](https://www.latticesemi.com/ja-JP/Support/AnswerDatabase/4/9/0/4902)。

