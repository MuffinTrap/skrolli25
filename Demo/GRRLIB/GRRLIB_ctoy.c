#include "GRRLIB_ctoy.h"
#include "GRRLIB.h"

#include <opengl_include.h>

// Wii includes
#include <ogc/lwp_watchdog.h>
#include <wiiuse/wpad.h>
#include <fat.h>
#include "mp3play.h"

#include "../rocket/rocket_ctoy.h"


#include <surface.h>
#include <display.h>

static u32 framesTotal = 0;
static double elapsedTimeS = 0.0;
static u64 deltaTimeStart;
static float deltaTimeS;

static u8 CalculateFrameRate(void) {
    static u8 frameCount = 0;
    static u32 lastTime = 0;
    static u8 FPS = 0;
    const u32 currentTime = ticks_to_millisecs(gettick());

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

    struct Mp3Song song = Mp3_LoadSong("assets/Brian-Psy_Rabbit.mp3");
    if (song.mp3file != NULL)
    {
        Mp3_PlaySong(&song);
    }

    deltaTimeStart = gettime();

    while(true)
    {
        WPAD_ScanPads();
        u32 buttonsDown = WPAD_ButtonsDown(0);
        if (buttonsDown & WPAD_BUTTON_HOME)
        {
            break;
        }
        if (ctoy_demo_over() == true)
        {
            break;
        }

		u64 now = gettime();
		deltaTimeS = (float)(now - deltaTimeStart) / (float)(TB_TIMER_CLOCK * 1000); // division is to convert from ticks to seconds
		deltaTimeStart = now;
		elapsedTimeS += deltaTimeS;

        // Do rocket udpdate
        set_rocket_track_seconds(elapsedTimeS);

        ctoy_main_loop();

    }

    Mp3_Stop(&song);
    ctoy_end();

    // Exit gracefully
	ICSync();
	VIDEO_SetBlack(TRUE);
	VIDEO_Flush();
	VIDEO_WaitVSync();
	VIDEO_WaitVSync();
	exit(0);
}



void display_init(resolution_t res, bitdepth_t bit, uint32_t num_buffers, gamma_t gamme, filter_options_t filters)
{
    // NOP

}

void ctoy_swap_buffer(struct m_image *src)
{
    glFlush();
    GRRLIB_Render();
}

unsigned long ctoy_t(void)
{
    return framesTotal;
}

double ctoy_get_time(void)
{
    return elapsedTimeS;
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
