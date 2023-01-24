//
// Created by djshaji on 1/22/23.
//

#include <logging_macros.h>
#include "Meter.h"

vringbuffer_t * Meter::vringbuffer ;
Meter::buffer_t *Meter::current_buffer;
int Meter::bufferUsed  = 0;
Meter::staticBuffer_t Meter::buffers [1024] ;
int Meter::jack_samplerate = 48000 ;
int Meter::block_size = 384 ;
int Meter::MAX_STATIC_BUFFER  = 32;
jmethodID Meter::setMixerMeter ;
jclass Meter::mainActivity ;
JNIEnv * Meter::env = NULL;
JavaVM *Meter:: vm = NULL  ;

JavaVM* gJvm = nullptr;
static jobject gClassLoader;
static jmethodID gFindClassMethod;
bool Meter::enabled = false ;
float Meter::lastTotal = 0 ;
bool Meter::isInput = true;

JNIEnv* getEnv() {
    JNIEnv *env;
    int status = gJvm->GetEnv((void**)&env, JNI_VERSION_1_6);
    if(status < 0) {
        status = gJvm->AttachCurrentThread(&env, NULL);
        if(status < 0) {
            return nullptr;
        }
    }
    return env;
}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *pjvm, void *reserved) {
    gJvm = pjvm;  // cache the JavaVM pointer
    auto env = getEnv();
    //replace with one of your classes in the line below
    auto randomClass = env->FindClass("com/shajikhan/ladspa/amprack/MainActivity");
    jclass classClass = env->GetObjectClass(randomClass);
    auto classLoaderClass = env->FindClass("java/lang/ClassLoader");
    auto getClassLoaderMethod = env->GetMethodID(classClass, "getClassLoader",
                                                 "()Ljava/lang/ClassLoader;");
    gClassLoader = (jclass) env->NewGlobalRef(env->CallObjectMethod(randomClass, getClassLoaderMethod));
    gFindClassMethod = env->GetMethodID(classLoaderClass, "findClass",
                                        "(Ljava/lang/String;)Ljava/lang/Class;");

    return JNI_VERSION_1_6;
}


jclass Meter::findClass(const char* name) {
    return static_cast<jclass>(getEnv()->CallObjectMethod(gClassLoader, gFindClassMethod, getEnv()->NewStringUTF(name)));
}

jclass Meter::findClassWithEnv(JNIEnv *env, const char* name) {
    return static_cast<jclass>(env->CallObjectMethod(gClassLoader, gFindClassMethod, env->NewStringUTF(name)));
}

int Meter::autoincrease_callback(vringbuffer_t *vrb, bool first_call, int reading_size, int writing_size) {
    if(buffers_to_seconds(writing_size) < 1) {
        return 2; // autoincrease_callback is called approx. at every block. So it should not be necessary to return a value higher than 2. Returning a very low number might also theoretically put a lower constant strain on the memory bus, thus theoretically lower the chance of xruns.
    }

    return 0 ;
}


Meter::Meter(JavaVM *pVm) {
    IN
    vm = pVm;
    // sane defaults
    jack_samplerate = 48000 ;
    block_size = 384 ;
    vringbuffer = vringbuffer_create(JC_MAX(4,seconds_to_buffers(1)),
                                     JC_MAX(4,seconds_to_buffers(40)),
                                     (size_t) buffer_size_in_bytes);

    if(vringbuffer == NULL){
        HERE LOGF ("Unable to create ringbuffer!") ;
        OUT
        return ;
    }

    /// TODO: Free this memory!
    vringbuffer_set_autoincrease_callback(vringbuffer,autoincrease_callback,0);
    current_buffer = static_cast<buffer_t *>(vringbuffer_get_writing(vringbuffer));
    empty_buffer   = static_cast<float *>(my_calloc(sizeof(float), block_size * 1));

    OUT
}

void Meter::enable () {
    IN
    if (enabled)
        return ;
//    vm-> GetEnv((void**)&env, JNI_VERSION_1_6);
    /*
    mainActivity = env->FindClass("com/shajikhan/ladspa/amprack/MainActivity");
    if (mainActivity == nullptr) {
        HERE LOGF("cannot find class mainactivity!");
    }

    setMixerMeter = env->GetStaticMethodID(mainActivity, "setMixerMeterSwitch", "(FZ)V");
    if (setMixerMeter == nullptr) {
        LOGF("cannot find method!");
    }
     */

    vringbuffer_set_receiver_callback(vringbuffer,meter_callback);
    enabled = true ;
//    env->CallStaticVoidMethod(mainActivity, setMixerMeter, (jfloat ) 1.0f, true);
    OUT
}

int64_t Meter::seconds_to_frames(float seconds){
    return (int64_t) (((long double)seconds)*((long double)jack_samplerate));
}


float Meter::frames_to_seconds(int frames){
    return ((float)frames)/jack_samplerate;
}

// round up.
int Meter::seconds_to_blocks(float seconds){
    return (int)ceilf((seconds*jack_samplerate/(float)block_size));
}

float Meter::buffers_to_seconds(int buffers){
    return blocks_to_seconds(buffers);
}

float Meter::blocks_to_seconds(int blocks){
    return (float)blocks*(float)block_size/jack_samplerate;
}

void* Meter:: my_calloc(size_t size1,size_t size2){
    size_t size = size1*size2;
    void*  ret  = malloc(size);
    if(ret==NULL){
        fprintf(stderr,"\nOut of memory. Try a smaller buffer.\n");
        return NULL; }
    memset(ret,0,size);
    return ret;
}

int Meter::seconds_to_buffers(float seconds){
    return seconds_to_blocks(seconds);
}

int Meter::updateInput (float *data, size_t frames) {
//    IN
    float avg = 0 ;
    for (int i = 0 ; i < frames - 10 ; i ++ ) {
//        avg += data [i];
        LOGD("%f, %d of %d", data [i], i, frames);
    }

//    avg = avg / frames ;
//    LOGD("setting input values [%f]", avg);
//    env->CallStaticVoidMethod(mainActivity, setMixerMeter, avg, true);
//    OUT
}

int Meter::updateOutput (float *data, size_t frames) {
    float avg = 0 ;
    for (int i = 0 ; i < frames ; i ++ ) {
        avg += data [i];
    }

    avg = avg / frames ;
    LOGD("setting output values [%f]", avg);
    env->CallStaticVoidMethod(mainActivity, setMixerMeter, avg, false);
}

void Meter::process (int nframes, const float * data) {
//    IN
    if (bufferUsed < MAX_STATIC_BUFFER) {
        for (int i = 0; i < nframes; i++) {
            buffers[bufferUsed].data [i] = data[i];
        }

        buffers[bufferUsed].pos = nframes;
        bufferUsed++;
//        OUT
        return ;

    } else {
//        LOGD("return writing");
        vringbuffer_return_writing(vringbuffer,buffers);
    }


    /// does the following do ANYTHING?
    vringbuffer_trigger_autoincrease_callback(vringbuffer);
//    OUT
}

