#include "command_animation.h"

String cmd_animation = "CR:0,16;IM:HappyFace_94x94.bmp";
#define RECT1 "0,0,14,14"
#define RECT2 "82,0,14,14"
#define RECT3 "0,114,14,14"
#define RECT4 "82,114,14,14"
#define RECT_ON "255,128,0"

#define RECT1ON "0,0,14,14,255,128,0"
#define RECT1OFF "0,0,14,14,0,0,0"
#define RECT2ON "82,0,14,14,255,128,0"
#define RECT2OFF "82,0,14,14,0,0,0"
#define RECT3ON "0,114,14,14,255,128,0"
#define RECT3OFF "0,114,14,14,0,0,0"
#define RECT4ON "82,114,14,14,255,128,0"
#define RECT4OFF "82,114,14,14,0,0,0"

#define IMAGE1 "HappyFace_94x94.bmp"
#define IMAGE2 "NormalFace_94x94.bmp"
#define IMAGE3 "SadFace_94x94.bmp"

#define DELAY1 "2000"
#define DELAY2 "1000"

#define IMAGE_POS "1,17"
#define SPEED_POS2 "3,94"  // 3=(96-90)/2, 94=(128-60)/2+60
#define SPEED_POS1 "25,94" // 25=(96-45)/2, 94=(128-60)/2+60
#define SPEED_FONT "FreeSerif46pt7b"
#define TEXT_FONT "FreeSerif14pt7b"
#define SLOW_POS "9,59"
#define DOWN_POS "5,88"
#define CMD_ANIMATION_DESC_SZ 76
LED_COMMAND_DESCRIPTION cmd_animation_desc[CMD_ANIMATION_DESC_SZ] = {
    //----------------------------
    // CL;DL:2000;FT:FreeSerif46pt7b;TC:0,255,0;CR:3,94;PT:37;DL:1000;
    // CL;CR:1,17;IM:HappyFace_94x94.bmp;DL:1000
    {{"CL", ""}, NULL},
    {{"DL", DELAY1}, NULL},

    {{"FT", SPEED_FONT}, NULL},
    {{"TC", "0,255,0"}, NULL},
    {{"CR", SPEED_POS2}, NULL},
    {{"PT", "37"}, NULL},
    {{"DL", DELAY2}, NULL},

    {{"CL", ""}, NULL},
    {{"CR", IMAGE_POS}, NULL},
    {{"IM", IMAGE1}, NULL},
    {{"DL", DELAY2}, NULL},
    //----------------------------
    // CL;DL:2000;FT:FreeSerif46pt7b;TC:255,128,0;CR:3,94;PT:40;
    // RT:82,0,14,14,255,128,0;RT:0,114,14,14,255,128,0;DL:1000;
    // RT:82,0,14,14,0,0,0;RT:0,114,14,14,0,0,0;RT:0,0,14,14,255,128,0;RT:82,114,14,14,255,128,0;DL:1000;
    // CL;CR:1,17;IM:NormalFace_94x94.bmp;
    // RT:82,0,14,14,255,128,0;RT:0,114,14,14,255,128,0;DL:1000;
    // RT:82,0,14,14,0,0,0;RT:0,114,14,14,0,0,0;RT:0,0,14,14,255,128,0;RT:82,114,14,14,255,128,0;DL:1000;
    {{"CL", ""}, NULL},
    {{"DL", DELAY1}, NULL},

    {{"FT", SPEED_FONT}, NULL},
    {{"TC", "255,128,0"}, NULL},
    {{"CR", SPEED_POS2}, NULL},
    {{"PT", "40"}, NULL},
    {{"RT", RECT2ON}, NULL},
    {{"RT", RECT3ON}, NULL},
    {{"DL", DELAY2}, NULL},
    {{"RT", RECT2OFF}, NULL},
    {{"RT", RECT3OFF}, NULL},
    {{"RT", RECT1ON}, NULL},
    {{"RT", RECT4ON}, NULL},
    {{"DL", DELAY2}, NULL},

    {{"CL", ""}, NULL},
    {{"CR", IMAGE_POS}, NULL},
    {{"IM", IMAGE2}, NULL},
    {{"RT", RECT2ON}, NULL},
    {{"RT", RECT3ON}, NULL},
    {{"DL", DELAY2}, NULL},
    {{"RT", RECT2OFF}, NULL},
    {{"RT", RECT3OFF}, NULL},
    {{"RT", RECT1ON}, NULL},
    {{"RT", RECT4ON}, NULL},
    {{"DL", DELAY2}, NULL},
    //----------------------------
    // CL;DL:2000;FT:FreeSerif46pt7b;TC:255,0,0;CR:3,94;PT:45;
    // RT:82,0,14,14,255,128,0;RT:0,114,14,14,255,128,0;DL:1000;
    // RT:82,0,14,14,0,0,0;RT:0,114,14,14,0,0,0;RT:0,0,14,14,255,128,0;RT:82,114,14,14,255,128,0;DL:1000;
    // CL;CR:1,17;IM:SadFace_94x94.bmp;
    // RT:82,0,14,14,255,128,0;RT:0,114,14,14,255,128,0;DL:1000;
    // RT:82,0,14,14,0,0,0;RT:0,114,14,14,0,0,0;RT:0,0,14,14,255,128,0;RT:82,114,14,14,255,128,0;DL:1000;
    // CL;FT:FreeSerif14pt7b;TC:255,0,0;CR:9,59;PT:SLOW;CR:5,88;PT:DOWN;
    // RT:82,0,14,14,255,128,0;RT:0,114,14,14,255,128,0;DL:1000;
    // RT:82,0,14,14,0,0,0;RT:0,114,14,14,0,0,0;RT:0,0,14,14,255,128,0;RT:82,114,14,14,255,128,0;DL:1000;
    {{"CL", ""}, NULL},
    {{"DL", DELAY1}, NULL},

    {{"FT", SPEED_FONT}, NULL},
    {{"TC", "255,0,0"}, NULL},
    {{"CR", SPEED_POS2}, NULL},
    {{"PT", "45"}, NULL},
    {{"RT", RECT2ON}, NULL},
    {{"RT", RECT3ON}, NULL},
    {{"DL", DELAY2}, NULL},
    {{"RT", RECT2OFF}, NULL},
    {{"RT", RECT3OFF}, NULL},
    {{"RT", RECT1ON}, NULL},
    {{"RT", RECT4ON}, NULL},
    {{"DL", DELAY2}, NULL},

    {{"CL", ""}, NULL},
    {{"CR", IMAGE_POS}, NULL},
    {{"IM", IMAGE3}, NULL},
    {{"RT", RECT2ON}, NULL},
    {{"RT", RECT3ON}, NULL},
    {{"DL", DELAY2}, NULL},
    {{"RT", RECT2OFF}, NULL},
    {{"RT", RECT3OFF}, NULL},
    {{"RT", RECT1ON}, NULL},
    {{"RT", RECT4ON}, NULL},
    {{"DL", DELAY2}, NULL},

    {{"CL", ""}, NULL},
    {{"FT", TEXT_FONT}, NULL},
    {{"TC", "255,0,0"}, NULL},
    {{"CR", SLOW_POS}, NULL},
    {{"PT", "SLOW"}, NULL},
    {{"CR", DOWN_POS}, NULL},
    {{"PT", "DOWN"}, NULL},
    {{"RT", RECT2ON}, NULL},
    {{"RT", RECT3ON}, NULL},
    {{"DL", DELAY2}, NULL},
    {{"RT", RECT2OFF}, NULL},
    {{"RT", RECT3OFF}, NULL},
    {{"RT", RECT1ON}, NULL},
    {{"RT", RECT4ON}, NULL},
    {{"DL", DELAY2}, NULL},
    //-----------------------------
};

void command_animation_init() {
  for (int i = 1; i < CMD_ANIMATION_DESC_SZ; i++) {
    cmd_animation_desc[i - 1].qe_next = &cmd_animation_desc[i];
  }
  cmd_animation_desc[CMD_ANIMATION_DESC_SZ - 1].qe_next = &cmd_animation_desc[0];
}

LED_COMMAND_DESCRIPTION *get_animation_desc() { return &cmd_animation_desc[0]; }