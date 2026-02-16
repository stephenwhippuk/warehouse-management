#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <http-framework/ServiceCollection.hpp>
#include <http-framework/IServiceProvider.hpp>
#include <http-framework/IServiceScope.hpp>
#include <memory>
#include <thread>
#include <vector>

// Test interfaces and implementations
class ILogger {
public:
    virtual ~ILogger() = default;
    virtual std::string getName() const = 0;
    virtual int getInstanceId() const = 0;
};

class IDatabase {
public:
    virtual ~IDatabase() = default;
    virtual std::string getConnectionString() const = 0;
    virtual int getInstanceId() const = 0;
};

class IRepository {
public:
    virtual ~IRepository() = default;
    virtual std::string getData() const = 0;
    virtual int getInstanceId() const = 0;
    virtual std::shared_ptr<ILogger> getLogger() const = 0;
    virtual std::shared_ptr<IDatabase> getDatabase() const = 0;
};

// Implementations
static int g_loggerInstanceCounter = 0;
static int g_databaseInstanceCounter = 0;
static int g_repositoryInstanceCounter = 0;

class TestLogger : public ILogger {
public:
    explicit TestLogger(http::IServiceProvider& provider) 
        : instanceId_(++g_loggerInstanceCounter) {
        (void)provider;  // Unused
    }
    
    std::string getName() const override { return "TestLogger"; }
    int getInstanceId() const override { return instanceId_; }

private:
    int instanceId_;
};

class TestDatabase : public IDatabase {
public:
    explicit TestDatabase(http::IServiceProvider& provider)
        : instanceId_(++g_databaseInstanceCounter) {
        (void)provider;  // Unused
    }
    
    std::string getConnectionString() const override { return "postgresql://localhost"; }
    int getInstanceId() const override { return instanceId_; }

private:
    int instanceId_;
};

class TestRepository : public IRepository {
public:
    explicit TestRepository(http::IServiceProvider& provider)
        : instanceId_(++g_repositoryInstanceCounter) {
        
        // Resolve dependencies from provider
        logger_ = provider.getService<ILogger>();
        database_ = provider.getService<IDatabase>();
    }
    
    std::string getData() const override { 
        return "data-" + std::to_string(instanceId_); 
    }
    
    int getInstanceId() const override { return instanceId_; }
    
    std::shared_ptr<ILogger> getLogger() const { return logger_; }
    std::shared_ptr<IDatabase> getDatabase() const { return database_; }

private:
    int instanceId_;
    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<IDatabase> database_;
};

// Helper to reset counters
void resetCounters() {
    g_loggerInstanceCounter = 0;
    g_databaseInstanceCounter = 0;
    g_repositoryInstanceCounter = 0;
}

TEST_CASE("ServiceCollection - Transient services", "[di][transient]") {
    resetCounters();
    
    http::ServiceCollection services;
    services.addTransient<ILogger, TestLogger>();
    
    auto provider = services.buildServiceProvider();
    auto scope = provider->createScope();
    auto& scopedProvider = scope->getServiceProvider();
    
    SECTION("Creates new instance on each request") {
        auto logger1 = scopedProvider.getService<ILogger>();
        auto logger2 = scopedProvider.getService<ILogger>();
        auto logger3 = scopedProvider.getService<ILogger>();
        
        REQUIRE(logger1 != nullptr);
        REQUIRE(logger2 != nullptr);
        REQUIRE(logger3 != nullptr);
        
        // Different instances
        REQUIRE(logger1->getInstanceId() != logger2->getInstanceId());
        REQUIRE(logger2->getInstanceId() != logger3->getInstanceId());
        REQUIRE(logger1->getInstanceId() == 1);
        REQUIRE(logger2->getInstanceId() == 2);
        REQUIRE(logger3->getInstanceId() == 3);
    }
}

