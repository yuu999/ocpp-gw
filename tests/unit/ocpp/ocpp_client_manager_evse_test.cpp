#include "gtest/gtest.h"
#include "ocpp_gateway/ocpp/ocpp_client_manager.h"
#include "ocpp_gateway/ocpp/evse_state_machine.h"
#include <boost/asio.hpp>
#include <thread>
#include <chrono>

using namespace ocpp_gateway::ocpp;

class OcppClientManagerEvseTest : public ::testing::Test {
protected:
    void SetUp() override {
        io_context_ = std::make_shared<boost::asio::io_context>();
        work_guard_ = std::make_shared<boost::asio::io_context::work>(*io_context_);
        io_thread_ = std::thread([this]() { io_context_->run(); });
        
        // Create client configuration
        OcppClientConfig config;
        config.csms_url = "wss://example.com/ocpp";
        config.ca_cert_path = "ca.pem";
        config.heartbeat_interval = std::chrono::seconds(5);
        
        // Create client manager
        client_manager_ = OcppClientManager::create(*io_context_, config);
    }
    
    void TearDown() override {
        client_manager_->stop();
        work_guard_.reset();
        io_context_->stop();
        if (io_thread_.joinable()) {
            io_thread_.join();
        }
    }
    
    std::shared_ptr<boost::asio::io_context> io_context_;
    std::shared_ptr<boost::asio::io_context::work> work_guard_;
    std::thread io_thread_;
    std::shared_ptr<OcppClientManager> client_manager_;
};

// Test adding and removing EVSEs
TEST_F(OcppClientManagerEvseTest, AddRemoveEvse) {
    // Add EVSE
    EXPECT_TRUE(client_manager_->addEvse(1, 1));
    
    // Try to add the same EVSE again (should fail)
    EXPECT_FALSE(client_manager_->addEvse(1, 1));
    
    // Add another EVSE
    EXPECT_TRUE(client_manager_->addEvse(1, 2));
    
    // Get EVSE state machine
    auto evse1 = client_manager_->getEvseStateMachine(1, 1);
    ASSERT_NE(evse1, nullptr);
    EXPECT_EQ(evse1->getEvseId(), 1);
    EXPECT_EQ(evse1->getConnectorId(), 1);
    
    // Get another EVSE state machine
    auto evse2 = client_manager_->getEvseStateMachine(1, 2);
    ASSERT_NE(evse2, nullptr);
    EXPECT_EQ(evse2->getEvseId(), 1);
    EXPECT_EQ(evse2->getConnectorId(), 2);
    
    // Try to get non-existent EVSE
    auto evse3 = client_manager_->getEvseStateMachine(2, 1);
    EXPECT_EQ(evse3, nullptr);
    
    // Remove EVSE
    EXPECT_TRUE(client_manager_->removeEvse(1, 1));
    
    // Try to remove the same EVSE again (should fail)
    EXPECT_FALSE(client_manager_->removeEvse(1, 1));
    
    // Try to get removed EVSE
    auto evse1_after = client_manager_->getEvseStateMachine(1, 1);
    EXPECT_EQ(evse1_after, nullptr);
    
    // Second EVSE should still be there
    auto evse2_after = client_manager_->getEvseStateMachine(1, 2);
    ASSERT_NE(evse2_after, nullptr);
}

// Test EVSE event processing
TEST_F(OcppClientManagerEvseTest, ProcessEvseEvent) {
    // Add EVSE
    EXPECT_TRUE(client_manager_->addEvse(1, 1));
    
    // Process PLUG_IN event
    EXPECT_TRUE(client_manager_->processEvseEvent(1, 1, EvseEvent::PLUG_IN));
    
    // Get EVSE state machine and check state
    auto evse = client_manager_->getEvseStateMachine(1, 1);
    ASSERT_NE(evse, nullptr);
    EXPECT_EQ(evse->getCurrentState(), EvseState::PREPARING);
    
    // Process AUTHORIZE_START event
    nlohmann::json authData;
    authData["idTag"] = "TEST_TAG_123";
    EXPECT_TRUE(client_manager_->processEvseEvent(1, 1, EvseEvent::AUTHORIZE_START, authData));
    
    // Check state
    EXPECT_EQ(evse->getCurrentState(), EvseState::CHARGING);
    EXPECT_TRUE(evse->getCurrentTransaction().has_value());
    
    // Process STOP_CHARGING event
    EXPECT_TRUE(client_manager_->processEvseEvent(1, 1, EvseEvent::STOP_CHARGING));
    
    // Check state
    EXPECT_EQ(evse->getCurrentState(), EvseState::FINISHING);
    EXPECT_FALSE(evse->getCurrentTransaction().has_value());
    
    // Process PLUG_OUT event
    EXPECT_TRUE(client_manager_->processEvseEvent(1, 1, EvseEvent::PLUG_OUT));
    
    // Check state
    EXPECT_EQ(evse->getCurrentState(), EvseState::AVAILABLE);
    
    // Try to process event for non-existent EVSE
    EXPECT_FALSE(client_manager_->processEvseEvent(2, 1, EvseEvent::PLUG_IN));
}

