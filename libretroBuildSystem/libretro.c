#include "libretro.h"

#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#include "../src/emulator.h"
#include "cursors.h"


#ifndef PATH_MAX_LENGTH
#if defined(__CELLOS_LV2__)
#define PATH_MAX_LENGTH CELL_FS_MAX_FS_PATH_LENGTH
#elif defined(_XBOX1) || defined(_3DS) || defined(PSP) || defined(GEKKO)|| defined(WIIU)
#define PATH_MAX_LENGTH 512
#else
#define PATH_MAX_LENGTH 4096
#endif
#endif

#ifdef _WIN32
#define path_default_slash() "\\"
#else
#define path_default_slash() "/"
#endif

#define JOYSTICK_MULTIPLIER 1.0


static retro_log_printf_t         log_cb;
static retro_video_refresh_t      video_cb;
static retro_audio_sample_batch_t audio_cb;
static retro_environment_t        environ_cb;
static retro_input_poll_t         input_poll_cb;
static retro_input_state_t        input_state_cb;

static bool      screenHires;
static uint16_t  screenWidth;
static uint16_t  screenHeight;
static uint16_t  screenData[320 * 440];
static uint32_t  emuFeatures;
static bool      useJoystickAsMouse;
static double    touchCursorX;
static double    touchCursorY;



static void renderMouseCursor(int16_t screenX, int16_t screenY){
   static const uint16_t* joystickCursor16 = cursor16x16;
   static const uint16_t* joystickCursor32 = cursor32x32;
   
   if(screenHires){
      //align cursor to side of image
      screenX -= 6;
      
      for(int8_t y = 0; y < 32; y++)
         for(int8_t x = 6; x < 26; x++)
            if(screenX + x >= 0 && screenY + y >= 0 && screenX + x < 320 && screenY + y < 440)
               if(joystickCursor32[y * 32 + x] != 0x0000)
                  screenData[(screenY + y) * 320 + screenX + x] = joystickCursor32[y * 32 + x];
   }
   else{
      //align cursor to side of image
      screenX -= 3;
      
      for(int8_t y = 0; y < 16; y++)
         for(int8_t x = 3; x < 13; x++)
            if(screenX + x >= 0 && screenY + y >= 0 && screenX + x < 160 && screenY + y < 220)
               if(joystickCursor16[y * 16 + x] != 0x0000)
                  screenData[(screenY + y) * 160 + screenX + x] = joystickCursor16[y * 16 + x];
   }
}


static void fallback_log(enum retro_log_level level, const char *fmt, ...)
{
   va_list va;

   (void)level;

   va_start(va, fmt);
   vfprintf(stderr, fmt, va);
   va_end(va);
}

static void check_variables(bool booting)
{
   struct retro_variable var = {0};

   if(booting){
      emuFeatures = FEATURE_ACCURATE;
      
      var.key = "palm_emu_feature_ram_huge";
      if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
         if (!strcmp(var.value, "true"))
            emuFeatures |= FEATURE_RAM_HUGE;
      
      var.key = "palm_emu_feature_fast_cpu";
      if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
         if (!strcmp(var.value, "true"))
            emuFeatures |= FEATURE_FAST_CPU;
      
      var.key = "palm_emu_feature_hybrid_cpu";
      if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
         if (!strcmp(var.value, "true"))
            emuFeatures |= FEATURE_HYBRID_CPU;
      
      var.key = "palm_emu_feature_320x320";
      if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
         if (!strcmp(var.value, "true"))
            emuFeatures |= FEATURE_320x320;
      
      var.key = "palm_emu_feature_synced_rtc";
      if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
         if (!strcmp(var.value, "true"))
            emuFeatures |= FEATURE_SYNCED_RTC;
      
      var.key = "palm_emu_feature_hle_apis";
      if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
         if (!strcmp(var.value, "true"))
            emuFeatures |= FEATURE_HLE_APIS;
      
      var.key = "palm_emu_feature_emu_honest";
      if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
         if (!strcmp(var.value, "true"))
            emuFeatures |= FEATURE_EMU_HONEST;
      
      var.key = "palm_emu_feature_ext_keys";
      if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
         if (!strcmp(var.value, "true"))
            emuFeatures |= FEATURE_EXT_KEYS;
   }

   var.key = "palm_emu_use_joystick_as_mouse";
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value){
      if (!strcmp(var.value, "true"))
         useJoystickAsMouse = true;
      else
         useJoystickAsMouse = false;
   }
}

void retro_init(void)
{
   enum retro_pixel_format rgb565 = RETRO_PIXEL_FORMAT_RGB565;
   
   if(environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &rgb565))
      log_cb(RETRO_LOG_INFO, "Frontend supports RGB565 - will use that instead of XRGB1555.\n");
}

void retro_deinit(void)
{
   
}

