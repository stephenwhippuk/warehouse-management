#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <http-framework/ServiceCollection.hpp>
#include <http-framework/IServiceProvider.hpp>
#include <http-framework/ServiceNamespace.hpp>
#include <http-framework/NamespacedServiceCollection.hpp>
#include <memory>
#include <map>

namespace {

// Test service interfaces
class ILogger {
public:
    virtual ~ILogger() = default;
    virtual void log(const std::string&) = 0;
    virtual std::string getName() const = 0;
};

class ICache {
public:
    virtual ~ICache() = default;
    virtual void set(const std::string&, const std::string&) = 0;
    virtual std::string get(const std::string&) = 0;
};

// Test logger implementations
class GlobalLogger : public ILogger {
public:
    GlobalLogger(http::IServiceProvider&) { }
    void log(const std::string& msg) override { lastMessage = msg; }
    std::string getName() const override { return "GlobalLogger"; }
    static std::string lastMessage;
};
std::string GlobalLogger::lastMessage;

class PluginLogger : public ILogger {
public:
    PluginLogger(http::IServiceProvider&) { }
    void log(const std::string& msg) override { lastMessage = msg; }
    std::string getName() const override { return "PluginLogger"; }
    static std::string lastMessage;
};
std::string PluginLogger::lastMessage;

// Test cache implementations
class GlobalCache : public ICache {
public:
    GlobalCache(http::IServiceProvider&) { }
    void set(const std::string& k, const std::string& v) override { data[k] = v; }
    std::string get(const std::string& k) override { return data[k]; }
    static std::map<std::string, std::string> data;
};
std::map<std::string, std::string> GlobalCache::data;

class PluginCache : public ICache {
public:
    PluginCache(http::IServiceProvider&) { }
    void set(const std::string& k, const std::string& v) override { data[k] = v; }
    std::string get(const std::string& k) override { return data[k]; }
    static std::map<std::string, std::string> data;
};
std::map<std::string, std::string> PluginCache::data;

} // namespace

TEST_CASE("ServiceNamespace utilities", "[namespace][utilities]") {
    SECTION("global() returns 'global'") {
        auto ns = std::string(http::ServiceNamespace::global());
        REQUIRE(ns == "global");
    }

    SECTION("pluginNamespace() formats correctly") {
        std::string ns = http::ServiceNamespace::pluginNamespace("auth");
        REQUIRE(ns == "plugin:auth");
    }

    SECTION("isPluginNamespace() detects plugin namespaces") {
        REQUIRE(http::ServiceNamespace::isPluginNamespace("plugin:auth"));
        REQUIRE(http::ServiceNamespace::isPluginNamespace("plugin:payment"));
        REQUIRE(!http::ServiceNamespace::isPluginNamespace("global"));
        REQUIRE(!http::ServiceNamespace::isPluginNamespace("custom-namespace"));
    }

    SECTION("extractPluginName() extracts plugin name") {
        std::string name = http::ServiceNamespace::extractPluginName("plugin:auth");
        REQUIRE(name == "auth");
        
        name = http::ServiceNamespace::extractPluginName("plugin:payment-service");
        REQUIRE(name == "payment-service");
    }

    SECTION("validate() accepts valid namespaces") {
        REQUIRE_NOTHROW(http::ServiceNamespace::validate("global"));
        REQUIRE_NOTHROW(http::ServiceNamespace::validate("plugin:auth"));
        REQUIRE_NOTHROW(http::ServiceNamespace::validate("plugin:my-service"));
    }

    SECTION("validate() rejects invalid namespaces") {
        REQUIRE_THROWS(http::ServiceNamespace::validate(""));
        REQUIRE_THROWS(http::ServiceNamespace::validate("plugin:"));
    }
}

TEST_CASE("ServiceDescriptor namespace and visibility", "[namespace][descriptor]") {
    SECTION("ServiceDescriptor stores namespace and visibility") {
        http::ServiceDescriptor desc(
            std::type_index(typeid(ILogger)),
            [](http::IServiceProvider& provider) { return std::make_shared<GlobalLogger>(provider); },
            http::ServiceLifetime::Scoped,
            "plugin:auth",
            http::ServiceVisibility::Exported
        );
        
        REQUIRE(desc.getNamespace() == "plugin:auth");
        REQUIRE(desc.getVisibility() == http::ServiceVisibility::Exported);
    }

    SECTION("ServiceDescriptor defaults to global namespace") {
        http::ServiceDescriptor desc(
            std::type_index(typeid(ILogger)),
            [](http::IServiceProvider& provider) { return std::make_shared<GlobalLogger>(provider); },
            http::ServiceLifetime::Scoped
        );
        
        REQUIRE(desc.getNamespace() == "global");
        REQUIRE(desc.getVisibility() == http::ServiceVisibility::Exported);
    }

    SECTION("ServiceDescriptor supports Internal visibility") {
        http::ServiceDescriptor desc(
            std::type_index(typeid(ILogger)),
            [](http::IServiceProvider& provider) { return std::make_shared<GlobalLogger>(provider); },
            http::ServiceLifetime::Scoped,
            "plugin:auth",
            http::ServiceVisibility::Internal
        );
        
        REQUIRE(desc.getVisibility() == http::ServiceVisibility::Internal);
    }
}

