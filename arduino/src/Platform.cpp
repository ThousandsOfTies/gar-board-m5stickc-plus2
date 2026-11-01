#include "GarStickCPlus2.h"
#include "gar_board_config.h"
#include <Arduino.h>
#include <M5Unified.h>
#include <BluetoothSerial.h>
#include <freertos/stream_buffer.h>
#include <atomic>
#include <algorithm>
#include <utility>
#include <lvgl.h>

#if !defined(CONFIG_BT_SPP_ENABLED)
#error This board adapter requires ESP32 Bluetooth Classic SPP.
#endif

namespace gar::stickc_plus2 {
namespace {
Hooks callbacks;
Hardware facts;
BluetoothSerial bluetooth;
StreamBufferHandle_t sppRx=nullptr;
std::atomic<bool> sppOverflow{false};
std::atomic<unsigned> sppEpoch{0};
std::atomic<uint32_t> confirmNumber{UINT32_MAX};
std::atomic<int> authenticationResult{0};
unsigned observedEpoch=0;
lv_disp_buf_t drawBuffer;
lv_color_t pixels[gar_board::display::width*16];
uint32_t clockAt=0,batteryAt=0;

void flush(lv_disp_drv_t* driver,const lv_area_t* area,lv_color_t* colors) {
  M5.Display.pushImage(area->x1,area->y1,area->x2-area->x1+1,area->y2-area->y1+1,
                       reinterpret_cast<lgfx::rgb565_t*>(colors));
  lv_disp_flush_ready(driver);
}
void button(m5::Button_Class& input,char key) {
  if(input.wasPressed())callbacks.button(key,1);
  if(input.wasReleased())callbacks.button(key,0);
}
void readUsb() {
  char bytes[256];
  for(int i=0;i<4 && Serial.available();++i) {
    size_t count=0;
    while(count<sizeof(bytes) && Serial.available())bytes[count++]=static_cast<char>(Serial.read());
    callbacks.receive(Link::usb,bytes,count);
  }
}
void readSpp() {
  if(!facts.sppReady)return;
  const unsigned epoch=sppEpoch.load();
  if(epoch!=observedEpoch) {
    observedEpoch=epoch;callbacks.reset(Link::spp,false);
    uint8_t discarded[256];
    while(xStreamBufferReceive(sppRx,discarded,sizeof(discarded),0)){}
  }
  facts.sppClient=bluetooth.hasClient();callbacks.hardware(facts);
  if(sppOverflow.exchange(false))callbacks.reset(Link::spp,true);
  char bytes[256];
  // Callbacks buffer bytes; the application stays on one main task.
  for(int i=0;i<4;++i) {
    const size_t count=xStreamBufferReceive(sppRx,bytes,sizeof(bytes),0);
    if(!count)break;
    if(!facts.sppClient)continue;
    if(sppOverflow.exchange(false))callbacks.reset(Link::spp,true);
    callbacks.receive(Link::spp,bytes,count);
  }
}
}
void begin(const Options& options,Hooks hooks) {
  callbacks=std::move(hooks);
  pinMode(gar_board::power::hold_gpio,OUTPUT);digitalWrite(gar_board::power::hold_gpio,HIGH);
  Serial.begin(gar_board::usb::baud);
  auto config=M5.config();config.internal_mic=false;config.internal_spk=false;
  config.fallback_board=m5::board_t::board_M5StickCPlus2;M5.begin(config);
  M5.Display.setRotation(options.rotation);M5.Display.setBrightness(options.brightness);M5.Display.fillScreen(TFT_BLACK);
  facts.battery=std::max(0,static_cast<int>(M5.Power.getBatteryLevel()));
  facts.width=M5.Display.width();facts.height=M5.Display.height();facts.board=static_cast<int>(M5.getBoard());
  lv_init();lv_disp_buf_init(&drawBuffer,pixels,nullptr,gar_board::display::width*16);
  lv_disp_drv_t driver;lv_disp_drv_init(&driver);driver.hor_res=facts.width;driver.ver_res=facts.height;
  driver.buffer=&drawBuffer;driver.flush_cb=flush;lv_disp_drv_register(&driver);
  sppRx=xStreamBufferCreate(16384,1);
  if(sppRx) {
    bluetooth.register_callback([](esp_spp_cb_event_t event,esp_spp_cb_param_t*) {
      if(event==ESP_SPP_SRV_OPEN_EVT || event==ESP_SPP_CLOSE_EVT)sppEpoch.fetch_add(1);
    });
    bluetooth.onData([](const uint8_t* data,size_t size) {
      if(xStreamBufferSend(sppRx,data,size,0)!=size)sppOverflow.store(true);
    });
    bluetooth.enableSSP();
    bluetooth.onConfirmRequest([](uint32_t number) { confirmNumber.store(number); });
    bluetooth.onAuthComplete([](bool success) { authenticationResult.store(success?1:-1); });
    facts.sppReady=bluetooth.begin(options.bluetoothName);
  }
  clockAt=millis();
}
void poll() {
  const auto now=millis();lv_tick_inc(now-clockAt);clockAt=now;
  M5.update();
  const auto code=confirmNumber.exchange(UINT32_MAX);
  if(code!=UINT32_MAX)callbacks.pairing(code);
  const auto result=authenticationResult.exchange(0);
  if(result)callbacks.authenticated(result>0);
  readUsb();readSpp();
  button(M5.BtnA,'A');button(M5.BtnB,'B');button(M5.BtnPWR,'C');
  if(M5.BtnPWR.pressedFor(gar_board::power::off_hold_ms))M5.Power.powerOff();
  if(now-batteryAt>=10000) {
    batteryAt=now;facts.battery=std::max(0,static_cast<int>(M5.Power.getBatteryLevel()));callbacks.hardware(facts);
  }
}
Hardware hardware() { return facts; }
void send(Link link,const std::string& bytes) {
  if(link==Link::usb)Serial.write(reinterpret_cast<const uint8_t*>(bytes.data()),bytes.size());
  else if(facts.sppReady && bluetooth.hasClient())
    bluetooth.write(reinterpret_cast<const uint8_t*>(bytes.data()),bytes.size());
}
void confirmPairing(bool accepted) { bluetooth.confirmReply(accepted); }
}
