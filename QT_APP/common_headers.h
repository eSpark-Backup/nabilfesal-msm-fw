#ifndef COMMON_HEADERS_H
#define COMMON_HEADERS_H

#include <array>
#include <QMetaType>
#include "logs.h"

struct EnergyData {
    double posActEnergy[3]  = {0.0,0.0,0.0};
    double negActEnergy[3]  = {0.0,0.0,0.0};
    double posReacEnergy[3] = {0.0,0.0,0.0};
    double negReacEnergy[3] = {0.0,0.0,0.0};
};

using EnergyArray = std::array<EnergyData, 13>;

Q_DECLARE_METATYPE(EnergyData)
Q_DECLARE_METATYPE(EnergyArray)

enum Mode {
    Factory,       // factory with all topics
    Admin          // admin mode with fewer topics
};


#endif // COMMON_HEADERS_H
