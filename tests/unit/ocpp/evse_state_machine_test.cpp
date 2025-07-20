#include "gtest/gtest.h"
#include "ocpp_gateway/ocpp/evse_state_machine.h"
#include <boost/asio.hpp>
#include <thread>
#include <chrono>

using namespace ocpp_gateway::ocpp;

class EvseStateMachineTest : public ::testing::Test {
protected:
    void SetUp() override {
        io_context_ = std::make_shared<boost::asio::io_context>();
        work_guard_ = std::make_shared<boost::asio::io_context::work>(*io_context_);
        io_thread_ = std::thread([this]() { io_context_->run(); });
        
        evse_state_machine_ = EvseStateMachine::create(*io_context_, 1, 1);
    }
    
    void TearDown() override {
        work_guard_.reset();
        io_context_->stop();
        if (io_thread_.joinable()) {
            io_thread_.join();
        }
    }
    
    std::shared_ptr<boost::asio::io_context> io_context_;
    std::shared_ptr<boost::asio::io_context::work> work_guard_;
    std::thread io_thread_;
    std::shared_ptr<EvseStateMachine> evse_state_machine_;
};

// Test initial state
TEST_F(EvseStateMachineTest, InitialState) {
    EXPECT_EQ(evse_state_machine_->getCurrentState(), EvseState::AVAILABLE);
    EXPECT_EQ(evse_state_machine_->getConnectorStatus(), ConnectorStatus::AVAILABLE);
    EXPECT_FALSE(evse_state_machine_->getCurrentTransaction().has_value());
}

// Test state transitions
TEST_F(EvseStateMachineTest, StateTransitions) {
    // Available -> Preparing
    EXPECT_TRUE(evse_state_machine_->processEvent(EvseEvent::PLUG_IN));
    EXPECT_EQ(evse_state_machine_->getCurrentState(), EvseState::PREPARING);
    EXPECT_EQ(evse_state_machine_->getConnectorStatus(), ConnectorStatus::OCCUPIED);
    
    // Preparing -> Charging
    nlohmann::json authData;
    authData["idTag"] = "TEST_TAG_123";
    EXPECT_TRUE(evse_state_machine_->processEvent(EvseEvent::AUTHORIZE_START, authData));
    EXPECT_EQ(evse_state_machine_->getCurrentState(), EvseState::CHARGING);
    EXPECT_EQ(evse_state_machine_->getConnectorStatus(), ConnectorStatus::OCCUPIED);
    EXPECT_TRUE(evse_state_machine_->getCurrentTransaction().has_value());
    
    // Charging -> SuspendedEV
    EXPECT_TRUE(evse_state_machine_->processEvent(EvseEvent::SUSPEND_CHARGING_EV));
    EXPECT_EQ(evse_state_machine_->getCurrentState(), EvseState::SUSPENDED_EV);
    EXPECT_EQ(evse_state_machine_->getConnectorStatus(), ConnectorStatus::OCCUPIED);
    
    // SuspendedEV -> Charging
    EXPECT_TRUE(evse_state_machine_->processEvent(EvseEvent::RESUME_CHARGING));
    EXPECT_EQ(evse_state_machine_->getCurrentState(), EvseState::CHARGING);
    
    // Charging -> SuspendedEVSE
    EXPECT_TRUE(evse_state_machine_->processEvent(EvseEvent::SUSPEND_CHARGING_EVSE));
    EXPECT_EQ(evse_state_machine_->getCurrentState(), EvseState::SUSPENDED_EVSE);
    
    // SuspendedEVSE -> Charging
    EXPECT_TRUE(evse_state_machine_->processEvent(EvseEvent::RESUME_CHARGING));
    EXPECT_EQ(evse_state_machine_->getCurrentState(), EvseState::CHARGING);
    
    // Charging -> Finishing
    EXPECT_TRUE(evse_state_machine_->processEvent(EvseEvent::STOP_CHARGING));
    EXPECT_EQ(evse_state_machine_->getCurrentState(), EvseState::FINISHING);
    EXPECT_FALSE(evse_state_machine_->getCurrentTransaction().has_value());
    
    // Finishing -> Available
    EXPECT_TRUE(evse_state_machine_->processEvent(EvseEvent::PLUG_OUT));
    EXPECT_EQ(evse_state_machine_->getCurrentState(), EvseState::AVAILABLE);
    EXPECT_EQ(evse_state_machine_->getConnectorStatus(), ConnectorStatus::AVAILABLE);
}

