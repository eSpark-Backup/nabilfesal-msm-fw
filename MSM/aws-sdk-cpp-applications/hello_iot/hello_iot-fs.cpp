#include <aws/core/Aws.h>
#include <aws/core/utils/DateTime.h>
#include <aws/core/utils/logging/ConsoleLogSystem.h>
#include <aws/iot/IoTClient.h>
#include <aws/iot/model/CreateThingRequest.h>
#include <aws/iot/model/CreateKeysAndCertificateRequest.h>
#include <aws/iot/model/AttachThingPrincipalRequest.h>
#include <aws/iot/model/AttachPolicyRequest.h>

#include <fstream>
#include <iostream>
#include <filesystem>

namespace fs = std::experimental::filesystem;

struct Config {
    std::string region = "eu-north-1";
    std::string policyName = "ESP90000005-Policy";
    std::string iotEndpoint = "a1dxcxg260l7m-ats.iot.eu-north-1.amazonaws.com";
    std::string credsPath = "./creds";
};

class DeviceProvisioner {
public:
    DeviceProvisioner(const Config& cfg) : config(cfg) {
        Aws::Client::ClientConfiguration clientConfig;
        clientConfig.region = config.region;
        iotClient = std::make_shared<Aws::IoT::IoTClient>(clientConfig);
        std::cout << "IoT client initialized\n";
    }

    Aws::IoT::Model::CreateThingOutcome createThing(const std::string& thingName) {
        Aws::IoT::Model::CreateThingRequest request;
        request.SetThingName(thingName);

        Aws::IoT::Model::AttributePayload payload;
        Aws::Map<Aws::String, Aws::String> attributes;
        attributes["serialNumber"] = "SN-" + Aws::Utils::DateTime::Now().ToGmtString(Aws::Utils::DateFormat::ISO_8601);
        attributes["deviceType"] = "ubuntu";
        attributes["createdAt"] = Aws::Utils::DateTime::Now().ToGmtString(Aws::Utils::DateFormat::ISO_8601);

        payload.SetAttributes(attributes);
        request.SetAttributePayload(payload);

        auto outcome = iotClient->CreateThing(request);
        if (outcome.IsSuccess()) {
            std::cout << "IoT Thing created successfully\n";
        } else {
            std::cerr << "Error creating Thing: " << outcome.GetError().GetMessage() << "\n";
        }
        return outcome;
    }

    Aws::IoT::Model::CreateKeysAndCertificateOutcome createCertificates() {
        Aws::IoT::Model::CreateKeysAndCertificateRequest request;
        request.SetSetAsActive(true);

        auto outcome = iotClient->CreateKeysAndCertificate(request);
        if (outcome.IsSuccess()) {
            std::cout << "Device certificates created successfully\n";
        } else {
            std::cerr << "Error creating certificates: " << outcome.GetError().GetMessage() << "\n";
        }
        return outcome;
    }

    bool attachCertificateToThing(const std::string& thingName, const std::string& certArn) {
        Aws::IoT::Model::AttachThingPrincipalRequest request;
        request.SetThingName(thingName);
        request.SetPrincipal(certArn);

        auto outcome = iotClient->AttachThingPrincipal(request);
        if (outcome.IsSuccess()) {
            std::cout << "Certificate attached to thing\n";
            return true;
        } else {
            std::cerr << "Error attaching certificate: " << outcome.GetError().GetMessage() << "\n";
            return false;
        }
    }

    bool attachPolicyToCertificate(const std::string& certArn) {
        Aws::IoT::Model::AttachPolicyRequest request;
        request.SetPolicyName(config.policyName);
        request.SetTarget(certArn);

        auto outcome = iotClient->AttachPolicy(request);
        if (outcome.IsSuccess()) {
            std::cout << "Policy attached to certificate\n";
            return true;
        } else {
            std::cerr << "Error attaching policy: " << outcome.GetError().GetMessage() << "\n";
            return false;
        }
    }

    void saveCertificates(const Aws::IoT::Model::CreateKeysAndCertificateOutcome& certs,
                          const std::string& thingName) {
        if (!certs.IsSuccess()) return;

        auto timestamp = Aws::Utils::DateTime::Now().ToGmtString(Aws::Utils::DateFormat::ISO_8601);
        for (auto& c : timestamp) if (c == ':' || c == '.') c = '-';

        fs::path certDir = fs::path(config.credsPath) / "permanent" / thingName;
        fs::create_directories(certDir);

        auto certPem = certs.GetResult().GetCertificatePem();
        auto privKey = certs.GetResult().GetKeyPair().GetPrivateKey();
        auto pubKey  = certs.GetResult().GetKeyPair().GetPublicKey();

        std::ofstream(certDir / (thingName + "-certificate.pem")) << certPem;
        std::ofstream(certDir / (thingName + "-private.key")) << privKey;
        std::ofstream(certDir / (thingName + "-public.key")) << pubKey;

        std::cout << "Certificates saved to " << certDir << "\n";
    }

    bool provision(const std::string& thingName) {
        std::cout << "Starting provisioning for: " << thingName << "\n";

        auto thingOutcome = createThing(thingName);
        if (!thingOutcome.IsSuccess()) return false;

        auto certOutcome = createCertificates();
        if (!certOutcome.IsSuccess()) return false;

        auto certArn = certOutcome.GetResult().GetCertificateArn();
        if (!attachCertificateToThing(thingName, certArn)) return false;
        if (!attachPolicyToCertificate(certArn)) return false;

        saveCertificates(certOutcome, thingName);

        std::cout << "Provisioning completed successfully\n";
        return true;
    }

private:
    Config config;
    std::shared_ptr<Aws::IoT::IoTClient> iotClient;
};

int main() {
    Aws::SDKOptions options;
    Aws::InitAPI(options);
    {
        Config config;
        DeviceProvisioner provisioner(config);

        std::string deviceName = "ESP-" + Aws::Utils::DateTime::Now().ToGmtString(Aws::Utils::DateFormat::ISO_8601);
        provisioner.provision(deviceName);
    }
    Aws::ShutdownAPI(options);
    return 0;
}
