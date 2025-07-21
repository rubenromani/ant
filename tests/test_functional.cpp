#include <gtest/gtest.h>
#include <signal.hpp>
#include <memory>
#include <vector>
#include <string>
#include <thread>
#include <chrono>

class Document : public ant::auto_disconnect {
public:
    SIGNAL(content_changed, const std::string&);
    SIGNAL(saved);
    SIGNAL(title_changed, const std::string&);
    
private:
    std::string content_;
    std::string title_;
    
public:
    void setContent(const std::string& content) {
        content_ = content;
        content_changed(content);
    }
    
    void setTitle(const std::string& title) {
        title_ = title;
        title_changed(title);
    }
    
    void save() {
        saved();
    }
    
    const std::string& getContent() const { return content_; }
    const std::string& getTitle() const { return title_; }
};

class TextView : public ant::auto_disconnect {
public:
    std::string displayed_content;
    std::string displayed_title;
    bool is_dirty = false;
    
    void connectToDocument(Document& doc) {
        add_connection(doc.content_changed.connect([this](const std::string& content) {
            displayed_content = content;
            is_dirty = true;
        }));
        
        add_connection(doc.title_changed.connect([this](const std::string& title) {
            displayed_title = title;
        }));
        
        add_connection(doc.saved.connect([this]() {
            is_dirty = false;
        }));
    }
};

class Logger : public ant::auto_disconnect {
public:
    std::vector<std::string> logs;
    
    void connectToDocument(Document& doc) {
        add_connection(doc.content_changed.connect([this](const std::string& content) {
            logs.push_back("Content changed: " + content);
        }));
        
        add_connection(doc.title_changed.connect([this](const std::string& title) {
            logs.push_back("Title changed: " + title);
        }));
        
        add_connection(doc.saved.connect([this]() {
            logs.push_back("Document saved");
        }));
    }
};

class FunctionalTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(FunctionalTest, DocumentViewerPattern) {
    Document doc;
    TextView view;
    Logger logger;
    
    view.connectToDocument(doc);
    logger.connectToDocument(doc);
    
    doc.setTitle("My Document");
    EXPECT_EQ(view.displayed_title, "My Document");
    EXPECT_EQ(logger.logs.size(), 1);
    EXPECT_EQ(logger.logs[0], "Title changed: My Document");
    
    doc.setContent("Hello World");
    EXPECT_EQ(view.displayed_content, "Hello World");
    EXPECT_TRUE(view.is_dirty);
    EXPECT_EQ(logger.logs.size(), 2);
    EXPECT_EQ(logger.logs[1], "Content changed: Hello World");
    
    doc.save();
    EXPECT_FALSE(view.is_dirty);
    EXPECT_EQ(logger.logs.size(), 3);
    EXPECT_EQ(logger.logs[2], "Document saved");
}

class EventBus {
public:
    SIGNAL(user_logged_in, const std::string&);
    SIGNAL(user_logged_out, const std::string&);
    SIGNAL(message_sent, const std::string&, const std::string&);
};

class UserManager : public ant::auto_disconnect {
public:
    std::vector<std::string> active_users;
    
    void connectToEventBus(EventBus& bus) {
        add_connection(bus.user_logged_in.connect([this](const std::string& username) {
            active_users.push_back(username);
        }));
        
        add_connection(bus.user_logged_out.connect([this](const std::string& username) {
            auto it = std::find(active_users.begin(), active_users.end(), username);
            if (it != active_users.end()) {
                active_users.erase(it);
            }
        }));
    }
};

class MessageHistory : public ant::auto_disconnect {
public:
    std::vector<std::pair<std::string, std::string>> messages;
    
    void connectToEventBus(EventBus& bus) {
        add_connection(bus.message_sent.connect([this](const std::string& user, const std::string& message) {
            messages.emplace_back(user, message);
        }));
    }
};

TEST_F(FunctionalTest, EventBusPattern) {
    EventBus bus;
    UserManager user_mgr;
    MessageHistory msg_history;
    
    user_mgr.connectToEventBus(bus);
    msg_history.connectToEventBus(bus);
    
    bus.user_logged_in("alice");
    bus.user_logged_in("bob");
    
    EXPECT_EQ(user_mgr.active_users.size(), 2);
    EXPECT_TRUE(std::find(user_mgr.active_users.begin(), user_mgr.active_users.end(), "alice") != user_mgr.active_users.end());
    EXPECT_TRUE(std::find(user_mgr.active_users.begin(), user_mgr.active_users.end(), "bob") != user_mgr.active_users.end());
    
    bus.message_sent("alice", "Hello everyone!");
    bus.message_sent("bob", "Hi Alice!");
    
    EXPECT_EQ(msg_history.messages.size(), 2);
    EXPECT_EQ(msg_history.messages[0].first, "alice");
    EXPECT_EQ(msg_history.messages[0].second, "Hello everyone!");
    EXPECT_EQ(msg_history.messages[1].first, "bob");
    EXPECT_EQ(msg_history.messages[1].second, "Hi Alice!");
    
    bus.user_logged_out("alice");
    EXPECT_EQ(user_mgr.active_users.size(), 1);
    EXPECT_EQ(user_mgr.active_users[0], "bob");
}