TEST_CASE("ServiceCollection namespace registration", "[namespace][collection]") {
    http::ServiceCollection services;

    SECTION("addService with namespace registers in correct namespace") {
        services.addService<ILogger, GlobalLogger>(
            http::ServiceLifetime::Scoped,
            "global",
            http::ServiceVisibility::Exported
        );
        
        services.addService<ILogger, PluginLogger>(
            http::ServiceLifetime::Scoped,
            "plugin:auth",
            http::ServiceVisibility::Exported
        );
        
        REQUIRE(services.hasService<ILogger>("global"));
        REQUIRE(services.hasService<ILogger>("plugin:auth"));
    }

    SECTION("Services in different namespaces are separate") {
        services.addService<ILogger, GlobalLogger>(
            http::ServiceLifetime::Scoped,
            "global"
        );
        
        services.addService<ICache, GlobalCache>(
            http::ServiceLifetime::Scoped,
            "plugin:auth"
        );
        
        REQUIRE(services.hasService<ILogger>("global"));
        REQUIRE(!services.hasService<ICache>("global"));
        REQUIRE(services.hasService<ICache>("plugin:auth"));
        REQUIRE(!services.hasService<ILogger>("plugin:auth"));
    }

    SECTION("Duplicate services in same namespace throw error") {
        services.addService<ILogger, GlobalLogger>(
            http::ServiceLifetime::Scoped,
            "global"
        );
        
        REQUIRE_THROWS(
            services.addService<ILogger, PluginLogger>(
                http::ServiceLifetime::Scoped,
                "global"
            )
        );
    }

    SECTION("Same service type in different namespaces allowed") {
        services.addService<ILogger, GlobalLogger>(
            http::ServiceLifetime::Scoped,
            "global"
        );
        
        REQUIRE_NOTHROW(
            services.addService<ILogger, PluginLogger>(
                http::ServiceLifetime::Scoped,
                "plugin:auth"
            )
        );
    }

    SECTION("getNamespaceServiceCount() returns correct count") {
        services.addService<ILogger, GlobalLogger>(
            http::ServiceLifetime::Scoped,
            "global"
        );
        services.addService<ICache, GlobalCache>(
            http::ServiceLifetime::Scoped,
            "global"
        );
        services.addService<ILogger, PluginLogger>(
            http::ServiceLifetime::Scoped,
            "plugin:auth"
        );
        
        REQUIRE(services.getNamespaceServiceCount("global") == 2);
        REQUIRE(services.getNamespaceServiceCount("plugin:auth") == 1);
        REQUIRE(services.getNamespaceServiceCount("plugin:payment") == 0);
    }
}

TEST_CASE("ServiceProvider namespace resolution", "[namespace][provider]") {
    SECTION("Resolves services from correct namespace") {
        http::ServiceCollection services;
        
        services.addService<ILogger, GlobalLogger>(
            http::ServiceLifetime::Transient,
            "global"
        );
        services.addService<ILogger, PluginLogger>(
            http::ServiceLifetime::Transient,
            "plugin:auth"
        );
        
        auto provider = services.buildServiceProvider();
        
        auto globalLogger = provider->getService<ILogger>("global");
        REQUIRE(globalLogger);
        REQUIRE(globalLogger->getName() == "GlobalLogger");
        
        auto pluginLogger = provider->getService<ILogger>("plugin:auth");
        REQUIRE(pluginLogger);
        REQUIRE(pluginLogger->getName() == "PluginLogger");
    }

    SECTION("Correctly returns nullptr for non-existent services") {
        http::ServiceCollection services;
        services.addService<ILogger, GlobalLogger>(
            http::ServiceLifetime::Transient,
            "global"
        );
        
        auto provider = services.buildServiceProvider();
        
        auto missing = provider->getOptionalService<ICache>("global");
        REQUIRE(!missing);
    }

    SECTION("Plugin namespace falls back to global Exported services") {
        http::ServiceCollection services;
        
        services.addService<ILogger, GlobalLogger>(
            http::ServiceLifetime::Transient,
            "global",
            http::ServiceVisibility::Exported
        );
        
        auto provider = services.buildServiceProvider();
        
        // Plugin can access global Exported service
        auto logger = provider->getService<ILogger>("plugin:auth");
        REQUIRE(logger);
        REQUIRE(logger->getName() == "GlobalLogger");
    }

    SECTION("Plugin namespace cannot access global Internal services") {
        http::ServiceCollection services;
        
        services.addService<ILogger, GlobalLogger>(
            http::ServiceLifetime::Transient,
            "global",
            http::ServiceVisibility::Internal
        );
        
        auto provider = services.buildServiceProvider();
        
        // Plugin cannot access internal service from global namespace
        auto logger = provider->getOptionalService<ILogger>("plugin:auth");
        REQUIRE(!logger);
    }

    SECTION("Explicit namespace takes precedence over fallback") {
        http::ServiceCollection services;
        
        services.addService<ILogger, GlobalLogger>(
            http::ServiceLifetime::Transient,
            "global",
            http::ServiceVisibility::Exported
        );
        services.addService<ILogger, PluginLogger>(
            http::ServiceLifetime::Transient,
            "plugin:auth",
            http::ServiceVisibility::Exported
        );
        
        auto provider = services.buildServiceProvider();
        
        // Plugin should get its own service, not global fallback
        auto logger = provider->getService<ILogger>("plugin:auth");
        REQUIRE(logger->getName() == "PluginLogger");
    }
}