// Test EVSE variable handling
TEST_F(OcppClientManagerEvseTest, EvseVariables) {
    // Add EVSE
    EXPECT_TRUE(client_manager_->addEvse(1, 1));
    
    // Get EVSE state machine
    auto evse = client_manager_->getEvseStateMachine(1, 1);
    ASSERT_NE(evse, nullptr);
    
    // Set variable
    evse->setVariable("MeterValue.Energy.Active.Import.Register", "1000", "float", 0.1, "kWh");
    
    // Get variable
    EXPECT_EQ(evse->getVariableValue("MeterValue.Energy.Active.Import.Register"), "1000");
    
    auto var = evse->getVariable("MeterValue.Energy.Active.Import.Register");
    ASSERT_TRUE(var.has_value());
    EXPECT_EQ(var->name, "MeterValue.Energy.Active.Import.Register");
    EXPECT_EQ(var->value, "1000");
    EXPECT_EQ(var->dataType, "float");
    ASSERT_TRUE(var->scale.has_value());
    EXPECT_EQ(*var->scale, 0.1);
    ASSERT_TRUE(var->unit.has_value());
    EXPECT_EQ(*var->unit, "kWh");
}

// Test EVSE state transitions through client manager
TEST_F(OcppClientManagerEvseTest, EvseStateTransitions) {
    // Add EVSE
    EXPECT_TRUE(client_manager_->addEvse(1, 1));
    
    // Get EVSE state machine
    auto evse = client_manager_->getEvseStateMachine(1, 1);
    ASSERT_NE(evse, nullptr);
    
    // Initial state
    EXPECT_EQ(evse->getCurrentState(), EvseState::AVAILABLE);
    EXPECT_EQ(evse->getConnectorStatus(), ConnectorStatus::AVAILABLE);
    
    // Available -> Reserved
    EXPECT_TRUE(client_manager_->processEvseEvent(1, 1, EvseEvent::RESERVE));
    EXPECT_EQ(evse->getCurrentState(), EvseState::RESERVED);
    EXPECT_EQ(evse->getConnectorStatus(), ConnectorStatus::RESERVED);
    
    // Reserved -> Preparing
    EXPECT_TRUE(client_manager_->processEvseEvent(1, 1, EvseEvent::PLUG_IN));
    EXPECT_EQ(evse->getCurrentState(), EvseState::PREPARING);
    EXPECT_EQ(evse->getConnectorStatus(), ConnectorStatus::OCCUPIED);
    
    // Preparing -> Charging
    nlohmann::json authData;
    authData["idTag"] = "TEST_TAG_123";
    EXPECT_TRUE(client_manager_->processEvseEvent(1, 1, EvseEvent::AUTHORIZE_START, authData));
    EXPECT_EQ(evse->getCurrentState(), EvseState::CHARGING);
    EXPECT_EQ(evse->getConnectorStatus(), ConnectorStatus::OCCUPIED);
    EXPECT_TRUE(evse->getCurrentTransaction().has_value());
    
    // Charging -> SuspendedEV
    EXPECT_TRUE(client_manager_->processEvseEvent(1, 1, EvseEvent::SUSPEND_CHARGING_EV));
    EXPECT_EQ(evse->getCurrentState(), EvseState::SUSPENDED_EV);
    EXPECT_EQ(evse->getConnectorStatus(), ConnectorStatus::OCCUPIED);
    
    // SuspendedEV -> Charging
    EXPECT_TRUE(client_manager_->processEvseEvent(1, 1, EvseEvent::RESUME_CHARGING));
    EXPECT_EQ(evse->getCurrentState(), EvseState::CHARGING);
    
    // Charging -> Finishing
    EXPECT_TRUE(client_manager_->processEvseEvent(1, 1, EvseEvent::STOP_CHARGING));
    EXPECT_EQ(evse->getCurrentState(), EvseState::FINISHING);
    EXPECT_FALSE(evse->getCurrentTransaction().has_value());
    
    // Finishing -> Available
    EXPECT_TRUE(client_manager_->processEvseEvent(1, 1, EvseEvent::PLUG_OUT));
    EXPECT_EQ(evse->getCurrentState(), EvseState::AVAILABLE);
    EXPECT_EQ(evse->getConnectorStatus(), ConnectorStatus::AVAILABLE);
}

// Test fault handling
TEST_F(OcppClientManagerEvseTest, FaultHandling) {
    // Add EVSE
    EXPECT_TRUE(client_manager_->addEvse(1, 1));
    
    // Get EVSE state machine
    auto evse = client_manager_->getEvseStateMachine(1, 1);
    ASSERT_NE(evse, nullptr);
    
    // Available -> Faulted
    EXPECT_TRUE(client_manager_->processEvseEvent(1, 1, EvseEvent::FAULT_DETECTED));
    EXPECT_EQ(evse->getCurrentState(), EvseState::FAULTED);
    EXPECT_EQ(evse->getConnectorStatus(), ConnectorStatus::FAULTED);
    
    // Faulted -> Available
    EXPECT_TRUE(client_manager_->processEvseEvent(1, 1, EvseEvent::FAULT_CLEARED));
    EXPECT_EQ(evse->getCurrentState(), EvseState::AVAILABLE);
    EXPECT_EQ(evse->getConnectorStatus(), ConnectorStatus::AVAILABLE);
    
    // Test fault during charging
    EXPECT_TRUE(client_manager_->processEvseEvent(1, 1, EvseEvent::PLUG_IN));
    nlohmann::json authData;
    authData["idTag"] = "TEST_TAG_123";
    EXPECT_TRUE(client_manager_->processEvseEvent(1, 1, EvseEvent::AUTHORIZE_START, authData));
    EXPECT_EQ(evse->getCurrentState(), EvseState::CHARGING);
    EXPECT_TRUE(evse->getCurrentTransaction().has_value());
    
    // Charging -> Faulted
    EXPECT_TRUE(client_manager_->processEvseEvent(1, 1, EvseEvent::FAULT_DETECTED));
    EXPECT_EQ(evse->getCurrentState(), EvseState::FAULTED);
    EXPECT_FALSE(evse->getCurrentTransaction().has_value()); // Transaction should be stopped
}