TEST_CASE("ServiceCollection - Scoped services", "[di][scoped]") {
    resetCounters();
    
    http::ServiceCollection services;
    services.addScoped<IRepository, TestRepository>();
    services.addScoped<ILogger, TestLogger>();  // Dependency
    services.addSingleton<IDatabase, TestDatabase>();  // Dependency
    
    auto provider = services.buildServiceProvider();
    
    SECTION("Reuses same instance within scope") {
        auto scope1 = provider->createScope();
        auto& scopedProvider1 = scope1->getServiceProvider();
        
        auto repo1a = scopedProvider1.getService<IRepository>();
        auto repo1b = scopedProvider1.getService<IRepository>();
        
        REQUIRE(repo1a != nullptr);
        REQUIRE(repo1b != nullptr);
        
        // Same instance within scope
        REQUIRE(repo1a->getInstanceId() == repo1b->getInstanceId());
        REQUIRE(repo1a.get() == repo1b.get());
    }
    
    SECTION("Creates new instance in different scope") {
        auto scope1 = provider->createScope();
        auto scope2 = provider->createScope();
        
        auto repo1 = scope1->getServiceProvider().getService<IRepository>();
        auto repo2 = scope2->getServiceProvider().getService<IRepository>();
        
        REQUIRE(repo1 != nullptr);
        REQUIRE(repo2 != nullptr);
        
        // Different instances across scopes
        REQUIRE(repo1->getInstanceId() != repo2->getInstanceId());
        REQUIRE(repo1.get() != repo2.get());
    }
    
    SECTION("Scoped dependencies are resolved from same scope") {
        auto scope = provider->createScope();
        auto& scopedProvider = scope->getServiceProvider();
        
        auto repo1 = scopedProvider.getService<IRepository>();
        auto repo2 = scopedProvider.getService<IRepository>();
        
        // Logger dependency should be same instance (scoped)
        REQUIRE(repo1->getLogger() == repo2->getLogger());
        REQUIRE(repo1->getLogger()->getInstanceId() == repo2->getLogger()->getInstanceId());
    }
}

TEST_CASE("ServiceCollection - Singleton services", "[di][singleton]") {
    resetCounters();
    
    http::ServiceCollection services;
    services.addSingleton<IDatabase, TestDatabase>();
    
    auto provider = services.buildServiceProvider();
    
    SECTION("Returns same instance across all requests") {
        auto scope1 = provider->createScope();
        auto scope2 = provider->createScope();
        
        auto db1 = scope1->getServiceProvider().getService<IDatabase>();
        auto db2 = scope2->getServiceProvider().getService<IDatabase>();
        auto db3 = scope1->getServiceProvider().getService<IDatabase>();
        
        REQUIRE(db1 != nullptr);
        REQUIRE(db2 != nullptr);
        REQUIRE(db3 != nullptr);
        
        // Same instance everywhere
        REQUIRE(db1->getInstanceId() == db2->getInstanceId());
        REQUIRE(db2->getInstanceId() == db3->getInstanceId());
        REQUIRE(db1.get() == db2.get());
        REQUIRE(db2.get() == db3.get());
    }
}

TEST_CASE("ServiceCollection - Thread safety", "[di][singleton][thread-safety]") {
    resetCounters();
    
    http::ServiceCollection services;
    services.addSingleton<IDatabase, TestDatabase>();
    
    auto provider = services.buildServiceProvider();
    
    SECTION("Singleton creation is thread-safe") {
        std::vector<std::thread> threads;
        std::vector<std::shared_ptr<IDatabase>> results(10);
        
        // Create 10 threads that all try to get singleton simultaneously
        for (int i = 0; i < 10; ++i) {
            threads.emplace_back([&provider, &results, i]() {
                auto scope = provider->createScope();
                results[i] = scope->getServiceProvider().getService<IDatabase>();
            });
        }
        
        // Wait for all threads
        for (auto& thread : threads) {
            thread.join();
        }
        
        // All should have same instance (only one created)
        int firstInstanceId = results[0]->getInstanceId();
        for (const auto& db : results) {
            REQUIRE(db != nullptr);
            REQUIRE(db->getInstanceId() == firstInstanceId);
            REQUIRE(db.get() == results[0].get());
        }
        
        // Only one database instance created
        REQUIRE(g_databaseInstanceCounter == 1);
    }
}

TEST_CASE("ServiceCollection - Service not found", "[di][error]") {
    http::ServiceCollection services;
    services.addScoped<ILogger, TestLogger>();
    
    auto provider = services.buildServiceProvider();
    auto scope = provider->createScope();
    auto& scopedProvider = scope->getServiceProvider();
    
    SECTION("getService throws if not found") {
        REQUIRE_THROWS_AS(
            scopedProvider.getService<IDatabase>(),
            std::runtime_error
        );
    }
    
    SECTION("getOptionalService returns nullptr if not found") {
        auto db = scopedProvider.getOptionalService<IDatabase>();
        REQUIRE(db == nullptr);
    }
}

