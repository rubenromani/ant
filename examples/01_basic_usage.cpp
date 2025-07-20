/**
 * @file 01_basic_usage.cpp
 * @brief Basic usage examples of the ant signal library
 * 
 * This example demonstrates the fundamental features of the ant signal library:
 * - Creating signals with different argument types
 * - Connecting lambda functions to signals
 * - Connecting member functions to signals
 * - Using the SIGNAL macro for convenience
 * - Manual connection management
 * - Automatic connection cleanup
 */

#include <signal.hpp>
#include <iostream>
#include <string>
#include <memory>

// Example class to demonstrate member function connections
class Printer {
public:
    void printMessage(const std::string& msg) {
        std::cout << "[Printer] " << msg << std::endl;
    }
    
    void printNumber(int number) {
        std::cout << "[Printer] Number: " << number << std::endl;
    }
    
    void printCoordinates(int x, int y) {
        std::cout << "[Printer] Position: (" << x << ", " << y << ")" << std::endl;
    }
};

// Example of using the SIGNAL macro
class Publisher {
public:
    // These create named signals with type information
    SIGNAL(message_published, const std::string&);
    SIGNAL(number_generated, int);
    SIGNAL(coordinates_updated, int, int);
    
    void publishMessage(const std::string& msg) {
        std::cout << "Publishing message: " << msg << std::endl;
        message_published(msg);
    }
    
    void generateNumber(int num) {
        std::cout << "Generating number: " << num << std::endl;
        number_generated(num);
    }
    
    void updateCoordinates(int x, int y) {
        std::cout << "Updating coordinates: (" << x << ", " << y << ")" << std::endl;
        coordinates_updated(x, y);
    }
};

void demonstrateBasicConnections() {
    std::cout << "=== Basic Signal Connections ===" << std::endl;
    
    // Create a simple signal that takes a string parameter
    ant::signal<std::string> text_signal;
    
    // Connect a lambda function to the signal
    auto connection1 = text_signal.connect([](const std::string& text) {
        std::cout << "Lambda received: " << text << std::endl;
    });
    
    // Connect another lambda with different behavior
    auto connection2 = text_signal.connect([](const std::string& text) {
        std::cout << "Lambda 2 received (uppercase): ";
        for (char c : text) {
            std::cout << static_cast<char>(std::toupper(c));
        }
        std::cout << std::endl;
    });
    
    // Emit the signal - both lambdas will be called
    text_signal.emit("Hello World!");
    std::cout << "Active connections: " << text_signal.slot_count() << std::endl;
    
    // Manual disconnection
    connection1.disconnect();
    std::cout << "After disconnecting first lambda: " << text_signal.slot_count() << std::endl;
    
    text_signal.emit("Second emission");
    
    std::cout << std::endl;
}

void demonstrateMemberFunctionConnections() {
    std::cout << "=== Member Function Connections ===" << std::endl;
    
    // Create signals with different signatures
    ant::signal<const std::string&> string_signal;
    ant::signal<int> int_signal;
    ant::signal<int, int> coordinate_signal;
    
    // Create a shared pointer to our printer object
    auto printer = std::make_shared<Printer>();
    
    // Connect member functions to the signals
    auto conn1 = string_signal.connect(printer, &Printer::printMessage);
    auto conn2 = int_signal.connect(printer, &Printer::printNumber);
    auto conn3 = coordinate_signal.connect(printer, &Printer::printCoordinates);
    
    std::cout << "String signal connections: " << string_signal.slot_count() << std::endl;
    std::cout << "Int signal connections: " << int_signal.slot_count() << std::endl;
    std::cout << "Coordinate signal connections: " << coordinate_signal.slot_count() << std::endl;
    
    // Emit the signals
    string_signal("Hello from member function!");
    int_signal(42);
    coordinate_signal(10, 20);
    
    std::cout << std::endl;
}

