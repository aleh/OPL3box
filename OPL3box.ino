// OPL3box. Allows to control an OPL3 chip via MIDI using Arduino.
// Copyright (C) 2018, Aleh Dzenisiuk.
//
// This is made for Arduino Pro Micro, so MIDI over USB can be used, but Uno/Nano will work with pin number changes.

#include "MIDIUSB.h"

#include <a21.hpp>
using namespace a21;

/** 
 * Low level wrapper for an OPL3 chip (YM262).
 * Parameters: 
 * - `dataIO` should be `a21::PinBus<>` grouping 8 data pins of OPL3.
 * - the pins should be `a21::FastPin<>` or similar wrapping the control pins.
 */
template<
  typename dataIO,
  typename pinIC,
  typename pinCS, typename pinRD, typename pinWR, typename pinA0, typename pinA1
>
class YM262 {

protected:

  static const int delayMultiple = 2;

  static inline void _writePulse() {

    // Start the write pulse.
    pinWR::setLow();

    // Write pulse width, Tww = 100ns. Covers the data setup time as well (Twds = 10ns).
    _delay_us(delayMultiple * 0.100);

    // End of the write pulse.
    pinWR::setHigh();  

    // Write data hold time, Twdh = 20ns. Also covers address hold time (Tah = 10ns). Again, this is too short, but let's keep it.
    _delay_us(delayMultiple * 0.020);      
  }

public:

  static void write(uint16_t reg, uint8_t data) {
    
    // Always assuming that the bus is in inactive state.

    // Address mode.
    pinA0::setLow();

    // Select register set 0 or 1.
    pinA1::write((reg >> 8) & 1);

    // Address setup time, Tas = 10ns. (This is too short, but let's put it here just in case and for documentation.)
    _delay_us(delayMultiple * 0.010);

    // Chip select. Chip select write width, Tcsw = 100ns will be ensured by the width of the write pulse.
    pinCS::setLow();

    // Set up the register address. (Write data setup time, Twds=10ns, will be a part of the larger write pulse width.)
    dataIO::write(reg);

    _writePulse();

    // Set up the register data.
    dataIO::write(data);

    // Switch into the data mode.
    pinA0::setHigh();
    
    // Address setup time, Tas = 10ns.
    _delay_us(delayMultiple * 0.010);
    
    _writePulse();
  }
  
public:

  /** The clock frequency of the chip, Hz. */
  static const uint32_t F = 14318180;

  static void reset() {

    if (!pinIC::unused) {
      
      // We are assuming reset is HIGH now, but if it is LOW for some reason already, then not a problem, 
      // we'll just keep it it LOW for some more time and then release to complete the reset.
      pinIC::setOutput();
      pinIC::setLow();
  
      // The minimum reset pulse width is (400 / F) seconds, but we want a bit more just in case.
      _delay_ms(16 * 1000L * 400 / F);
      
      // The chip should have its own pull-up to drive the reset signal HIGH, so we could switch to hi-Z here, 
      // but don't want to rely on it, as it seemed too weak on the scope.
      pinIC::setInput(true);
      
    } else {

      // IC pin is not attached, let's clean the registers one by one.
      
      // Wipe OPL2 regs first.
      for (uint16_t reg = 0x01; reg <= 0xF5; reg++) {
        write(reg, 0);
      }

      // Now OPL3 ones, but need to have OPL3 mode enabled first, the regs are not writable otherwise.
      write(0x105, _BV(0));
      // Disable 4 operator modes for now.
      write(0x104, 0);
      // And wipe the test reg just in case.
      write(0x101, 0);
      for (uint16_t reg = 0x120; reg <= 0x1F5; reg++) {
        write(reg, 0);
      }
    }
  }