// Test reservation flow
TEST_F(EvseStateMachineTest, ReservationFlow) {
    // Available -> Reserved
    EXPECT_TRUE(evse_state_machine_->processEvent(EvseEvent::RESERVE));
    EXPECT_EQ(evse_state_machine_->getCurrentState(), EvseState::RESERVED);
    EXPECT_EQ(evse_state_machine_->getConnectorStatus(), ConnectorStatus::RESERVED);
    
    // Reserved -> Preparing
    EXPECT_TRUE(evse_state_machine_->processEvent(EvseEvent::PLUG_IN));
    EXPECT_EQ(evse_state_machine_->getCurrentState(), EvseState::PREPARING);
    
    // Preparing -> Available (plug out without charging)
    EXPECT_TRUE(evse_state_machine_->processEvent(EvseEvent::PLUG_OUT));
    EXPECT_EQ(evse_state_machine_->getCurrentState(), EvseState::AVAILABLE);
    
    // Available -> Reserved again
    EXPECT_TRUE(evse_state_machine_->processEvent(EvseEvent::RESERVE));
    EXPECT_EQ(evse_state_machine_->getCurrentState(), EvseState::RESERVED);
    
    // Reserved -> Available (cancel reservation)
    EXPECT_TRUE(evse_state_machine_->processEvent(EvseEvent::CANCEL_RESERVATION));
    EXPECT_EQ(evse_state_machine_->getCurrentState(), EvseState::AVAILABLE);
}

// Test fault handling
TEST_F(EvseStateMachineTest, FaultHandling) {
    // Available -> Faulted
    EXPECT_TRUE(evse_state_machine_->processEvent(EvseEvent::FAULT_DETECTED));
    EXPECT_EQ(evse_state_machine_->getCurrentState(), EvseState::FAULTED);
    EXPECT_EQ(evse_state_machine_->getConnectorStatus(), ConnectorStatus::FAULTED);
    
    // Faulted -> Available
    EXPECT_TRUE(evse_state_machine_->processEvent(EvseEvent::FAULT_CLEARED));
    EXPECT_EQ(evse_state_machine_->getCurrentState(), EvseState::AVAILABLE);
    EXPECT_EQ(evse_state_machine_->getConnectorStatus(), ConnectorStatus::AVAILABLE);
    
    // Test fault during charging
    EXPECT_TRUE(evse_state_machine_->processEvent(EvseEvent::PLUG_IN));
    nlohmann::json authData;
    authData["idTag"] = "TEST_TAG_123";
    EXPECT_TRUE(evse_state_machine_->processEvent(EvseEvent::AUTHORIZE_START, authData));
    EXPECT_EQ(evse_state_machine_->getCurrentState(), EvseState::CHARGING);
    EXPECT_TRUE(evse_state_machine_->getCurrentTransaction().has_value());
    
    // Charging -> Faulted
    EXPECT_TRUE(evse_state_machine_->processEvent(EvseEvent::FAULT_DETECTED));
    EXPECT_EQ(evse_state_machine_->getCurrentState(), EvseState::FAULTED);
    EXPECT_FALSE(evse_state_machine_->getCurrentTransaction().has_value()); // Transaction should be stopped
}

// Test variable handling
TEST_F(EvseStateMachineTest, VariableHandling) {
    // Set a variable
    evse_state_machine_->setVariable("MeterValue.Energy.Active.Import.Register", "1000", "float", 0.1, "kWh");
    
    // Get the variable
    EXPECT_EQ(evse_state_machine_->getVariableValue("MeterValue.Energy.Active.Import.Register"), "1000");
    
    auto var = evse_state_machine_->getVariable("MeterValue.Energy.Active.Import.Register");
    ASSERT_TRUE(var.has_value());
    EXPECT_EQ(var->name, "MeterValue.Energy.Active.Import.Register");
    EXPECT_EQ(var->value, "1000");
    EXPECT_EQ(var->dataType, "float");
    ASSERT_TRUE(var->scale.has_value());
    EXPECT_EQ(*var->scale, 0.1);
    ASSERT_TRUE(var->unit.has_value());
    EXPECT_EQ(*var->unit, "kWh");
    
    // Update the variable
    evse_state_machine_->setVariable("MeterValue.Energy.Active.Import.Register", "2000", "float", 0.1, "kWh");
    EXPECT_EQ(evse_state_machine_->getVariableValue("MeterValue.Energy.Active.Import.Register"), "2000");
    
    // Test non-existent variable
    EXPECT_EQ(evse_state_machine_->getVariableValue("NonExistentVariable"), "");
    EXPECT_FALSE(evse_state_machine_->getVariable("NonExistentVariable").has_value());
}

