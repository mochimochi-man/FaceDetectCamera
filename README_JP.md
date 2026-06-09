# FaceDetectCamera for XIAO ESP32S3 Sense

Xiao ESP32S3 Sense の内蔵カメラを使用した、MediaPipe による顔認識を行うWebカメラです。
ESP32はカメラサーバーとして動作し、映像をブラウザに配信します。ブラウザ上で顔検出を行い、結果をJSON形式でESP32のシリアル出力に送信します。

## 機能概要

- **WiFi設定のWebブラウザ設定**
  - 初回起動時やWiFi設定が未保存の場合、自動的にアクセスポイント（AP）モードで起動
  - SSID:`Camera-Setup`に接続し、WebブラウザからWiFi設定が可能
  - 設定保存後、自動再起動してWiFi接続モードに移行

- **MediaPipe顔認識**
  - ブラウザ側でMediaPipe Face Detectionを実行
  - 検出された顔の位置・スコア・サムネイル画像をJSONでシリアル出力

- **シリアル出力制御**
  - 設定モード（APモード）時はカメラ認識結果（JSON）をシリアルに出力しません
  - カメラモード（Wi-Fi接続）時のみシリアルにJSONを出力します

## 必要なもの

- XIAO ESP32S3 Sense
- Arduino IDE（ESP32 Board Package 3.x系対応）
- 同一ネットワークからアクセスできるブラウザ（MediaPipeモデルの初回読み込みにインターネット接続が必要）

## ピン配置（XIAO ESP32S3 Sense）

内蔵カメラを使用しますので、基本的にピン配置を気にする必要はありません。

| 機能 | GPIO |
|------|------|
| XCLK | 10 |
| SIOD (SDA) | 40 |
| SIOC (SCL) | 39 |
| D7 (Y9) | 48 |
| D6 (Y8) | 11 |
| D5 (Y7) | 12 |
| D4 (Y6) | 14 |
| D3 (Y5) | 16 |
| D2 (Y4) | 18 |
| D1 (Y3) | 17 |
| D0 (Y2) | 15 |
| VSYNC | 38 |
| HREF | 47 |
| PCLK | 13 |

## 初回セットアップ手順

1. スケッチを書き込む
2. シリアルモニタを開く（115200bps）
3. 以下のような出力が表示されたら設定モードで起動しています：
   ```
   Access Point started. Connect to:
     SSID: Camera-Setup
     Password: setup1234
     Config URL: http://192.168.4.1/
   ```
4. PCやスマホのWiFi設定から `Camera-Setup` に接続（パスワード: `setup1234`）
5. ブラウザで `http://192.168.4.1/` を開く
6. WiFiのSSID・Password・送信間隔（秒）を入力して「保存して再起動」
7. 自動的に再起動し、WiFiに接続してカメラサーバーとして動作開始

## 通常動作（顔認識Webカメラ）

WiFi接続後、ブラウザでESP32のIPアドレスにアクセスします。

- **Start** ボタン: カメラ映像の取得と顔認識を開始
- **Stop** ボタン: 停止

## シリアル出力（JSON形式）

顔が検出されると、設定した間隔で以下のJSONがシリアル出力されます：

```json
{
  "faces": [
    {
      "xMin": 0.3125,
      "yMin": 0.2083,
      "width": 0.2187,
      "height": 0.2916,
      "score": 0.97
    }
  ],
  "count": 1,
  "time": {
    "year": 2026,
    "month": 6,
    "day": 8,
    "hour": 10,
    "minute": 35,
    "second": 12,
    "ms": 345,
    "iso": "2026-06-08T01:35:12.345Z"
  },
  "images": [
    "data:image/jpeg;base64,/9j/4AAQSkZJRgABAQAAAQ..."
  ]
}
```

### 各フィールドの説明

| フィールド | 説明 |
|-----------|------|
| `faces` | 検出された顔の配列 |
| `faces[].xMin` | 顔の左端の正規化座標（0.0〜1.0） |
| `faces[].yMin` | 顔の上端の正規化座標（0.0〜1.0） |
| `faces[].width` | 顔の幅の正規化座標（0.0〜1.0） |
| `faces[].height` | 顔の高さの正規化座標（0.0〜1.0） |
| `faces[].score` | 検出信頼度（0.0〜1.0） |
| `count` | 検出された顔の数 |
| `time` | 検出時刻 |
| `images` | 64x64ピクセルの顔画像（JPEG base64） |

### 座標について

`xMin`, `yMin`, `width`, `height` は画像サイズに対する比率（0.0〜1.0）です。
例えば画像幅320pxの場合、`xMin: 0.5` は160pxの位置を意味します。

### 時刻フォーマットについて

```python
# Pythonでの使用例
if data["time"]["hour"] >= 9 and data["time"]["hour"] <= 18:
    # 営業時間内の検出のみ処理
    pass

# 日付ごとに集計
date_key = f"{data['time']['year']}-{data['time']['month']:02d}-{data['time']['day']:02d}"
```

`iso` フィールドにはISO 8601形式の文字列も含まれており、標準的な日時パースライブラリでも扱えます。

## 設定のリセット

WiFi設定を消去して再度設定モードで起動したい場合は、Erase All Flash Before Sketch Upload: をEnabledにしてスケッチを書き込んでください。

## 注意事項

- 初回のみMediaPipeモデル（約2MB）のダウンロードが発生します
- 顔画像のbase64データは1枚あたり約2〜4KBです。受信バッファ上限は8KBのため、同時検出できる人数は3〜4人程度が目安です
- 送信間隔は1秒〜300秒の範囲で設定可能です（デフォルト: 15秒）

## 使用している外部サービス・ライブラリ

本プロジェクトは以下の外部サービスおよびライブラリを使用しています。

### jsDelivr CDN

Webページ内のJavaScriptライブラリ配信に [jsDelivr](https://www.jsdelivr.com/) を使用しています。
jsDelivrは無料の公開CDNサービスです。配信されるファイルのライセンスは、各パッケージのライセンス条項に準じます。

- 使用URL: `https://cdn.jsdelivr.net/npm/@mediapipe/face_detection@0.4/face_detection.js`
- サービス: https://www.jsdelivr.com/

### MediaPipe Face Detection

ブラウザ側の顔認識処理に [Google MediaPipe](https://ai.google.dev/edge/mediapipe) の Face Detection ソリューションを使用しています。

- ライセンス: Apache License 2.0
- 公式サイト: https://ai.google.dev/edge/mediapipe/solutions/vision/face_detector
- GitHub: https://github.com/google-ai-edge/mediapipe

```
Copyright 2023 The MediaPipe Authors

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
```

## ライセンス

本プロジェクト（FaceDetectCamera）は MIT ライセンスで提供されています。

開発者: うっ Uh / もちもちマン MochiMochi-Man (X: @calorie0)

```
MIT License

Copyright (c) 2026 うっ Uh / もちもちマン MochiMochi-Man (X: @calorie0)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```
