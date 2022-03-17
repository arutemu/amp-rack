#ifndef __ENGINE__H
#define __ENGINE__H
#include <jni.h>
#include <oboe/Oboe.h>
#include <string>
#include <thread>
#include "FullDuplexPass.h"
#include "SharedLibrary.h"
#include "Plugin.h"
#include "FileWriter.h"

class Engine : public oboe::AudioStreamCallback {
public:
    Engine() ;

    int deletePluginFromRack(int pIndex);
    void setRecordingDeviceId(int32_t deviceId);

    void setPlaybackDeviceId(int32_t deviceId);

    bool setEffectOn(bool isOn);
    /*
     * oboe::AudioStreamDataCallback interface implementation
     */
    oboe::DataCallbackResult onAudioReady(oboe::AudioStream *oboeStream,
                                          void *audioData, int32_t numFrames) override;

    /*
     * oboe::AudioStreamErrorCallback interface implementation
     */
    void onErrorBeforeClose(oboe::AudioStream *oboeStream, oboe::Result error) override;
    void onErrorAfterClose(oboe::AudioStream *oboeStream, oboe::Result error) override;

    bool setAudioApi(oboe::AudioApi);
    bool isAAudioRecommended(void);
    void addPluginToRack(int libraryIndex, int pluginIndex);

    int32_t           mSampleRate = oboe::kUnspecified;
    void * handle ;

    FullDuplexPass    mFullDuplexPass;
    bool bootComplete = false ;

    FileWriter * fileWriter ;
    std::string externalStoragePath ;

    std::vector <SharedLibrary *> libraries ;
    std::vector<Plugin *> activePlugins ;

    void loadPlugin(char *filename);
    void loadPlugins();
    int moveActivePluginDown(int _p);
    int moveActivePluginUp(int _p);
    void buildPluginChain();
private:
    bool              mIsEffectOn = false;
    int32_t           mRecordingDeviceId = oboe::kUnspecified;
    int32_t           mPlaybackDeviceId = oboe::kUnspecified;
    const oboe::AudioFormat mFormat = oboe::AudioFormat::Float; // for easier processing
    oboe::AudioApi    mAudioApi = oboe::AudioApi::AAudio;
    const int32_t     mInputChannelCount = oboe::ChannelCount::Stereo;
    const int32_t     mOutputChannelCount = oboe::ChannelCount::Stereo;

    std::shared_ptr<oboe::AudioStream> mRecordingStream;
    std::shared_ptr<oboe::AudioStream> mPlayStream;

    oboe::Result openStreams();

    void closeStreams();

    void closeStream(std::shared_ptr<oboe::AudioStream> &stream);

    oboe::AudioStreamBuilder *setupCommonStreamParameters(
            oboe::AudioStreamBuilder *builder);
    oboe::AudioStreamBuilder *setupRecordingStreamParameters(
            oboe::AudioStreamBuilder *builder, int32_t sampleRate);
    oboe::AudioStreamBuilder *setupPlaybackStreamParameters(
            oboe::AudioStreamBuilder *builder);
    void warnIfNotLowLatency(std::shared_ptr<oboe::AudioStream> &stream);


    void discoverPlugins();


} ;

#endif // __ENGINE__H