TEST_CASE("ServiceCollection - Dependency injection", "[di][dependencies]") {
    resetCounters();
    
    http::ServiceCollection services;
    services.addScoped<IRepository, TestRepository>();
    services.addScoped<ILogger, TestLogger>();
    services.addSingleton<IDatabase, TestDatabase>();
    
    auto provider = services.buildServiceProvider();
    auto scope = provider->createScope();
    auto& scopedProvider = scope->getServiceProvider();
    
    SECTION("Service receives provider and resolves dependencies") {
        auto repo = scopedProvider.getService<IRepository>();
        
        REQUIRE(repo != nullptr);
        REQUIRE(repo->getLogger() != nullptr);
        REQUIRE(repo->getDatabase() != nullptr);
        
        REQUIRE(repo->getLogger()->getName() == "TestLogger");
        REQUIRE(repo->getDatabase()->getConnectionString() == "postgresql://localhost");
    }
    
    SECTION("Dependencies have correct lifetimes") {
        auto scope1 = provider->createScope();
        auto scope2 = provider->createScope();
        
        auto repo1 = scope1->getServiceProvider().getService<IRepository>();
        auto repo2 = scope2->getServiceProvider().getService<IRepository>();
        
        // Repositories are scoped - different instances
        REQUIRE(repo1->getInstanceId() != repo2->getInstanceId());
        
        // Loggers are scoped - different instances across scopes
        REQUIRE(repo1->getLogger()->getInstanceId() != repo2->getLogger()->getInstanceId());
        
        // Database is singleton - same instance everywhere
        REQUIRE(repo1->getDatabase()->getInstanceId() == repo2->getDatabase()->getInstanceId());
        REQUIRE(repo1->getDatabase().get() == repo2->getDatabase().get());
    }
}

TEST_CASE("ServiceCollection - Custom factory", "[di][factory]") {
    resetCounters();
    
    http::ServiceCollection services;
    
    // Register with custom factory
    services.addService<ILogger>(
        [](http::IServiceProvider& provider) -> std::shared_ptr<ILogger> {
            (void)provider;  // Unused
            return std::make_shared<TestLogger>(provider);
        },
        http::ServiceLifetime::Transient
    );
    
    auto provider = services.buildServiceProvider();
    auto scope = provider->createScope();
    auto& scopedProvider = scope->getServiceProvider();
    
    SECTION("Custom factory works") {
        auto logger1 = scopedProvider.getService<ILogger>();
        auto logger2 = scopedProvider.getService<ILogger>();
        
        REQUIRE(logger1 != nullptr);
        REQUIRE(logger2 != nullptr);
        
        // Transient - different instances
        REQUIRE(logger1->getInstanceId() != logger2->getInstanceId());
    }
}

TEST_CASE("ServiceCollection - Multiple scopes", "[di][scope]") {
    resetCounters();
    
    http::ServiceCollection services;
    services.addScoped<IRepository, TestRepository>();
    services.addScoped<ILogger, TestLogger>();
    services.addSingleton<IDatabase, TestDatabase>();
    
    auto provider = services.buildServiceProvider();
    
    SECTION("Multiple scopes are independent") {
        auto scope1 = provider->createScope();
        auto scope2 = provider->createScope();
        auto scope3 = provider->createScope();
        
        auto repo1 = scope1->getServiceProvider().getService<IRepository>();
        auto repo2 = scope2->getServiceProvider().getService<IRepository>();
        auto repo3 = scope3->getServiceProvider().getService<IRepository>();
        
        // All different repositories
        REQUIRE(repo1->getInstanceId() == 1);
        REQUIRE(repo2->getInstanceId() == 2);
        REQUIRE(repo3->getInstanceId() == 3);
        
        // All different loggers (scoped)
        REQUIRE(repo1->getLogger()->getInstanceId() == 1);
        REQUIRE(repo2->getLogger()->getInstanceId() == 2);
        REQUIRE(repo3->getLogger()->getInstanceId() == 3);
        
        // Same database (singleton)
        REQUIRE(repo1->getDatabase()->getInstanceId() == 1);
        REQUIRE(repo2->getDatabase()->getInstanceId() == 1);
        REQUIRE(repo3->getDatabase()->getInstanceId() == 1);
    }
}
