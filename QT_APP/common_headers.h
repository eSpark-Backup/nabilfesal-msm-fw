#ifndef COMMON_HEADERS_H
#define COMMON_HEADERS_H

#include <array>
#include <QMetaType>
#include "logs.h"
#include <QMutex>

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


#if defined(BUILD_X86)
#define FLEET_PROV_SCRIPT (QDir::homePath() + "/Downloads/aws-iot-device-sdk-embedded-C/build/bin")
#define CERT_DIR_PATH (QDir::homePath() + "/Downloads/aws-iot-device-sdk-embedded-C/build/bin/certificates/")
#define CONFIG_JSON_PATH ("/home/nehalespark/Downloads/aws-iot-device-sdk-embedded-C/build/bin/config.json")
#elif defined(BUILD_V3S)
#define FLEET_PROV_SCRIPT ("/MSM/potential-pancake/MSM/bin/fleet_provisioner")
#define CERT_DIR_PATH (QString("/MSM/potential-pancake/MSM/creds/"))
#define CONFIG_JSON_PATH ("/MSM/potential-pancake/MSM/bin/fleet_provisioner/config.json")
#endif
#define CLAIM_PATH (CERT_DIR_PATH + "claim_certs/")
#define PROV_PATH  (CERT_DIR_PATH + "prov_certs/")

#endif // COMMON_HEADERS_H
