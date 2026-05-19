# StackChan_mp3
独り言を言うｽﾀｯｸﾁｬﾝです。セリフをSDカードにmp3ファイルで入れておくとランダムに再生します。
<br><br>
@mongonta555 さんの[ｽﾀｯｸﾁｬﾝ M5GoBottom版組み立てキット](https://raspberrypi.mongonta.com/about-products-stackchan-m5gobottom-version/ "Title")に対応しています。<br>

サーボ制御には[stackchan-arduino](https://github.com/stack-chan/stackchan-arduino)を使用しています。サーボのピン、初期角度、オフセット、可動範囲、サーボ種別、音量はSDカード上の`/yaml/SC_BasicConfig.yaml`から読み込みます。<br>

Avatar表示は、meganetaaanさんのm5stack-avatarをベースにさせていただきました。<br>
オリジナルはこちら。<br>
An M5Stack library for rendering avatar faces <https://github.com/meganetaaan/m5stack-avator><br>

---

### M5GoBottom版ｽﾀｯｸﾁｬﾝ本体を作るのに必要な物、及び作り方 ###
こちらを参照してください。<br>
* [ｽﾀｯｸﾁｬﾝ M5GoBottom版組み立てキット](https://raspberrypi.mongonta.com/about-products-stackchan-m5gobottom-version/ "Title")<br>

---

### 必要な物 ###
* [M5Stack](http://www.m5stack.com/ "Title") (M5Stack Core2で動作確認しました。)<br>
* VSCode<br>
* PlatformIO<br>

使用しているライブラリ等は"platformio.ini"を参照してください。<br>

---

### サーボモーターと音量の設定 ###
* SDカードに`yaml`というディレクトリを作り、`SC_BasicConfig.yaml`を入れてください。<br>
* サンプル設定ファイルは`M5Unified_StackChan/data/yaml/SC_BasicConfig.yaml`にあります。<br>
* 現在のサンプル設定は`stackchan-arduino`の`M5_SCS`向けです。<br>
* PWMサーボなど別構成で使用する場合は、`SC_BasicConfig.yaml`の`servo.pin`、`servo.center`、`servo.offset`、`servo.lower_limit`、`servo.upper_limit`、`servo_type`を変更してください。<br>
* 起動時の音量は`bluetooth.start_volume`で設定します。<br>

`M5_SCS`向けの主な設定値は下記です。<br>

```yaml
servo:
  pin:
    x: 7
    y: 6
  center:
    x: 150
    y: 90
  lower_limit:
    x: 0
    y: 0
  upper_limit:
    x: 300
    y: 90
servo_type: "M5_SCS"
bluetooth:
  start_volume: 180
```

---

### 使い方 ###
* SDカードに、mp3というディレクトリを作りそこにmp3ファイルを入れておきます<br>
* SDカードに、yamlというディレクトリを作り`SC_BasicConfig.yaml`を入れておきます<br>
* mp3ファイルのサンプリング周波数は16khzか24khzにしてください。<br>
* ファイル名に全角文字や長いファイル名はつかえません。10文字以内で作成してください。<br>
* サンプルのmp3ファイルがmp3ディレクトリに入っています。<br>
* サイズの大きいMP3ファイルを再生すると途切れたり、画面の左端に短い線が表示されることがあります。<br>
この音声データは[「VOICEVOX;ずんだもん」](https://voicevox.hiroshiba.jp/ "Title")を使用して作成しました。<br>
<br><br>
