#pragma once

namespace ntMirrors {

    enum nameTableMirrorMode {
    
        horizontal,     // Use two internal NTs, horizontal config
        vertical,       // Use two internal NTs, vertical config
        singleScreenLo, // Use lower bank from internal NT mirrored 4 times
        singleScreenHi, // Use upper bank from internal NT mirrored 4 times
        fourScreen,     // Use external RAM for four individual nametables
    
    };

}
