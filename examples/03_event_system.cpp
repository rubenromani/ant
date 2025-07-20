/**
 * @file 03_event_system.cpp
 * @brief Event-driven system using ant signals for decoupled communication
 * 
 * This example demonstrates how to build event-driven systems using signals:
 * - Central event bus architecture
 * - Event publishing and subscription between different components
 * - Decoupled system components
 * - Cross-component communication
 * - System-wide event coordination
 */

#include <signal.hpp>
#include <iostream>
#include <string>
#include <memory>
#include <vector>
#include <map>
#include <set>
#include <chrono>
#include <iomanip>
#include <sstream>

/**
 * @brief Centralized event bus for system-wide communication
 * 
 * The EventBus acts as a mediator between system components,
 * allowing them to communicate without direct dependencies.
 */
class EventBus {
public:
    // User-related events
    SIGNAL(user_registered, const std::string&, const std::string&);  // username, email
    SIGNAL(user_logged_in, const std::string&);                       // username
    SIGNAL(user_logged_out, const std::string&);                      // username
    
    // System events
    SIGNAL(system_error, const std::string&);                         // error message
    SIGNAL(performance_warning, const std::string&, double);          // component, metric
    
    // Application events
    SIGNAL(file_uploaded, const std::string&, const std::string&, size_t); // user, filename, size
    SIGNAL(message_sent, const std::string&, const std::string&, const std::string&); // from, to, message
    
    // Business events
    SIGNAL(order_created, int, const std::string&, double);           // order_id, customer, amount
    SIGNAL(payment_processed, int, double);                           // order_id, amount
    
    static EventBus& getInstance() {
        static EventBus instance;
        return instance;
    }
    
private:
    EventBus() = default;
};

/**
 * @brief User management system - publishes user events
 */
class UserManager {
private:
    std::map<std::string, std::string> users_; // username -> email
    std::set<std::string> logged_in_users_;
    
public:
    void registerUser(const std::string& username, const std::string& email) {
        if (users_.find(username) != users_.end()) {
            EventBus::getInstance().system_error("User already exists: " + username);
            return;
        }
        
        users_[username] = email;
        std::cout << "[UserManager] Registering user: " << username << std::endl;
        
        // Publish event - other components will react to this
        EventBus::getInstance().user_registered(username, email);
    }
    
    void loginUser(const std::string& username) {
        if (users_.find(username) == users_.end()) {
            EventBus::getInstance().system_error("User not found: " + username);
            return;
        }
        
        logged_in_users_.insert(username);
        std::cout << "[UserManager] User logging in: " << username << std::endl;
        
        // Publish event
        EventBus::getInstance().user_logged_in(username);
    }
    
    void logoutUser(const std::string& username) {
        if (logged_in_users_.find(username) == logged_in_users_.end()) {
            EventBus::getInstance().system_error("User not logged in: " + username);
            return;
        }
        
        logged_in_users_.erase(username);
        std::cout << "[UserManager] User logging out: " << username << std::endl;
        
        // Publish event
        EventBus::getInstance().user_logged_out(username);
    }
    
    bool isUserLoggedIn(const std::string& username) const {
        return logged_in_users_.find(username) != logged_in_users_.end();
    }
    
    size_t getTotalUsers() const { return users_.size(); }
    size_t getActiveUsers() const { return logged_in_users_.size(); }
};

/**
 * @brief Notification system - subscribes to events from other components
 */
class NotificationSystem : public ant::auto_disconnect {
private:
    std::map<std::string, std::vector<std::string>> notifications_; // user -> notifications
    
public:
    NotificationSystem() {
        auto& bus = EventBus::getInstance();
        
        // React to events from UserManager
        add_connection(bus.user_registered.connect([this](const std::string& username, const std::string&) {
            sendNotification(username, "Welcome to the platform!");
        }));
        
        // React to events from FileManager
        add_connection(bus.file_uploaded.connect([this](const std::string& username, const std::string& filename, size_t size) {
            std::ostringstream msg;
            msg << "File uploaded: " << filename << " (" << size << " bytes)";
            sendNotification(username, msg.str());
        }));
        
        // React to events from MessageSystem
        add_connection(bus.message_sent.connect([this](const std::string&, const std::string& to, const std::string& message) {
            sendNotification(to, "New message: " + message.substr(0, 30) + "...");
        }));
        
        // React to events from OrderSystem
        add_connection(bus.order_created.connect([this](int order_id, const std::string& customer, double amount) {
            std::ostringstream msg;
            msg << "Order #" << order_id << " created for $" << std::fixed << std::setprecision(2) << amount;
            sendNotification(customer, msg.str());
        }));
    }
    