TEST_CASE("NamespacedServiceCollection wrapper", "[namespace][wrapper]") {
    SECTION("Wrapper automatically applies namespace to registrations") {
        http::ServiceCollection services;
        http::NamespacedServiceCollection wrapper(services, "plugin:auth");
        
        wrapper.addScoped<ILogger, PluginLogger>();
        wrapper.addSingleton<ICache, PluginCache>();
        
        REQUIRE(services.hasService<ILogger>("plugin:auth"));
        REQUIRE(services.hasService<ICache>("plugin:auth"));
        REQUIRE(!services.hasService<ILogger>("global"));
    }

    SECTION("Wrapper getNamespace() returns wrapped namespace") {
        http::ServiceCollection services;
        http::NamespacedServiceCollection wrapper(services, "plugin:payment");
        
        REQUIRE(wrapper.getNamespace() == "plugin:payment");
    }

    SECTION("Wrapper addInternal() registers Internal services") {
        http::ServiceCollection services;
        http::NamespacedServiceCollection wrapper(services, "plugin:auth");
        
        wrapper.addInternal<ILogger, PluginLogger>(http::ServiceLifetime::Scoped);
        
        auto provider = services.buildServiceProvider();
        
        // Internal service not accessible from outside namespace
        auto logger = provider->getOptionalService<ILogger>("global");
        REQUIRE(!logger);
    }

    SECTION("Multiple wrappers manage separate namespaces") {
        http::ServiceCollection services;
        
        http::NamespacedServiceCollection authWrapper(services, "plugin:auth");
        http::NamespacedServiceCollection paymentWrapper(services, "plugin:payment");
        
        authWrapper.addScoped<ILogger, PluginLogger>();
        paymentWrapper.addScoped<ICache, PluginCache>();
        
        REQUIRE(services.hasService<ILogger>("plugin:auth"));
        REQUIRE(services.hasService<ICache>("plugin:payment"));
        REQUIRE(!services.hasService<ILogger>("plugin:payment"));
        REQUIRE(!services.hasService<ICache>("plugin:auth"));
    }
}

TEST_CASE("Complete plugin isolation scenario", "[namespace][integration]") {
    SECTION("Two plugins with same service interface isolated completely") {
        http::ServiceCollection services;
        
        // Auth plugin
        {
            http::NamespacedServiceCollection authServices(services, "plugin:auth");
            authServices.addTransient<ILogger, PluginLogger>();
        }
        
        // Payment plugin  
        {
            http::NamespacedServiceCollection paymentServices(services, "plugin:payment");
            paymentServices.addTransient<ILogger, GlobalLogger>();
        }
        
        auto provider = services.buildServiceProvider();
        
        auto authLogger = provider->getService<ILogger>("plugin:auth");
        auto paymentLogger = provider->getService<ILogger>("plugin:payment");
        
        REQUIRE(authLogger->getName() == "PluginLogger");
        REQUIRE(paymentLogger->getName() == "GlobalLogger");
    }

    SECTION("Plugin namespace validation prevents invalid names") {
        http::ServiceCollection services;
        
        REQUIRE_THROWS(
            [&services]() {
                http::NamespacedServiceCollection wrapper(services, "@invalid");
            }()
        );
    }

    SECTION("Services count correct per namespace") {
        http::ServiceCollection services;
        
        services.addService<ILogger, GlobalLogger>(http::ServiceLifetime::Scoped, "global");
        services.addService<ICache, GlobalCache>(http::ServiceLifetime::Scoped, "global");
        
        {
            http::NamespacedServiceCollection authServices(services, "plugin:auth");
            authServices.addScoped<ILogger, PluginLogger>();
            authServices.addScoped<ICache, PluginCache>();
        }
        
        REQUIRE(services.getNamespaceServiceCount("global") == 2);
        REQUIRE(services.getNamespaceServiceCount("plugin:auth") == 2);
    }
}

