// MIT License
//
// Author(s): Greg Steiert
// Copyright (c) 2019 Lattice Semiconductor Corporation
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

/* Lattice MachXO2（XO3もできるらしいけど試してません）にスレーブSPI（I2Cもできr略）
 * でアクセスできるシリアルコンソールです。
 * 書き込みは、コンフィギュレーションデータ（.hex）を格納したSDカードをArduinoに
 * 接続して行います。UnoとかMegaとかの常識的な8bit系Arduinoで使えるようにするために
 * 元あったTinyUSBのような凝った仕組みを排除しています。
 * 依存ライブラリ
 *  - MachXO （このライブラリ）
 *  - SdFat https://github.com/adafruit/SdFat（adafruit版でなくてもよいかも:未確認）
 *
 * [注意]フィーチャ行書き込み機能が未実装です。
 *       うっかり変なこと書き込む可能性があるよりは無害かも。
 *
 */

#include "SPI.h"
#include "MachXO.h"
#include "SdFat.h"
#include "sdios.h"

// Lattice MachXO プログラマ
//MachXO machXO;  // I2C default address 0b1000000
//MachXO machXO(5); // ハードウェア SPI, CS pin=5
#define MOSI (16)
#define MISO (15)
#define SCK  (14)
MachXO machXO(21, MOSI, MISO, SCK); // ソフトウェア SPI, CS pin=21

// SD chip select pin
const uint8_t chipSelect = SS;

// Additional Programming I/O
#define XO_PRGMN (9)
#define XO_INITN (10)

// Set to true when PC write to flash
bool changed;

#define COMMAND_BUFFER_LENGTH  127
int  charCnt;
char c;
char commandBuf[COMMAND_BUFFER_LENGTH +1];
const char *delimiters            = ", \t";

// ファイルシステム
SdFat sd;
//FatFileSystem fatfs;
//FatFile root;
FatFile file;

// セットアップ
void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(XO_PRGMN, INPUT_PULLUP);
  pinMode(XO_INITN, INPUT_PULLUP);
  digitalWrite(XO_PRGMN, LOW);
  digitalWrite(XO_INITN, LOW);

  machXO.begin(1); // デバッグメッセージ付きでmachXOインスタンス起動、引数なしでメッセージなしになる

  Serial.begin(115200);
  while ( !Serial ) delay(10);   // シリアル通信起動を待つ

  Serial.println("Lattice MachXO I2C/SPI Programming example, utilizing");
  //Serial.println("Adafruit TinyUSB Mass Storage External Flash example");
  //Serial.print("JEDEC ID: "); Serial.println(flash.getJEDECID(), HEX);
  //Serial.print("Flash size: "); Serial.println(flash.size());
  Serial.println("SdFat Storage.");

  changed = true; // 内容を表示するために、変化があったことにする
  charCnt = 0;
  commandBuf[0] = 0;

  // SDカードをオープン
  if(!sd.begin(chipSelect, SD_SCK_MHZ(50))){
    Serial.println("sd open error.");
  }
}

// メインループ
void loop()
{
  while (Serial.available()) {
    c = Serial.read();
    switch (c) {
      case '\r':
      case '\n':
        commandBuf[charCnt] = 0;
        if (charCnt > 0) {
          runCommand();
          charCnt = 0;
        }
        break;
      case '\b':
        if (charCnt >0) {
          commandBuf[--charCnt] = 0;
          //Serial << byte('\b') << byte(' ') << byte('\b');
          Serial.print(byte('\b'));
          Serial.print(byte(' '));
          Serial.print(byte('\b'));
        }
        break;
      default:
        if (charCnt < COMMAND_BUFFER_LENGTH) {
          commandBuf[charCnt++] = c;
        }
        commandBuf[charCnt] = 0;
        break;
    }
  }

  if ( changed )
  {
    changed = false;
    //printDir();
  }

//  Serial.println();
  delay(100); // 0.1 秒毎にリフレッシュ

}

// 16進数表示
void printBufHEX(String msg, uint8_t *buf, int cnt) {
  Serial.print(msg);
  Serial.print("0x");
  char tmpStr[3];
  for (int i = 0; i < cnt; i++) {
    sprintf(tmpStr, "%02X", buf[i]);
    Serial.print(tmpStr);
  }
  Serial.println();
}

// xo詳細表示
void xoDetails() {
  uint8_t dataBuf[8];
  machXO.readDeviceID(dataBuf);
  printBufHEX("Device ID:  ", dataBuf, 4);
  machXO.readUserCode(dataBuf);
  printBufHEX("User Code:  ", dataBuf, 4);
  machXO.readFeatureRow(dataBuf);
  printBufHEX("Feature Row:  ", dataBuf, 8);
  machXO.readFeatureBits(dataBuf);
  printBufHEX("Feature Bits:  ", dataBuf, 2);
  machXO.readStatus(dataBuf);
  printBufHEX("Status:  ", dataBuf, 4);
}

// xoステータス表示
void xoStatus() {
  uint8_t dataBuf[4];
  machXO.readStatus(dataBuf);
  printBufHEX("Status:  ", dataBuf, 4);
}

/*
// CFMを読み取って表示
// 機能しません
void xoReadFlash() {
  uint8_t dataBuf[16];
  machXO.resetConfigAddress();
  //machXO.setConfigAddress(0);
  machXO.readFlash(dataBuf);
  printBufHEX("Read CFM First Page:  ", dataBuf, 16);
}
*/

void xoConfig() {
  machXO.enableConfigOffline();
  Serial.println("Enabled Offline Configuration");
}

