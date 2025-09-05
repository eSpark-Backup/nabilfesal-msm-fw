#include <aws/core/Aws.h>
#include <aws/core/utils/json/JsonSerializer.h>
#include <aws/core/utils/memory/stl/AWSStringStream.h>
#include <aws/core/auth/AWSCredentialsProvider.h>
#include <aws/core/utils/Outcome.h>
#include <aws/core/utils/StringUtils.h>
#include <aws/core/utils/UUID.h>
#include <aws/core/utils/logging/LogMacros.h>
#include <aws/core/utils/FileSystemUtils.h>

#include <aws/lambda/LambdaClient.h>
#include <aws/lambda/model/InvokeRequest.h>
#include <aws/lambda/model/InvokeResult.h>

#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <cerrno>

class DeviceProvisioner {
public:
    DeviceProvisioner(const Aws::String& functionName,
                      const Aws::String& region,
                      const Aws::String& credsPath,
                      const Aws::String& iotEndpoint)
        : m_functionName(functionName), m_region(region),
          m_credsPath(credsPath), m_iotEndpoint(iotEndpoint) {

        Aws::Client::ClientConfiguration config;
        config.region = region;
        m_lambdaClient = std::make_unique<Aws::Lambda::LambdaClient>(config);

        std::cout << "✓ Lambda client initialized (" << region << ")\n";
    }

    // Load claim certificates from disk
    bool LoadClaimCertificates(const Aws::String& thingName) {
        try {
            std::string certPath   = m_credsPath.c_str() + std::string("/") + thingName.c_str() + ".cert.pem";
            std::string keyPath    = m_credsPath.c_str() + std::string("/") + thingName.c_str() + ".private.key";
            std::string rootCAPath = m_credsPath.c_str() + std::string("/AmazonRootCA1.pem");

            m_certificate = ReadFile(certPath);
            m_privateKey  = ReadFile(keyPath);
            m_rootCA      = ReadFile(rootCAPath);

            std::cout << "✓ Claim certificates loaded successfully\n";
            return true;
        } catch (const std::exception& e) {
            std::cerr << "✗ Error loading claim certificates: " << e.what() << "\n";
            return false;
        }
    }

    // Invoke Lambda provisioning function
    Aws::Utils::Json::JsonValue ProvisionDevice(const Aws::String& thingName,
                                                const Aws::Map<Aws::String, Aws::String>& parameters) {
        Aws::Lambda::Model::InvokeRequest invokeRequest;
        invokeRequest.SetFunctionName(m_functionName);
        invokeRequest.SetInvocationType(Aws::Lambda::Model::InvocationType::RequestResponse);

        // Build JSON payload
        Aws::Utils::Json::JsonValue payload;
        payload.WithString("thingName", thingName);

        Aws::Utils::Json::JsonValue params;
        for (auto& kv : parameters) {
            params.WithString(kv.first, kv.second);
        }
        payload.WithObject("parameters", std::move(params));

        auto payloadStr = payload.View().WriteReadable();
        invokeRequest.SetBody(Aws::MakeShared<Aws::StringStream>("ProvisionerStream", payloadStr));

        auto outcome = m_lambdaClient->Invoke(invokeRequest);

        if (!outcome.IsSuccess()) {
            std::cerr << "✗ Lambda invocation failed: "
                      << outcome.GetError().GetMessage() << "\n";
            return Aws::Utils::Json::JsonValue();
        }

        auto& result = outcome.GetResult();
        Aws::OStringStream oss;
        oss << result.GetPayload().rdbuf();
        auto payloadResponse = oss.str();

        std::cout << "✓ Lambda executed successfully\n";

        return Aws::Utils::Json::JsonValue(payloadResponse);
    }

