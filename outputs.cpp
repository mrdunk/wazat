#include "outputs.h"
#include "config.h"

DisplaySdl::DisplaySdl(void* inputBuffer_, unsigned int* inputBufferLength_) {
  firstFrameRead = 0;
  setBuffer(inputBuffer_, inputBufferLength_);
}

DisplaySdl::~DisplaySdl() {
  displayCleanup();
}

void DisplaySdl::setBuffer(void* inputBuffer_, unsigned int* inputBufferLength_) {
  inputBuffer = inputBuffer_;
  inputBufferLength = inputBufferLength_;
}


int DisplaySdl::update(int& keyPress){
  struct jpeg_decompress_struct cinfo;
  struct jpeg_error_mgr jerr;
  cinfo.err = jpeg_std_error(&jerr);
  jpeg_create_decompress(&cinfo);

  jpeg_mem_src(&cinfo, (unsigned char*)inputBuffer, *inputBufferLength);

  jpeg_read_header(&cinfo, TRUE);
  // std::cout << cinfo.image_width << "," << cinfo.image_height << "," << cinfo.num_components << "\n";
  if(! firstFrameRead) {
    firstFrameRead = 1;
    displayInit(cinfo.image_width, cinfo.image_height);
  }

  // Create a stream based on our buffer.
  bufferStream = SDL_RWFromMem((unsigned char*)inputBuffer, *inputBufferLength);

  // Create a surface using the data coming out of the above stream.
  frame = IMG_Load_RW(bufferStream, 0);

  // Blit the surface and flip the screen.
  SDL_BlitSurface(frame, NULL, screen, &position);
  SDL_Flip(screen);

  while(SDL_PollEvent(&events)){
    switch(events.type) {
      case SDL_QUIT:
        std::cout << "SDL_QUIT" << std::endl;
        return 0;
      case SDL_KEYDOWN:
        if(events.key.keysym.unicode) {
          keyPress = events.key.keysym.unicode;
        } else {
          keyPress = events.key.keysym.sym;
        }
        break;
    }
  }
  return 1;
}

void DisplaySdl::displayInit(int width, int height){
  // Initialise everything.
  SDL_Init(SDL_INIT_VIDEO);
  IMG_Init(IMG_INIT_JPG);

  // Get the screen's surface.
  screen = SDL_SetVideoMode( width, height, 32, SDL_HWSURFACE);

  position = {.x = 0, .y = 0};
  SDL_EnableUNICODE( 1 );
}

void DisplaySdl::displayCleanup(){
  // Free everything, and unload SDL & Co.
  SDL_FreeSurface(frame);
  if(firstFrameRead) {
    SDL_RWclose(bufferStream);
  }
  IMG_Quit();
  SDL_Quit();
}

DisplayAsci::DisplayAsci(std::vector<unsigned char>& inputBuffer_,
                         unsigned int width_, unsigned int height_) {
  setBuffer(inputBuffer_, width_, height_);
  displayMenu = 0;
  currentSubMenu = 0;
  whichMenu = 0;
  cursesInit();
}

void DisplayAsci::setBuffer(std::vector<unsigned char>& inputBuffer_,
                            unsigned int width_,
                            unsigned int height_) {
  inputBuffer = &inputBuffer_;
  width = width_;
  height = height_;
}

DisplayAsci::~DisplayAsci() {
  cursesCleanup();
}