void xoRefresh() {
  machXO.refresh();
  Serial.println("Refreshed MachXO Configuration");
}

void xoErase() {
  //machXO.erase(MACHXO_ERASE_CONFIG_FLASH | MACHXO_ERASE_UFM);
  machXO.erase(MACHXO_ERASE_CONFIG_FLASH | MACHXO_ERASE_UFM);
  Serial.println("Erasing...");
  machXO.waitBusy();
  Serial.println("Erased Configuration and UFM");
}

// プログラミング
void xoLoadHEX(char *filename) {
  if (!file.open(filename)) {
      Serial.print("Failed to open ");
      Serial.println(filename);
      return;    
  } else {
    Serial.print("Opened ");
    Serial.println(filename);
    Serial.println("Entering Offline Configuration");
    machXO.enableConfigOffline();
    Serial.println("Erasing Configuration and UFM");
    machXO.erase(MACHXO_ERASE_CONFIG_FLASH | MACHXO_ERASE_UFM);
    Serial.println("Waiting for erase to complete");
    machXO.waitBusy();
    Serial.print("Loading ");
    Serial.println(filename);
    machXO.loadHex(file);
    file.close();
    Serial.println("Programming DONE status bit");
    machXO.programDone(); 
    Serial.println("Refreshing image");
    machXO.refresh();
    Serial.print("Loaded ");    
    Serial.println(filename);   
  }
}

// PRGMNピンを操作する
void xoProgPin(char *prog) {
  switch (prog[0]) {
    case '0':
    case 'L':
    case 'l':
      digitalWrite(XO_PRGMN, LOW);
      pinMode(XO_PRGMN, OUTPUT);
      Serial.println("Program_N pin low");
      break;
    case '1':
    case 'H':
    case 'h':
    default:
      pinMode(XO_PRGMN, INPUT_PULLUP);
      Serial.println("Program_N pin high");
      break;  
  }
}

// Initピンを操作する
void xoInitPin(char *init) {
  switch (init[0]) {
    case '0':
    case 'L':
    case 'l':
      digitalWrite(XO_INITN, LOW);
      pinMode(XO_INITN, OUTPUT);
      Serial.println("Init_N pin low");
      break;
    case '1':
    case 'H':
    case 'h':
    default:
      pinMode(XO_INITN, INPUT_PULLUP);
      Serial.println("Init_N pin high");
      break;  
  }
}

// ディレクトリを表示
/*
void printDir() {
    if ( !root.open("/") )
    {
      Serial.println("open root failed");
      return;
    }

    Serial.println("Flash contents:");

    // Open next file in root.
    // Warning, openNext starts at the current directory position
    // so a rewind of the directory may be required.
    while ( file.openNext(&root, O_RDONLY) )
    {
      file.printFileSize(&Serial);
      Serial.write(' ');
      file.printName(&Serial);
      if ( file.isDir() )
      {
        // Indicate a directory.
        Serial.write('/');
      }
      Serial.println();
      file.close();
    }
    root.close();  
    Serial.println();
}
*/

void printHelp(){
  Serial.println("MachXO Programming Example Command Interface");
  Serial.println("Valid commands:");
  Serial.println("H/h/? - Print this help information");
  //Serial.println("D/d   - Print root directory");
  Serial.println("X/x   - MachXO Device Details");
  Serial.println("S/s   - MaxhXO Status");
  Serial.println("C/c   - MaxhXO enable offline Configuration");
  Serial.println("E/e   - MaxhXO Erase configuration and UFM");
  Serial.println("R/r   - MachXO Refresh");
  Serial.println("I/i [0/low/1/high] - MachXO INITN pin");
  Serial.println("P/p [0/low/1/high] - MachXO PROGRAMN pin");
  Serial.println("L/l [filename] - MachXO Load hex file");
  Serial.println("  The load command performs these steps:");
  Serial.println("    - Open file to confirm exists");
  Serial.println("    - Enter offline configuration mode");
  Serial.println("    - Erase Configuration and UFM");
  Serial.println("    - Load hex file into MachXO");
  Serial.println("    - Send Program Done command");
  Serial.println("    - Send Refresh command");
  Serial.println("");
  Serial.println("It may be necessary to drive PROGRAMN low to access ");
  Serial.println("the configuration port if Port Persistence is not "); 
  Serial.println("enabled in the feature bits of the MachXO2/3 device");
  Serial.println("");
}

// コマンド実行
void runCommand() {
  char * commandName = strtok(commandBuf, delimiters);
  switch (commandName[0]) {
    /*
    case 'F':
    case 'f':
      xoReadFlash();
      break;
      */
    case 'H':
    case 'h':
    case '?':
      printHelp();
      break;
    case 'X':
    case 'x':
      xoDetails();
      break;
    case 'S':
    case 's':
      xoStatus();
      break;
    case 'C':
    case 'c':
      xoConfig();
      break;
    case 'E':
    case 'e':
      xoErase();
      break;
    case 'R':
    case 'r':
      xoRefresh();
      break;
    case 'L':
    case 'l':
      xoLoadHEX(strtok(NULL, delimiters));
      //xoLoadHEX();
      break;
    case 'P':
    case 'p':
      xoProgPin(strtok(NULL, delimiters));
      break;
    case 'I':
    case 'i':
      xoInitPin(strtok(NULL, delimiters));
      break;
      /*
    case 'D':
    case 'd':
      printDir();
      break;
      */
    default:
      Serial.print("Unknown command:  ");
      Serial.println(commandName);
      printHelp();
  }
}

