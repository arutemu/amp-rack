#include <cstdlib>
#include <cstring>
#include "FileWriter.h"

SNDFILE * FileWriter::soundfile = NULL;
buffer_t *FileWriter::current_buffer;
int FileWriter::num_channels = 2 ;
int FileWriter::block_size = 384 ;
float FileWriter::min_buffer_time = -1.0f,
        FileWriter::max_buffer_time = 40.0f ;
int FileWriter::jack_samplerate ;
int FileWriter::buffer_size_in_bytes;
bool FileWriter::ready = false ;
vringbuffer_t * FileWriter::vringbuffer ;
FileType FileWriter::fileType = OPUS ;
OpusEncoder *FileWriter::encoder ;
opus_int16 FileWriter::opusIn[960 * 2];
int FileWriter::opusRead = 0;
unsigned char FileWriter::opusOut[MAX_PACKET_SIZE];
FILE * FileWriter::outputFile = NULL;
OggOpusEnc * FileWriter:: oggOpusEnc ;

static char * extensions [] = {
        ".wav",
        ".opus"
} ;

int FileWriter::autoincrease_callback(vringbuffer_t *vrb, bool first_call, int reading_size, int writing_size) {
    IN ;
    if(buffers_to_seconds(writing_size) < min_buffer_time) {
        LOGD("return 2");
        OUT
        return 2; // autoincrease_callback is called approx. at every block. So it should not be necessary to return a value higher than 2. Returning a very low number might also theoretically put a lower constant strain on the memory bus, thus theoretically lower the chance of xruns.
    }

    LOGD("return 0");
    OUT
    return 0 ;
}

int64_t FileWriter::seconds_to_frames(float seconds){
    return (int64_t) (((long double)seconds)*((long double)jack_samplerate));
}


float FileWriter::frames_to_seconds(int frames){
    return ((float)frames)/jack_samplerate;
}

// round up.
int FileWriter::seconds_to_blocks(float seconds){
    return (int)ceilf((seconds*jack_samplerate/(float)block_size));
}

void* FileWriter:: my_calloc(size_t size1,size_t size2){
    size_t size = size1*size2;
    void*  ret  = malloc(size);
    if(ret==NULL){
        fprintf(stderr,"\nOut of memory. Try a smaller buffer.\n");
        return NULL; }
    memset(ret,0,size);
    return ret;
}

// same return value as seconds_to_blocks
int FileWriter::seconds_to_buffers(float seconds){
    return seconds_to_blocks(seconds);
}

FileWriter::~FileWriter () {
    free (empty_buffer);
    vringbuffer_delete(vringbuffer);
}
FileWriter::FileWriter () {
    IN

    OUT
}

void FileWriter::openFile () {
    IN
    memset(&sf_info,0,sizeof(SF_INFO));
    memset (&opusIn, 0, 960*2);
    memset (&opusOut, 0, 3*1276);

    sf_info.channels = 1 ;
    sf_info.samplerate = jack_samplerate * num_channels ;
    sf_info.format = SF_FORMAT_WAV ;
    sf_info.format |= SF_FORMAT_FLOAT ;

    if(sf_format_check(&sf_info)==0){
        LOGE ("\nFileformat not supported by libsndfile. Try other options.\n");
        return ;
    }

    if (fileType == WAV) {
        LOGD("[%s] %d %d %d", __PRETTY_FUNCTION__, sf_info.samplerate, sf_info.channels,
             sf_info.format);
        FileWriter::soundfile = sf_open(filename.c_str(), SFM_WRITE, &sf_info);
        if (soundfile == NULL) { // || ai==10){
            HERE
            LOGF ("\nCan not open sndfile \"%s\" for output (%s)\n", filename.c_str(),
                  sf_strerror(NULL));
        } else {
            LOGD("[%s] Opened file %s", __PRETTY_FUNCTION__, filename.c_str());
        }
    }

    else if (fileType == OPUS) {
        int err;
        encoder = opus_encoder_create(48000, 2, OPUS_APPLICATION_AUDIO, &err);
        if (err<0) {
            HERE LOGF("failed to create an encoder: %s\n", opus_strerror(err));
        }

        err = opus_encoder_ctl(encoder, OPUS_SET_BITRATE(bitRate));
        if (err<0) {
            LOGF("failed to set bitrate: %s\n", opus_strerror(err));
        }

//        outputFile = fopen(filename.c_str(), "wb");
        outputFile = NULL ;
        int error = 0 ;
        oggOpusEnc = ope_encoder_create_file(filename.c_str(), NULL, 48000, 2, 0, &error) ;
        if (!error) {
            HERE LOGF("cannot create encoder: %s", ope_strerror(error));
        }
    }

    OUT
}

void FileWriter::closeFile () {
    IN
    if (fileType == OPUS && outputFile)  {
        fclose (outputFile);
    }
    if (soundfile)
        sf_close(soundfile);
    OUT
}