    // Save permanent certificates
    bool SavePermanentCertificates(const Aws::Utils::Json::JsonValue& certs,
                                   const Aws::String& thingName) {
        try {
            std::string certDir = m_credsPath.c_str() + std::string("/permanent/") + thingName.c_str();
            MakeDirRecursive(certDir);

            WriteFile(certDir + "/" + thingName.c_str() + std::string("-certificate.pem"),
                      certs.View().GetObject("body").GetString("certificatePem"));
            WriteFile(certDir + "/" + thingName.c_str() + std::string("-private.key"),
                      certs.View().GetObject("body").GetString("privateKey"));
            WriteFile(certDir + "/" + thingName.c_str() + std::string("-public.key"),
                      certs.View().GetObject("body").GetString("publicKey"));

            Aws::Utils::Json::JsonValue metadata;
            metadata.WithString("thingName", thingName);
            metadata.WithString("certificateId", certs.View().GetObject("body").GetString("certificateId"));
            metadata.WithString("createdAt", Aws::Utils::DateTime::Now().ToGmtString(Aws::Utils::DateFormat::ISO_8601));
            metadata.WithString("iotEndpoint", m_iotEndpoint);
            metadata.WithString("region", m_region);

            WriteFile(certDir + "/" + thingName.c_str() + std::string("-metadata.json"),
                      metadata.View().WriteReadable());

            std::cout << "✓ Permanent certificates saved to " << certDir << "\n";
            return true;
        } catch (const std::exception& e) {
            std::cerr << "✗ Error saving permanent certificates: " << e.what() << "\n";
            return false;
        }
    }

private:
    std::unique_ptr<Aws::Lambda::LambdaClient> m_lambdaClient;
    Aws::String m_functionName, m_region, m_credsPath, m_iotEndpoint;
    Aws::String m_certificate, m_privateKey, m_rootCA;

    // Recursively create directories
    static void MakeDirRecursive(const std::string& path) {
        size_t pos = 0;
        do {
            pos = path.find('/', pos + 1);
            std::string sub = path.substr(0, pos);

            if (sub.empty()) continue;
            if (mkdir(sub.c_str(), 0755) && errno != EEXIST) {
                throw std::runtime_error("Failed to create directory " + sub +
                                         ": " + std::strerror(errno));
            }
        } while (pos != std::string::npos);
    }

    // Read entire file
    static Aws::String ReadFile(const std::string& path) {
        FILE* f = fopen(path.c_str(), "rb");
        if (!f) throw std::runtime_error("Failed to open " + path);

        fseek(f, 0, SEEK_END);
        long size = ftell(f);
        rewind(f);

        std::string buffer;
        buffer.resize(size);

        if (fread(&buffer[0], 1, size, f) != (size_t)size) {
            fclose(f);
            throw std::runtime_error("Failed to read " + path);
        }
        fclose(f);

        return Aws::String(buffer.c_str(), buffer.size());
    }

    // Write to file
    static void WriteFile(const std::string& path, const Aws::String& contents) {
        FILE* f = fopen(path.c_str(), "wb");
        if (!f) throw std::runtime_error("Failed to write to " + path);

        size_t written = fwrite(contents.c_str(), 1, contents.length(), f);
        fclose(f);

        if (written != contents.length()) {
            throw std::runtime_error("Failed to write complete file " + path);
        }
    }
};

// --- Main program ---
int main() {
    Aws::SDKOptions options;
    Aws::InitAPI(options);

    {
        Aws::String functionName = "MSMDeviceProvisioning";
        Aws::String region = "eu-north-1";
        Aws::String credsPath = "../creds";
        Aws::String iotEndpoint = "a1dxcxg260l7m-ats.iot.eu-north-1.amazonaws.com";

        DeviceProvisioner provisioner(functionName, region, credsPath, iotEndpoint);

        Aws::Utils::UUID uuid = Aws::Utils::UUID::RandomUUID();
        Aws::String uuidStr = uuid;  // implicit conversion to Aws::String
        Aws::String thingName = Aws::String("ESP-LINUX-") + uuidStr;

        Aws::Map<Aws::String, Aws::String> params;
        params["serialNumber"] = "SN-12345";
        params["deviceType"] = "esp32";
        params["firmware"] = "1.0.0";

        // Step 1: Load claim certs
        provisioner.LoadClaimCertificates("ESP90000005");

        // Step 2: Provision via Lambda
        auto result = provisioner.ProvisionDevice(thingName, params);

        if (!result.WasParseSuccessful()) {
            std::cerr << "Provisioning failed: invalid Lambda response\n";
            Aws::ShutdownAPI(options);
            return 1;
        }

        // Step 3: Save permanent certs
        provisioner.SavePermanentCertificates(result, thingName);

        std::cout << "\n✅ Provisioning completed for " << thingName << "\n";
    }

    Aws::ShutdownAPI(options);
    return 0;
}
