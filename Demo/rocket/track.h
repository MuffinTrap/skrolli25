#ifndef SYNC_TRACK_H
#define SYNC_TRACK_H
#define TRACK_VERSION 1

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MRAPI
#define MRAPI extern
#endif

#include <string.h>
#include <stdlib.h>
#include "base.h"

enum key_type {
	KEY_STEP,   /* stay constant */
	KEY_LINEAR, /* lerp to the next value */
	KEY_SMOOTH, /* smooth curve to the next value */
	KEY_RAMP,
	KEY_TYPE_COUNT
};

struct track_key {
	int row;
	float value;
	enum key_type type;
};

struct sync_track {
	char *name;
	struct track_key *keys;
	int num_keys;
};

int sync_find_key(const struct sync_track *, int);
static inline int key_idx_floor(const struct sync_track *t, int row)
{
	int idx = sync_find_key(t, row);
	if (idx < 0)
		idx = -idx - 2;
	return idx;
}

MRAPI void start_save_sync(const char *filename);
MRAPI void save_sync(const struct sync_track *t, const char *filename);
MRAPI void end_save_sync(const char *filename);

#ifndef SYNC_PLAYER
int sync_set_key(struct sync_track *, const struct track_key *);
int sync_del_key(struct sync_track *, int);
static inline int is_key_frame(const struct sync_track *t, int row)
{
	return sync_find_key(t, row) >= 0;
}

#ifdef __cplusplus
}
#endif
#endif /* !defined(SYNC_PLAYER) */

#endif /* SYNC_TRACK_H */

#ifdef TRACK_IMPLEMENTATION
MRAPI void start_save_sync(const char *filename) {
    printf("Saving to JSON file: %s\n", filename);
    FILE *file = fopen(filename, "w");
    if (file) {
        fprintf(file, "{\"tracks\":[\n");
        fclose(file);
    }
}
MRAPI void save_sync(const struct sync_track *t, const char *filename) {
    FILE *file = fopen(filename, "a");
    if (!file) return;

    static int first_track = 1;
    if (!first_track) {
        fprintf(file, ",\n");
    } else {
        first_track = 0;
    }

    fprintf(file, "  {\"name\":\"%s\", \"keys\":[", t->name);
    for (int i = 0; i < t->num_keys; i++) {
        if (i > 0) fprintf(file, ",");
        fprintf(file, "{\"row\":%d,\"value\":%f,\"type\":%d}",
                t->keys[i].row, t->keys[i].value, t->keys[i].type);
    }
    fprintf(file, "]}");
    fclose(file);
}
MRAPI void end_save_sync(const char *filename) {
    FILE *file = fopen(filename, "a");
    if (file) {
        fprintf(file, "\n]}\n");
        fclose(file);
    }
}
#endif /* TRACK_IMPLEMENTATION */
