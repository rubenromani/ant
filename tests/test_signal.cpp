#include <gtest/gtest.h>
#include <signal.hpp>
#include <memory>
#include <vector>

class SignalTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(SignalTest, DefaultConstructor) {
    ant::signal<int> sig;
    EXPECT_EQ(sig.slot_count(), 0);
}

TEST_F(SignalTest, NamedConstructor) {
    ant::signal<int> sig("test_signal");
    EXPECT_EQ(sig.slot_count(), 0);
}

TEST_F(SignalTest, ConnectLambda) {
    ant::signal<int> sig;
    int received_value = 0;
    
    auto conn = sig.connect([&](int value) {
        received_value = value;
    });
    
    EXPECT_EQ(sig.slot_count(), 1);
    
    sig.emit(42);
    EXPECT_EQ(received_value, 42);
}

TEST_F(SignalTest, ConnectMultipleLambdas) {
    ant::signal<int> sig;
    std::vector<int> received_values;
    
    auto conn1 = sig.connect([&](int value) {
        received_values.push_back(value * 2);
    });
    
    auto conn2 = sig.connect([&](int value) {
        received_values.push_back(value * 3);
    });
    
    EXPECT_EQ(sig.slot_count(), 2);
    
    sig.emit(10);
    EXPECT_EQ(received_values.size(), 2);
    EXPECT_TRUE(std::find(received_values.begin(), received_values.end(), 20) != received_values.end());
    EXPECT_TRUE(std::find(received_values.begin(), received_values.end(), 30) != received_values.end());
}

class TestObject {
public:
    int value = 0;
    void setValue(int v) { value = v; }
    void addValue(int v) { value += v; }
};

TEST_F(SignalTest, ConnectMemberFunction) {
    ant::signal<int> sig;
    auto obj = std::make_shared<TestObject>();
    
    auto conn = sig.connect(obj, &TestObject::setValue);
    EXPECT_EQ(sig.slot_count(), 1);
    
    sig.emit(100);
    EXPECT_EQ(obj->value, 100);
}

TEST_F(SignalTest, AutoDisconnectOnObjectDestruction) {
    ant::signal<int> sig;
    
    {
        auto obj = std::make_shared<TestObject>();
        auto conn = sig.connect(obj, &TestObject::setValue);
        EXPECT_EQ(sig.slot_count(), 1);
        
        sig.emit(50);
        EXPECT_EQ(obj->value, 50);
    }
    
    sig.emit(100);
    EXPECT_EQ(sig.slot_count(), 0);
}

TEST_F(SignalTest, FunctionCallOperator) {
    ant::signal<int> sig;
    int received_value = 0;
    
    auto conn = sig.connect([&](int value) {
        received_value = value;
    });
    
    sig(75);
    EXPECT_EQ(received_value, 75);
}

TEST_F(SignalTest, DisconnectAll) {
    ant::signal<int> sig;
    
    auto conn1 = sig.connect([](int) {});
    auto conn2 = sig.connect([](int) {});
    auto conn3 = sig.connect([](int) {});
    
    EXPECT_EQ(sig.slot_count(), 3);
    
    sig.disconnect_all();
    EXPECT_EQ(sig.slot_count(), 0);
}

TEST_F(SignalTest, ConnectionLifetime) {
    ant::signal<int> sig;
    int call_count = 0;
    
    {
        auto conn = sig.connect([&](int) { call_count++; });
        sig.emit(1);
        EXPECT_EQ(call_count, 1);
    }
    
    sig.emit(2);
    EXPECT_EQ(call_count, 1);
    EXPECT_EQ(sig.slot_count(), 0);
}

TEST_F(SignalTest, MultipleArguments) {
    ant::signal<int, std::string, double> sig;
    int int_val = 0;
    std::string str_val;
    double double_val = 0.0;
    
    auto conn = sig.connect([&](int i, const std::string& s, double d) {
        int_val = i;
        str_val = s;
        double_val = d;
    });
    
    sig.emit(42, "hello", 3.14);
    
    EXPECT_EQ(int_val, 42);
    EXPECT_EQ(str_val, "hello");
    EXPECT_DOUBLE_EQ(double_val, 3.14);
}

TEST_F(SignalTest, ExceptionSafety) {
    ant::signal<int> sig;
    int safe_call_count = 0;
    
    auto conn1 = sig.connect([](int) {
        throw std::runtime_error("test exception");
    });
    
    auto conn2 = sig.connect([&](int) {
        safe_call_count++;
    });
    
    EXPECT_NO_THROW(sig.emit(1));
    EXPECT_EQ(safe_call_count, 1);
}