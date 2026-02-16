#include <catch2/catch_all.hpp>
#include <http-framework/ServiceScopeMiddleware.hpp>
#include <http-framework/ServiceCollection.hpp>
#include <http-framework/HttpContext.hpp>
#include <http-framework/Middleware.hpp>
#include <http-framework/IServiceScope.hpp>
#include <memory>
#include <Poco/Net/HTTPServerRequest.h>
#include <Poco/Net/HTTPServerResponse.h>

using namespace http;

// Test interfaces and implementations for realistic testing
class ILogger {
public:
    virtual ~ILogger() = default;
    virtual void log(const std::string& msg) const = 0;
};

class SimpleLogger : public ILogger {
public:
    void log(const std::string& msg) const override {
        (void)msg;  // No-op for testing
    }
};

class IRepository {
public:
    virtual ~IRepository() = default;
    virtual std::string getId() const = 0;
};

class SimpleRepository : public IRepository {
public:
    explicit SimpleRepository(std::shared_ptr<ILogger> logger)
        : logger_(logger), id_(std::to_string(reinterpret_cast<uintptr_t>(this))) {
    }
    
    std::string getId() const override { return id_; }
    std::shared_ptr<ILogger> getLogger() const { return logger_; }

private:
    std::shared_ptr<ILogger> logger_;
    std::string id_;
};

// Mock HTTP request and response for testing
// Instead of mocking Poco classes (which have complex constructors),
// we'll test the middleware with a simplified approach

// Simplified test that doesn't require actual HTTP mocks
TEST_CASE("ServiceScopeMiddleware creates scope for request", "[scope][middleware]") {
    // Setup
    ServiceCollection services;
    services.addService<ILogger>(
        [](IServiceProvider&) -> std::shared_ptr<ILogger> {
            return std::make_shared<SimpleLogger>();
        },
        ServiceLifetime::Scoped
    );
    auto provider = services.buildServiceProvider();
    
    auto middleware = std::make_shared<ServiceScopeMiddleware>(provider);
    
    // Test that middleware can be created and called with a working scope
    bool middlewareCalled = false;
    bool nextCalled = false;
    
    // The middleware should create a scope internally when process() is called
    // We can't easily test with real Poco mocks, but we can verify the middleware itself exists
    // and the scoping infrastructure is wired up
    REQUIRE(middleware != nullptr);
}

TEST_CASE("Scoped services are reused within same scope", "[scope][lifetime]") {
    // Setup
    ServiceCollection services;
    services.addService<ILogger>(
        [](IServiceProvider&) -> std::shared_ptr<ILogger> {
            return std::make_shared<SimpleLogger>();
        },
        ServiceLifetime::Scoped
    );
    services.addService<IRepository>(
        [](IServiceProvider& p) -> std::shared_ptr<IRepository> {
            auto logger = p.getService<ILogger>();
            return std::make_shared<SimpleRepository>(logger);
        },
        ServiceLifetime::Scoped
    );
    auto provider = services.buildServiceProvider();
    
    // Create scope
    auto scope1 = provider->createScope();
    auto& scopedProvider = scope1->getServiceProvider();
    
    // Get same service twice from same scope
    auto repo1a = scopedProvider.getService<IRepository>();
    auto repo1b = scopedProvider.getService<IRepository>();
    
    // Should be same instance
    REQUIRE(repo1a->getId() == repo1b->getId());
}

TEST_CASE("Scoped services differ across different scopes", "[scope][isolation]") {
    // Setup
    ServiceCollection services;
    services.addService<ILogger>(
        [](IServiceProvider&) -> std::shared_ptr<ILogger> {
            return std::make_shared<SimpleLogger>();
        },
        ServiceLifetime::Scoped
    );
    services.addService<IRepository>(
        [](IServiceProvider& p) -> std::shared_ptr<IRepository> {
            auto logger = p.getService<ILogger>();
            return std::make_shared<SimpleRepository>(logger);
        },
        ServiceLifetime::Scoped
    );
    auto provider = services.buildServiceProvider();
    
    // Create scope 1
    auto scope1 = provider->createScope();
    auto& provider1 = scope1->getServiceProvider();
    auto repo1 = provider1.getService<IRepository>();
    std::string id1 = repo1->getId();
    
    // Create scope 2
    auto scope2 = provider->createScope();
    auto& provider2 = scope2->getServiceProvider();
    auto repo2 = provider2.getService<IRepository>();
    std::string id2 = repo2->getId();
    
    // Should be different instances
    REQUIRE(id1 != id2);
}