void DisplayAsci::enableMenu(int state) {
  int n_choices = sizeof(configArray) / sizeof(configArray[0]);
  if(state && !displayMenu) {
    clear();
    init_pair(7, COLOR_WHITE, COLOR_BLACK);
    init_pair(8, COLOR_GREEN, COLOR_BLACK);
    init_pair(9, COLOR_YELLOW, COLOR_BLACK);
    init_pair(10, COLOR_RED, COLOR_BLACK);

    displayMenu = 1;

    menuItems = (ITEM **)calloc(n_choices + 1, sizeof(ITEM *));
    for(int i = 0; i < n_choices; ++i) {
      menuItems[i] = new_item(configArray[i]->label, " ");
    }
    menuItems[n_choices] = (ITEM *)NULL;
    menu = new_menu((ITEM **)menuItems);
    set_menu_mark(menu, " * ");

    /* Set fore ground and back ground of the menu */
    set_menu_fore(menu, COLOR_PAIR(7) | A_REVERSE);
    set_menu_back(menu, COLOR_PAIR(8));
    set_menu_grey(menu, COLOR_PAIR(9));
    set_menu_pad(menu, COLOR_PAIR(10));

    for(int i = 0; i < n_choices; ++i) {
      menuItemsSub[i] = (ITEM **)calloc(configArray[i]->values.size() + 1, sizeof(ITEM *));
      for(unsigned int j = 0; j < configArray[i]->values.size(); ++j) {
        sprintf(configArray[i]->values[j].valueString, "%f", configArray[i]->values[j].value);
        menuItemsSub[i][j] =
          new_item(configArray[i]->values[j].label, configArray[i]->values[j].valueString);
      }
      menuItemsSub[i][configArray[i]->values.size()] = (ITEM *)NULL;
      menuSub[i] = new_menu((ITEM **)(menuItemsSub[i]));
      set_menu_mark(menuSub[i], " # ");
    
      /* Set fore ground and back ground of the menu */
      set_menu_fore(menuSub[i], COLOR_PAIR(7) | A_REVERSE);
      set_menu_back(menuSub[i], COLOR_PAIR(8));
      set_menu_grey(menuSub[i], COLOR_PAIR(9));
      set_menu_pad(menuSub[i], COLOR_PAIR(10));
    }


    window = newwin(10, 32, 2, 2);
    keypad(window, TRUE);
    set_menu_win(menu, window);
    set_menu_sub(menu, derwin(window, 6, 30, 3, 1));
    box(window, 0, 0);
    mvwaddch(window, 2, 0, ACS_LTEE);
    mvwhline(window, 2, 1, ACS_HLINE, 30);
    mvwaddch(window, 2, 31, ACS_RTEE);

    windowSub = newwin(10, 32, 2, 34);
    keypad(windowSub, TRUE);
    set_menu_win(menuSub[0], windowSub);
    set_menu_sub(menuSub[0], derwin(windowSub, 6, 30, 3, 1));
    box(windowSub, 0, 0);
    mvwaddch(windowSub, 2, 0, ACS_LTEE);
    mvwhline(windowSub, 2, 1, ACS_HLINE, 30);
    mvwaddch(windowSub, 2, 31, ACS_RTEE);

    /* Make the menu multi valued */
    menu_opts_off(menu, O_ONEVALUE);

    // Set values.
    post_menu(menu);
    post_menu(menuSub[0]);
    refresh();
    wrefresh(window);
    wrefresh(windowSub);
    for(int i = 0; i < n_choices; ++i) {
      set_item_value(menuItems[i], configArray[i]->enabled);
    }
  } else if(displayMenu) {
    displayMenu = 0;
    free_menu(menu);
    for(int i = 0; i < n_choices; ++i){
      free_item(menuItems[i]);
      free_menu(menuSub[i]);
      for(unsigned int j = 0; j < configArray[i]->values.size(); ++j){
        free_item(menuItemsSub[i][j]);
      }
    }
    endwin();
    clear();
  }
}

void DisplayAsci::enableSubMenu() {
  int index = item_index(current_item(menu));
  unpost_menu(menuSub[currentSubMenu]);
  set_menu_win(menuSub[index], windowSub);
  set_menu_sub(menuSub[index], derwin(windowSub, 6, 30, 3, 1));
  post_menu(menuSub[index]);

  currentSubMenu = index;
}

void DisplayAsci::menuOperation(int operation) {
  if(whichMenu) {
    menu_driver(menuSub[currentSubMenu], operation);
  } else {
    menu_driver(menu, operation);
  }
}

