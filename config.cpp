#include "config.h"

Config config = {
  { true, "blurGaussian" },
  { true, "filterThin" },
  { true, "filterSmallFeatures" }
};

ConfigEntry* configArray[3] = {
  &config.blurGaussian,
  &config.filterThin,
  &config.filterSmallFeatures
};

