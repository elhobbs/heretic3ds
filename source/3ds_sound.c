#include "DoomDef.h"
#include "sounds.h"
#include "soundst.h"


int snd_card = 1;
int mus_card = 1;
int idmusnum;

int default_numChannels;

int snd_MaxVolume = 120;
int snd_SfxVolume = 15;
int snd_MusicVolume = 15;

int nosfxparm = 0;
int nomusicparm = 0;

int snd_samplerate = 11025;

// music currently being played
static musicinfo_t *mus_playing;

extern musicinfo_t S_music[];

//============================================================

void S_PauseSound(void) {
}

void S_ResumeSound(void) {
}

void S_SetSfxVolume(int volume) {
	snd_MaxVolume = volume*8;
}


//============================================================
void S_StartMusic(int music_id) {
	//jff 1/22/98 return if music is not enabled
	if (!mus_card || nomusicparm)
		return;
	S_StartSong(music_id, false);
}

void S_StopMusic(void)
{
	//jff 1/22/98 return if music is not enabled
	if (!mus_card || nomusicparm)
		return;

	if (mus_playing)
	{
		//if (mus_paused)
		//	I_ResumeSong(mus_playing->handle);

		mus_stop_music();
		if (mus_playing->data) {
			Z_ChangeTag(mus_playing->data, PU_CACHE); // cph - release the music data
		}

		mus_playing->data = 0;
		mus_playing = 0;
	}
}


void S_StartSong(int music_id, boolean looping) {
	musicinfo_t *music;
	int music_file_failed; // cournia - if true load the default MIDI music
	char* music_filename;  // cournia

	//jff 1/22/98 return if music is not enabled
	if (!mus_card || nomusicparm)
		return;

	if (music_id < 0 || music_id >= NUMMUSIC) {
		return;
	}

	music = &S_music[music_id];

	if (mus_playing == music)
		return;

	// shutdown old music
	S_StopMusic();

	// get lumpnum if neccessary
	if (!music->lumpnum)
	{
		music->lumpnum = W_GetNumForName(music->name);
	}
	music->data = W_CacheLumpNum(music->lumpnum, PU_MUSIC);
	printf("mus: %s %08x\n", music->name, music->data);
	mus_play_music(music->data);

	mus_playing = music;
}

void S_SetMusicVolume() {
	if (!mus_card || nomusicparm)
		return;
	mus_update_volume();
}

void S_Start(void) {

	if (!mus_card || nomusicparm)
		return;

	// kill all playing sounds at start of level
	//  (trust me - a good idea)

	S_Stop();

	//jff 1/22/98 return if music is not enabled
	if (!mus_card || nomusicparm)
		return;

	S_StartSong((gameepisode - 1) * 9 + gamemap - 1, true);
}

//============================================================

void MIX_UpdateSounds(mobj_t *listener);
void MIX_init();
void MIX_exit();
void mux_exit();

void S_UpdateSounds(mobj_t* listener) {
	if (!snd_card || nosfxparm)
		return;
	MIX_UpdateSounds(listener);
}
void mus_init();
extern int audio_initialized;


void S_SetMaxVolume(boolean fullprocess)
{
#if 0
	int i;

	if (!fullprocess)
	{
		soundCurve[0] = (*((byte *)W_CacheLumpName("SNDCURVE", PU_CACHE))*(snd_MaxVolume * 8)) >> 7;
	}
	else
	{
		for (i = 0; i < MAX_SND_DIST; i++)
		{
			soundCurve[i] = (*((byte *)W_CacheLumpName("SNDCURVE", PU_CACHE) + i)*(snd_MaxVolume * 8)) >> 7;
		}
	}
#endif
}

void S_Init() {
	nosfxparm = 0;
	nomusicparm = 0;
	audio_initialized = 1;

#ifdef _3DS
	if (csndInit() != 0) {
		printf("csndInit failed!\n");
		nosfxparm = 1;
		nomusicparm = 1;
		return;
	}
#endif
	MIX_init();
	mus_init();
}

void S_ShutDown() {
	if (!audio_initialized) {
		return;
	}

	S_StopMusic();
	S_Stop();
	MIX_exit();
	mus_exit();
#ifdef _3DS
	//flush csnd command buffers
	csndExecCmds(true);
	csndExit();
	//svcSleepThread(5000000000LL);
#endif
}