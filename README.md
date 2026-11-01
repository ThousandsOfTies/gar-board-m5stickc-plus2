# gar-board-m5stickc-plus2

M5Stack StickC-Plus2の基板情報とArduino用I/Oアダプターです。
Productがsubmoduleで固定し、gar-toolsのcompositionから利用します。
初代StickC / StickC Plus用の設定ではありません。

- `board.json`: 基板とプロファイルの選択肢。
- `profiles/arduino-esp32.json`: LCD・ボタン・電源端子、Flash、固定版ビルド依存。
- `arduino/`: LCD、USB、SPP、電池、ボタン、時計のI/O。JSONLやアプリ画面を解釈しません。

資源番号の正本はプロファイルです。gar-toolsがPlatformIOの`[board]`設定と
`gar_board_config.h`を生成します。SDKやコンパイラー本体は同梱しません。
M5UnifiedがLCD・ボタンの実際の初期化を担当します。

ProductはBluetoothの表示名、画面の向き・明るさ、アプリとI/Oの接続を指定します。
SPPのバイト列・切断・オーバーフローとペアリング通知をcallbackへ渡し、
ペアリングを許可するかどうかはアプリが決めます。Wi-Fi/LAN機能は実装していません。

資源の出典は[M5Stack公式資料](https://docs.m5stack.com/en/core/StickC-Plus2)です。
対応する内蔵部品の範囲だけを記載しており、全端子表ではありません。
この抽出後の実機書き込み試験は未実施です。
