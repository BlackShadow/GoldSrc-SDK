#pragma once

#include "net.h"

struct client_s;

typedef struct
{
	qboolean active;
	qboolean net_log;
	netadr_t net_address;
	void* file;
} server_log_t;

typedef struct
{
	int num_samples;
	int at_capacity;
	int at_empty;
	float capacity_percent;
	float empty_percent;
	int minusers;
	int maxusers;
	float cumulative_occupancy;
	float occupancy;
	int num_sessions;
	float cumulative_sessiontime;
	float average_session_len;
	float cumulative_latency;
	float average_latency;
} server_stats_t;

typedef struct
{
	qboolean dll_initialized; // Have we loaded the game dll.
	client_s* clients; // array of up to [maxclients] client slots.
	int maxclients; // Current max #
	int maxclientslimit; // Max allowed on server.
	int spawncount;
	int serverflags; // episode completion information
	server_log_t log;
	double next_cleartime;
	double next_sampletime;
	server_stats_t stats;
	qboolean isSecure;
} server_static_t;

extern server_static_t svs; // persistant server info