unsigned retro_api_version(void)
{
   return RETRO_API_VERSION;
}

void retro_set_controller_port_device(unsigned port, unsigned device)
{
   (void)port;
   (void)device;
}

void retro_get_system_info(struct retro_system_info *info)
{
   memset(info, 0, sizeof(*info));
   info->library_name     = "Mu";
#ifndef GIT_VERSION
#define GIT_VERSION ""
#endif
   info->library_version  = "0.86+" GIT_VERSION;
   info->need_fullpath    = false;
   info->valid_extensions = "prc|pdb|pqa";
}

void retro_get_system_av_info(struct retro_system_av_info *info)
{
   info->timing.fps = EMU_FPS;
   info->timing.sample_rate = CRYSTAL_FREQUENCY * AUDIO_DUTY_CYCLE_SIZE;

   info->geometry.base_width   = 160;
   info->geometry.base_height  = 220;
   info->geometry.max_width    = 320;
   info->geometry.max_height   = 440;
   info->geometry.aspect_ratio = 160.0 / 220.0;
}

void retro_set_environment(retro_environment_t cb)
{
   struct retro_log_callback logging;
   bool no_rom = true;

   environ_cb = cb;

   environ_cb(RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME, &no_rom);

   if (environ_cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &logging))
      log_cb = logging.log;
   else
      log_cb = fallback_log;
   
   struct retro_variable vars[] = {
      { "palm_emu_feature_ram_huge", "Extra RAM Hack; false|true" },
      { "palm_emu_feature_fast_cpu", "Overclock 2x; false|true" },
      { "palm_emu_feature_hybrid_cpu", "Extra RAM Hack; false|true" },
      { "palm_emu_feature_320x320", "Double Resolution; false|true" },
      { "palm_emu_feature_synced_rtc", "Force Match System Clock; false|true" },
      { "palm_emu_feature_hle_apis", "HLE API Implementations; false|true" },
      { "palm_emu_feature_emu_honest", "Tell Programs Their In An Emulator(for test programs); false|true" },
      { "palm_emu_feature_ext_keys", "Enable OS5 Keys; false|true" },
      { "palm_emu_use_joystick_as_mouse", "Use Left Joystick As Mouse; false|true" },
      { 0 }
   };
   environ_cb(RETRO_ENVIRONMENT_SET_VARIABLES, vars);
}

void retro_set_audio_sample(retro_audio_sample_t cb)
{
   
}

void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb)
{
   audio_cb = cb;
}

void retro_set_input_poll(retro_input_poll_t cb)
{
   input_poll_cb = cb;
}

void retro_set_input_state(retro_input_state_t cb)
{
   input_state_cb = cb;
}

void retro_set_video_refresh(retro_video_refresh_t cb)
{
   video_cb = cb;
}

void retro_reset(void)
{
   emulatorReset();
}

void retro_run(void)
{
   input_poll_cb();
   
   //touchscreen
   if(useJoystickAsMouse){
      touchCursorX += input_state_cb(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X) * (screenHires ? JOYSTICK_MULTIPLIER * 2.0 : JOYSTICK_MULTIPLIER);
      touchCursorY += input_state_cb(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y) * (screenHires ? JOYSTICK_MULTIPLIER * 2.0 : JOYSTICK_MULTIPLIER);
      
      if(touchCursorX < 0)
         touchCursorX = 0;
      else if(touchCursorX > 159)
         touchCursorX = 159;
      
      if(touchCursorY < 0)
         touchCursorY = 0;
      else if(touchCursorY > 219)
         touchCursorY = 219;
      
      palmInput.touchscreenX = touchCursorX;
      palmInput.touchscreenY = touchCursorY;
      palmInput.touchscreenTouched = input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2);
   }
   else{
      //TODO use physical touchscreen of host device
   }

   //dpad
   palmInput.buttonUp = input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP);
   palmInput.buttonRight = input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT);
   palmInput.buttonDown = input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN);
   palmInput.buttonLeft = input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT);
   palmInput.buttonSelect = input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R);
   
   //app buttons
   palmInput.buttonCalendar = input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y);
   palmInput.buttonAddress = input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X);
   palmInput.buttonTodo = input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B);
   palmInput.buttonNotes = input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A);
   
   //special buttons
   palmInput.buttonPower = input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START);
   
   //run emulator
   emulateFrame();
   memcpy(screenData, screenHires ? palmExtendedFramebuffer : palmFramebuffer, screenWidth * screenHeight * sizeof(uint16_t));
   
   //draw mouse
   if(useJoystickAsMouse)
      renderMouseCursor(touchCursorX, touchCursorY);
   
   video_cb(screenData, screenWidth, screenHeight, screenWidth * sizeof(uint16_t));
   audio_cb(palmAudio, AUDIO_SAMPLES);
}

