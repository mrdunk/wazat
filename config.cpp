#include "config.h"

Config config = {};

ConfigEntry* configArray[MENU_ITEMS] = {
  &config.blurGaussian,
  &config.filterThin,
  &config.filterSmallFeatures
};

