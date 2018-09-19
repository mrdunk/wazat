#ifndef WAZAT_OUTPUT_H
#define WAZAT_OUTPUT_H

#include <iostream>
#include <vector>
#include <fcntl.h>    /* For O_RDWR */
#include <unistd.h>   /* For open(), creat() */
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <jpeglib.h>
#include <curses.h>
#include <menu.h>

#include "config.h"

void errno_exit(const char *s);

class DisplaySdl {
  void* inputBuffer;
  unsigned int* inputBufferLength;
  SDL_Surface* frame;
  SDL_RWops* bufferStream;
  SDL_Surface* screen;
  SDL_Rect position;
  SDL_Event events;
  int firstFrameRead;

 public:
  DisplaySdl(void* inputBuffer_, unsigned int* inputBufferLength_);

  ~DisplaySdl();

  void setBuffer(void* inputBuffer_, unsigned int* inputBufferLength_);

  int update(int& keyPress);

 private:
  void displayInit(int width, int height);

  void displayCleanup();
};

class DisplayAsci {
  std::vector<unsigned char>* inputBuffer;
  unsigned int width;
  unsigned int height;
  int displayMenu;
  ITEM **menuItems;
  ITEM **menuItemsSub[MENU_ITEMS];
  MENU *menu;
  MENU *menuSub[MENU_ITEMS];
  WINDOW *window;
  WINDOW *windowSub;
  int currentSubMenu;
  int whichMenu;  // Currently selected menu: 0 = main menu. 1 = a sub menu.

 public:
  DisplayAsci(std::vector<unsigned char>& inputBuffer_,
              unsigned int width_, unsigned int height_);

  void setBuffer(std::vector<unsigned char>& inputBuffer_,
                 unsigned int width_, 
                 unsigned int height_);

  ~DisplayAsci();

  void update(int keyPress);
  void enableMenu(int state);
  void updateMenu(int keyPress);

 private:
  void cursesInit();
  void cursesCleanup();
  void enableSubMenu();
  void menuOperation(int operation);
  void prosessSubMenu(int keyPress);
};

void makeJpeg(std::vector<unsigned char>& inputBuffer,
              void** outputBuffer, unsigned int& outputBufferLength,
              unsigned int width, unsigned int height);

#endif  // WAZAT_OUTPUT_H