bool retro_load_game(const struct retro_game_info *info)
{
   const char* systemDirectory;
   char palmRomPath[PATH_MAX_LENGTH];
   char palmBootloaderPath[PATH_MAX_LENGTH];
   uint8_t palmRom[ROM_SIZE];
   uint8_t palmBootloader[BOOTLOADER_SIZE];
   buffer_t rom;
   buffer_t bootloader;
   FILE* romFile;
   FILE* bootloaderFile;
   time_t rawTime;
   struct tm* timeInfo;
   
   struct retro_input_descriptor desc[] = {
      { 0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X, "Touchscreen Mouse X" },
      { 0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y, "Touchscreen Mouse Y" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2,     "Touchscreen Mouse Click" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,   "Dpad Left" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,     "Dpad Up" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,   "Dpad Down" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT,  "Dpad Right" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R,      "Dpad Middle" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START,  "Power" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,      "Notes" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,      "Todo" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X,      "Address Book" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y,      "Calender" },
      { 0 }
   };
   environ_cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, desc);
   
   environ_cb(RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY, &systemDirectory);
   strncpy(palmRomPath, systemDirectory, PATH_MAX_LENGTH);
   palmRomPath[PATH_MAX_LENGTH - 1] = '\0';
   strncat(palmRomPath, path_default_slash(), PATH_MAX_LENGTH - strlen(palmRomPath));
   strncat(palmRomPath, "palmos41-en-m515.rom", PATH_MAX_LENGTH - strlen(palmRomPath));
   strncpy(palmBootloaderPath, systemDirectory, PATH_MAX_LENGTH);
   palmBootloaderPath[PATH_MAX_LENGTH - 1] = '\0';
   strncat(palmBootloaderPath, path_default_slash(), PATH_MAX_LENGTH - strlen(palmBootloaderPath));
   strncat(palmBootloaderPath, "bootloader-en-m515.rom", PATH_MAX_LENGTH - strlen(palmBootloaderPath));
   
   romFile = fopen(palmRomPath, "rb");
   if(romFile == NULL)
      return false;
   rom.size = fread(palmRom, 1, ROM_SIZE, romFile);
   fclose(romFile);
   rom.data = palmRom;
   
   bootloaderFile = fopen(palmBootloaderPath, "rb");
   if(bootloaderFile != NULL){
      bootloader.size = fread(palmBootloader, 1, BOOTLOADER_SIZE, bootloaderFile);
      fclose(bootloaderFile);
      bootloader.data = palmBootloader;
   }
   else{
      bootloader.data = NULL;
      bootloader.size = 0;
   }
   
   //updates the emulator configuration
   check_variables(true);
   
   uint32_t error = emulatorInit(rom, bootloader, emuFeatures);
   if(error != EMU_ERROR_NONE)
      return false;
   
   time(&rawTime);
   timeInfo = localtime(&rawTime);
   emulatorSetRtc(timeInfo->tm_yday, timeInfo->tm_hour, timeInfo->tm_min, timeInfo->tm_sec);
   
   if(info != NULL){
      buffer_t prc;
      prc.data = (uint8_t*)info->data;
      prc.size = info->size;
      uint32_t prcSuccess = emulatorInstallPrcPdb(prc);
      if(prcSuccess != EMU_ERROR_NONE)
         return false;
   }
   
   screenHires = emuFeatures & FEATURE_320x320;
   screenWidth = screenHires ? 320 : 160;
   screenHeight = screenHires ? 440 : 220;
   touchCursorX = screenWidth / 2;
   touchCursorY = screenHeight / 2;

   return true;
}

void retro_unload_game(void)
{
   emulatorExit();
}

unsigned retro_get_region(void)
{
   return RETRO_REGION_NTSC;
}

bool retro_load_game_special(unsigned type, const struct retro_game_info *info, size_t num)
{
   (void)type;
   (void)info;
   (void)num;
   return false;
}

size_t retro_serialize_size(void)
{
   return emulatorGetStateSize();
}

bool retro_serialize(void *data, size_t size)
{
   buffer_t saveBuffer;
   
   saveBuffer.data = (uint8_t*)data;
   saveBuffer.size = size;
   
   return emulatorSaveState(saveBuffer);
}

bool retro_unserialize(const void *data, size_t size)
{
   buffer_t saveBuffer;
   
   saveBuffer.data = (uint8_t*)data;
   saveBuffer.size = size;
   
   return emulatorLoadState(saveBuffer);
}

void *retro_get_memory_data(unsigned id)
{
   return emulatorGetRamBuffer().data;
}

size_t retro_get_memory_size(unsigned id)
{
   return emulatorGetRamBuffer().size;
}

void retro_cheat_reset(void)
{
   
}

void retro_cheat_set(unsigned index, bool enabled, const char *code)
{
   (void)index;
   (void)enabled;
   (void)code;
}

