#ifndef TUTOR_H
#define TUTOR_H

#include "cvardef.h"
#include "tutor_base_tutor.h"

extern CBaseTutor*	TheTutor;

extern cvar_t	cv_tutor_message_repeats;
extern cvar_t	cv_tutor_debug_level;
extern cvar_t	cv_tutor_view_distance;
extern cvar_t	cv_tutor_viewable_check_interval;
extern cvar_t	cv_tutor_look_distance;
extern cvar_t	cv_tutor_look_angle;
extern cvar_t	cv_tutor_examine_time;
extern cvar_t	cv_tutor_message_minimum_display_time;
extern cvar_t	cv_tutor_message_character_display_time_coefficient;
extern cvar_t	cv_tutor_hint_interval_time;

void	InstallTutor( bool start );
void	MonitorTutorStatus();
void	Tutor_RegisterCVars();

#endif
