/**
 * @file signal.hpp
 * @brief A modern C++ signal-slot implementation with automatic connection management
 * 
 * This library provides a type-safe, thread-safe signal-slot system that allows
 * decoupled communication between objects. It supports:
 * - Function object connections
 * - Member function connections with automatic lifetime management
 * - RAII-based connection management
 * - Automatic cleanup of expired connections
 * 
 * @author Generated
 * @version 1.0
 */

#ifndef SIGNAL_HPP
#define SIGNAL_HPP

#include <functional>
#include <vector>
#include <memory>
#include <algorithm>
#include <concepts>
#include <optional>
#include <deque>

/**
 * @brief Convenience macro for creating named signals
 * @param name Variable name for the signal
 * @param ... Template arguments for the signal type
 * 
 * Example:
 * @code
 * SIGNAL(dataChanged, int, std::string); // Creates ant::signal<int, std::string> dataChanged;
 * @endcode
 */
#define SIGNAL(name, ...) ant::signal<__VA_ARGS__> name

namespace ant {

/**
 * @class connection
 * @brief RAII wrapper for managing signal-slot connections
 * 
 * connection objects are returned by signal::connect() and automatically
 * disconnect the associated slot when destroyed. They use move semantics
 * and cannot be copied to ensure unique ownership of the connection.
 * 
 * Example:
 * @code
 * signal<int> sig;
 * {
 *     auto conn = sig.connect([](int x) { std::cout << x; });
 *     sig.emit(42); // prints 42
 * } // connection automatically disconnected here
 * sig.emit(42); // nothing happens
 * @endcode
 */
class connection {
public:
        /// Default constructor creates an invalid connection
        connection() = default;
        /**
         * @brief Constructs a connection with a disconnect function
         * @param func Function to call when disconnecting
         */
        connection(std::function<void()> func) : disconnect_func(std::move(func)){
        }
        
        /**
         * @brief Move constructor
         * @param other connection to move from
         */
        connection(connection&& other) noexcept :
                disconnect_func(std::move(other.disconnect_func)), connected(other.connected){
                        other.connected = false;
                }

        /**
         * @brief Move assignment operator
         * @param other connection to move from
         * @return Reference to this connection
         */
        connection& operator=(connection&& other) noexcept {
                if (this != &other) {
                        disconnect();
                        disconnect_func = std::move(other.disconnect_func);
                        connected = other.connected;
                        other.connected = false;
                }

                return *this;
        }

        /// Copying connections is not allowed to ensure unique ownership
        connection(const connection&) = delete;
        /// Copy assignment is not allowed to ensure unique ownership
        connection& operator=(const connection&) = delete;

        /**
         * @brief Destructor automatically disconnects the slot
         */
        ~connection(){
                disconnect();
        }

        /**
         * @brief Manually disconnect the slot
         * 
         * After calling this function, the connection becomes invalid.
         * Safe to call multiple times.
         */
        void disconnect() {
                if( connected && disconnect_func) {
                        disconnect_func();
                }
                connected = false;
        }

private: 
        std::function<void()> disconnect_func; ///< Function to call for disconnection
        bool connected = true; ///< Whether this connection is still active
};

/**
 * @class signal
 * @brief Type-safe signal class for implementing the observer pattern
 * 
 * @tparam Args Types of arguments passed to connected slots
 * 
 * signal allows multiple functions (slots) to be connected and called
 * simultaneously when emit() is invoked. It supports both free functions
 * and member functions with automatic lifetime management.
 * 
 * Example:
 * @code
 * signal<int, std::string> sig;
 * 
 * // Connect a lambda
 * auto conn1 = sig.connect([](int i, const std::string& s) {
 *     std::cout << i << ": " << s << std::endl;
 * });
 * 
 * // Connect a member function
 * auto obj = std::make_shared<MyClass>();
 * auto conn2 = sig.connect(obj, &MyClass::handleSignal);
 * 
 * sig.emit(42, "Hello"); // calls both slots
 * @endcode
 */
template<typename... Args>
class signal {
public:
        /// Default constructor
        signal() = default;

        /// Signals cannot be copied to avoid connection confusion
        signal(const signal&) = delete;
        /// Copy assignment is not allowed
        signal& operator=(const signal&) = delete;
        /// Move constructor is allowed
        signal(signal&&) = default;
        /// Move assignment is allowed
        signal& operator=(signal&&) = default;

        /**
         * @brief Connect a function object to this signal
         * @param slot Function to call when signal is emitted
         * @return connection object for managing the connection lifetime
         * 
         * The returned connection must be kept alive to maintain the connection.
         * When the connection is destroyed, the slot is automatically disconnected.
         */
        [[nodiscard]] connection connect(std::function<void(Args...)> slot) {
                cleanup_expired();
                size_t id = next_id++;

                slots.emplace_back(std::make_shared<SlotWrapper>(std::move(slot), std::nullopt, id));
                return connection([this, id](){
                                this->disconnect_by_id(id);
                                });
        }

