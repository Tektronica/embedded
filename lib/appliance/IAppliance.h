#pragma once

// Plugin contract: the app loop drives every appliance through this interface. Implementations
// are selected at compile time (one appliance per build for now). The destructor is protected
// and non-virtual on purpose — appliances are never owned or deleted through this base, so we
// avoid the vtable/dtor overhead a virtual destructor would add on an 8-bit MCU.
class IAppliance {
 public:
  virtual void begin() = 0;
  virtual void update() = 0;   // read inputs, advance state
  virtual void render() = 0;   // draw current state to outputs

 protected:
  ~IAppliance() = default;
};
