#include "config.h"

Config config = {};

ConfigEntry* configArray[MENU_ITEMS] = {
  &config.blurGaussian,
  &config.getFeatures,
  &config.filterThin,
  &config.filterSmallFeatures
};