void DisplayAsci::prosessSubMenu(int keyPress) {
  int index = item_index(current_item(menuSub[currentSubMenu]));

  ConfigEntryValue* configEntry = &(configArray[currentSubMenu]->values[index]);

  if(keyPress == '+'){
    configEntry->value += configEntry->modSize;
    if(configEntry->value > configEntry->max) {
      configEntry->value = configEntry->max;
    }
  } else if(keyPress == '-'){
    configEntry->value -= configEntry->modSize;
    if(configEntry->value < configEntry->min) {
      configEntry->value = configEntry->min;
    }
  }
  sprintf(configEntry->valueString, "%f", configEntry->value);
    
  enableSubMenu();
}

void DisplayAsci::updateMenu(int keyPress){
  if(keyPress == 'm' || keyPress == 'M') {
    enableMenu(!displayMenu);
  }
  if(!displayMenu) {
    return;
  }

  int dirtyMenu = 0;

  switch(keyPress) {
    case KEY_DOWN:
    case SDLK_DOWN:
      menuOperation(REQ_DOWN_ITEM);
      enableSubMenu();
      break;
    case KEY_UP:
    case SDLK_UP:
      menuOperation(REQ_UP_ITEM);
      enableSubMenu();
      break;
    case KEY_RIGHT:
    case SDLK_RIGHT:
      mvwprintw(window, 1, 1, "        ");
      mvwprintw(windowSub, 1, 1, "Selected");
      whichMenu = 1;
      break;
    case KEY_LEFT:
    case SDLK_LEFT:
      mvwprintw(window, 1, 1, "Selected");
      mvwprintw(windowSub, 1, 1, "        ");
      whichMenu = 0;
      break;
    case ' ':
      menuOperation(REQ_TOGGLE_ITEM);
      dirtyMenu = 1;
      break;
    case '+':
    case '-':
      prosessSubMenu(keyPress);
      break;

  }

  if(dirtyMenu) {
    for(int i = 0; i < MENU_ITEMS; ++i) {
      configArray[i]->enabled = item_value(menuItems[i]);
    }
  }

  post_menu(menu);
  wrefresh(window);
  wrefresh(windowSub);
  refresh();
}

void DisplayAsci::update(int keyPress){
  updateMenu(keyPress);
  unsigned int row_stride = inputBuffer->size() / height;

  move(displayMenu * 20, 0);
  printw("%i, %i %i\n", width, height, inputBuffer->size());
  for(unsigned int line = 0; line < height; line += 15){
    for(unsigned int col = 0; col < row_stride; col += 30){
      int r = (*inputBuffer)[line * row_stride + col];
      int g = (*inputBuffer)[line * row_stride + col +1];
      int b = (*inputBuffer)[line * row_stride + col +2];

      int y = (r + g) / 1.9;
      int m = (r + b) / 1.9;
      int c = (g + b) / 1.9;
      int w = (r + g + b) / 2.8;

      if(r > g && r > b && r > y && r > m && r > c && r > w){
        // Red
        attron(COLOR_PAIR(1));
        if(r > 128){
          addstr("R");
        } else if(r > 75){
          addstr("r");
        } else if(r > 37){
          addstr(".");
        } else {
          addstr(" ");
        }
        attroff(COLOR_PAIR(1));
      } else if(g > r && g > b && g > y && g > m && g > c && g > w){
        // Green
        attron(COLOR_PAIR(2));
        if(g > 128){
          addstr("G");
        } else if(g > 75){
          addstr("g");
        } else if(g > 37){
          addstr(".");
        } else {
          addstr(" ");
        }
        attroff(COLOR_PAIR(2));
      } else if(b > r && b > g && b > y && b > m && b > c && b > w){
        // Blue
        attron(COLOR_PAIR(3));
        if(b > 128){
          addstr("B");
        } else if(b > 75){
          addstr("b");
        } else if(b > 37){
          addstr(".");
        } else {
          addstr(" ");
        }
        attroff(COLOR_PAIR(3));
      } else if(y > r && y > g && y > b && y > m && y > c && y > w){
        // Yellow
        attron(COLOR_PAIR(4));
        if(y > 128){
          addstr("Y");
        } else if(y > 75){
          addstr("y");
        } else if(y > 37){
          addstr(".");
        } else {
          addstr(" ");
        }
        attroff(COLOR_PAIR(4));
      } else if(m > r && m > g && m > b && m > y && m > c && m > w){
        // Magenta
        attron(COLOR_PAIR(5));
        if(m > 128){
          addstr("M");
        } else if(m > 75){
          addstr("m");
        } else if(m > 37){
          addstr(".");
        } else {
          addstr(" ");
        }
        attroff(COLOR_PAIR(5));
      } else if(c > r && c > g && c > b && c > y && c > m && c > w){
        // Cyan
        attron(COLOR_PAIR(6));
        if(c > 128){
          addstr("C");
        } else if(c > 75){
          addstr("c");
        } else if(c > 37){
          addstr(".");
        } else {
          addstr(" ");
        }
        attroff(COLOR_PAIR(6));
      } else if(w > r && w > g && w > b && w > y && w > m && w > c){
        //White
        if(w > 128){
          addstr("W");
        } else if(w > 75){
          addstr("w");
        } else if(w > 37){
          addstr(".");
        } else {
          addstr(" ");
        }
      } else {
        addstr(" ");
      }
    }
    addstr("\n");
  }
  mvprintw(LINES - 3, 0, "\"M\" to display menu");
  mvprintw(LINES - 2, 0, "\"Q\" to exit");
  refresh();
}

