#pragma once
#include <SDL3/SDL.h>
#include <string>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <memory>
#include <algorithm>
#include <cmath>

class AudioClip {
public:
    Uint8* audioData = nullptr;
    Uint32 audioLen = 0;
    SDL_AudioSpec spec;
    std::string name;

    AudioClip(const std::string& clipName, const char* filePath) : name(clipName) {
        if (!SDL_LoadWAV(filePath, &spec, &audioData, &audioLen)) {
            SDL_Log("[Audio Error] Failed to load WAV: %s", SDL_GetError());
        }
        else {
            SDL_Log("[Audio System] Loaded Clip: %s (Size: %d bytes)", name.c_str(), audioLen);
        }
    }

    ~AudioClip() {
        if (audioData) {
            SDL_free(audioData);
            audioData = nullptr;
        }
    }

    AudioClip(const AudioClip&) = delete;
    AudioClip& operator=(const AudioClip&) = delete;
};

class AudioVoice {
private:
    SDL_AudioStream* stream = nullptr;
    bool isActive = false;
    Uint64 startTime = 0;
    std::vector<Sint16> dspBuffer;

public:
    AudioVoice(SDL_AudioDeviceID deviceId, const SDL_AudioSpec& engineSpec) {
        stream = SDL_CreateAudioStream(nullptr, &engineSpec);
        if (stream) {
            SDL_BindAudioStream(deviceId, stream);
        }
        dspBuffer.reserve(512000);
    }

    ~AudioVoice() {
        if (stream) {
            SDL_DestroyAudioStream(stream);
        }
    }

    void Play(AudioClip* clip, float volume) {
        if (volume < 0.0f) volume = 0.0f;
        if (!stream || !clip || !clip->audioData) return;

        SDL_ClearAudioStream(stream);
        SDL_SetAudioStreamFormat(stream, &clip->spec, nullptr);
        SDL_SetAudioStreamGain(stream, volume);
        SDL_PutAudioStreamData(stream, clip->audioData, clip->audioLen);
        SDL_ResumeAudioStreamDevice(stream);

        isActive = true;
        startTime = SDL_GetTicks();
    }

    void Play3D(AudioClip* clip, float distance, float pan) {
        if (!stream || !clip || !clip->audioData) return;

        const float MAX_DIST = 600.0f;
        float baseVolume = 1.0f - (distance / MAX_DIST);
        if (baseVolume <= 0.01f) return;

        pan = std::clamp(pan, -1.0f, 1.0f);
        float leftGain = baseVolume * ((1.0f - pan) / 2.0f);
        float rightGain = baseVolume * ((1.0f + pan) / 2.0f);

        Sint16* srcData = reinterpret_cast<Sint16*>(clip->audioData);
        size_t numSamples = clip->audioLen / sizeof(Sint16);

        dspBuffer.clear();

        for (size_t i = 0; i < numSamples; ++i) {
            Sint16 originalSample = srcData[i];
            dspBuffer.push_back(static_cast<Sint16>(originalSample * leftGain));
            dspBuffer.push_back(static_cast<Sint16>(originalSample * rightGain));
        }

        SDL_ClearAudioStream(stream);

        SDL_AudioSpec stereoSpec = clip->spec;
        stereoSpec.channels = 2;
        SDL_SetAudioStreamFormat(stream, &stereoSpec, nullptr);
        SDL_SetAudioStreamGain(stream, 1.0f);

        size_t bytesToPut = dspBuffer.size() * sizeof(Sint16);
        SDL_PutAudioStreamData(stream, dspBuffer.data(), bytesToPut);
        SDL_ResumeAudioStreamDevice(stream);

        isActive = true;
        startTime = SDL_GetTicks();
    }

    void AudioVoiceUpdate() {
        if (!isActive) return;
        if (SDL_GetAudioStreamAvailable(stream) == 0) {
            isActive = false;
        }
    }

    void Stop() {
        if (!isActive) return;
        SDL_ClearAudioStream(stream);
        isActive = false;
    }

    bool IsActive() const { return isActive; }
    Uint64 GetStartTime() const { return startTime; }
};

class AudioEngine {
private:
    SDL_AudioDeviceID audioDevice;
    SDL_AudioSpec engineSpec;
    std::unordered_map<std::string, std::unique_ptr<AudioClip>> clipBank;
    std::vector<std::unique_ptr<AudioVoice>> voicePool;
    const int MAX_VOICES = 32;

    AudioEngine() {}

public:
    static AudioEngine& GetInstance() {
        static AudioEngine instance;
        return instance;
    }

    bool Initialize() {
        if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
            std::cerr << "SDL Audio Init Failed!" << std::endl;
            return false;
        }

        audioDevice = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr);
        if (!audioDevice) return false;

        SDL_GetAudioDeviceFormat(audioDevice, &engineSpec, nullptr);

        for (int i = 0; i < MAX_VOICES; ++i) {
            voicePool.push_back(std::make_unique<AudioVoice>(audioDevice, engineSpec));
        }

        SDL_Log("[Audio System] Initialized with %d voices.", MAX_VOICES);
        return true;
    }

    void LoadSound(const std::string& name, const char* path) {
        clipBank[name] = std::make_unique<AudioClip>(name, path);
        SDL_Log("[Audio System] Clip Loaded: %s", name.c_str());
    }

    void PlaySound(const std::string& name, float volume = 1.0f) {
        auto it = clipBank.find(name);
        if (it == clipBank.end()) {
            std::cerr << "[Audio Warning] Clip not found: " << name << std::endl;
            return;
        }
        AudioClip* clipToPlay = it->second.get();

        AudioVoice* targetVoice = nullptr;
        AudioVoice* oldestVoice = nullptr;
        Uint64 oldestTime = SDL_GetTicks();

        for (auto& voice : voicePool) {
            if (!voice->IsActive()) {
                targetVoice = voice.get();
                break;
            }
            if (voice->GetStartTime() < oldestTime) {
                oldestTime = voice->GetStartTime();
                oldestVoice = voice.get();
            }
        }

        if (!targetVoice) {
            if (oldestVoice) {
                targetVoice = oldestVoice;
                targetVoice->Stop();
            }
            else {
                return;
            }
        }

        targetVoice->Play(clipToPlay, volume);
    }

    void PlaySound3D(const std::string& name, float sourceX, float sourceY, float listenerX, float listenerY) {
        auto it = clipBank.find(name);
        if (it == clipBank.end()) return;

        float dx = sourceX - listenerX;
        float dy = sourceY - listenerY;
        float distance = std::sqrt(dx * dx + dy * dy);

        float pan = dx / 320.0f;

        AudioVoice* targetVoice = nullptr;
        AudioVoice* oldestVoice = nullptr;
        Uint64 oldestTime = SDL_GetTicks();

        for (auto& voice : voicePool) {
            if (!voice->IsActive()) {
                targetVoice = voice.get();
                break;
            }
            if (voice->GetStartTime() < oldestTime) {
                oldestTime = voice->GetStartTime();
                oldestVoice = voice.get();
            }
        }

        if (!targetVoice) {
            if (oldestVoice) {
                targetVoice = oldestVoice;
                targetVoice->Stop();
            }
            else {
                return;
            }
        }

        if (targetVoice) {
            targetVoice->Play3D(it->second.get(), distance, pan);
        }
    }

    void AudioEngineUpdate() {
        for (auto& voice : voicePool) {
            voice->AudioVoiceUpdate();
        }
    }

    void Shutdown() {
        voicePool.clear();
        clipBank.clear();
        SDL_CloseAudioDevice(audioDevice);
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
    }
};