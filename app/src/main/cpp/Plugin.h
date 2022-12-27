#ifndef __PLUGIN_H
#define __PLUGIN_H
#include <ladspa.h>
#include <lv2.h>

#include <cstddef>
#include <logging_macros.h>
#include <vector>

#include "PluginControl.h"
#include "SharedLibrary.h"

class Plugin {
    LADSPA_Data ** portControls ;
    unsigned long sampleRate ;
public:
    bool active = true ;
    SharedLibrary::PluginType type ;
    LADSPA_Data run_adding_gain = 1 ;
    std::vector <PluginControl *> pluginControls ;
    const LADSPA_Descriptor * descriptor ;
    const LV2_Descriptor * lv2Descriptor;
    SharedLibrary * sharedLibrary;
    int inputPort = -1;
    int inputPort2 = -1;
    int outputPort = -1;
    int outputPort2 = -1;
    LADSPA_Data dummy_output_control_port = 0; // from th pulseaudio ladspa sink module
    LADSPA_Handle *handle ;
    Plugin(const LADSPA_Descriptor * descriptor, unsigned long _sampleRate, SharedLibrary::PluginType _type = SharedLibrary::LADSPA);
    void print();

    void free();

    void load();
};

#endif // __PLUGIN_H