  static void begin() {
    
    dataIO::setOutput();

    pinCS::setOutput();
    pinCS::setHigh();
    
    pinRD::setOutput();
    pinRD::setHigh();
    
    pinWR::setOutput();
    pinWR::setHigh();
    
    pinA0::setOutput();
    pinA0::setHigh();
    
    pinA1::setOutput();
    pinA1::setHigh();

    reset();

    // Set Waveform Select Enable bit, so Waveform Select registers work.
    write(0x01, _BV(5));

    // Set the Keyboard Split bit to 1, so Key Scale Number is determined by the block number together with the bit 9 of the f-number (instead of bit 8).
    // The Key Scale Numbers are used with Key Scale Rate parameter.
    write(0x08, _BV(6));

    // Set OPL3 Mode Enable bit, so we are not in OPL2 compatibility mode.
    write(0x105, _BV(0));

    // Enable 4 operator mode for all possible 6 channels (bits 0-5).
    //! write(0x104, 0x3F);
  }

protected:

  /** Register offset for the given zero-based OPL2 operator index (0-17), i.e. offset for operators within the register set 0. */
  static inline uint8_t _offsetForOPL2Operator(uint8_t op) {
    if (op < 6)
      return op;
    else if (op < 12)
      return op - 6 + 0x08;
    else
      return op - 12 + 0x10;
  }

public:

  /** 
   * Register offset for the given zero-based OPL3 operator index (0-35). 
   * This function automatically distiniguishes between register sets 0 and 1, so you can just add 
   * the returned value to your base register (e.g. 0x80 for sustain/release), and you'll get a register 
   * suitable for write() function.
   */
  static uint16_t offsetForOperator(uint8_t op) {
    if (op < 18) {
      return _offsetForOPL2Operator(op);
    } else {
      return 0x100 + _offsetForOPL2Operator(op - 18);
    }
  }

  /** 
   * Offset for a register corresponding to one of the 18 OPL3 channels (0-17). 
   * Again, the returned value added to channel's register base (e.g. 0xA0 for f-numbers) 
   * is directly suitable for write() function. 
   */
  static uint16_t offsetForChannel(uint8_t channel) {
    return (channel < 9) ? channel : 0x100 + channel - 9;
  }

  union __attribute__((packed)) ChannelSetup {

    uint8_t regs[3];
    
    struct __attribute__((packed)) {
      
        // A0+ and B0+
        uint16_t fnumber : 10;
        uint16_t block : 3;
        uint16_t kon : 1;
        uint16_t unused : 2;

        // C0+
        uint8_t cnt : 1;
        uint8_t fb : 3;
        uint8_t cha : 1;
        uint8_t chb : 1;
        uint8_t chc : 1;
        uint8_t chd : 1; 
    };
  };

  static void setChannelFrequency(ChannelSetup& ch, uint16_t freq) {
    
    uint8_t b = 0;  
    uint32_t f = ((uint32_t)freq << 20) / (F / 288);
    while (f >= (1 << 10)) {
      f >>= 1;
      b++;
    }
    
    ch.fnumber = f;
    ch.block = b;
  }  

  static void channelKeyOn(uint8_t index, const ChannelSetup& ch) {
    uint16_t offset = offsetForChannel(index);
    write(0xA0 + offset, ch.regs[0]);
    write(0xC0 + offset, ch.regs[2]);
    write(0xB0 + offset, ch.regs[1]);
  }  

  static void channelKeyOff(uint8_t index, const ChannelSetup& ch) {
    uint16_t offset = offsetForChannel(index);
    write(0xB0 + offset, ch.regs[1]);
  }  

  enum Waveform : uint8_t {
    WaveformSine = 0,
    WaveformHalfSine = 1,
    WaveformAbsSine = 2,
    WaveformPulseSine = 3
  };

  union __attribute__((packed)) OperatorSetup {
    
    uint8_t regs[5];

    struct __attribute__((packed)) {

      // 20+
      uint8_t mult : 4;
      uint8_t ksr : 1;
      uint8_t egt : 1;
      uint8_t vib : 1;
      uint8_t am : 1;

      // 40+
      uint8_t tl : 6;
      uint8_t ksl : 2;

