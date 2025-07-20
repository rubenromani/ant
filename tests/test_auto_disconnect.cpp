#include <gtest/gtest.h>
#include <signal.hpp>
#include <memory>

class TestWidget : public ant::auto_disconnect {
public:
    int value = 0;
    
    void connectToSignal(ant::signal<int>& sig) {
        add_connection(sig.connect([this](int v) { value = v; }));
    }
    
    void connectMultipleSignals(ant::signal<int>& sig1, ant::signal<std::string>& sig2) {
        add_connection(sig1.connect([this](int v) { value = v; }));
        add_connection(sig2.connect([this](const std::string& s) { 
            value = static_cast<int>(s.length()); 
        }));
    }
};

class AutoDisconnectTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(AutoDisconnectTest, BasicAutoDisconnect) {
    ant::signal<int> sig;
    
    {
        TestWidget widget;
        widget.connectToSignal(sig);
        
        EXPECT_EQ(sig.slot_count(), 1);
        
        sig.emit(42);
        EXPECT_EQ(widget.value, 42);
    }
    
    EXPECT_EQ(sig.slot_count(), 0);
}

TEST_F(AutoDisconnectTest, MultipleConnections) {
    ant::signal<int> sig1;
    ant::signal<std::string> sig2;
    
    {
        TestWidget widget;
        widget.connectMultipleSignals(sig1, sig2);
        
        EXPECT_EQ(sig1.slot_count(), 1);
        EXPECT_EQ(sig2.slot_count(), 1);
        
        sig1.emit(100);
        EXPECT_EQ(widget.value, 100);
        
        sig2.emit("hello");
        EXPECT_EQ(widget.value, 5);
    }
    
    EXPECT_EQ(sig1.slot_count(), 0);
    EXPECT_EQ(sig2.slot_count(), 0);
}

TEST_F(AutoDisconnectTest, PartialDestruction) {
    ant::signal<int> sig;
    auto widget1 = std::make_unique<TestWidget>();
    auto widget2 = std::make_unique<TestWidget>();
    
    widget1->connectToSignal(sig);
    widget2->connectToSignal(sig);
    
    EXPECT_EQ(sig.slot_count(), 2);
    
    sig.emit(50);
    EXPECT_EQ(widget1->value, 50);
    EXPECT_EQ(widget2->value, 50);
    
    widget1.reset();
    EXPECT_EQ(sig.slot_count(), 1);
    
    sig.emit(75);
    EXPECT_EQ(widget2->value, 75);
    
    widget2.reset();
    EXPECT_EQ(sig.slot_count(), 0);
}

class DerivedWidget : public TestWidget {
public:
    int derived_value = 0;
    
    void connectAdditional(ant::signal<int>& sig) {
        add_connection(sig.connect([this](int v) { derived_value = v * 2; }));
    }
};

TEST_F(AutoDisconnectTest, InheritanceSupport) {
    ant::signal<int> sig;
    
    {
        DerivedWidget widget;
        widget.connectToSignal(sig);
        widget.connectAdditional(sig);
        
        EXPECT_EQ(sig.slot_count(), 2);
        
        sig.emit(10);
        EXPECT_EQ(widget.value, 10);
        EXPECT_EQ(widget.derived_value, 20);
    }
    
    EXPECT_EQ(sig.slot_count(), 0);
}