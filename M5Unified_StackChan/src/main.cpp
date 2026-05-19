#include <Arduino.h>
#include <M5Unified.h>
#include "AudioOutputM5Speaker.h"
#include <AudioFileSourceSD.h>
#include <AudioGeneratorMP3.h>
#include <AudioFileSourceBuffer.h>
#include <Avatar.h> // https://github.com/meganetaaan/m5stack-avatar
#include <Stackchan_system_config.h> // https://github.com/stack-chan/stackchan-arduino
#include <Stackchan_servo.h> // https://github.com/stack-chan/stackchan-arduino
#if defined(ARDUINO_M5STACK_CORES3)
  #include <gob_unifiedButton.hpp>
  goblib::UnifiedButton unifiedButton;
#endif

#define USE_SERVO
#ifdef USE_SERVO
#if defined(ARDUINO_M5STACK_Core2)
  // #define SERVO_PIN_X 13  //Core2 PORT C
  // #define SERVO_PIN_Y 14
 #define SERVO_PIN_X 13  //Core2 PORT A
 #define SERVO_PIN_Y 14
#elif defined( ARDUINO_M5STACK_FIRE )
  #define SERVO_PIN_X 21
  #define SERVO_PIN_Y 22
#elif defined( ARDUINO_M5Stack_Core_ESP32 )
  #define SERVO_PIN_X 21
  #define SERVO_PIN_Y 22
#elif defined( ARDUINO_M5STACK_CORES3 )
  #define SERVO_PIN_X 1
  #define SERVO_PIN_Y 2
#endif
#endif

/// set your wav filename
const int maxFile = 100;
String fileList[maxFile];
int fileCount = 0;

static constexpr size_t WAVE_SIZE = 320;
static AudioOutputM5Speaker out(&M5.Speaker, m5spk_virtual_channel);
static AudioGeneratorMP3 mp3;
static AudioFileSourceSD *file = nullptr;
static AudioFileSourceBuffer *buff = nullptr;
const int preallocateBufferSize = 100*1024;
uint8_t *preallocateBuffer;

using namespace m5avatar;
Avatar avatar;
StackchanSystemConfig system_config;

void stop(void)
{
  if (file == nullptr) return;
  out.stop();
  mp3.stop();
//  id3->RegisterMetadataCB(nullptr, nullptr);
//  id3->close();
  file->close();
  delete file;
  file = nullptr;
}

void play(const char* fname)
{
  Serial.printf("play file fname = %s\r\n", fname);
  if (file != nullptr) { stop(); }
  file = new AudioFileSourceSD(fname);
  buff = new AudioFileSourceBuffer(file, preallocateBuffer, preallocateBufferSize);
//  wav.begin(file, &out);
  mp3.begin(buff, &out);
  delay(10);
  while (mp3.isRunning())
  {
//    while(wav.loop()) {delay(1);}
    while(mp3.loop()) {}
      mp3.stop(); 
      file->close();
      delete file;
      delete buff;
      file = nullptr;
      buff = nullptr;
      avatar.setExpression(Expression::Neutral);
//    }
  }
}

#ifdef USE_SERVO
static constexpr uint32_t SERVO_MOVE_DURATION_MS = 500;
StackchanSERVO stackchanServo;
#endif
static fft_t fft;
static int16_t raw_data[WAVE_SIZE * 2];
static float lipsync_level_max = 10.0f; // リップシンクの上限初期値
float mouth_ratio = 0.0f;

void lipSync(void *args)
{
  float gazeX, gazeY;
  int level = 0;
  DriveContext *ctx = (DriveContext *)args;
  Avatar *avatar = ctx->getAvatar();
   for (;;)
  {
    uint64_t level = 0;
    auto buf = out.getBuffer();
    if (buf) {
     memcpy(raw_data, buf, WAVE_SIZE * 2 * sizeof(int16_t));
      fft.exec(raw_data);
      for (size_t bx = 5; bx <= 60; ++bx) { // リップシンクで抽出する範囲はここで指定(低音)0〜64（高音）
        int32_t f = fft.get(bx);
        level += abs(f);
        //Serial.printf("bx:%d, f:%d\n", bx, f) ;
      }
      //Serial.printf("level:%d\n", level >> 16);
    }

    // スレッド内でログを出そうとすると不具合が起きる場合があります。
    //Serial.printf("data=%d\n\r", level >> 16);
    mouth_ratio = (float)(level >> 16)/lipsync_level_max;
    if (mouth_ratio > 1.2f) {
      if (mouth_ratio > 1.5f) {
        lipsync_level_max += 10.0f; // リップシンク上限を大幅に超えるごとに上限を上げていく。
      }
      mouth_ratio = 1.2f;
    }
    avatar->setMouthOpenRatio(mouth_ratio);

    vTaskDelay(50/portTICK_PERIOD_MS);
  }
}

void servo(void *args)
{
  float gazeX, gazeY;
  DriveContext *ctx = (DriveContext *)args;
  Avatar *avatar = ctx->getAvatar();
   for (;;)
  {
#ifdef USE_SERVO
    avatar->getGaze(&gazeY, &gazeX);
    int servoX = system_config.getServoInfo(AXIS_X)->start_degree + (int)(20.0 * gazeX);
    int servoY = system_config.getServoInfo(AXIS_Y)->start_degree;
    if(gazeY < 0) {
      int tmp = (int)(15.0 * gazeY);
      if(tmp > 15) tmp = 15;
      servoY += tmp;
    } else {
      servoY += (int)(10.0 * gazeY);
    }
    servoX = constrain(servoX, system_config.getServoInfo(AXIS_X)->lower_limit, system_config.getServoInfo(AXIS_X)->upper_limit);
    servoY = constrain(servoY, system_config.getServoInfo(AXIS_Y)->lower_limit, system_config.getServoInfo(AXIS_Y)->upper_limit);
    stackchanServo.moveXY(servoX, servoY, SERVO_MOVE_DURATION_MS);
#endif
    delay(5000);
  }
}

