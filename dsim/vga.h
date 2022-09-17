#define SCREEN_WIDTH    320
#define SCREEN_SEGMENT  0xA000   // start of VGA memory address
#define SCREEN_ADDRESS  ((SCREEN_SEGMENT) << 4) // Linear address of VGA
#define SCREEN_MODE     0x0013   // mode 13h = 320x200x256 chunky pixel
#define TEXT_MODE       0x0003   // mode 3h = text
#define VGA_INTERRUPT   0x10

#define PHARLAP_SCREEN  0x1c     // VGA memory selector for Phar Lap extender
