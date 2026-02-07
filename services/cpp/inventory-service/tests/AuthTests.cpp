#include <catch2/catch_all.hpp>

#include "inventory/utils/Auth.hpp"

#include <Poco/Net/NameValueCollection.h>
#include <cstdlib>

using inventory::utils::Auth;
using inventory::utils::AuthStatus;

namespace {

void setEnv(const char* name, const char* value) {
    if (value) {
        ::setenv(name, value, 1);
    } else {
        ::unsetenv(name);
    }
}

} // namespace

TEST_CASE("Auth returns NotConfigured when no API key is set", "[auth][service]") {
    setEnv("SERVICE_API_KEY", nullptr);

    Poco::Net::NameValueCollection headers;
    auto status = Auth::authorizeServiceHeaders(headers);
    REQUIRE(status == AuthStatus::NotConfigured);
}

TEST_CASE("Auth handles missing and invalid tokens", "[auth][service]") {
    setEnv("SERVICE_API_KEY", "test-key");

    SECTION("Missing token yields MissingToken") {
        Poco::Net::NameValueCollection headers;
        auto status = Auth::authorizeServiceHeaders(headers);
        REQUIRE(status == AuthStatus::MissingToken);
    }

    SECTION("Invalid X-Service-Api-Key yields InvalidToken") {
        Poco::Net::NameValueCollection headers;
        headers.set("X-Service-Api-Key", "wrong-key");
        auto status = Auth::authorizeServiceHeaders(headers);
        REQUIRE(status == AuthStatus::InvalidToken);
    }

    SECTION("Invalid Authorization header yields InvalidToken") {
        Poco::Net::NameValueCollection headers;
        headers.set("Authorization", "ApiKey wrong-key");
        auto status = Auth::authorizeServiceHeaders(headers);
        REQUIRE(status == AuthStatus::InvalidToken);
    }
}

TEST_CASE("Auth authorizes valid tokens", "[auth][service]") {
    setEnv("SERVICE_API_KEY", "test-key");

    SECTION("Valid X-Service-Api-Key is Authorized") {
        Poco::Net::NameValueCollection headers;
        headers.set("X-Service-Api-Key", "test-key");
        auto status = Auth::authorizeServiceHeaders(headers);
        REQUIRE(status == AuthStatus::Authorized);
    }

    SECTION("Valid Authorization ApiKey is Authorized") {
        Poco::Net::NameValueCollection headers;
        headers.set("Authorization", "ApiKey test-key");
        auto status = Auth::authorizeServiceHeaders(headers);
        REQUIRE(status == AuthStatus::Authorized);
    }
}