static void speachTask(void*)
{
}

void Servo_setup() {
#ifdef USE_SERVO
  stackchanServo.begin(system_config.getServoInfo(AXIS_X)->pin,
                       system_config.getServoInfo(AXIS_X)->start_degree,
                       system_config.getServoInfo(AXIS_X)->offset,
                       system_config.getServoInfo(AXIS_Y)->pin,
                       system_config.getServoInfo(AXIS_Y)->start_degree,
                       system_config.getServoInfo(AXIS_Y)->offset,
                       (ServoType)system_config.getServoType(),
                       &M5.In_I2C);
#endif
}

void config_read()
{
  int time_out = 0;
  while (false == SD.begin(GPIO_NUM_4, SPI, 15000000)) {
    if(time_out++ > 6) break;
    Serial.println("SD Wait...");
    M5.Lcd.println("SD Wait...");
    delay(500);
  }
  system_config.loadConfig(SD, "");
}

void file_read()
{
 // SDカードマウント待ち
 int time_out = 0;
  while (false == SD.begin(GPIO_NUM_4, SPI, 15000000)) {
    if(time_out++ > 6) return;
    Serial.println("SD Wait...");
    M5.Lcd.println("SD Wait...");
    delay(500);
  }
  File root = SD.open("/mp3");
  if (root) {
    File file = root.openNextFile();
    while (file) {
      if (file.isDirectory()) {
        // Dir skip
      } else {
        // File
        String filename = file.name();
        String dirname = "/mp3/";
        Serial.println(filename);
//        M5.Lcd.println(filename.indexOf(".wav"));
        if (filename.indexOf("mp3") != -1) {
          // Find
          fileList[fileCount] = dirname + filename;
          fileCount++;
          if (maxFile <= fileCount) {
            break;
          }
        }
      }
      file = root.openNextFile();
    }
    root.close();
  }

  Serial.println("File List");
  M5.Lcd.println("File List");
  for (int i = 0; i < fileCount; i++) {
    Serial.println(fileList[i]);
    M5.Lcd.println(fileList[i]);
  }
  delay(2000);
}

void setup() {
  
  auto cfg = M5.config();

  cfg.external_spk = true;    /// use external speaker (SPK HAT / ATOMIC SPK)
//cfg.external_spk_detail.omit_atomic_spk = true; // exclude ATOMIC SPK
//cfg.external_spk_detail.omit_spk_hat    = true; // exclude SPK HAT

  M5.begin(cfg);
#if defined( ARDUINO_M5STACK_CORES3 )
  unifiedButton.begin(&M5.Display, goblib::UnifiedButton::appearance_t::transparent_all);
#endif

  preallocateBuffer = (uint8_t *)malloc(preallocateBufferSize);
  if (!preallocateBuffer) {
    M5.Display.printf("FATAL ERROR:  Unable to preallocate %d bytes for app\n", preallocateBufferSize);
    for (;;) { delay(1000); }
  }

  { /// custom setting
    auto spk_cfg = M5.Speaker.config();
    /// Increasing the sample_rate will improve the sound quality instead of increasing the CPU load.
    spk_cfg.sample_rate = 48000; // default:64000 (64kHz)  e.g. 48000 , 50000 , 80000 , 96000 , 100000 , 128000 , 144000 , 192000 , 200000
//    spk_cfg.sample_rate = 48000; // default:64000 (64kHz)  e.g. 48000 , 50000 , 80000 , 96000 , 100000 , 128000 , 144000 , 192000 , 200000
    //spk_cfg.task_priority = configMAX_PRIORITIES - 2;
    // 音声が途切れる場合は下記3つのパラメータを調整してみてください。（あまり増やすと動かなくなる場合あり）
    spk_cfg.task_priority = 1;
    spk_cfg.dma_buf_count = 20;
    spk_cfg.dma_buf_len = 512;
    spk_cfg.task_pinned_core = PRO_CPU_NUM;
    M5.Speaker.config(spk_cfg);
  }
  M5.begin(cfg);

  M5.Lcd.clear();
  M5.Lcd.setCursor(0,0);
  M5.Lcd.setTextSize(2);
  M5.Speaker.begin();
  config_read();
  M5.Speaker.setChannelVolume(m5spk_virtual_channel, system_config.getBluetoothSetting()->start_volume);
  M5.Speaker.setVolume(system_config.getBluetoothSetting()->start_volume);

  M5.Speaker.tone(2000, 100);
  Servo_setup();
  delay(1000);
  file_read();
  delay(100);

  avatar.init();
  avatar.addTask(lipSync, "lipSync");
  avatar.addTask(servo, "servo");
//  xTaskCreateUniversal(speachTask, "speachTask", 4096, nullptr, 1, nullptr, APP_CPU_NUM);
}

void loop() {
  M5.update();
#if defined( ARDUINO_M5STACK_CORES3 )
  unifiedButton.update();
#endif
  float gazeX, gazeY;
  int data_index = 0;
  avatar.getGaze(&gazeY, &gazeX);
  if(!mp3.isRunning()) {
    data_index = random(0, fileCount);
    Serial.printf("data_index = %d fileCount = %d \r\n", data_index, fileCount);
    if(data_index < fileCount){
      avatar.setExpression(Expression::Happy);
      // Serial.printf("data_index-data_num = %d\r\n", data_index-data_num);
      play(fileList[data_index].c_str());
    }
    vTaskDelay(2000 + 1500 * random(20));
  }
}
