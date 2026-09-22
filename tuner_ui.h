#pragma once
#include <string>
#include <vector>

struct Transponder {
    int frequency;
    int symbol_rate;
    int polarization;
    int fec_inner;
    int system;
    int modulation;
};

struct Satellite {
    std::string name;
    int position;
    int flags;
    std::vector<Transponder> transponders;
};

void TunerUI_Init();
void TunerUI_Render();