void demonstrateAutomaticCleanup() {
    std::cout << "=== Automatic Cleanup on Object Destruction ===" << std::endl;
    
    ant::signal<const std::string&> signal;
    
    // Connect a lambda first
    auto lambda_conn = signal.connect([](const std::string& msg) {
        std::cout << "Lambda: " << msg << std::endl;
    });
    
    {
        // Create a printer object in a limited scope
        auto printer = std::make_shared<Printer>();
        auto member_conn = signal.connect(printer, &Printer::printMessage);
        
        std::cout << "Connections before scope exit: " << signal.slot_count() << std::endl;
        signal("Message while printer exists");
        
        // printer goes out of scope here and is destroyed
    }
    
    // The member function connection should be automatically cleaned up
    std::cout << "Connections after printer destruction: " << signal.slot_count() << std::endl;
    signal("Message after printer destroyed");
    
    std::cout << std::endl;
}

void demonstrateSIGNALMacro() {
    std::cout << "=== SIGNAL Macro Usage ===" << std::endl;
    
    Publisher pub;
    auto printer = std::make_shared<Printer>();
    
    // Connect to the named signals using the SIGNAL macro
    auto conn1 = pub.message_published.connect(printer, &Printer::printMessage);
    auto conn2 = pub.number_generated.connect(printer, &Printer::printNumber);
    auto conn3 = pub.coordinates_updated.connect(printer, &Printer::printCoordinates);
    
    // Also connect some lambdas
    auto conn4 = pub.message_published.connect([](const std::string& msg) {
        std::cout << "[Lambda] Got message: " << msg << std::endl;
    });
    
    auto conn5 = pub.number_generated.connect([](int num) {
        std::cout << "[Lambda] Got number: " << num << " (squared: " << num*num << ")" << std::endl;
    });
    
    // Publish some events
    pub.publishMessage("Important announcement!");
    pub.generateNumber(7);
    pub.updateCoordinates(100, 200);
    
    std::cout << std::endl;
}

void demonstrateMultipleArguments() {
    std::cout << "=== Multiple Argument Signals ===" << std::endl;
    
    // Signal with multiple arguments of different types
    ant::signal<std::string, int, double, bool> complex_signal;
    
    auto connection = complex_signal.connect([](const std::string& name, int id, double value, bool active) {
        std::cout << "Received complex data:" << std::endl;
        std::cout << "  Name: " << name << std::endl;
        std::cout << "  ID: " << id << std::endl;
        std::cout << "  Value: " << value << std::endl;
        std::cout << "  Active: " << (active ? "true" : "false") << std::endl;
    });
    
    complex_signal.emit("TestObject", 12345, 3.14159, true);
    
    std::cout << std::endl;
}

void demonstrateConnectionLifetime() {
    std::cout << "=== Connection Lifetime Management ===" << std::endl;
    
    ant::signal<int> counter_signal;
    
    // Store connections in a container for dynamic management
    std::vector<std::unique_ptr<ant::connection>> connections;
    
    // Create multiple connections
    for (int i = 0; i < 3; ++i) {
        connections.push_back(std::make_unique<ant::connection>(
            counter_signal.connect([i](int value) {
                std::cout << "Handler " << i << " received: " << value << std::endl;
            })
        ));
    }
    
    std::cout << "Created " << counter_signal.slot_count() << " connections" << std::endl;
    counter_signal.emit(100);
    
    // Remove the middle connection
    connections.erase(connections.begin() + 1);
    std::cout << "After removing middle connection: " << counter_signal.slot_count() << std::endl;
    counter_signal.emit(200);
    
    // Clear all connections
    connections.clear();
    std::cout << "After clearing all connections: " << counter_signal.slot_count() << std::endl;
    counter_signal.emit(300);
    
    std::cout << std::endl;
}

int main() {
    std::cout << "ANT Signal Library - Basic Usage Examples" << std::endl;
    std::cout << "=========================================" << std::endl << std::endl;
    
    demonstrateBasicConnections();
    demonstrateMemberFunctionConnections();
    demonstrateAutomaticCleanup();
    demonstrateSIGNALMacro();
    demonstrateMultipleArguments();
    demonstrateConnectionLifetime();
    
    std::cout << "All basic examples completed successfully!" << std::endl;
    return 0;
}