/**
 * @file 02_observer_pattern.cpp
 * @brief Implementation of the Observer Pattern using ant signals
 * 
 * This example demonstrates how to implement the classic Observer pattern
 * using the ant signal library. It shows:
 * - Subject-Observer relationships
 * - Automatic observer registration/deregistration
 * - Multiple observers watching the same subject
 * - Type-safe notifications
 * - Automatic cleanup when observers are destroyed
 */

#include <signal.hpp>
#include <iostream>
#include <string>
#include <memory>
#include <vector>
#include <iomanip>
#include <map>
#include <numeric>
#include <algorithm>

/**
 * @brief Subject class that maintains some state and notifies observers
 * 
 * This represents the "Subject" in the Observer pattern. It maintains
 * state and automatically notifies all registered observers when state changes.
 */
class WeatherStation {
public:
    // Signals for different types of weather data updates
    SIGNAL(temperature_changed, double);
    SIGNAL(humidity_changed, double);
    SIGNAL(pressure_changed, double);
    SIGNAL(weather_alert, const std::string&);

private:
    double temperature_ = 20.0;  // Celsius
    double humidity_ = 50.0;     // Percentage
    double pressure_ = 1013.25;  // hPa
    
public:
    void setTemperature(double temp) {
        if (temperature_ != temp) {
            temperature_ = temp;
            temperature_changed(temperature_);
            
            // Generate alerts for extreme temperatures
            if (temp > 35.0) {
                weather_alert("High temperature warning: " + std::to_string(temp) + "°C");
            } else if (temp < -10.0) {
                weather_alert("Low temperature warning: " + std::to_string(temp) + "°C");
            }
        }
    }
    
    void setHumidity(double humidity) {
        if (humidity_ != humidity) {
            humidity_ = humidity;
            humidity_changed(humidity_);
            
            if (humidity > 80.0) {
                weather_alert("High humidity warning: " + std::to_string(humidity) + "%");
            }
        }
    }
    
    void setPressure(double pressure) {
        if (pressure_ != pressure) {
            pressure_ = pressure;
            pressure_changed(pressure_);
            
            if (pressure < 1000.0) {
                weather_alert("Low pressure warning: " + std::to_string(pressure) + " hPa");
            }
        }
    }
    
    // Getters
    double getTemperature() const { return temperature_; }
    double getHumidity() const { return humidity_; }
    double getPressure() const { return pressure_; }
};

/**
 * @brief Observer that displays current weather conditions
 */
class WeatherDisplay : public ant::auto_disconnect {
private:
    std::string name_;
    double temperature_ = 0.0;
    double humidity_ = 0.0;
    double pressure_ = 0.0;
    
public:
    WeatherDisplay(const std::string& name) : name_(name) {}
    
    void subscribeToWeatherStation(WeatherStation& station) {
        // Subscribe to all weather updates
        add_connection(station.temperature_changed.connect([this](double temp) {
            temperature_ = temp;
            displayUpdate("Temperature", std::to_string(temp) + "°C");
        }));
        
        add_connection(station.humidity_changed.connect([this](double humidity) {
            humidity_ = humidity;
            displayUpdate("Humidity", std::to_string(humidity) + "%");
        }));
        
        add_connection(station.pressure_changed.connect([this](double pressure) {
            pressure_ = pressure;
            displayUpdate("Pressure", std::to_string(pressure) + " hPa");
        }));
        
        add_connection(station.weather_alert.connect([this](const std::string& alert) {
            std::cout << "[" << name_ << "] ALERT: " << alert << std::endl;
        }));
        
        // Initialize with current values
        temperature_ = station.getTemperature();
        humidity_ = station.getHumidity();
        pressure_ = station.getPressure();
    }
    
    void displayCurrentConditions() const {
        std::cout << "[" << name_ << "] Current conditions:" << std::endl;
        std::cout << "  Temperature: " << std::fixed << std::setprecision(1) 
                  << temperature_ << "°C" << std::endl;
        std::cout << "  Humidity: " << humidity_ << "%" << std::endl;
        std::cout << "  Pressure: " << pressure_ << " hPa" << std::endl;
    }
    
private:
    void displayUpdate(const std::string& type, const std::string& value) {
        std::cout << "[" << name_ << "] " << type << " updated to " << value << std::endl;
    }
};

/**
 * @brief Observer that logs weather data to a file-like interface
 */
class WeatherLogger : public ant::auto_disconnect {
private:
    std::string log_name_;
    std::vector<std::string> log_entries_;
    
public:
    WeatherLogger(const std::string& log_name) : log_name_(log_name) {}
    
