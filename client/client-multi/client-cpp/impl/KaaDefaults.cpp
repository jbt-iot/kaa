/*
 * Copyright 2014-2016 CyberVision, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * NOTE: This is a auto-generated file. Do not edit it.
 */

#include "kaa/KaaDefaults.hpp"

#include <algorithm>
#include <cstdint>
#include <sstream>

#include "kaa/channel/GenericTransportInfo.hpp"

namespace kaa {

const char * const BUILD_VERSION = "0.9.0";

const char * const BUILD_COMMIT_HASH = "";

const char * const SDK_TOKEN = "pPSFs1AbXe-iHY2qGnlX5-kiIlI";

const std::uint32_t POLLING_PERIOD_SECONDS = 5;

const char * const CLIENT_PUB_KEY_LOCATION = "key.public";

const char * const CLIENT_PRIV_KEY_LOCATION = "key.private";

const char * const CLIENT_STATUS_FILE_LOCATION = "kaa.status";

const char * const DEFAULT_USER_VERIFIER_TOKEN = "";

ITransportConnectionInfoPtr createTransportInfo(const std::int32_t& accessPointId
                                              , const std::int32_t& protocolId
                                              , const std::int32_t& protocolVersion
                                              , const std::string& encodedConnectionData)
{
    auto buffer = Botan::base64_decode(encodedConnectionData);
    ITransportConnectionInfoPtr connectionInfo(
            new GenericTransportInfo(ServerType::BOOTSTRAP
                                   , accessPointId
                                   , TransportProtocolId(protocolId, protocolVersion)
                                   , std::vector<std::uint8_t>(buffer.begin(), buffer.end())));

    return connectionInfo;
}

const BootstrapServers& getBootstrapServers() 
{
    static BootstrapServers listOfServers;
    if (listOfServers.empty()) {
        listOfServers.push_back(createTransportInfo(0xd7e0906, 0xfb9a3cf0, 1, "AAABJjCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAJo3NWuC4hYZexwS+6VrtMkmvIK1sPICJZLELFAUEqIUxdUkoIKRYa92rMkqR8OMH9jSRHG1uNSQTDN9c5DoCbdKb2BuL0UwCGRQsrAnMQ5eqTE7Pm8Wo2uPRAIvXAMF+ed8OBbAGYwuERf/fJeKEqJbcOZ1ckr/TUDB+m63MDfOLPhZpCJp0Slzdh/8bsjQhueimqGn//1AU1flwisT29Qjw99bk/ElyeWFNNgCvk4+5ILube3k474rHDBeBcRpcOuyMO0OFaRLj/6dMUrZ5e+jNISfejIvdKwiKnzguD6gi3LNfVwKjuXmtY/Rz9ECxg1gUKif2TC3YfS09bMsc8sCAwEAAQAAAAo1Mi4zLjIwLjI5AAAAUA=="));
listOfServers.push_back(createTransportInfo(0xd7e0906, 0x56c8ff92, 1, "AAABJjCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAJo3NWuC4hYZexwS+6VrtMkmvIK1sPICJZLELFAUEqIUxdUkoIKRYa92rMkqR8OMH9jSRHG1uNSQTDN9c5DoCbdKb2BuL0UwCGRQsrAnMQ5eqTE7Pm8Wo2uPRAIvXAMF+ed8OBbAGYwuERf/fJeKEqJbcOZ1ckr/TUDB+m63MDfOLPhZpCJp0Slzdh/8bsjQhueimqGn//1AU1flwisT29Qjw99bk/ElyeWFNNgCvk4+5ILube3k474rHDBeBcRpcOuyMO0OFaRLj/6dMUrZ5e+jNISfejIvdKwiKnzguD6gi3LNfVwKjuXmtY/Rz9ECxg1gUKif2TC3YfS09bMsc8sCAwEAAQAAAAo1Mi4zLjIwLjI5AAAmoA=="));
;
        std::random_shuffle(listOfServers.begin(), listOfServers.end());
    }
    return listOfServers;
}

const Botan::secure_vector<std::uint8_t>& getDefaultConfigData() 
{
    static const Botan::secure_vector<std::uint8_t> configData = Botan::base64_decode("AAAAAABTBzYPmzBGbZ3GYqXoE8bQAObpw8KINUUHh2oIwKzj1H0AAAAAFTC/HINjRBKHGa3bD3UrWgC40BVNYjdCOagDAe8x332XAAAAAN4W3BHQUUIRml1R5RgC/2oAhcRGBB16TH+HBJG+ADGAGgCee2CIDu5E8I59IHJtl1i7AgICAgCnDLcZ4UhID6wHEVZMiZnF");
    return configData;
}

HashDigest getPropertiesHash()
{
    std::ostringstream ss;

    ss << SDK_TOKEN;
    ss << POLLING_PERIOD_SECONDS;
    ss << CLIENT_PUB_KEY_LOCATION;
    ss << CLIENT_PRIV_KEY_LOCATION;
    ss << CLIENT_STATUS_FILE_LOCATION;

    for (const auto& server : getBootstrapServers()) {
        const auto& connectionInfo = server->getConnectionInfo();
        ss.write(reinterpret_cast<const char*>(connectionInfo.data()), connectionInfo.size());
    }

    ss.write(reinterpret_cast<const char*>(
            getDefaultConfigData().data()), getDefaultConfigData().size());

    return EndpointObjectHash(ss.str()).getHashDigest();
}

}