        /**
         * @brief Connect a member function to this signal with automatic lifetime management
         * @tparam T Type of the object containing the member function
         * @param object Shared pointer to the object instance
         * @param method Pointer to member function to call
         * @return connection object for managing the connection lifetime
         * 
         * The slot will be automatically disconnected if the object is destroyed,
         * preventing dangling pointer access. The member function will only be
         * called if the object is still alive when the signal is emitted.
         */
        template<typename T>
        [[nodiscard]] connection connect(std::shared_ptr<T> object, void (T::*method)(Args...)) {
                cleanup_expired();
                size_t id = next_id++;
                auto slot = [weak_object = std::weak_ptr<T>(object), method](Args... args) {
                        if (auto ptr = weak_object.lock()) {
                                std::invoke(method, ptr.get(), args...);
                        }
                };

                slots.emplace_back(std::make_shared<SlotWrapper>(std::move(slot), std::weak_ptr<void>(object), id));
                return connection([this, id]{
                                disconnect_by_id(id);
                                });
        }

        /**
         * @brief Emit the signal, calling all connected slots
         * @param args Arguments to pass to each connected slot
         * 
         * All connected slots are called with the provided arguments.
         * Exceptions thrown by slots are caught and ignored to prevent
         * one slot from affecting others. Expired connections are cleaned
         * up before emission.
         */
        void emit(Args... args) {
                cleanup_expired();

                auto slots_copy = slots;

		for(const auto& wrapper : slots_copy)
		{
			slots_queue.push_back(wrapper);
		}

		if (!emitting) {
			emitting = true;
			while(!slots_queue.empty()){
				auto wrapper = slots_queue.front();
				slots_queue.pop_front();
				try {
				wrapper->slot(args...);
				} catch (...) {}
			}
			emitting = false;
		}
        }

        /**
         * @brief Function call operator - shorthand for emit()
         * @param args Arguments to pass to each connected slot
         */
        void operator()(Args... args) {
                emit(args...);
        }

        /**
         * @brief Disconnect all slots from this signal
         */
        void disconnect_all() {
                slots.clear();
        }

        /**
         * @brief Get the number of currently connected slots
         * @return Number of active connections
         */
        size_t slot_count() {
        	cleanup_expired();
                return slots.size();
        }

private:
        /**
         * @brief Internal wrapper for storing slot information
         * 
         * Contains the callable slot, optional weak reference for lifetime
         * management, and unique ID for identification.
         */
        struct SlotWrapper {
                std::function<void(Args...)> slot; ///< The callable slot function
                std::optional<std::weak_ptr<void>> weak_ref_opt; ///< Optional weak reference for object lifetime tracking
                size_t id; ///< Unique identifier for this slot

                SlotWrapper(std::function<void(Args...)> s, std::optional<std::weak_ptr<void>> ref, size_t i) : slot(std::move(s)), weak_ref_opt(std::move(ref)), id(i) {
                }

                ~SlotWrapper() {
                }
        };
        
        std::vector<std::shared_ptr<SlotWrapper>> slots; ///< Container for all connected slots
        size_t next_id = 0; ///< Counter for generating unique slot IDs
	static inline std::deque<std::shared_ptr<SlotWrapper>> slots_queue = {};
	static inline bool emitting = false;

        /**
         * @brief Remove slots whose associated objects have been destroyed
         * 
         * Iterates through all slots and removes those with expired weak_ptr
         * references, preventing calls to destroyed objects.
         */
        void cleanup_expired() {
                std::erase_if(
                                slots, 
                                        [this](const std::shared_ptr<SlotWrapper>& wrapper) {
                                                return wrapper->weak_ref_opt.has_value() && wrapper->weak_ref_opt.value().expired();
                                        });
        }

        /**
         * @brief Remove a specific slot by its unique ID
         * @param id Unique identifier of the slot to remove
         */
        void disconnect_by_id(size_t id) {
                std::erase_if(slots, 
                                [id](const std::shared_ptr<SlotWrapper>& wrapper) {
                                        return wrapper->id == id;
                                }
                           );
        }

};

/**
 * @class auto_disconnect
 * @brief Base class for automatic connection management
 * 
 * Classes that inherit from auto_disconnect can use add_connection()
 * to store connection objects. All connections will be automatically
 * disconnected when the object is destroyed, preventing dangling
 * connections.
 * 
 * Example:
 * @code
 * class MyWidget : public ant::auto_disconnect {
 * public:
 *     MyWidget(ant::signal<int>& sig) {
 *         add_connection(sig.connect([this](int x) { handleData(x); }));
 *     }
 *     // All connections automatically disconnected in destructor
 * };
 * @endcode
 */
class auto_disconnect {
public:
        /**
         * @brief Constructor reserves space for connections
         */
        auto_disconnect() { connections.reserve(10);}
        /**
         * @brief Virtual destructor automatically disconnects all connections
         */
        virtual ~auto_disconnect() = default;

protected:
        std::vector<connection> connections; ///< Storage for managed connections

        /**
         * @brief Add a connection to be managed by this object
         * @param conn connection to store and manage
         * 
         * The connection will be automatically disconnected when this
         * object is destroyed.
         */
        void add_connection(connection&& conn) {
                connections.push_back(std::move(conn));
        }
};

} // namespace ant

#endif /* SIGNAL_HPP */
