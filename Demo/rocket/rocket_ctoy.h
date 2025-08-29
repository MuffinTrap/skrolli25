#ifndef ROCKET_CTOY_H
#define ROCKET_CTOY_H

/* Rocket */
#define MAX_VARIABLES 512  // Adjust based on expected usage
#define MAX_NAME_LENGTH 64 // Adjust based on expected name length

// Structure to hold Rocket variable data
typedef struct {
    char name[MAX_NAME_LENGTH];
    const struct sync_track* track;  // Store track pointer for efficient access
} RocketVariable;


unsigned short add_to_rocket(const char *name);
float get_from_rocket(unsigned short id);
void set_BPM(double val);
void set_RPB(double val);
float get_rocket_track_seconds(void);

#endif