    void sendNotification(const std::string& username, const std::string& message) {
        notifications_[username].push_back(message);
        std::cout << "[NotificationSystem] → " << username << ": " << message << std::endl;
    }
    
    std::vector<std::string> getNotifications(const std::string& username) const {
        auto it = notifications_.find(username);
        return (it != notifications_.end()) ? it->second : std::vector<std::string>{};
    }
};

/**
 * @brief Analytics system - subscribes to events to track metrics
 */
class AnalyticsSystem : public ant::auto_disconnect {
private:
    struct Metrics {
        int user_registrations = 0;
        int logins = 0;
        int file_uploads = 0;
        int messages_sent = 0;
        int orders_created = 0;
        double total_revenue = 0.0;
        size_t total_upload_size = 0;
    } metrics_;
    
public:
    AnalyticsSystem() {
        auto& bus = EventBus::getInstance();
        
        // Subscribe to events from various components to track metrics
        add_connection(bus.user_registered.connect([this](const std::string&, const std::string&) {
            metrics_.user_registrations++;
            std::cout << "[Analytics] User registration count: " << metrics_.user_registrations << std::endl;
        }));
        
        add_connection(bus.user_logged_in.connect([this](const std::string&) {
            metrics_.logins++;
            std::cout << "[Analytics] Login count: " << metrics_.logins << std::endl;
        }));
        
        add_connection(bus.file_uploaded.connect([this](const std::string&, const std::string&, size_t size) {
            metrics_.file_uploads++;
            metrics_.total_upload_size += size;
            std::cout << "[Analytics] File upload #" << metrics_.file_uploads 
                      << ", total size: " << metrics_.total_upload_size << " bytes" << std::endl;
        }));
        
        add_connection(bus.message_sent.connect([this](const std::string&, const std::string&, const std::string&) {
            metrics_.messages_sent++;
            std::cout << "[Analytics] Message count: " << metrics_.messages_sent << std::endl;
        }));
        
        add_connection(bus.order_created.connect([this](int, const std::string&, double amount) {
            metrics_.orders_created++;
            metrics_.total_revenue += amount;
            std::cout << "[Analytics] Order #" << metrics_.orders_created 
                      << ", total revenue: $" << std::fixed << std::setprecision(2) << metrics_.total_revenue << std::endl;
        }));
    }
    
    void printMetrics() const {
        std::cout << "[Analytics] Final Metrics:" << std::endl;
        std::cout << "  User registrations: " << metrics_.user_registrations << std::endl;
        std::cout << "  Total logins: " << metrics_.logins << std::endl;
        std::cout << "  Files uploaded: " << metrics_.file_uploads << std::endl;
        std::cout << "  Total upload size: " << metrics_.total_upload_size << " bytes" << std::endl;
        std::cout << "  Messages sent: " << metrics_.messages_sent << std::endl;
        std::cout << "  Orders created: " << metrics_.orders_created << std::endl;
        std::cout << "  Total revenue: $" << std::fixed << std::setprecision(2) << metrics_.total_revenue << std::endl;
    }
};

/**
 * @brief Security monitor - subscribes to events to detect suspicious activity
 */