void DisplayAsci::cursesInit(){
  initscr();
  cbreak();
  timeout(50);  // 50ms wait for keypress.
  noecho();
  keypad(stdscr, TRUE);
  start_color();

  init_pair(1, COLOR_RED, COLOR_BLACK);
  init_pair(2, COLOR_GREEN, COLOR_BLACK);
  init_pair(3, COLOR_BLUE, COLOR_BLACK);
  init_pair(4, COLOR_YELLOW, COLOR_BLACK);
  init_pair(5, COLOR_MAGENTA, COLOR_BLACK);
  init_pair(6, COLOR_CYAN, COLOR_BLACK);
  
  clear();
}

void DisplayAsci::cursesCleanup(){
  endwin();
}

void saveJpeg(void* inputBuffer, unsigned int inputBufferLength){
  int jpgfile;
  if((jpgfile = open("/tmp/myimage.jpeg", O_WRONLY | O_CREAT, 0660)) < 0){
    errno_exit("open");
  }

  if(write(jpgfile, inputBuffer, inputBufferLength)){
    std::cout << "Could not write file: " << jpgfile << std::endl;
  }
  close(jpgfile);
}

boolean emptyBuffer(jpeg_compress_struct* cinfo) {
  return TRUE;
}
void init_buffer(jpeg_compress_struct* cinfo) {}
void term_buffer(jpeg_compress_struct* cinfo) {}

void makeJpeg(std::vector<unsigned char>& inputBuffer,
              void** outputBuffer, unsigned int& outputBufferLength,
              unsigned int width, unsigned int height) {

  struct jpeg_compress_struct cinfo;
  struct jpeg_error_mgr       jerr;
  struct jpeg_destination_mgr dmgr;

  /* Set up output buffer. */
  dmgr.init_destination    = init_buffer;
  dmgr.empty_output_buffer = emptyBuffer;
  dmgr.term_destination    = term_buffer;
  dmgr.next_output_byte    = (JOCTET*)(*outputBuffer);
  dmgr.free_in_buffer      = outputBufferLength;
   
  cinfo.err = jpeg_std_error(&jerr);
  jpeg_create_compress(&cinfo);
  
  cinfo.dest = &dmgr;
  cinfo.image_width      = width;
  cinfo.image_height     = height;
  cinfo.input_components = 3;
  cinfo.in_color_space   = JCS_RGB;

  jpeg_set_defaults(&cinfo);
  /*set the quality [0..100]  */
  jpeg_set_quality (&cinfo, 100, true);
  jpeg_start_compress(&cinfo, true);

  JSAMPROW row_pointer;

  /* main code to write jpeg data */
  while (cinfo.next_scanline < cinfo.image_height) {    
    row_pointer = (JSAMPROW) &inputBuffer[cinfo.next_scanline * width * 3];
    jpeg_write_scanlines(&cinfo, &row_pointer, 1);
  }

  jpeg_finish_compress(&cinfo);
}


