#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>

// Platform I/O only. No application protocol, sessions, UI or answer semantics.
namespace gar::stickc_plus2 {
enum class Link { usb, spp };
struct Hardware {
  bool powered=true, sppReady=false, sppClient=false;
  int battery=0, width=0, height=0, board=0;
};
struct Options {
  const char* bluetoothName;
  uint8_t rotation, brightness;
};
struct Hooks {
  std::function<void(Link,const char*,size_t)> receive;
  std::function<void(Link,bool)> reset;  // true: discard a broken byte stream
  std::function<void(char,int)> button;
  std::function<void(uint32_t)> pairing;
  std::function<void(bool)> authenticated;
  std::function<void(const Hardware&)> hardware;
};
void begin(const Options& options,Hooks hooks);
void poll();  // Owns the LVGL clock and hardware callbacks on the main task.
Hardware hardware();
void send(Link link,const std::string& bytes);
void confirmPairing(bool accepted);
}
