#include "rocket_ctoy.h"

#include <stdbool.h>
#include <stdio.h>

#include "device.h"
#include "sync.h"
#include "track.h"



static struct sync_device *rocket;
// Array to hold Rocket variables
static RocketVariable rocketVariables[MAX_VARIABLES];

static double bpm = 125, rpb = 8;
static double row_rate;
static double row;
static int rocket_initialized = 0;

struct sync_device * initialize_rocket_device() {
    if (rocket_initialized == 0) {
        rocket = sync_create_device("sync");
        if (!rocket) {
            printf("Out of memory?\n");
            return NULL;
        }

        #ifndef SYNC_PLAYER
        if (sync_tcp_connect(rocket, "localhost", SYNC_DEFAULT_PORT)) {
            printf("Failed to connect to host\n");
        }
        #endif
        rocket_initialized = 1;
    }
    return rocket;
}
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int rocketVariableCount = 0;  // Number of variables currently stored

// Function to add a new variable to the dictionary
unsigned short add_to_rocket(const char *name) {
    // Check if the variable already exists
    for (int i = 0; i < rocketVariableCount; i++) {
        if (strcmp(rocketVariables[i].name, name) == 0) {
            return i;
        }
    }

    // Add a new variable if it doesn't exist
    if (rocketVariableCount < MAX_VARIABLES) {
        strncpy(rocketVariables[rocketVariableCount].name, name, MAX_NAME_LENGTH);
        // Get and store the track pointer once during initialization
        rocketVariables[rocketVariableCount].track = sync_get_track(rocket, name);
        rocketVariableCount++;
    } else {
        printf("Rocket variable storage full! Cannot add more variables.\n");
    }
    return rocketVariableCount - 1;
}

// Function to retrieve the value of a variable from the dictionary
float get_from_rocket(unsigned short id) {
    if (id >= rocketVariableCount || !rocketVariables[id].track) {
        return 0.0f;  // Return default value for invalid ID or missing track
    }
    // Get the current value from the track using the current row
    return sync_get_val(rocketVariables[id].track, row);
}

void reset_rocket_device() {
    if (rocket_initialized == 1) {
        sync_destroy_device(rocket);
        rocket_initialized = 0;
    }
}
void set_BPM(double val) {
    bpm = val;
    row_rate = (bpm / 60.0) * rpb;
    printf("bpm %.2f rpb %.2f row_rate %.2f\n", bpm, rpb, row_rate);
}
void set_RPB(double val) {
    rpb = val;
    row_rate = (bpm / 60.0) * rpb;
    printf("bpm %.2f rpb %.2f row_rate %.2f\n", bpm, rpb, row_rate);
}

void set_rocket_track_seconds(double elapsed_seconds)
{
    row = elapsed_seconds * row_rate;
}

float get_rocket_track_seconds(void)
{
    return row/row_rate;
}