// Test callbacks
TEST_F(EvseStateMachineTest, Callbacks) {
    bool status_change_called = false;
    bool meter_value_called = false;
    bool transaction_event_called = false;
    
    // Set callbacks
    evse_state_machine_->setStatusChangeCallback(
        [&status_change_called](int connectorId, const std::string& errorCode, const std::string& status) {
            status_change_called = true;
        });
    
    evse_state_machine_->setMeterValueCallback(
        [&meter_value_called](int evseId, double meterValue) {
            meter_value_called = true;
        });
    
    evse_state_machine_->setTransactionEventCallback(
        [&transaction_event_called](const std::string& eventType, const std::string& timestamp, 
                                   const std::string& triggerReason, int seqNo, const std::string& transactionId, 
                                   int evseId, double meterValue) {
            transaction_event_called = true;
        });
    
    // Trigger status change
    EXPECT_TRUE(evse_state_machine_->processEvent(EvseEvent::PLUG_IN));
    EXPECT_TRUE(status_change_called);
    
    // Set meter value and start transaction to trigger callbacks
    evse_state_machine_->setVariable("MeterValue.Energy.Active.Import.Register", "1000", "float", 0.1, "kWh");
    
    nlohmann::json authData;
    authData["idTag"] = "TEST_TAG_123";
    EXPECT_TRUE(evse_state_machine_->processEvent(EvseEvent::AUTHORIZE_START, authData));
    EXPECT_TRUE(transaction_event_called);
    
    // Reset flags
    meter_value_called = false;
    transaction_event_called = false;
    
    // Start meter value timer to trigger meter value callback
    evse_state_machine_->startMeterValueTimer(std::chrono::seconds(1));
    
    // Wait for timer to fire
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    
    // Check if callbacks were called
    EXPECT_TRUE(meter_value_called);
    EXPECT_TRUE(transaction_event_called);
    
    // Stop timers
    evse_state_machine_->stopMeterValueTimer();
}

// Test invalid state transitions
TEST_F(EvseStateMachineTest, InvalidStateTransitions) {
    // Cannot stop charging when not charging
    EXPECT_FALSE(evse_state_machine_->processEvent(EvseEvent::STOP_CHARGING));
    
    // Cannot authorize stop when not charging
    nlohmann::json authData;
    authData["idTag"] = "TEST_TAG_123";
    EXPECT_FALSE(evse_state_machine_->processEvent(EvseEvent::AUTHORIZE_STOP, authData));
    
    // Cannot resume charging when not suspended
    EXPECT_FALSE(evse_state_machine_->processEvent(EvseEvent::RESUME_CHARGING));
    
    // Cannot plug out when not plugged in
    EXPECT_FALSE(evse_state_machine_->processEvent(EvseEvent::PLUG_OUT));
}

// Test utility functions
TEST_F(EvseStateMachineTest, UtilityFunctions) {
    // Test state to string conversion
    EXPECT_EQ(EvseStateMachine::stateToString(EvseState::AVAILABLE), "Available");
    EXPECT_EQ(EvseStateMachine::stateToString(EvseState::CHARGING), "Charging");
    EXPECT_EQ(EvseStateMachine::stateToString(EvseState::FAULTED), "Faulted");
    
    // Test connector status to string conversion
    EXPECT_EQ(EvseStateMachine::connectorStatusToString(ConnectorStatus::AVAILABLE), "Available");
    EXPECT_EQ(EvseStateMachine::connectorStatusToString(ConnectorStatus::OCCUPIED), "Occupied");
    EXPECT_EQ(EvseStateMachine::connectorStatusToString(ConnectorStatus::FAULTED), "Faulted");
    
    // Test string to connector status conversion
    EXPECT_EQ(EvseStateMachine::stringToConnectorStatus("Available"), ConnectorStatus::AVAILABLE);
    EXPECT_EQ(EvseStateMachine::stringToConnectorStatus("Occupied"), ConnectorStatus::OCCUPIED);
    EXPECT_EQ(EvseStateMachine::stringToConnectorStatus("Faulted"), ConnectorStatus::FAULTED);
    EXPECT_EQ(EvseStateMachine::stringToConnectorStatus("Unknown"), ConnectorStatus::AVAILABLE); // Default
}

// Test heartbeat mechanism
TEST_F(EvseStateMachineTest, HeartbeatMechanism) {
    bool heartbeat_called = false;
    
    // Start heartbeat timer
    evse_state_machine_->startHeartbeat(std::chrono::seconds(1));
    
    // Wait for timer to fire
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    
    // Stop heartbeat timer
    evse_state_machine_->stopHeartbeat();
}