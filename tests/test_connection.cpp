#include <gtest/gtest.h>
#include <signal.hpp>
#include <functional>

class ConnectionTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(ConnectionTest, DefaultConstructor) {
    ant::connection conn;
    conn.disconnect();
}

TEST_F(ConnectionTest, ConstructorWithFunction) {
    bool called = false;
    auto disconnect_func = [&called]() { called = true; };
    
    {
        ant::connection conn(disconnect_func);
    }
    
    EXPECT_TRUE(called);
}

TEST_F(ConnectionTest, MoveConstructor) {
    bool called = false;
    auto disconnect_func = [&called]() { called = true; };
    
    ant::connection conn1(disconnect_func);
    ant::connection conn2(std::move(conn1));
    
    conn1.disconnect();
    EXPECT_FALSE(called);
    
    conn2.disconnect();
    EXPECT_TRUE(called);
}

TEST_F(ConnectionTest, MoveAssignment) {
    bool called1 = false, called2 = false;
    auto disconnect_func1 = [&called1]() { called1 = true; };
    auto disconnect_func2 = [&called2]() { called2 = true; };
    
    ant::connection conn1(disconnect_func1);
    ant::connection conn2(disconnect_func2);
    
    conn1 = std::move(conn2);
    
    EXPECT_TRUE(called1);
    EXPECT_FALSE(called2);
    
    conn1.disconnect();
    EXPECT_TRUE(called2);
}

TEST_F(ConnectionTest, ManualDisconnect) {
    bool called = false;
    auto disconnect_func = [&called]() { called = true; };
    
    ant::connection conn(disconnect_func);
    conn.disconnect();
    EXPECT_TRUE(called);
    
    called = false;
    conn.disconnect();
    EXPECT_FALSE(called);
}