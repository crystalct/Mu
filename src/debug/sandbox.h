#pragma once

enum{
   SANDBOX_TEST_OS_VER = 0,
   SANDBOX_TEST_TOUCH_READ
};

void sandboxInit();
void sandboxTest(uint32_t test);
void sandboxOnOpcodeRun();
void sandboxReturn();//should only be called called by 68k code