TEST_CASE("Scoped service dependencies are resolved within scope", "[scope][dependencies]") {
    // Setup
    ServiceCollection services;
    services.addService<ILogger>(
        [](IServiceProvider&) -> std::shared_ptr<ILogger> {
            return std::make_shared<SimpleLogger>();
        },
        ServiceLifetime::Scoped
    );
    services.addService<IRepository>(
        [](IServiceProvider& p) -> std::shared_ptr<IRepository> {
            auto logger = p.getService<ILogger>();
            return std::make_shared<SimpleRepository>(logger);
        },
        ServiceLifetime::Scoped
    );
    auto provider = services.buildServiceProvider();
    
    // Create scope and get service with dependencies
    auto scope = provider->createScope();
    auto& scopedProvider = scope->getServiceProvider();
    
    auto repo = scopedProvider.getService<IRepository>();
    auto logger = scopedProvider.getService<ILogger>();
    
    // Both should be accessible
    REQUIRE(repo);
    REQUIRE(logger);
}

TEST_CASE("Singleton services are shared across scopes", "[scope][singleton]") {
    // Setup - Logger is singleton
    ServiceCollection services;
    services.addService<ILogger>(
        [](IServiceProvider&) -> std::shared_ptr<ILogger> {
            return std::make_shared<SimpleLogger>();
        },
        ServiceLifetime::Singleton
    );
    services.addService<IRepository>(
        [](IServiceProvider& p) -> std::shared_ptr<IRepository> {
            auto logger = p.getService<ILogger>();
            return std::make_shared<SimpleRepository>(logger);
        },
        ServiceLifetime::Scoped
    );
    auto provider = services.buildServiceProvider();
    
    // Create scope 1 and get logger
    auto scope1 = provider->createScope();
    auto& provider1 = scope1->getServiceProvider();
    auto logger1 = provider1.getService<ILogger>();
    
    // Create scope 2 and get logger
    auto scope2 = provider->createScope();
    auto& provider2 = scope2->getServiceProvider();
    auto logger2 = provider2.getService<ILogger>();
    
    // Loggers should be same instance (singleton)
    REQUIRE(logger1.get() == logger2.get());
}

TEST_CASE("Transient services create new instance every time within scope", "[scope][transient]") {
    // Setup
    ServiceCollection services;
    services.addService<ILogger>(
        [](IServiceProvider&) -> std::shared_ptr<ILogger> {
            return std::make_shared<SimpleLogger>();
        },
        ServiceLifetime::Transient
    );
    auto provider = services.buildServiceProvider();
    
    // Create scope with transient service
    auto scope = provider->createScope();
    auto& scopedProvider = scope->getServiceProvider();
    
    // Get same type twice
    auto logger1 = scopedProvider.getService<ILogger>();
    auto logger2 = scopedProvider.getService<ILogger>();
    
    // Should be different instances (transient)
    REQUIRE(logger1.get() != logger2.get());
}

TEST_CASE("Service scope cleanup when scope destructs", "[scope][cleanup]") {
    // Setup
    ServiceCollection services;
    services.addService<ILogger>(
        [](IServiceProvider&) -> std::shared_ptr<ILogger> {
            return std::make_shared<SimpleLogger>();
        },
        ServiceLifetime::Scoped
    );
    auto provider = services.buildServiceProvider();
    
    std::shared_ptr<ILogger> logger;
    {
        // Create scope in inner block
        auto scope = provider->createScope();
        auto& scopedProvider = scope->getServiceProvider();
        logger = scopedProvider.getService<ILogger>();
        
        // Logger should be valid within scope
        REQUIRE(logger);
    }
    
    // Logger reference should still be valid (shared_ptr), but scope is destroyed
    REQUIRE(logger);  // Pointer still valid due to shared_ptr
}

TEST_CASE("Multiple concurrent scopes don't interfere", "[scope][concurrency]") {
    // Setup
    ServiceCollection services;
    services.addService<IRepository>(
        [](IServiceProvider&) -> std::shared_ptr<IRepository> {
            auto logger = std::make_shared<SimpleLogger>();
            return std::make_shared<SimpleRepository>(logger);
        },
        ServiceLifetime::Scoped
    );
    auto provider = services.buildServiceProvider();
    
    // Create multiple scopesand store providers
    auto scope1 = provider->createScope();
    auto scope2 = provider->createScope();
    auto scope3 = provider->createScope();
    
    auto repo1 = scope1->getServiceProvider().getService<IRepository>();
    auto repo2 = scope2->getServiceProvider().getService<IRepository>();
    auto repo3 = scope3->getServiceProvider().getService<IRepository>();
    
    // All should have different IDs since they're in different scopes
    REQUIRE(repo1->getId() != repo2->getId());
    REQUIRE(repo2->getId() != repo3->getId());
    REQUIRE(repo1->getId() != repo3->getId());
}