int FileWriter::disk_write(void *data,size_t frames) {
    if (fileType == OPUS) {
        if (opusRead < 960) {
            float * f = static_cast<float *>(data);
            for (int x = 0 ; x < frames ; x ++) {
                opusIn[opusRead] = (opus_int )(size_t) f ;
                opusRead ++ ;
                *f ++ ;
            }

            return 1 ;

        } else {
            int nbBytes = opus_encode(encoder, opusIn, 960, opusOut, MAX_PACKET_SIZE);
            if (nbBytes<0) {
                LOGF("encode failed: %s\n", opus_strerror(nbBytes));
            } else {
                fwrite(opusOut, sizeof (unsigned char ), opusRead, outputFile) ;
                opusRead = 0 ;
                return 1 ;
            }
        }
    }

    if((size_t)sf_writef_float(FileWriter::soundfile, (float *)data,frames) != frames){
        LOGF("Error. Can not write sndfile (%s)\n",
                      sf_strerror(FileWriter::soundfile)
        );
        return 0;
    }

    return 1;
}

void FileWriter::startRecording () {
    IN
    openFile();
    vringbuffer_set_receiver_callback(vringbuffer,disk_callback);
    ready = true ;
    OUT
}

void FileWriter::stopRecording () {
    IN
    vringbuffer_stop_callbacks(vringbuffer);
    closeFile();
    OUT
}

void FileWriter::setBufferSize (int bufferSize) {
    IN
    block_size = bufferSize ;
    buffer_size_in_bytes = ALIGN_UP_DOUBLE(sizeof(buffer_t) + block_size*num_channels*sizeof(float ));

    vringbuffer = vringbuffer_create(JC_MAX(4,seconds_to_buffers(min_buffer_time)),
                                     JC_MAX(4,seconds_to_buffers(max_buffer_time)),
                                     (size_t) buffer_size_in_bytes);

    if(vringbuffer == NULL){
        HERE LOGF ("Unable to create ringbuffer!") ;
        OUT
        return ;
    }

    /// TODO: Free this memory!
    vringbuffer_set_autoincrease_callback(vringbuffer,autoincrease_callback,0);
    current_buffer = static_cast<buffer_t *>(vringbuffer_get_writing(vringbuffer));
    empty_buffer   = static_cast<float *>(my_calloc(sizeof(float), block_size * num_channels));
    OUT
}

void FileWriter::setSampleRate (int sampleRate) {
    IN
    if (sampleRate == 0) {
        HERE LOGE("sample rate passed as 0!") ;
        jack_samplerate = 48000 ;
    } else
        jack_samplerate = sampleRate ;
}

void FileWriter::setFileName (std::string name) {
    IN
    filename = std::string (name) + std::string (extensions [fileType]) ;
    LOGD("[%s] filename set to %s", __PRETTY_FUNCTION__ , name.c_str());
    OUT
}

void FileWriter::process_fill_buffer(float *in[], buffer_t *buffer, int i, int end){
    IN
    float *data=buffer->data;
    int pos=buffer->pos*num_channels;

    buffer->pos=pos/num_channels;
    OUT
}

bool FileWriter::process_new_current_buffer(int frames_left){
    current_buffer=(buffer_t*)vringbuffer_get_writing(vringbuffer);
    if(current_buffer==NULL){
        return false;
    }

    current_buffer->pos=0;
    return true;
}

void FileWriter::send_buffer_to_disk_thread(buffer_t *buffer){
    IN
    vringbuffer_return_writing(vringbuffer,buffer);
    OUT
}

void FileWriter::process_fill_buffers(void *data, int samples){
    IN
    if(current_buffer==NULL && process_new_current_buffer(samples)==false) {
        LOGD("no buffer and no samples!") ;
        OUT
        return;
    }

    current_buffer->data = (float*) data ;
    current_buffer->pos = samples ;
    send_buffer_to_disk_thread(current_buffer);
    current_buffer = NULL;
    return ;

    float *in[num_channels];
    int i = 0 ;
    while(i<samples){
        int size=JC_MIN(samples - i,
                        block_size - current_buffer->pos
        );

        process_fill_buffer(in,current_buffer,i,i+size);

        LOGD("%d %d %d %d", i, samples, size, current_buffer->pos) ;
        i+=size;

        if(current_buffer->pos == block_size){
            send_buffer_to_disk_thread(current_buffer);
            if(process_new_current_buffer(samples-i)==false)
                return;
        } else {
            LOGD("pos != block size") ;
        }

    }
    OUT
}

int FileWriter::process(float nframes, void *arg) {
    if (!ready)
        return 0 ;
    process_fill_buffers(arg, nframes);
    vringbuffer_trigger_autoincrease_callback(vringbuffer);
    return 0 ;
}

float FileWriter::buffers_to_seconds(int buffers){
    return blocks_to_seconds(buffers);
}

float FileWriter::blocks_to_seconds(int blocks){
    return (float)blocks*(float)block_size/jack_samplerate;
}
