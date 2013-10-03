// Copyright 2013 Samplecount S.L.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "Engine.hpp"

#include <methcla/common.h>
#include <methcla/plugins/pro/soundfile_api_extaudiofile.h>
#include <methcla/plugins/pro/disksampler.h>
#include <methcla/plugins/sampler.h>
#include <methcla/plugins/patch-cable.h>

#include <iostream>
#include <stdexcept>
#include <tinydir.h>

Sound::Sound(const Methcla::Engine& engine, const std::string& path)
    : m_path(path)
{
    Methcla_SoundFile* file;
    Methcla_SoundFileInfo info;
    Methcla_Error err = methcla_engine_soundfile_open(engine, m_path.c_str(), kMethcla_FileModeRead, &file, &info);
    if (err != kMethcla_NoError) {
        throw std::runtime_error("Opening sound file " + path + "failed");
    }
    file->close(file);
    m_duration = (double)info.frames / (double)info.samplerate;
}

// Return a list of sounds in directory path.
static std::vector<Sound> loadSounds(const Methcla::Engine& engine, const std::string& path)
{
    std::vector<Sound> result;

    tinydir_dir dir;
    tinydir_open(&dir, path.c_str());

    while (dir.has_next)
    {
        tinydir_file file;
        tinydir_readfile(&dir, &file);
        if (!file.is_dir) {
            try {
                result.push_back(Sound(engine, path + "/" + std::string(file.name)));
            } catch (std::exception& e) {
                std::cerr << "Exception while registering sound " << file.name << ": " << e.what() << std::endl;
            }
        }
        tinydir_next(&dir);
    }

    tinydir_close(&dir);

    return result;
}

Engine::Engine(const std::string& soundDir)
    : m_engine(nullptr)
    , m_nextSound(0)
{
    // Create the engine with a set of plugins.
    m_engine = new Methcla::Engine({
        Methcla::Option::driverBufferSize(256),
        Methcla::Option::pluginLibrary(methcla_soundfile_api_extaudiofile),
        Methcla::Option::pluginLibrary(methcla_plugins_sampler),
        Methcla::Option::pluginLibrary(methcla_plugins_disksampler),
        Methcla::Option::pluginLibrary(methcla_plugins_patch_cable)
    });

    m_sounds = loadSounds(*m_engine, soundDir);

    // Start the engine.
    engine().start();

    m_voiceGroup = engine().group(engine().root());

    for (auto bus : { 0, 1 })
    {
        Methcla::Engine::Bundle request(engine());
        auto synth = request.synth(METHCLA_PLUGINS_PATCH_CABLE_URI, engine().root(), {});
        request.activate(synth);
        request.mapInput(synth, 0, Methcla::AudioBusId(bus));
        request.mapOutput(synth, 0, Methcla::AudioBusId(bus), Methcla::kBusMappingExternal);
        request.send();
        m_patchCables.push_back(synth);
    }
}

Engine::~Engine()
{
    engine().free(m_voiceGroup);
    for (auto synth : m_patchCables) {
        engine().free(synth);
    }
    delete m_engine;
}

size_t Engine::nextSound()
{
    const size_t result = m_nextSound;
    m_nextSound++;
    if (m_nextSound >= m_sounds.size()) {
        m_nextSound = 0;
    }
    return result;
}

static Methcla_Time kLatency = 0.1;

void Engine::startVoice(VoiceId voice, size_t soundIndex, float amp)
{
    if (m_voices.find(voice) != m_voices.end()) {
        stopVoice(voice);
    }
    if (soundIndex < m_sounds.size()) {
        const Sound& sound = m_sounds[soundIndex];
        Methcla::Engine::Bundle request1(engine(), Methcla::immediately);
        const Methcla::SynthId synth = request1.synth(
            // Comment this line ...
            METHCLA_PLUGINS_DISKSAMPLER_URI,
            // ... and uncomment this one for memory-based playback.
            // METHCLA_PLUGINS_SAMPLER_URI,
            m_voiceGroup,
            { amp },
            { Methcla::Value(sound.path())
            , Methcla::Value(true) }
        );
        // Map to an internal bus for the fun of it
        request1.mapOutput(synth, 0, Methcla::AudioBusId(0));
        request1.mapOutput(synth, 1, Methcla::AudioBusId(1));
        Methcla::Engine::Bundle request2(request1, engine().currentTime() + kLatency);
        request2.activate(synth);
        request1.send();
        m_voices[voice] = synth;
        std::cout << "Synth " << synth.id()
                  << sound.path()
                  << " duration=" << sound.duration()
                  << " amp=" << amp
                  << std::endl;
    }
}

void Engine::updateVoice(VoiceId voice, float amp)
{
    auto it = m_voices.find(voice);
    assert( it != m_voices.end() );
    m_engine->set(it->second, 0, amp);
//    std::cout << "Synth " << it->second.id()
//              << " amp=" << amp
//              << std::endl;
}

void Engine::stopVoice(VoiceId voice)
{
    auto it = m_voices.find(voice);
    if (it != m_voices.end()) {
        Methcla::Engine::Bundle request(engine(), engine().currentTime() + kLatency);
        request.free(it->second);
        request.send();
        m_voices.erase(it);
    }
}
