#include <vector>

struct ConfigEntry {
  bool value;
  const char* label;
};

struct Config {
  ConfigEntry blurGaussian;
  ConfigEntry filterThin;
  ConfigEntry filterSmallFeatures;
};

extern Config config;
extern ConfigEntry* configArray[3];


