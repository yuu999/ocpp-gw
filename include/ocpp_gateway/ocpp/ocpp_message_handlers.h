#pragma once

#include "ocpp_gateway/ocpp/ocpp_message_processor.h"
#include <chrono>

namespace ocpp_gateway {
namespace ocpp {

/**
 * @class BootNotificationHandler
 * @brief Handles BootNotification messages
 */
class BootNotificationHandler : public OcppMessageHandler {
public:
    /**
     * @brief Create a BootNotificationHandler instance
     * @return Shared pointer to BootNotificationHandler
     */
    static std::shared_ptr<BootNotificationHandler> create();
    
    /**
     * @brief Handle a BootNotification message
     * @param message OCPP message
     * @return Response message
     */
    std::unique_ptr<OcppMessage> handleMessage(const OcppMessage& message) override;
    
    /**
     * @brief Create a BootNotification request
     * @param chargePointModel Charge point model
     * @param chargePointVendor Charge point vendor
     * @param firmwareVersion Firmware version (optional)
     * @return BootNotification request message
     */
    static OcppMessage createRequest(
        const std::string& chargePointModel,
        const std::string& chargePointVendor,
        const std::string& firmwareVersion = "");

private:
    BootNotificationHandler() = default;
};

/**
 * @class HeartbeatHandler
 * @brief Handles Heartbeat messages
 */
class HeartbeatHandler : public OcppMessageHandler {
public:
    /**
     * @brief Create a HeartbeatHandler instance
     * @return Shared pointer to HeartbeatHandler
     */
    static std::shared_ptr<HeartbeatHandler> create();
    
    /**
     * @brief Handle a Heartbeat message
     * @param message OCPP message
     * @return Response message
     */
    std::unique_ptr<OcppMessage> handleMessage(const OcppMessage& message) override;
    
    /**
     * @brief Create a Heartbeat request
     * @return Heartbeat request message
     */
    static OcppMessage createRequest();

private:
    HeartbeatHandler() = default;
};

/**
 * @class StatusNotificationHandler
 * @brief Handles StatusNotification messages
 */
class StatusNotificationHandler : public OcppMessageHandler {
public:
    /**
     * @brief Create a StatusNotificationHandler instance
     * @return Shared pointer to StatusNotificationHandler
     */
    static std::shared_ptr<StatusNotificationHandler> create();
    
    /**
     * @brief Handle a StatusNotification message
     * @param message OCPP message
     * @return Response message
     */
    std::unique_ptr<OcppMessage> handleMessage(const OcppMessage& message) override;
    
    /**
     * @brief Create a StatusNotification request
     * @param connectorId Connector ID
     * @param errorCode Error code
     * @param status Connector status
     * @param timestamp Timestamp (optional, defaults to now)
     * @return StatusNotification request message
     */
    static OcppMessage createRequest(
        int connectorId,
        const std::string& errorCode,
        const std::string& status,
        const std::string& timestamp = "");

private:
    StatusNotificationHandler() = default;
};

/**
 * @class TransactionEventHandler
 * @brief Handles TransactionEvent messages
 */
class TransactionEventHandler : public OcppMessageHandler {
public:
    /**
     * @brief Create a TransactionEventHandler instance
     * @return Shared pointer to TransactionEventHandler
     */
    static std::shared_ptr<TransactionEventHandler> create();
    
    /**
     * @brief Handle a TransactionEvent message
     * @param message OCPP message
     * @return Response message
     */
    std::unique_ptr<OcppMessage> handleMessage(const OcppMessage& message) override;
    
    /**
     * @brief Create a TransactionEvent request
     * @param eventType Event type (Started, Updated, Ended)
     * @param timestamp Timestamp (optional, defaults to now)
     * @param triggerReason Trigger reason
     * @param seqNo Sequence number
     * @param transactionId Transaction ID
     * @param evseId EVSE ID
     * @param meterValue Meter value (optional)
     * @return TransactionEvent request message
     */
    static OcppMessage createRequest(
        const std::string& eventType,
        const std::string& timestamp,
        const std::string& triggerReason,
        int seqNo,
        const std::string& transactionId,
        int evseId,
        double meterValue = -1.0);

private:
    TransactionEventHandler() = default;
};

/**
 * @class MeterValuesHandler
 * @brief Handles MeterValues messages
 */
class MeterValuesHandler : public OcppMessageHandler {
public:
    /**
     * @brief Create a MeterValuesHandler instance
     * @return Shared pointer to MeterValuesHandler
     */
    static std::shared_ptr<MeterValuesHandler> create();
    
    /**
     * @brief Handle a MeterValues message
     * @param message OCPP message
     * @return Response message
     */
    std::unique_ptr<OcppMessage> handleMessage(const OcppMessage& message) override;
    