    void subscribeToWeatherStation(WeatherStation& station) {
        add_connection(station.temperature_changed.connect([this](double temp) {
            logEntry("TEMP", std::to_string(temp) + "C");
        }));
        
        add_connection(station.humidity_changed.connect([this](double humidity) {
            logEntry("HUMIDITY", std::to_string(humidity) + "%");
        }));
        
        add_connection(station.pressure_changed.connect([this](double pressure) {
            logEntry("PRESSURE", std::to_string(pressure) + "hPa");
        }));
        
        add_connection(station.weather_alert.connect([this](const std::string& alert) {
            logEntry("ALERT", alert);
        }));
    }
    
    void printLog() const {
        std::cout << "[" << log_name_ << "] Log entries:" << std::endl;
        for (const auto& entry : log_entries_) {
            std::cout << "  " << entry << std::endl;
        }
    }
    
    size_t getLogSize() const { return log_entries_.size(); }
    
private:
    void logEntry(const std::string& type, const std::string& data) {
        std::string entry = "[" + type + "] " + data;
        log_entries_.push_back(entry);
        std::cout << "[" << log_name_ << "] Logged: " << entry << std::endl;
    }
};

/**
 * @brief Observer that calculates statistics
 */
class WeatherStatistics : public ant::auto_disconnect {
private:
    std::vector<double> temperatures_;
    std::vector<double> humidity_readings_;
    std::vector<double> pressure_readings_;
    
public:
    void subscribeToWeatherStation(WeatherStation& station) {
        add_connection(station.temperature_changed.connect([this](double temp) {
            temperatures_.push_back(temp);
        }));
        
        add_connection(station.humidity_changed.connect([this](double humidity) {
            humidity_readings_.push_back(humidity);
        }));
        
        add_connection(station.pressure_changed.connect([this](double pressure) {
            pressure_readings_.push_back(pressure);
        }));
    }
    
    void printStatistics() const {
        std::cout << "[Statistics] Weather summary:" << std::endl;
        
        if (!temperatures_.empty()) {
            auto [min_temp, max_temp] = std::minmax_element(temperatures_.begin(), temperatures_.end());
            double avg_temp = std::accumulate(temperatures_.begin(), temperatures_.end(), 0.0) / temperatures_.size();
            
            std::cout << "  Temperature: min=" << *min_temp << "°C, max=" << *max_temp 
                      << "°C, avg=" << std::fixed << std::setprecision(1) << avg_temp << "°C" << std::endl;
        }
        
        if (!humidity_readings_.empty()) {
            auto [min_hum, max_hum] = std::minmax_element(humidity_readings_.begin(), humidity_readings_.end());
            double avg_hum = std::accumulate(humidity_readings_.begin(), humidity_readings_.end(), 0.0) / humidity_readings_.size();
            
            std::cout << "  Humidity: min=" << *min_hum << "%, max=" << *max_hum 
                      << "%, avg=" << std::fixed << std::setprecision(1) << avg_hum << "%" << std::endl;
        }
        
        std::cout << "  Total readings: " << temperatures_.size() << std::endl;
    }
};

/**
 * @brief Stock price subject for financial data
 */
class StockPrice {
public:
    SIGNAL(price_changed, const std::string&, double);
    SIGNAL(volume_changed, const std::string&, int);
    SIGNAL(dividend_announced, const std::string&, double);

private:
    std::string symbol_;
    double price_;
    int volume_;
    
public:
    StockPrice(const std::string& symbol, double initial_price) 
        : symbol_(symbol), price_(initial_price), volume_(0) {}
    
    void updatePrice(double new_price) {
        if (price_ != new_price) {
            price_ = new_price;
            price_changed(symbol_, price_);
        }
    }
    
    void updateVolume(int new_volume) {
        if (volume_ != new_volume) {
            volume_ = new_volume;
            volume_changed(symbol_, volume_);
        }
    }
    
    void announceDividend(double amount) {
        dividend_announced(symbol_, amount);
    }
    
    const std::string& getSymbol() const { return symbol_; }
    double getPrice() const { return price_; }
    int getVolume() const { return volume_; }
};

/**
 * @brief Portfolio observer that tracks multiple stocks
 */
class Portfolio : public ant::auto_disconnect {
private:
    std::string owner_name_;
    std::map<std::string, std::pair<double, int>> holdings_; // symbol -> (price, shares)
    
public:
    Portfolio(const std::string& owner) : owner_name_(owner) {}
    
