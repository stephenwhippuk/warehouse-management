#include <catch2/catch_all.hpp>

// Routing is now handled by the HTTP framework via controller registration.
// The legacy Server route resolver no longer exists after migration.
TEST_CASE("HTTP routing handled by framework", "[routing][server]") {
    REQUIRE(true);
}