    /**
     * @brief Create a MeterValues request
     * @param evseId EVSE ID
     * @param meterValue Meter value
     * @param timestamp Timestamp (optional, defaults to now)
     * @return MeterValues request message
     */
    static OcppMessage createRequest(
        int evseId,
        double meterValue,
        const std::string& timestamp = "");

private:
    MeterValuesHandler() = default;
};

/**
 * @class AuthorizeHandler
 * @brief Handles Authorize messages
 */
class AuthorizeHandler : public OcppMessageHandler {
public:
    /**
     * @brief Create an AuthorizeHandler instance
     * @return Shared pointer to AuthorizeHandler
     */
    static std::shared_ptr<AuthorizeHandler> create();
    
    /**
     * @brief Handle an Authorize message
     * @param message OCPP message
     * @return Response message
     */
    std::unique_ptr<OcppMessage> handleMessage(const OcppMessage& message) override;
    
    /**
     * @brief Create an Authorize request
     * @param idToken ID token
     * @return Authorize request message
     */
    static OcppMessage createRequest(const std::string& idToken);

private:
    AuthorizeHandler() = default;
};

/**
 * @class RemoteStartTransactionHandler
 * @brief Handles RemoteStartTransaction messages
 */
class RemoteStartTransactionHandler : public OcppMessageHandler {
public:
    /**
     * @brief Create a RemoteStartTransactionHandler instance
     * @return Shared pointer to RemoteStartTransactionHandler
     */
    static std::shared_ptr<RemoteStartTransactionHandler> create();
    
    /**
     * @brief Handle a RemoteStartTransaction message
     * @param message OCPP message
     * @return Response message
     */
    std::unique_ptr<OcppMessage> handleMessage(const OcppMessage& message) override;

private:
    RemoteStartTransactionHandler() = default;
};

/**
 * @class RemoteStopTransactionHandler
 * @brief Handles RemoteStopTransaction messages
 */
class RemoteStopTransactionHandler : public OcppMessageHandler {
public:
    /**
     * @brief Create a RemoteStopTransactionHandler instance
     * @return Shared pointer to RemoteStopTransactionHandler
     */
    static std::shared_ptr<RemoteStopTransactionHandler> create();
    
    /**
     * @brief Handle a RemoteStopTransaction message
     * @param message OCPP message
     * @return Response message
     */
    std::unique_ptr<OcppMessage> handleMessage(const OcppMessage& message) override;

private:
    RemoteStopTransactionHandler() = default;
};

/**
 * @class UnlockConnectorHandler
 * @brief Handles UnlockConnector messages
 */
class UnlockConnectorHandler : public OcppMessageHandler {
public:
    /**
     * @brief Create an UnlockConnectorHandler instance
     * @return Shared pointer to UnlockConnectorHandler
     */
    static std::shared_ptr<UnlockConnectorHandler> create();
    
    /**
     * @brief Handle an UnlockConnector message
     * @param message OCPP message
     * @return Response message
     */
    std::unique_ptr<OcppMessage> handleMessage(const OcppMessage& message) override;

private:
    UnlockConnectorHandler() = default;
};

/**
 * @class TriggerMessageHandler
 * @brief Handles TriggerMessage messages
 */
class TriggerMessageHandler : public OcppMessageHandler {
public:
    /**
     * @brief Create a TriggerMessageHandler instance
     * @return Shared pointer to TriggerMessageHandler
     */
    static std::shared_ptr<TriggerMessageHandler> create();
    
    /**
     * @brief Handle a TriggerMessage message
     * @param message OCPP message
     * @return Response message
     */
    std::unique_ptr<OcppMessage> handleMessage(const OcppMessage& message) override;

private:
    TriggerMessageHandler() = default;
};

/**
 * @class SetChargingProfileHandler
 * @brief Handles SetChargingProfile messages
 */
class SetChargingProfileHandler : public OcppMessageHandler {
public:
    /**
     * @brief Create a SetChargingProfileHandler instance
     * @return Shared pointer to SetChargingProfileHandler
     */
    static std::shared_ptr<SetChargingProfileHandler> create();
    
    /**
     * @brief Handle a SetChargingProfile message
     * @param message OCPP message
     * @return Response message
     */
    std::unique_ptr<OcppMessage> handleMessage(const OcppMessage& message) override;

private:
    SetChargingProfileHandler() = default;
};

/**
 * @class DataTransferHandler
 * @brief Handles DataTransfer messages
 */
class DataTransferHandler : public OcppMessageHandler {
public:
    /**
     * @brief Create a DataTransferHandler instance
     * @return Shared pointer to DataTransferHandler
     */
    static std::shared_ptr<DataTransferHandler> create();
    
    /**
     * @brief Handle a DataTransfer message
     * @param message OCPP message
     * @return Response message
     */
    std::unique_ptr<OcppMessage> handleMessage(const OcppMessage& message) override;
    
    /**
     * @brief Create a DataTransfer request
     * @param vendorId Vendor ID
     * @param messageId Message ID (optional)
     * @param data Data (optional)
     * @return DataTransfer request message
     */
    static OcppMessage createRequest(
        const std::string& vendorId,
        const std::string& messageId = "",
        const nlohmann::json& data = nlohmann::json());

private:
    DataTransferHandler() = default;
};

} // namespace ocpp
} // namespace ocpp_gateway