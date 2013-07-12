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

#include "SoundFileAPI_ExtAudioFile.hpp"
#include "CAStreamBasicDescription.h"

#include <iostream>
#include <memory>

#include <AudioToolbox/AudioToolbox.h>

// Current sound file handle format
enum SoundFileHandleFormat
{
    kSoundFileHandleNoFormat,
    kSoundFileHandleInt16,
    kSoundFileHandleFloat
};

struct SoundFileHandle
{
    ExtAudioFileRef file;
    size_t numChannels;
    double sampleRate;
    SoundFileHandleFormat format;
    Methcla_SoundFile soundFile;
};

extern "C" {
static Methcla_Error soundfile_close(const Methcla_SoundFile* file);
static Methcla_Error soundfile_seek(const Methcla_SoundFile* file, int64_t numFrames);
static Methcla_Error soundfile_tell(const Methcla_SoundFile* file, int64_t* numFrames);
static Methcla_Error soundfile_read_float(const Methcla_SoundFile* file, float* buffer, size_t inNumFrames, size_t* outNumFrames);
static Methcla_Error soundfile_open(const Methcla_SoundFileAPI* api, const char* path, Methcla_FileMode mode, Methcla_SoundFile** outFile, Methcla_SoundFileInfo* info);
static const Methcla_SystemError* soundfile_last_system_error(const Methcla_SoundFileAPI* api);
static const char* soundfile_system_error_description(const Methcla_SystemError* error);
static void soundfile_system_error_destroy(const Methcla_SystemError* error);
} // extern "C"

Methcla_Error soundfile_close(const Methcla_SoundFile* file)
{
//    std::cout << "extAudioFile_close " << file << " " << file->handle << std::endl;
    SoundFileHandle* handle = (SoundFileHandle*)file->handle;
    if (handle->file != nullptr) {
        ExtAudioFileDispose(handle->file);
        handle->file = nullptr;
    }
    free(handle);
    return kMethcla_NoError;
}

Methcla_Error soundfile_seek(const Methcla_SoundFile* file, int64_t numFrames)
{
    ExtAudioFileRef extFile = ((SoundFileHandle*)file->handle)->file;
    if (extFile == nullptr)
        return kMethcla_ArgumentError;

    OSStatus err = ExtAudioFileSeek(extFile, numFrames);
    if (err != noErr)
        return kMethcla_UnspecifiedError;

    return kMethcla_NoError;
}

Methcla_Error soundfile_tell(const Methcla_SoundFile* file, int64_t* numFrames)
{
    ExtAudioFileRef extFile = ((SoundFileHandle*)file->handle)->file;
    if (extFile == nullptr)
        return kMethcla_ArgumentError;

    SInt64 outNumFrames;
    OSStatus err = ExtAudioFileTell(extFile, &outNumFrames);
    if (err != noErr)
        return kMethcla_UnspecifiedError;

    *numFrames = outNumFrames;

    return kMethcla_NoError;
}

Methcla_Error soundfile_read_float(const Methcla_SoundFile* file, float* buffer, size_t inNumFrames, size_t* outNumFrames)
{
    SoundFileHandle* handle = (SoundFileHandle*)file->handle;
    ExtAudioFileRef extFile = handle->file;
    if (extFile == nullptr)
        return kMethcla_ArgumentError;

    // Setup the client format: 32 bit float little-endian interleaved
    if (handle->format != kSoundFileHandleFloat) {
        // TODO: This should probably be set in read_float instead before each read
        CAStreamBasicDescription clientFormat;
        clientFormat.mSampleRate = handle->sampleRate;
        clientFormat.mFormatID = kAudioFormatLinearPCM;
        clientFormat.mChannelsPerFrame = handle->numChannels;
        clientFormat.mBitsPerChannel = sizeof(float) * 8;
        clientFormat.mBytesPerPacket = clientFormat.mBytesPerFrame = sizeof(float) * clientFormat.mChannelsPerFrame;
        clientFormat.mFramesPerPacket = 1;
        clientFormat.mFormatFlags = kLinearPCMFormatFlagIsPacked | kLinearPCMFormatFlagIsFloat;

//        printf("Client format: "); clientFormat.Print();

        UInt32 size = sizeof(clientFormat);
        OSStatus err = ExtAudioFileSetProperty(extFile, kExtAudioFileProperty_ClientDataFormat, size, &clientFormat);
        if (err != noErr)
            return kMethcla_UnspecifiedError;

        handle->format = kSoundFileHandleFloat;
    }

    UInt32 numFrames = inNumFrames;

    AudioBufferList bufferList;
    bufferList.mNumberBuffers = 1;
    bufferList.mBuffers[0].mNumberChannels = handle->numChannels;
    bufferList.mBuffers[0].mDataByteSize = handle->numChannels * inNumFrames * sizeof(float);
    bufferList.mBuffers[0].mData = buffer;

    OSStatus err = ExtAudioFileRead(extFile, &numFrames, &bufferList);
    if (err != noErr)
        return kMethcla_UnspecifiedError;

    *outNumFrames = numFrames;

    return kMethcla_NoError;
}