      // 60+
      uint8_t dr : 4;
      uint8_t ar : 4;

      // 80+
      uint8_t rr : 4;
      uint8_t sl : 4;

      // E0
      Waveform waveform; // WS
    };
  };

  static void updateOperator(uint8_t index, const OperatorSetup& op) {
    uint16_t offset = offsetForOperator(index);
    write(0x20 + offset, op.regs[0]);
    write(0x40 + offset, op.regs[1]);
    write(0x60 + offset, op.regs[2]);
    write(0x80 + offset, op.regs[3]);
    write(0xE0 + offset, op.regs[4]);
  }

public:
  
  /** Channels in 4 op melodic + percussion mode. */
  enum Channel : uint8_t {
    ChannelFourOp0,
    ChannelFourOp1,
    ChannelFourOp2,
    ChannelBD,
    ChannelSD,
    ChannelTT,
    ChannelCY,
    ChannelHH,
    ChannelFourOp3,
    ChannelFourOp4,
    ChannelFourOp5,
    ChannelTwoOp0,
    ChannelTwoOp1,
    ChannelTwoOp2
  };
};

// Instantiating it as OPL3 in this project.
typedef YM262< 
  PinBus< FastPin<14>, FastPin<10>, FastPin<9>, FastPin<8>, FastPin<7>, FastPin<6>, FastPin<5>, FastPin<4> >, // 8 pins for the data bus bits 0-7.
  UnusedPin<>, // IC# (reset), not using it on this board. The pin is connected to GND via a capacitor, so together with a built-in pull-up it will delay reset rising time.
  UnusedPin<>, // CS#, always LOW on this board, i.e. the chip is selected.
  UnusedPin<>, // RD#, always HIGH on this board, we never read from the chip.
  FastPin<15>, // WR#.
  FastPin<A1>, // A0 of the chip.
  FastPin<A0>  // A1 of the chip.
> OPL3;

// On Pro Micro the LED is attached to D5, which is a pin with internal number 30. 
// It is also connected to VCC, so the pin level should be LOW in order to light it, thus inversion.
typedef InvertedPin< FastPin<30> > DebugLED;

// We want to use a little OLED screen here and talk to it via I2C. 
// TODO: it would be better to use a hardware one.
typedef SoftwareI2C< 
  FastPin<2>, // SCK
  FastPin<3>, // SDA
  true // If true, then use buil-in pull-ups on the pins.
> I2C;

// 128x32 (i.e. 4 "pages" high).
typedef SSD1306<I2C, 4> LCD;

typedef FastPin<A2> encoder1PinA;
typedef FastPin<A3> encoder1PinB;
EC11 encoder1;

typedef SlowPin<16> encoderButton;

class OPL3box : protected a21::MIDIParser<OPL3box> {

protected:

  friend a21::MIDIParser<OPL3box>;

  typedef OPL3box Self;

  OPL3::OperatorSetup testOperator1;
  OPL3::OperatorSetup testOperator2;
  OPL3::ChannelSetup testChannel;

  static Self& getSelf() {
    static Self self = Self();
    return self;
  }

  /** @{ */
  /** MIDI */

  // TODO: use a table
  static uint16_t frequencyForNote(uint8_t note) {
    return (220.0 / 8) * pow(pow(2, 1.0 / 12), (note - 21));
  }

  void handleNoteOn(uint8_t channel, uint8_t note, uint8_t velocity) {
    
    DebugLED::setHigh();

    // Used this to test the channel mask as well.
    //~ testChannel.regs[2]++;

    OPL3::setChannelFrequency(testChannel, frequencyForNote(note));

    testChannel.kon = 1;
    OPL3::channelKeyOn(0, testChannel);
  }
  
  void handleNoteOff(uint8_t channel, uint8_t note, uint8_t velocity) {
    
    DebugLED::setLow();

    testChannel.kon = 0;
    OPL3::channelKeyOff(0, testChannel);
  }
  
