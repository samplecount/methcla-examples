// Copyright 2012-2013 Samplecount S.L.
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

#ifndef METHCLA_PRO_ENGINE_HPP_INCLUDED
#define METHCLA_PRO_ENGINE_HPP_INCLUDED

#include <methcla/engine.hpp>
#include <methcla/pro/engine.h>
#include <oscpp/detail/host.hpp>

#include <cstdio>
#include <string>

namespace Methcla
{
    namespace detail
    {
        class RenderEngine : public EngineInterface
        {
            Options         m_options;
            NodeIdAllocator m_nodeIds;
            PacketPool      m_packets;
            FILE*           m_commandFile;

        public:
            RenderEngine(std::string commandFile, const Options& options={})
                : m_options(options)
                , m_nodeIds(1, 1023) // FIXME: Get max number of nodes from options
                , m_packets(8192)
                , m_commandFile(fopen(commandFile.c_str(), "w"))
            {
                if (m_commandFile == nullptr)
                    throw std::runtime_error("Couldn't open file command file for writing: " + commandFile);
            }

            ~RenderEngine()
            {
                fclose(m_commandFile);
            }

            virtual NodeIdAllocator& nodeIdAllocator() override
            {
                return m_nodeIds;
            }

            virtual Packet* allocPacket() override
            {
                return new Packet(m_packets);
            }

            virtual void sendPacket(const Packet* packet) override
            {
                // Write packet to file
                const size_t size = packet->packet().size();
                const uint32_t sizeTag = OSCPP::convert32<OSCPP::NetworkByteOrder>(size);
                size_t n = fwrite(&sizeTag, sizeof(sizeTag), 1, m_commandFile);
                if (n != 1)
                    throw std::runtime_error("Couldn't write data byte count to command file");
                n = fwrite(packet->packet().data(), 1, size, m_commandFile);
                if (n != size)
                    throw std::runtime_error("Couldn't write data to command file");
            }
        };
    }

    inline static void render(
        const Options& options,
        const std::string& inputFile,
        const std::string& outputFile,
        Methcla_SoundFileType outputFileType,
        Methcla_SoundFileFormat outputFileFormat,
        const std::string& commandFile,
        std::function<void(EngineInterface*)> func
    )
    {
        {
            detail::RenderEngine engine(commandFile, options);
            func(&engine);
        }
        OSCPP::Client::DynamicPacket bundle(8192);
        bundle.openBundle(1);
        for (auto option : options) {
            option->put(bundle);
        }
        bundle.closeBundle();
        const Methcla_OSCPacket packet = { .data = bundle.data(), .size = bundle.size() };
        detail::checkReturnCode(
            methcla_engine_render(
                &packet,
                inputFile.empty() ? nullptr : inputFile.c_str(),
                outputFile.c_str(),
                outputFileType,
                outputFileFormat,
                commandFile.c_str()
            )
        );
    }

    //* Render without input file.
    inline static void render(
        const Options& options,
        const std::string& outputFile,
        Methcla_SoundFileType outputFileType,
        Methcla_SoundFileFormat outputFileFormat,
        const std::string& commandFile,
        std::function<void(EngineInterface*)> func
    )
    {
        render(options, "", outputFile, outputFileType, outputFileFormat, commandFile, func);
    }
};

#endif // METHCLA_PRO_ENGINE_HPP_INCLUDED
