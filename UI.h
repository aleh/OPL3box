
struct OperatorValue {

protected:
  OPL3::OperatorSetup &oplOperator;

private:
  const uint8_t operatorNr;
  const int     maxValue;
  const char *  displayNameFormat;
  const char ** valueNames;
  
  virtual int    getValue() = 0;
  virtual void   setValue(int value) = 0;
  
public:
  
  OperatorValue(
    OPL3::OperatorSetup &oplOperator,
    uint8_t             operatorNr,
    int                 maxValue,
    const char          *displayName,
    const char          **valueNames = nullptr
  ):
    oplOperator(oplOperator),
    operatorNr(operatorNr),
    maxValue(maxValue),
    displayNameFormat(displayName),
    valueNames(valueNames)
  { }

  void onEncoderDelta(int delta) {
    setValue(max(0, min(maxValue - 1, getValue() + delta)));
  }

  void getParamString(char * buf, size_t len) {
    snprintf(buf, len, displayNameFormat, operatorNr);
  }
  
  void getValueString(char * buf, size_t len) {
    int          value       = getValue();
    const char * valueString = nullptr;

    if (valueNames) {
      valueString = valueNames[value];
    }
    
    if (valueString)
      snprintf(buf, len, "%s", valueString);
    else
      snprintf(buf, len, "%x", value);
  }
  
};

const char * WaveformChoices[] = {
  "Sine",
  "HalfSine",
  "AbsSine",
  "PulseSine",
};

struct WaveformValue : OperatorValue {
    
    WaveformValue(OPL3::OperatorSetup &oplOperator, uint8_t nr)
    : OperatorValue(
        oplOperator,
        nr,
        sizeof(WaveformChoices) / sizeof(char *), 
        "OP%d Waveform",
        WaveformChoices
      )
  { }
  
  int getValue() { return oplOperator.waveform; }
  void setValue(int v) { oplOperator.waveform = (OPL3::Waveform) v; }
  
};

const char * FrequencyMultiplierChoices[] = {
  "0.5",
  "1",
  "2",
  "3",
  "4",
  "5",
  "6",
  "7",
  "8",
  "9",
  "10",
  "10",
  "12",
  "12",
  "15",
  "15",
};

struct FrequencyMutliplierValue : OperatorValue {
  FrequencyMutliplierValue(OPL3::OperatorSetup &oplOperator, uint8_t nr)
    : OperatorValue(
        oplOperator,
        nr,
        sizeof(FrequencyMultiplierChoices) / sizeof(char *), 
        "OP%d Freq Mult",
        FrequencyMultiplierChoices
      )
  { }
    
  int getValue() { return oplOperator.mult; }
  void setValue(int v) { oplOperator.mult = v; }
};


static const char * BoolChoices[] = {
  "OFF",
  "ON",
};

struct EnvScalingValue : OperatorValue {
  EnvScalingValue(OPL3::OperatorSetup &oplOperator, uint8_t nr)
    : OperatorValue(
        oplOperator,
        nr,
        sizeof(BoolChoices) / sizeof(char *), 
        "OP%d Env Scale",
        BoolChoices
      )
  { }
  
  int getValue() { return oplOperator.ksr; }
  void setValue(int v) { oplOperator.ksr = v; }
};

struct SustainHoldValue: OperatorValue {
    SustainHoldValue(OPL3::OperatorSetup &oplOperator, uint8_t nr)
     : OperatorValue(
        oplOperator,
        nr,
        sizeof(BoolChoices) / sizeof(char *), 
        "OP%d Sus Hold",
        BoolChoices
      )
  { }

  int getValue() { return oplOperator.egt; }
  void setValue(int v) { oplOperator.egt = v; }
};

struct VibratoValue : OperatorValue {
  VibratoValue(OPL3::OperatorSetup &oplOperator, uint8_t nr)
     : OperatorValue(
        oplOperator,
        nr,
        sizeof(BoolChoices) / sizeof(char *), 
        "OP%d Vibrato",
        BoolChoices
      )
    { }

  int getValue() { return oplOperator.vib; }
  void setValue(int v) { oplOperator.vib = v; }
};

struct TremoloValue : OperatorValue {
  TremoloValue(OPL3::OperatorSetup &oplOperator, uint8_t nr)
     : OperatorValue(
        oplOperator,
        nr,
        sizeof(BoolChoices) / sizeof(char *), 
        "OP%d Tremolo",
        BoolChoices
      )
  { }
  
  int getValue() { return oplOperator.am; }
  void setValue(int v) { oplOperator.am = v; }
};

struct AttackValue : OperatorValue {
  AttackValue(OPL3::OperatorSetup &oplOperator, uint8_t nr)
     : OperatorValue(
        oplOperator,
        nr,
        0x10, 
        "OP%d Attack",
        NULL
      )
  { }
  
  int getValue() { return oplOperator.ar; }
  void setValue(int v) { oplOperator.ar = v; }
};

struct DecayValue : OperatorValue {
  DecayValue(OPL3::OperatorSetup &oplOperator, uint8_t nr)
     : OperatorValue(
        oplOperator,
        nr,
        0x10, 
        "OP%d Decay",
        NULL
      )
  { }

  int getValue() { return oplOperator.dr; }
  void setValue(int v) { oplOperator.dr = v; }
};

struct SustainValue : OperatorValue {
  SustainValue(OPL3::OperatorSetup &oplOperator, uint8_t nr)
     : OperatorValue(
        oplOperator,
        nr,
        0x10, 
        "OP%d Sustain",
        NULL
      )
  { }

  int getValue() { return oplOperator.sl; }
  void setValue(int v) { oplOperator.sl = v; }
};

struct ReleaseValue : OperatorValue {
  ReleaseValue(OPL3::OperatorSetup &oplOperator, uint8_t nr)
     : OperatorValue(
        oplOperator,
        nr,
        0x10, 
        "OP%d Release", 
        NULL
      )

  { }
  
  int getValue() { return oplOperator.rr; }
  void setValue(int v) { oplOperator.rr = v; }
};
