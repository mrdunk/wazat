#ifndef WAZAT_CONFIG_H
#define WAZAT_CONFIG_H

#include <vector>

#define MENU_ITEMS 5


struct ConfigEntryValue {
  const char* label;
  double value;
  double modSize;
  double min;
  double max;
  char valueString[40];
};

struct ConfigEntry {
  const char* label;
  const void* method;
  bool enabled;
  std::vector<ConfigEntryValue> values;
};
  
struct Config {
  ConfigEntry blurGaussian =
    {"blurGaussian",
      nullptr,
      false,
      {
        {"blurDepth", 3.0, 2.0, 3.0, 7.0},
        {"blurSigma", 1.0, 0.2, 0.2, 3.0}
      }
    };
  ConfigEntry getFeatures =
    {"getFeatures",
      nullptr,
      true,
      {
        {"threshold", 50, 1, 5, 250},
        {"border", 5, 1, 1, 50}
      }
    };
  ConfigEntry filterThin =
    {"filterThin",
      nullptr,
      true,
      { }
    };
  ConfigEntry filterSmallFeatures =
    {"filterSmallFeatures",
      nullptr,
      true,
      { }
    };
  ConfigEntry hough =
    {"hough",
      nullptr,
      true,
      { 
        {"threshold", 70, 10, 10, 250},
        {"dilate count", 2, 1, 0, 20}
      }
    };
};

extern Config config;
extern ConfigEntry* configArray[MENU_ITEMS];

#endif  // WAZAT_CONFIG_H