class SecurityMonitor : public ant::auto_disconnect {
private:
    std::map<std::string, int> login_attempts_;
    std::map<std::string, size_t> upload_sizes_;
    
public:
    SecurityMonitor() {
        auto& bus = EventBus::getInstance();
        
        // Monitor login patterns
        add_connection(bus.user_logged_in.connect([this](const std::string& username) {
            login_attempts_[username]++;
            if (login_attempts_[username] > 5) {
                std::cout << "[SecurityMonitor] ⚠️  Frequent logins detected for user: " << username << std::endl;
            }
        }));
        
        // Monitor file upload sizes
        add_connection(bus.file_uploaded.connect([this](const std::string& username, const std::string& filename, size_t size) {
            upload_sizes_[username] += size;
            if (size > 10 * 1024 * 1024) { // 10MB
                std::cout << "[SecurityMonitor] ⚠️  Large file upload: " << filename 
                          << " (" << size << " bytes) by " << username << std::endl;
            }
            if (upload_sizes_[username] > 100 * 1024 * 1024) { // 100MB total
                std::cout << "[SecurityMonitor] ⚠️  User " << username 
                          << " has uploaded over 100MB total" << std::endl;
            }
        }));
        
        // Monitor system errors
        add_connection(bus.system_error.connect([this](const std::string& error) {
            std::cout << "[SecurityMonitor] 🚨 System error logged: " << error << std::endl;
        }));
    }
};

/**
 * @brief File management system - publishes file events
 */
class FileManager {
private:
    std::map<std::string, std::vector<std::pair<std::string, size_t>>> user_files_;
    
public:
    void uploadFile(const std::string& username, const std::string& filename, size_t size) {
        user_files_[username].emplace_back(filename, size);
        std::cout << "[FileManager] Storing file: " << filename << " for " << username << std::endl;
        
        // Publish event - other components will react
        EventBus::getInstance().file_uploaded(username, filename, size);
    }
    
    bool downloadFile(const std::string& username, const std::string& filename) {
        auto it = user_files_.find(username);
        if (it != user_files_.end()) {
            auto file_it = std::find_if(it->second.begin(), it->second.end(),
                [&filename](const auto& pair) { return pair.first == filename; });
            
            if (file_it != it->second.end()) {
                std::cout << "[FileManager] File downloaded: " << filename << " by " << username << std::endl;
                return true;
            }
        }
        
        EventBus::getInstance().system_error("File not found: " + filename + " for user " + username);
        return false;
    }
};

/**
 * @brief Message system - publishes message events
 */
class MessageSystem {
public:
    void sendMessage(const std::string& from, const std::string& to, const std::string& message) {
        std::cout << "[MessageSystem] Delivering message from " << from << " to " << to << std::endl;
        
        // Publish event - NotificationSystem will pick this up
        EventBus::getInstance().message_sent(from, to, message);
    }
};

/**
 * @brief E-commerce order system - publishes order events
 */
class OrderSystem {
private:
    int next_order_id_ = 1000;
    std::map<int, std::string> orders_; // order_id -> customer
    
public:
    int createOrder(const std::string& customer, double amount) {
        int order_id = next_order_id_++;
        orders_[order_id] = customer;
        
        std::cout << "[OrderSystem] Creating order #" << order_id << " for " << customer << std::endl;
        
        // Publish event - Analytics and Notifications will react
        EventBus::getInstance().order_created(order_id, customer, amount);
        
        return order_id;
    }
    
    void processPayment(int order_id, double amount) {
        auto it = orders_.find(order_id);
        if (it != orders_.end()) {
            std::cout << "[OrderSystem] Processing payment for order #" << order_id << std::endl;
            
            // Publish event
            EventBus::getInstance().payment_processed(order_id, amount);
        } else {
            EventBus::getInstance().system_error("Order not found: " + std::to_string(order_id));
        }
    }
};

/**
 * @brief Performance monitor - publishes performance warnings
 */
class PerformanceMonitor : public ant::auto_disconnect {
public:
    PerformanceMonitor() {
        auto& bus = EventBus::getInstance();
        
        // React to system errors
        add_connection(bus.system_error.connect([this](const std::string& error) {
            std::cout << "[PerformanceMonitor] 📊 System error detected: " << error << std::endl;
        }));
    }
    
    void checkCPUUsage(double usage) {
        if (usage > 80.0) {
            std::cout << "[PerformanceMonitor] High CPU usage detected: " << usage << "%" << std::endl;
            EventBus::getInstance().performance_warning("CPU", usage);
        }
    }
    
