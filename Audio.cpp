#include "Globals.h"

#if __has_include("miniaudio.h")
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
#define HAVE_MINIAUDIO 1
#else
#define HAVE_MINIAUDIO 0
#endif



#if HAVE_MINIAUDIO

struct AudioVoice
{
    enum Kind { SINE_SWEEP, NOISE_BURST, CLICK, HUM } kind;
    float phase = 0.0f;
    float rumblePhase = 0.0f;
    float freqStart = 440.0f, freqEnd = 440.0f;
    float amp = 0.3f;
    float age = 0.0f, duration = 0.3f;
};

ma_device               g_audioDevice;
bool                    g_audioReady = false;
bool                    g_audioMuted = false;
std::mutex              g_audioMutex;
std::vector<AudioVoice> g_audioVoices;
unsigned int            g_audioNoiseSeed = 20260921u;

float audioNoise()
{
    g_audioNoiseSeed ^= g_audioNoiseSeed << 13;
    g_audioNoiseSeed ^= g_audioNoiseSeed >> 17;
    g_audioNoiseSeed ^= g_audioNoiseSeed << 5;
    return ((float)(g_audioNoiseSeed & 0xFFFFFF) / (float)0xFFFFFF) * 2.0f - 1.0f;
}

void audioDataCallback(ma_device* device, void* output, const void*, ma_uint32 frameCount)
{
    float* out = (float*)output;
    const float sr = (float)device->sampleRate;
    std::lock_guard<std::mutex> lock(g_audioMutex);

    for (ma_uint32 f = 0; f < frameCount; ++f)
    {
        float mix = 0.0f;
        for (AudioVoice& voice : g_audioVoices)
        {
            float t = (voice.kind == AudioVoice::HUM) ? 0.0f : (voice.age / voice.duration);
            float envelope = (voice.kind == AudioVoice::HUM) ? 1.0f : (1.0f - t);
            float sample = 0.0f;

            switch (voice.kind)
            {
            case AudioVoice::SINE_SWEEP:
            {
                float freq = voice.freqStart + (voice.freqEnd - voice.freqStart) * t;
                voice.phase += freq / sr;
                sample = sinf(voice.phase * 6.2831853f) * envelope;
                break;
            }
            case AudioVoice::NOISE_BURST:
            {
                voice.rumblePhase += voice.freqStart / sr;
                float rumble = sinf(voice.rumblePhase * 6.2831853f) * 0.6f;
                sample = (audioNoise() * 0.7f + rumble) * envelope * envelope;
                break;
            }
            case AudioVoice::CLICK:
            {
                voice.phase += voice.freqStart / sr;
                sample = (sinf(voice.phase * 6.2831853f) * 0.6f + audioNoise() * 0.4f) * envelope * envelope;
                break;
            }
            case AudioVoice::HUM:
            {
                voice.phase += voice.freqStart / sr;
                sample = sinf(voice.phase * 6.2831853f);
                break;
            }
            }

            mix += sample * voice.amp;
            voice.age += 1.0f / sr;
        }

        if (mix > 1.0f) mix = 1.0f;
        if (mix < -1.0f) mix = -1.0f;
        out[f * 2 + 0] = mix;
        out[f * 2 + 1] = mix;
    }

    for (size_t v = 0; v < g_audioVoices.size(); )
    {
        if (g_audioVoices[v].kind != AudioVoice::HUM && g_audioVoices[v].age >= g_audioVoices[v].duration)
            g_audioVoices.erase(g_audioVoices.begin() + v);
        else
            ++v;
    }
}

void audioInit()
{
    ma_device_config cfg = ma_device_config_init(ma_device_type_playback);
    cfg.playback.format = ma_format_f32;
    cfg.playback.channels = 2;
    cfg.sampleRate = 44100;
    cfg.dataCallback = audioDataCallback;

    if (ma_device_init(nullptr, &cfg, &g_audioDevice) != MA_SUCCESS)
    {
        std::cout << "Audio: could not init playback device -- continuing without sound\n";
        return;
    }
    if (ma_device_start(&g_audioDevice) != MA_SUCCESS)
    {
        std::cout << "Audio: could not start playback device -- continuing without sound\n";
        ma_device_uninit(&g_audioDevice);
        return;
    }
    g_audioReady = true;
}

void audioShutdown()
{
    if (!g_audioReady) return;
    ma_device_uninit(&g_audioDevice);
    g_audioReady = false;
}

void audioPushVoice(AudioVoice v)
{
    if (!g_audioReady || g_audioMuted) return;
    std::lock_guard<std::mutex> lock(g_audioMutex);
    if (g_audioVoices.size() > 32) g_audioVoices.erase(g_audioVoices.begin());
    g_audioVoices.push_back(v);
}

void audioPlayLaser()      { AudioVoice v; v.kind = AudioVoice::SINE_SWEEP; v.freqStart = 1200.0f; v.freqEnd = 300.0f; v.amp = 0.35f; v.duration = 0.3f;  audioPushVoice(v); }
void audioPlayLaunch()     { AudioVoice v; v.kind = AudioVoice::CLICK;      v.freqStart = 220.0f;  v.amp = 0.25f; v.duration = 0.12f; audioPushVoice(v); }
void audioPlayCollision()  { AudioVoice v; v.kind = AudioVoice::CLICK;      v.freqStart = 90.0f;   v.amp = 0.30f; v.duration = 0.15f; audioPushVoice(v); }
void audioPlayExplosion()  { AudioVoice v; v.kind = AudioVoice::NOISE_BURST; v.freqStart = 55.0f;  v.amp = 0.55f; v.duration = 0.9f;  audioPushVoice(v); }
void audioPlayUiBlip()     { AudioVoice v; v.kind = AudioVoice::SINE_SWEEP; v.freqStart = 700.0f;  v.freqEnd = 900.0f; v.amp = 0.15f; v.duration = 0.08f; audioPushVoice(v); }

void audioSetAmbient(bool enabled)
{
    if (!g_audioReady) return;
    std::lock_guard<std::mutex> lock(g_audioMutex);
    for (size_t v = 0; v < g_audioVoices.size(); )
    {
        if (g_audioVoices[v].kind == AudioVoice::HUM) g_audioVoices.erase(g_audioVoices.begin() + v);
        else ++v;
    }
    if (enabled)
    {
        AudioVoice v; v.kind = AudioVoice::HUM; v.freqStart = 52.0f; v.amp = 0.05f;
        g_audioVoices.push_back(v);
    }
}

#else

bool g_audioMuted = false;
void audioInit() {}
void audioShutdown() {}
void audioPlayLaser() {}
void audioPlayLaunch() {}
void audioPlayCollision() {}
void audioPlayExplosion() {}
void audioPlayUiBlip() {}
void audioSetAmbient(bool) {}

#endif

