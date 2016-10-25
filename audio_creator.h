#include <SDL2/SDL.h>
#include <SDL2/SDL_thread.h>
#include <SDL2/SDL_mixer.h>

#include <list>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

// Prints a message and exits with EXIT_FAILURE.
#define FAIL(...) { fprintf(stderr, __VA_ARGS__); exit(EXIT_FAILURE); }

// A class for some basic audio functions
class Audio
{
public:
    // Opens SDL Mixer
    static void Open();

    // Closes SDL Mixer
    static void Close();

    // Plays the given audio, immediately. Does not block.
    // buf: Samples for two channels, interleaved, at the frequency given by GetFrequency.
    // len: The number of samples in channel one plus the number of samples in 
    //      channel two
    static void Play(const float * buf, size_t len);

    // Sleeps until there is no more audio playing
    static void WaitForSilence();

    // Returns the frequency to use for playing samples
    static int GetFrequency()
    {
        return 44100;
    }

private:

    // A list which keeps track of which chunks have been played, and which channel they
    // were played on. We need to manage the chunks ourselves because otherwise SDL won't
    // automatically delete them.
    typedef std::list<std::pair<Mix_Chunk*, int /* channel number */ > > ChunkList;
    static ChunkList chunks_played;
};

// Declare this again to satisfy the linker
Audio::ChunkList Audio::chunks_played;

///////////////////////////////////////////////////////////////////////////////
void Audio::Open()
{
    // Mix_Init(0) will do nothing. If you wish to play audio from files,
    // however, then you will need to send the correct argument to Mix_Init.
    Mix_Init(0);

    int rc = Mix_OpenAudio(
        GetFrequency(), // Frequency
        AUDIO_F32,      // Format. Note: several formats are not portable.
        2,              // Channels
        1024);          // Chunk size. Should not be too small.
    if (rc == -1) FAIL("Could not open audio.");

    int frequency;
    Uint16 format;
    int channels;
    Mix_QuerySpec(&frequency, &format, &channels);
    if (format != AUDIO_F32) FAIL("Audio format is not correct.")
}
///////////////////////////////////////////////////////////////////////////////
void Audio::Close()
{
    Mix_CloseAudio();

    Audio::ChunkList::iterator chunk_i = chunks_played.begin();
    while (chunk_i != chunks_played.end())
    {
        Mix_FreeChunk((*chunk_i).first);
        chunk_i++;
    }

    Mix_Quit();
}
///////////////////////////////////////////////////////////////////////////////
void Audio::Play(const float * buf, size_t len)
{
    // Delete any chunks that are no longer playing
    Audio::ChunkList::iterator chunk_i = chunks_played.begin();
    while (chunk_i != chunks_played.end())
    {
        // Test the channel on which we played the original chunk
        // to see if it is still playing the same chunk
        if (Mix_GetChunk((*chunk_i).second) != (*chunk_i).first)
        {
        	// If the channel was not playing the same chunk, then the
        	// chunk is not being played, so delete it
            Mix_FreeChunk(chunk_i->first);
            chunks_played.erase(chunk_i++);
        }
        else
        {
            chunk_i++;
        }
    }

    float* bufcpy = (float*)SDL_malloc(sizeof(*bufcpy) * len);
    memcpy(bufcpy, buf, len * sizeof(*buf));

    // Create a chunk from the data
    Mix_Chunk *chunk = Mix_QuickLoad_RAW((Uint8*) bufcpy, len * sizeof(*buf));

    // Set chunk->allocated to non-zero to cause SDL to call
    // SQL_free on the chunk's data buffer when Mix_FreeChunk is called
    chunk->allocated    = true;

    // Play the chunk
    int rc = Mix_PlayChannel(-1, chunk, 0);
    if (rc == -1)
    {
        FAIL("Could not play chunk.");
    }
    else
    {
        // Add the chunk and the channel it was played on to the list of
        // chunks we played
        chunks_played.push_back(std::pair<Mix_Chunk*, int>(chunk, rc));
    }
}
///////////////////////////////////////////////////////////////////////////////
void Audio::WaitForSilence()
{
    while (Mix_Playing(-1)) SDL_Delay(10);
}

///////////////////////////////////////////////////////////////////////////////
// Plays a sequence of tones according to strings entered by the user
// argc: ignored
// argv: ignored
///////////////////////////////////////////////////////////////////////////////
/* int main(int argc, char* argv[]) */
/* { */
/*     Audio::Open(); */

/*     int input_character; */

/*     // Retrieve a character, or stop if Ctrl+D is entered (Ctrl+Z in Windows) */
/*     printf("Type a string to play it: "); */
/*     while ((input_character = getchar()) != EOF) */
/*     { */
/*         if (input_character == '\n') */
/*         { */
/*             printf("Type a string to play it: "); */
/*             continue; */
/*         } */

/*         // Use the character to create a frequency. */
/*         float frequency = input_character * 10 + 200.0f; */
/*         printf("Playing at %f Hz\n", frequency); */

/*         // Allocate a buffer for 0.1 seconds */
/*         const size_t len = Audio::GetFrequency() / 10; */
/*         float buf[len]; */

/*         // Write a sine wave */
/*         for (size_t i = 0; i < len / 2; i++) */
/*         { */
/*             // i * 2 is the frame number (2 samples per frame, one for each channel) */

/*             // Left */
/*             buf[i * 2 + 0] = sinf(i * 2 * frequency * 2 * float(M_PI) / Audio::GetFrequency()) * 0.5f; */
            
/*             // Right */
/*             buf[i * 2 + 1] = sinf(i * 2 * frequency * 2 * float(M_PI) / Audio::GetFrequency()) * 0.5f; */
/*         } */

/*         // Play the sound */
/*         Audio::Play(buf, len); */

/*         // Wait for it to stop playing */
/*         Audio::WaitForSilence(); */
/*     } */

/*     Audio::Close(); */

/*     return EXIT_SUCCESS; */
/* } */
