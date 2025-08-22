// Uses code from devkitpro's audio/mp3player example

#include <mp3player.h>
#include <asndlib.h>

#include <stdio.h>
#include <string.h>

#include "mp3play.h"

// Functions needed by Mp3player

s32 mp3_reader(void* user_data, void* buffer, s32 length)
{
    struct Mp3Song* song = (struct Mp3Song*)user_data;

    // Check if there is enough left
    s32 read_amount = length;
    if (song->data_left > 0)
    {
        read_amount = song->data_left;
    }

    // Read from file
    size_t read_bytes = fread(buffer, 1, read_amount, song->mp3file);

    // Notice if close to the end
    if (read_bytes < length)
    {
        song->data_left = read_bytes;
        if (feof(song->mp3file))
        {
            song->data_left = -1;
            // song is over, restart
            fseek(song->mp3file, 0, SEEK_SET);
        }
    }
    // return the amount read
    return (s32)read_bytes;
}

struct Mp3Song Mp3_LoadSong(const char* filename)
{
    ASND_Init();
    MP3Player_Init();
    struct Mp3Song song;
    FILE* mp3file = fopen(filename, "r");
    if (mp3file != NULL)
    {
        song.data_left = -1;
        song.mp3file = mp3file;
    }
    else
    {
        song.data_left = 0;
        song.mp3file = NULL;
    }
    return song;
}

void Mp3_PlaySong(struct Mp3Song* song)
{
    void* mp3_user_data = (void*)song;
    MP3Player_PlayFile(mp3_user_data, mp3_reader, NULL);
}

void Mp3_Stop(struct Mp3Song* song)
{
    // Stop mp3
    if (MP3Player_IsPlaying())
    {
        MP3Player_Stop();
    }
    // Close file
    if (song->mp3file != NULL)
    {
        fclose(song->mp3file);
        song->mp3file = NULL;
    }
    // Close sound system
    ASND_Pause(1);
    ASND_End();
}