    void checkMemoryUsage(double usage) {
        if (usage > 85.0) {
            std::cout << "[PerformanceMonitor] High memory usage detected: " << usage << "%" << std::endl;
            EventBus::getInstance().performance_warning("Memory", usage);
        }
    }
};

void demonstrateEventDrivenSystem() {
    std::cout << "=== Event-Driven System Demo ===" << std::endl;
    std::cout << "Components communicate through events, not direct calls\n" << std::endl;
    
    // Create all system components
    UserManager user_mgr;
    NotificationSystem notifications;  // Subscribes to user, file, message, order events
    AnalyticsSystem analytics;         // Subscribes to all events for metrics
    SecurityMonitor security;          // Subscribes to login, upload, error events
    FileManager file_mgr;
    MessageSystem msg_system;
    OrderSystem order_system;
    PerformanceMonitor perf_monitor;   // Subscribes to error events
    
    std::cout << "--- User Registration & Login ---" << std::endl;
    // UserManager publishes events → NotificationSystem & Analytics react
    user_mgr.registerUser("alice", "alice@example.com");
    user_mgr.registerUser("bob", "bob@example.com");
    
    user_mgr.loginUser("alice");
    user_mgr.loginUser("bob");
    
    std::cout << "\n--- File Operations ---" << std::endl;
    // FileManager publishes events → NotificationSystem, Analytics, SecurityMonitor react
    file_mgr.uploadFile("alice", "document.pdf", 1024000);
    file_mgr.uploadFile("alice", "large_video.mp4", 50 * 1024 * 1024); // Large file - security alert
    file_mgr.uploadFile("bob", "presentation.pptx", 5120000);
    
    file_mgr.downloadFile("alice", "document.pdf");
    file_mgr.downloadFile("bob", "nonexistent.txt"); // Error - SecurityMonitor will see this
    
    std::cout << "\n--- Messaging ---" << std::endl;
    // MessageSystem publishes events → NotificationSystem & Analytics react
    msg_system.sendMessage("alice", "bob", "Hello Bob, how are you?");
    msg_system.sendMessage("bob", "alice", "Hi Alice! I'm doing great!");
    
    std::cout << "\n--- E-commerce Orders ---" << std::endl;
    // OrderSystem publishes events → NotificationSystem & Analytics react
    int order1 = order_system.createOrder("alice", 99.99);
    int order2 = order_system.createOrder("bob", 149.50);
    
    order_system.processPayment(order1, 99.99);
    order_system.processPayment(999, 50.0); // Non-existent order - error
    
    std::cout << "\n--- Performance Monitoring ---" << std::endl;
    // PerformanceMonitor publishes warnings
    perf_monitor.checkCPUUsage(45.2);  // Normal
    perf_monitor.checkCPUUsage(85.7);  // Warning
    perf_monitor.checkMemoryUsage(92.3); // Warning
    
    std::cout << "\n--- Simulate Suspicious Activity ---" << std::endl;
    // Multiple logins to trigger security alert
    for (int i = 0; i < 6; ++i) {
        user_mgr.logoutUser("alice");
        user_mgr.loginUser("alice");
    }
    
    std::cout << "\n--- Final System State ---" << std::endl;
    analytics.printMetrics();
    
    std::cout << "\nActive users: " << user_mgr.getActiveUsers() 
              << " / " << user_mgr.getTotalUsers() << std::endl;
    
    // Clean shutdown
    user_mgr.logoutUser("alice");
    user_mgr.logoutUser("bob");
}

int main() {
    std::cout << "ANT Signal Library - Event System Examples" << std::endl;
    std::cout << "==========================================" << std::endl;
    std::cout << "This demonstrates proper event-driven architecture where:" << std::endl;
    std::cout << "- Components publish events when they do something" << std::endl;
    std::cout << "- Other components subscribe to events they care about" << std::endl;
    std::cout << "- No component directly calls methods on other components" << std::endl << std::endl;
    
    demonstrateEventDrivenSystem();
    
    std::cout << "\nEvent system examples completed successfully!" << std::endl;
    return 0;
}