  void handlePolyAftertouch(uint8_t channel, uint8_t note, uint8_t velocity) {
  }
  
  void handleControlChange(uint8_t channel, uint8_t control, uint8_t value) {
  }
  
  void handleProgramChange(uint8_t channel, uint8_t program) {
  }
  
  void handleAftertouch(uint8_t channel, uint8_t value) {
  }
  
  void handlePitchBend(uint8_t channel, uint16_t value) {
  }
  
  /** @} */

  uint16_t prevTickMillis;

  void tick() {
  }
  
public:

  static void begin() {

    Self& self = getSelf();

    DebugLED::setOutput();
    DebugLED::setHigh();

    OPL3::begin();
    
    Serial1.begin(31250);

    static_cast< MIDIParser<OPL3box>& >(self).begin();

    I2C::begin();
    LCD::begin();
    LCD::setFlippedVertically(false);
    LCD::setContrast(10);
    
    LCD::clear();
    LCD::drawTextCentered(Font8Console::data(), 0, 1, LCD::Cols, "OPL3 BOX", Font8::DrawingScale2);
    
    LCD::turnOn();
    
    self.prevTickMillis = millis();
    
    self.tick();

    // Test operators.
    self.testChannel.cnt = 0;
    self.testChannel.fb = 0;
   
    self.testChannel.cha = true;
    self.testChannel.chb = true;
        
    self.testOperator1.egt = true;
    self.testOperator1.tl = 0;
    self.testOperator1.ar = 0x5;
    self.testOperator1.dr = 0x5;
    self.testOperator1.sl = 0;
    self.testOperator1.rr = 0x3;
    self.testOperator1.mult = 0;
    self.testOperator1.waveform = OPL3::WaveformSine;

    self.testOperator2 = self.testOperator1;
    self.testOperator2.tl = 1;
    self.testOperator2.mult = 1;

    for (int i = 0; i < 18; i++) {
      OPL3::updateOperator(i + 0, self.testOperator1);
      OPL3::updateOperator(i + 3, self.testOperator2);
    }

    DebugLED::setLow();
  }

  uint8_t value;

  bool buttonPressed;

  void draw() {
      LCD::clear();
      char str[10];
      itoa(value, str, 16);
      LCD::drawTextCentered(Font8Console::data(), 0, 1, LCD::Cols, str, Font8::DrawingScale2);      
  }  
  
  static void check() {

    Self& self = getSelf();

    // Calling the tick handler without a dedicated timer for now.
    uint16_t now = millis();
    if ((uint16_t)(now - getSelf().prevTickMillis) >= 20) {
      self.prevTickMillis = now;
      self.tick();
    }
    
    if (Serial1.available()) {
      self.handleByte(Serial1.read());
    }

    bool needsRedraw = false;

    // Simplified USB MIDI for now.
    midiEventPacket_t event = MidiUSB.read();
    if (event.header != 0) {
      self.handleByte(event.byte1);
      self.handleByte(event.byte2);
      self.handleByte(event.byte3);
    }

    bool buttonPressedNow = !encoderButton::read();
    if (self.buttonPressed && !buttonPressedNow) {
      self.value = 0;
      needsRedraw = true;
    }
    self.buttonPressed = buttonPressedNow;

    EC11Event e;
    if (encoder1.read(&e)) {
      
      if (e.type == EC11Event::StepCW) {        
        self.value++;
      } else {
        self.value--;
      }
      
      needsRedraw = true;
    }

    if (needsRedraw) {

      // This is just to test the channels.
      self.testChannel.cha = (self.value >> 0) & 1;
      self.testChannel.chb = (self.value >> 1) & 1;
      
      self.draw();
    }
  }
};

void setup() {

  encoder1PinA::setInput(true);
  encoder1PinB::setInput(true);
  encoderButton::setInput(true);
  
  OPL3box::begin();
}

void loop() {
  encoder1.checkPins(encoder1PinA::read(), encoder1PinB::read());
  OPL3box::check();
}

