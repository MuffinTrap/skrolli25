#ifndef MP3_PLAY_H
#define MP3_PLAY_H

struct Mp3Song
{
    s32 data_left;
    FILE* mp3file;
};


struct Mp3Song Mp3_LoadSong(const char* filename);
void Mp3_PlaySong(struct Mp3Song* song);
void Mp3_Stop(struct Mp3Song* song);


#endif
