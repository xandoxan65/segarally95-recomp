/* One native race tick (code/race/race_tick.c). */
#ifndef SR_RACE_TICK_H
#define SR_RACE_TICK_H
#include <stdint.h>

/* Run one tick of the race from the current RAM and TGP state; the I/O board (sr_io_board_set) holds the controls
 * the tick's poll samples for the next one. */
void sr_race_tick(void);

/* Node handlers of the race list, owned by their modules. */
void sr_player_scene_frame(uint32_t node); /* 0x34F40, player's car */
void sr_desert_node_frame(uint32_t node);  /* 0x427A0, the desert's animated object */

/* Demo drives: keep the race clock from running out (30 s more below 15 s left), as the host reference does with
 * I960_HOST_ENDLESS_TIME=1. Off (0) for everything exact. */
extern int sr_demo_endless_time;

/* TICK_PROF=1: cycles summed per part of the race frame: 0 camera, 1 race flow (sub-state slots, binds), 2 car
 * pens and particle spawn, 3 course view, 4 particle submit and the followed car, 5 the race list (the cars'
 * physics), 6 the PRG slot (countdown, HUD). */
#define SR_TICK_PROF_PARTS 7
extern uint64_t sr_tick_prof[SR_TICK_PROF_PARTS];

#endif
