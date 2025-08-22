#include "GRRLIB_ctoy.h"
#include "GRRLIB.h"

#include <opengl_include.h>

// Wii includes
#include <ogc/lwp_watchdog.h>
#include <wiiuse/wpad.h>
#include <fat.h>
#include "mp3play.h"



#include <surface.h>
#include <display.h>

static double elapsedSeconds = 0.0;
static u32 framesTotal = 0;

static u8 CalculateFrameRate(void) {
    static u8 frameCount = 0;
    static u32 lastTime = 0;
    static u8 FPS = 0;
    const u32 currentTime = ticks_to_millisecs(gettime());

    frameCount++;
    framesTotal++;
    if(currentTime - lastTime > 1000) {
        lastTime = currentTime;
        FPS = frameCount;
        frameCount = 0;
    }
    return FPS;
}

void grrlib_init()
{

    VIDEO_Init();
    WPAD_Init();
    fatInitDefault();
    WPAD_SetDataFormat(WPAD_CHAN_0, WPAD_FMT_BTNS_ACC_IR);
    WPAD_SetVRes(0, 640, 480);


    GRRLIB_InitVideo();
    ogx_initialize();
    GRRLIB_Start();
}

int main()
{
    grrlib_init();
    ctoy_begin();

    // Wait loop for recording
    /*
    while(true)
    {
        WPAD_ScanPads();
        u32 buttonsDown = WPAD_ButtonsDown(0);
        if (buttonsDown & WPAD_BUTTON_A)
        {
            break;
        }
    }
    */

    struct Mp3Song song = Mp3_LoadSong("assets/Soulbringer - Goa Trolls.mp3");
    if (song.mp3file != NULL)
    {
        Mp3_PlaySong(&song);
    }

    // Discard first frame
    CalculateFrameRate();


    while(true)
    {
        WPAD_ScanPads();
        u32 buttonsDown = WPAD_ButtonsDown(0);
        if (buttonsDown & WPAD_BUTTON_HOME)
        {
            break;
        }

        float delta = 1.0f / (float)CalculateFrameRate();
        // Avoid 0 delta
        if (delta < 0.016f)
        {
            delta = 0.016f;
        }
        elapsedSeconds += delta;

        ctoy_main_loop();
    }

    Mp3_Stop(&song);
    ctoy_end();
}



void display_init(resolution_t res, bitdepth_t bit, uint32_t num_buffers, gamma_t gamme, filter_options_t filters)
{
    // NOP

}

void ctoy_swap_buffer(struct m_image *src)
{
    GRRLIB_Render();
}

unsigned long ctoy_t(void)
{
    return framesTotal;
}

double ctoy_get_time(void)
{
    return elapsedSeconds;
}

void ctoy_sleep(long sec, long nsec)
{
}

int ctoy_argc(void)
{
    return 0;
}

char** ctoy_argv(void)
{
    return NULL;
}

// Window management

int ctoy_window_width(void)
{
    return 640;
}

int ctoy_window_height(void)
{
    return 480;
}

int ctoy_frame_buffer_width(void)
{
    return 640;
}
int ctoy_frame_buffer_height(void)
{
    return 480;
}

void ctoy_window_size(int width, int height)
{
    // TODO
}

void ctoy_window_title(const char* title)
{
    // TODO
}

void ctoy_window_fullscreen(int state)
{
    // TODO
}