    void addStock(StockPrice& stock, int shares) {
        const std::string& symbol = stock.getSymbol();
        holdings_[symbol] = {stock.getPrice(), shares};
        
        add_connection(stock.price_changed.connect([this](const std::string& sym, double price) {
            if (holdings_.find(sym) != holdings_.end()) {
                holdings_[sym].first = price;
                std::cout << "[Portfolio " << owner_name_ << "] " << sym 
                          << " price updated to $" << price << std::endl;
            }
        }));
        
        add_connection(stock.dividend_announced.connect([this](const std::string& sym, double amount) {
            if (holdings_.find(sym) != holdings_.end()) {
                int shares = holdings_[sym].second;
                double total_dividend = amount * shares;
                std::cout << "[Portfolio " << owner_name_ << "] Dividend from " << sym 
                          << ": $" << amount << " per share, total: $" << total_dividend << std::endl;
            }
        }));
    }
    
    void printPortfolioValue() const {
        double total_value = 0.0;
        std::cout << "[Portfolio " << owner_name_ << "] Holdings:" << std::endl;
        
        for (const auto& [symbol, data] : holdings_) {
            double price = data.first;
            int shares = data.second;
            double value = price * shares;
            total_value += value;
            
            std::cout << "  " << symbol << ": " << shares << " shares @ $" 
                      << std::fixed << std::setprecision(2) << price 
                      << " = $" << value << std::endl;
        }
        
        std::cout << "  Total portfolio value: $" << total_value << std::endl;
    }
};

void demonstrateWeatherObserver() {
    std::cout << "=== Weather Station Observer Pattern ===" << std::endl;
    
    WeatherStation station;
    
    // Create observers
    auto display1 = std::make_unique<WeatherDisplay>("Home Display");
    auto display2 = std::make_unique<WeatherDisplay>("Office Display");
    auto logger = std::make_unique<WeatherLogger>("WeatherLog");
    auto stats = std::make_unique<WeatherStatistics>();
    
    // Subscribe observers to the weather station
    display1->subscribeToWeatherStation(station);
    display2->subscribeToWeatherStation(station);
    logger->subscribeToWeatherStation(station);
    stats->subscribeToWeatherStation(station);
    
    std::cout << "Initial conditions:" << std::endl;
    display1->displayCurrentConditions();
    
    std::cout << "\nUpdating weather data..." << std::endl;
    station.setTemperature(25.5);
    station.setHumidity(65.0);
    station.setPressure(1015.3);
    
    std::cout << "\nExtreme weather conditions..." << std::endl;
    station.setTemperature(38.0);  // Should trigger alert
    station.setHumidity(85.0);     // Should trigger alert
    
    std::cout << "\nMore updates..." << std::endl;
    station.setTemperature(22.0);
    station.setPressure(995.0);    // Should trigger alert
    
    std::cout << "\nFinal statistics:" << std::endl;
    stats->printStatistics();
    
    std::cout << "\nLog summary:" << std::endl;
    logger->printLog();
    
    // Demonstrate automatic cleanup
    std::cout << "\nRemoving one display..." << std::endl;
    display2.reset();  // This should automatically disconnect from signals
    
    station.setTemperature(30.0);
    std::cout << "Only one display should have updated." << std::endl;
    
    std::cout << std::endl;
}

void demonstrateStockObserver() {
    std::cout << "=== Stock Portfolio Observer Pattern ===" << std::endl;
    
    // Create some stocks
    StockPrice apple("AAPL", 150.00);
    StockPrice google("GOOGL", 2500.00);
    StockPrice microsoft("MSFT", 300.00);
    
    // Create portfolios (observers)
    Portfolio portfolio1("Alice");
    Portfolio portfolio2("Bob");
    
    // Add stocks to portfolios
    portfolio1.addStock(apple, 100);
    portfolio1.addStock(google, 10);
    
    portfolio2.addStock(apple, 50);
    portfolio2.addStock(microsoft, 200);
    
    std::cout << "Initial portfolio values:" << std::endl;
    portfolio1.printPortfolioValue();
    std::cout << std::endl;
    portfolio2.printPortfolioValue();
    
    std::cout << "\nStock price updates:" << std::endl;
    apple.updatePrice(155.50);
    google.updatePrice(2600.00);
    microsoft.updatePrice(310.00);
    
    std::cout << "\nDividend announcements:" << std::endl;
    apple.announceDividend(0.25);
    microsoft.announceDividend(0.75);
    
    std::cout << "\nUpdated portfolio values:" << std::endl;
    portfolio1.printPortfolioValue();
    std::cout << std::endl;
    portfolio2.printPortfolioValue();
    
    std::cout << std::endl;
}

int main() {
    std::cout << "ANT Signal Library - Observer Pattern Examples" << std::endl;
    std::cout << "==============================================" << std::endl << std::endl;
    
    demonstrateWeatherObserver();
    demonstrateStockObserver();
    
    std::cout << "Observer pattern examples completed successfully!" << std::endl;
    return 0;
}