# ANT Signal Library

A modern C++23 header-only signal-slot implementation with automatic connection management and RAII-based lifetime handling.

## Features

- **Header-only**: No compilation required, just include the header
- **Type-safe**: Template-based signals with compile-time type checking
- **RAII**: Automatic connection cleanup when objects are destroyed
- **Weak pointer support**: Automatic disconnection when connected objects are destroyed
- **Exception safe**: Slot exceptions don't affect other slots
- **Modern C++**: Uses C++23 features for clean, efficient code
- **Zero dependencies**: Only uses standard library

## Quick Start

```cpp
#include <signal.hpp>
#include <iostream>

int main() {
    // Create a signal that takes an int parameter
    ant::signal<int> number_signal;
    
    // Connect a lambda to the signal
    auto connection = number_signal.connect([](int value) {
        std::cout << "Received: " << value << std::endl;
    });
    
    // Emit the signal
    number_signal.emit(42);  // Output: "Received: 42"
    
    return 0;
}
```

## Basic Usage

### Creating Signals

```cpp
// Signal with no parameters
ant::signal<> simple_signal;

// Signal with one parameter
ant::signal<int> int_signal;

// Signal with multiple parameters
ant::signal<std::string, int, double> complex_signal;

// Using the SIGNAL macro for named signals
class Publisher {
public:
    SIGNAL(data_changed, const std::string&);
    SIGNAL(value_updated, int);
};
```

### Connecting Slots

```cpp
// Connect lambda functions
auto conn1 = signal.connect([](int value) {
    std::cout << "Lambda received: " << value << std::endl;
});

// Connect member functions
class Handler {
public:
    void handleValue(int value) {
        std::cout << "Handler received: " << value << std::endl;
    }
};

auto handler = std::make_shared<Handler>();
auto conn2 = signal.connect(handler, &Handler::handleValue);
```

### Emitting Signals

```cpp
// Using emit()
signal.emit(42);

// Using function call operator
signal(42);
```

### Connection Management

```cpp
// Manual disconnection
connection.disconnect();

// Automatic disconnection when connection object is destroyed
{
    auto conn = signal.connect([](int value) { /* ... */ });
    signal.emit(1);  // Lambda is called
}  // Connection destroyed here
signal.emit(2);     // Lambda is not called
```

### Automatic Cleanup with auto_disconnect

```cpp
class Widget : public ant::auto_disconnect {
public:
    void connectToSignal(ant::signal<int>& sig) {
        // Connection will be automatically cleaned up when Widget is destroyed
        add_connection(sig.connect([this](int value) {
            handleValue(value);
        }));
    }
    
private:
    void handleValue(int value) { /* ... */ }
};
```

## Building the Library

Since this is a header-only library, no compilation is required. However, you can build the tests and examples.

### Prerequisites

- C++23 compatible compiler (GCC 12+, Clang 15+, MSVC 2022+)
- CMake 3.20+
- Google Test (for tests only)

### Building Tests

```bash
# Create build directory
mkdir build && cd build

# Configure
cmake .. -DBUILD_TESTING=ON

# Build
cmake --build .

# Run tests
ctest
```

### Building Examples

```bash
# In the examples directory
cd examples
mkdir build && cd build

# Configure and build
cmake ..
cmake --build .

# Run examples
./01_basic_usage
./02_observer_pattern
./03_event_system
```

## Using as External Library

### Method 1: CMake Integration

Add this to your `CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.20)
project(MyProject)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add ant signal library
add_subdirectory(path/to/ant-signal)

# Your executable
add_executable(my_app main.cpp)

# Link against ant signal library
target_link_libraries(my_app PRIVATE ant)
```

### Method 2: Include Path

If you just want to include the header:

```cmake
cmake_minimum_required(VERSION 3.20)
project(MyProject)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add include directory
include_directories(path/to/ant-signal/src)

add_executable(my_app main.cpp)
target_compile_features(my_app PRIVATE cxx_std_23)
```

### Method 3: Copy Header

Simply copy `src/signal.hpp` to your project and include it:

```cpp
#include "signal.hpp"  // Adjust path as needed
```

## Examples

### Observer Pattern

```cpp
#include <signal.hpp>

class WeatherStation {
public:
    SIGNAL(temperature_changed, double);
    
    void setTemperature(double temp) {
        temperature_ = temp;
        temperature_changed(temp);
    }
    
private:
    double temperature_ = 20.0;
};

class Display : public ant::auto_disconnect {
public:
    void connectToStation(WeatherStation& station) {
        add_connection(station.temperature_changed.connect([this](double temp) {
            std::cout << "Temperature: " << temp << "°C" << std::endl;
        }));
    }
};

int main() {
    WeatherStation station;
    Display display;
    
    display.connectToStation(station);
    station.setTemperature(25.5);  // Output: "Temperature: 25.5°C"
    
    return 0;
}
```

### Event System

```cpp
#include <signal.hpp>

class EventBus {
public:
    SIGNAL(user_logged_in, const std::string&);
    SIGNAL(file_uploaded, const std::string&, size_t);
    
    static EventBus& instance() {
        static EventBus bus;
        return bus;
    }
};

class Analytics : public ant::auto_disconnect {
public:
    Analytics() {
        auto& bus = EventBus::instance();
        
        add_connection(bus.user_logged_in.connect([](const std::string& user) {
            std::cout << "Analytics: User " << user << " logged in" << std::endl;
        }));
        
        add_connection(bus.file_uploaded.connect([](const std::string& user, size_t size) {
            std::cout << "Analytics: " << user << " uploaded " << size << " bytes" << std::endl;
        }));
    }
};

int main() {
    Analytics analytics;  // Automatically subscribes to events
    
    auto& bus = EventBus::instance();
    bus.user_logged_in("alice");
    bus.file_uploaded("alice", 1024);
    
    return 0;
}
```

## API Reference

### Classes

- **`ant::signal<Args...>`**: Main signal class template
- **`ant::connection`**: RAII connection handle
- **`ant::auto_disconnect`**: Base class for automatic connection management

### Macros

- **`SIGNAL(name, ...)`**: Creates a named signal member variable

### Key Methods

- **`signal::connect(func)`**: Connect function/lambda to signal
- **`signal::connect(obj, method)`**: Connect member function to signal
- **`signal::emit(args...)`**: Emit signal with arguments
- **`signal::operator()(args...)`**: Alternative to emit()
- **`signal::disconnect_all()`**: Disconnect all slots
- **`signal::slot_count()`**: Get number of connected slots
- **`connection::disconnect()`**: Manually disconnect slot
- **`auto_disconnect::add_connection(conn)`**: Add connection for automatic cleanup

## Requirements

- C++23 compatible compiler
- Standard library with `<functional>`, `<memory>`, `<vector>`, `<algorithm>`, `<concepts>`, `<optional>`

## Thread Safety

This library is **not thread-safe**. If you need to use signals across multiple threads, you must provide your own synchronization.

## License

[Add your license here]

## Contributing

[Add contribution guidelines here]