class GameEntity : public ant::auto_disconnect {
public:
    SIGNAL(health_changed, int);
    SIGNAL(position_changed, int, int);
    SIGNAL(died);
    
private:
    int health_ = 100;
    int x_ = 0, y_ = 0;
    
public:
    void takeDamage(int damage) {
        health_ -= damage;
        health_changed(health_);
        if (health_ <= 0) {
            died();
        }
    }
    
    void moveTo(int x, int y) {
        x_ = x;
        y_ = y;
        position_changed(x, y);
    }
    
    int getHealth() const { return health_; }
    std::pair<int, int> getPosition() const { return {x_, y_}; }
};

class HealthBar : public ant::auto_disconnect {
public:
    int displayed_health = 100;
    
    void connectToEntity(GameEntity& entity) {
        add_connection(entity.health_changed.connect([this](int health) {
            displayed_health = health;
        }));
    }
};

class GameMap : public ant::auto_disconnect {
public:
    std::vector<std::pair<int, int>> entity_positions;
    
    void connectToEntity(GameEntity& entity) {
        add_connection(entity.position_changed.connect([this](int x, int y) {
            entity_positions.emplace_back(x, y);
        }));
        
        add_connection(entity.died.connect([this]() {
            entity_positions.clear();
        }));
    }
};

TEST_F(FunctionalTest, GameSystemPattern) {
    GameEntity player;
    HealthBar health_ui;
    GameMap game_map;
    
    health_ui.connectToEntity(player);
    game_map.connectToEntity(player);
    
    player.moveTo(10, 20);
    EXPECT_EQ(game_map.entity_positions.size(), 1);
    EXPECT_EQ(game_map.entity_positions[0].first, 10);
    EXPECT_EQ(game_map.entity_positions[0].second, 20);
    
    player.moveTo(30, 40);
    EXPECT_EQ(game_map.entity_positions.size(), 2);
    
    player.takeDamage(50);
    EXPECT_EQ(health_ui.displayed_health, 50);
    EXPECT_EQ(game_map.entity_positions.size(), 2);
    
    player.takeDamage(60);
    EXPECT_EQ(health_ui.displayed_health, -10);
    EXPECT_EQ(game_map.entity_positions.size(), 0);
}

TEST_F(FunctionalTest, DynamicConnectionManagement) {
    ant::signal<int> data_signal;
    std::vector<int> collected_data;
    
    std::vector<std::unique_ptr<ant::connection>> connections;
    
    for (int i = 0; i < 5; ++i) {
        connections.push_back(std::make_unique<ant::connection>(
            data_signal.connect([&collected_data, i](int value) {
                collected_data.push_back(value * (i + 1));
            })
        ));
    }
    
    EXPECT_EQ(data_signal.slot_count(), 5);
    
    data_signal.emit(10);
    EXPECT_EQ(collected_data.size(), 5);
    
    connections.erase(connections.begin() + 2);
    EXPECT_EQ(data_signal.slot_count(), 4);
    
    collected_data.clear();
    data_signal.emit(5);
    EXPECT_EQ(collected_data.size(), 4);
}

TEST_F(FunctionalTest, WeakPtrLifetimeManagement) {
    ant::signal<int> sig;
    std::vector<int> results;
    
    class TestHandler {
    public:
        void handle(int value) { last_value = value; }
        int last_value = 0;
    };
    
    auto handler1 = std::make_shared<TestHandler>();
    auto handler2 = std::make_shared<TestHandler>();
    
    auto conn1 = sig.connect(handler1, &TestHandler::handle);
    auto conn2 = sig.connect(handler2, &TestHandler::handle);
    
    EXPECT_EQ(sig.slot_count(), 2);
    
    sig.emit(100);
    EXPECT_EQ(handler1->last_value, 100);
    EXPECT_EQ(handler2->last_value, 100);
    
    handler1.reset();
    sig.emit(200);
    
    EXPECT_EQ(sig.slot_count(), 1);
    EXPECT_EQ(handler2->last_value, 200);
    
    handler2.reset();
    sig.emit(300);
    EXPECT_EQ(sig.slot_count(), 0);
}