// Helper class for releasing a CFTypeRef when it goes out of scope
template <class T> class CFRef
{
public:
    CFRef(T ref)
        : m_ref(ref)
    { }
    ~CFRef()
    {
        CFRelease(m_ref);
    }

    CFRef(const CFRef<T>& other) = delete;
    CFRef<T>& operator=(const CFRef<T>& other) = delete;

    operator T ()
    {
        return m_ref;
    }

private:
    T m_ref;
};

Methcla_Error soundfile_open(const Methcla_SoundFileAPI*, const char* path, Methcla_FileMode mode, Methcla_SoundFile** outFile, Methcla_SoundFileInfo* info)
{
    if (path == nullptr)
        return kMethcla_ArgumentError;
    if (outFile == nullptr)
        return kMethcla_ArgumentError;

    ExtAudioFileRef extFile = nullptr;

    // open the source file
    CFRef<CFURLRef> url(CFURLCreateFromFileSystemRepresentation(kCFAllocatorDefault, (UInt8*)path, strlen(path), false));
    OSStatus err = ExtAudioFileOpenURL(url, &extFile);
    if (err != noErr)
        return kMethcla_UnspecifiedError;

    CAStreamBasicDescription srcFormat;
    UInt32 size = sizeof(srcFormat);
    err = ExtAudioFileGetProperty(extFile, kExtAudioFileProperty_FileDataFormat, &size, &srcFormat);
    if (err != noErr) {
        ExtAudioFileDispose(extFile);
        return kMethcla_UnspecifiedError;
    }

    SInt64 numFrames;
    size = sizeof(numFrames);
    err = ExtAudioFileGetProperty(extFile, kExtAudioFileProperty_FileLengthFrames, &size, &numFrames);
    if (err != noErr) {
        ExtAudioFileDispose(extFile);
        return kMethcla_UnspecifiedError;
    }

//    printf("Source file format: "); srcFormat.Print();
//    printf("Source file length: %lld\n", numFrames);

    SoundFileHandle* handle = (SoundFileHandle*)malloc(sizeof(SoundFileHandle));
    if (handle == nullptr) {
        ExtAudioFileDispose(extFile);
        return kMethcla_UnspecifiedError;
    }

    handle->file = extFile;
    handle->sampleRate = srcFormat.mSampleRate;
    handle->numChannels = srcFormat.NumberChannels();
    handle->format = kSoundFileHandleNoFormat;

    Methcla_SoundFile* file = &handle->soundFile;
    memset(file, 0, sizeof(*file));
    file->handle = handle;
    file->close = soundfile_close;
    file->seek = soundfile_seek;
    file->tell = soundfile_tell;
    file->read_float = soundfile_read_float;

    *outFile = file;

    if (info != nullptr) {
        info->frames = numFrames;
        info->channels = handle->numChannels;
        info->samplerate = handle->sampleRate;
    }

    return kMethcla_NoError;
}

static const char* soundfile_system_error_description(const Methcla_SystemError* error)
{
    return "Unknown system error (ExtAudioFile)";
}

static void soundfile_system_error_destroy(const Methcla_SystemError* error)
{
    delete error;
}

static const Methcla_SystemError* soundfile_last_system_error(const Methcla_SoundFileAPI* api)
{
    Methcla_SystemError* error = new (std::nothrow) Methcla_SystemError;
    if (error) {
        error->handle = nullptr;
        error->description = soundfile_system_error_description;
        error->destroy = soundfile_system_error_destroy;
    }
    return error;
}

static const Methcla_SoundFileAPI kSoundFileAPI = {
    nullptr,
    soundfile_open,
    soundfile_last_system_error
};

const Methcla_SoundFileAPI* SoundFileAPI_ExtAudioFile()
{
    return &kSoundFileAPI;
}
