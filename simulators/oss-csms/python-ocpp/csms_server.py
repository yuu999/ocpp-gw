#!/usr/bin/env python3
"""
OCPP 2.0.1 CSMS Server using python-ocpp
"""

import asyncio
import logging
from datetime import datetime
from typing import Dict, List, Optional

from ocpp.routing import on
from ocpp.v201 import ChargePoint as ChargePointV201
from ocpp.v201 import call, call_result
from ocpp.v201.enums import (
    AuthorizeStatus,
    BootReason,
    ChargingProfileStatus,
    ConnectorStatus,
    DataTransferStatus,
    GenericStatus,
    RegistrationStatus,
    RequestStartStopStatus,
    ReservationStatus,
    TransactionEvent,
    TriggerReason,
    UnlockStatus,
)
from ocpp.v201.datatypes import (
    AuthorizeData,
    BootNotificationResponse,
    ChargingProfile,
    ChargingSchedule,
    ClearChargingProfileData,
    ClearChargingProfileResponse,
    GetChargingProfilesData,
    GetChargingProfilesResponse,
    GetDisplayMessagesData,
    GetDisplayMessagesResponse,
    GetInstalledCertificateIdsData,
    GetInstalledCertificateIdsResponse,
    GetLocalListVersionData,
    GetLocalListVersionResponse,
    GetLogData,
    GetLogResponse,
    GetMonitoringReportData,
    GetMonitoringReportResponse,
    GetReportData,
    GetReportResponse,
    GetTransactionStatusData,
    GetTransactionStatusResponse,
    GetVariablesData,
    GetVariablesResponse,
    InstallCertificateData,
    InstallCertificateResponse,
    LogParameters,
    MessageInfo,
    MeterValue,
    MonitoringData,
    PublishFirmwareData,
    PublishFirmwareResponse,
    ReportData,
    RequestStartTransactionData,
    RequestStartTransactionResponse,
    RequestStopTransactionData,
    RequestStopTransactionResponse,
    Reservation,
    ReserveNowData,
    ReserveNowResponse,
    ResetData,
    ResetResponse,
    SampledValue,
    SendLocalListData,
    SendLocalListResponse,
    SetChargingProfileData,
    SetChargingProfileResponse,
    SetDisplayMessageData,
    SetDisplayMessageResponse,
    SetMonitoringBaseData,
    SetMonitoringBaseResponse,
    SetMonitoringLevelData,
    SetMonitoringLevelResponse,
    SetNetworkProfileData,
    SetNetworkProfileResponse,
    SetVariableData,
    SetVariableResponse,
    SetVariablesData,
    SetVariablesResponse,
    Transaction,
    TriggerMessageData,
    TriggerMessageResponse,
    UnlockConnectorData,
    UnlockConnectorResponse,
    UnpublishFirmwareData,
    UnpublishFirmwareResponse,
    UpdateFirmwareData,
    UpdateFirmwareResponse,
)

logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)


class CSMS(ChargePointV201):
    """CSMS implementation using python-ocpp"""

    def __init__(self, id: str, connection, response_timeout: int = 30):
        super().__init__(id, connection, response_timeout)
        self.charging_stations: Dict[str, Dict] = {}
        self.transactions: Dict[str, List[Transaction]] = {}

    @on("BootNotification")
    def on_boot_notification(
        self,
        charging_station: dict,
        reason: BootReason,
        **kwargs,
    ):
        """Handle BootNotification from charging station"""
        logger.info(f"Boot notification from {self.id}: {charging_station}")
        
        # Store charging station info
        self.charging_stations[self.id] = {
            "charging_station": charging_station,
            "reason": reason,
            "connected_at": datetime.utcnow().isoformat(),
            "status": "connected"
        }

        return call_result.BootNotificationResponse(
            status=RegistrationStatus.accepted,
            current_time=datetime.utcnow().isoformat(),
            interval=300,  # 5 minutes
        )

    @on("StatusNotification")
    def on_status_notification(
        self,
        timestamp: str,
        connector_status: ConnectorStatus,
        evse_id: int,
        connector_id: int,
        **kwargs,
    ):
        """Handle StatusNotification from charging station"""
        logger.info(f"Status notification from {self.id}: connector {connector_id} = {connector_status}")
        
        # Update connector status
        if self.id not in self.charging_stations:
            self.charging_stations[self.id] = {}
        
        if "connectors" not in self.charging_stations[self.id]:
            self.charging_stations[self.id]["connectors"] = {}
        
        self.charging_stations[self.id]["connectors"][connector_id] = {
            "status": connector_status,
            "evse_id": evse_id,
            "timestamp": timestamp
        }

    @on("Authorize")
    def on_authorize(
        self,
        id_token: dict,
        **kwargs,
    ):
        """Handle Authorize request from charging station"""
        logger.info(f"Authorize request from {self.id}: {id_token}")
        
        return call_result.AuthorizeResponse(
            id_token_info={
                "status": AuthorizeStatus.accepted,
            }
        )

    @on("TransactionEvent")
    def on_transaction_event(
        self,
        event_type: TransactionEvent,
        timestamp: str,
        transaction_info: Transaction,
        **kwargs,
    ):
        """Handle TransactionEvent from charging station"""
        logger.info(f"Transaction event from {self.id}: {event_type} - {transaction_info}")
        
        # Store transaction info
        if self.id not in self.transactions:
            self.transactions[self.id] = []
        
        self.transactions[self.id].append(transaction_info)

        return call_result.TransactionEventResponse(
            total_cost=0,
            charging_priority=0,
            id_token_info={
                "status": AuthorizeStatus.accepted,
            },
            updated_personal_message={
                "format": "UTF8",
                "code": "OK",
            },
        )

    @on("MeterValues")
    def on_meter_values(
        self,
        evse_id: int,
        meter_value: List[MeterValue],
        **kwargs,
    ):
        """Handle MeterValues from charging station"""
        logger.info(f"Meter values from {self.id}: evse {evse_id} = {meter_value}")
        
        # Store meter values
        if self.id not in self.charging_stations:
            self.charging_stations[self.id] = {}
        
        if "meter_values" not in self.charging_stations[self.id]:
            self.charging_stations[self.id]["meter_values"] = []
        
        self.charging_stations[self.id]["meter_values"].append({
            "evse_id": evse_id,
            "meter_value": meter_value,
            "timestamp": datetime.utcnow().isoformat()
        })

    @on("Heartbeat")
    def on_heartbeat(self):
        """Handle Heartbeat from charging station"""
        logger.info(f"Heartbeat from {self.id}")
        
        return call_result.HeartbeatResponse(
            current_time=datetime.utcnow().isoformat()
        )

    @on("DataTransfer")
    def on_data_transfer(
        self,
        vendor_id: str,
        **kwargs,
    ):
        """Handle DataTransfer from charging station"""
        logger.info(f"Data transfer from {self.id}: vendor {vendor_id}")
        
        return call_result.DataTransferResponse(
            status=DataTransferStatus.accepted
        )

    async def send_authorize(self, id_token: dict):
        """Send Authorize request to charging station"""
        request = call.AuthorizeRequest(
            id_token=id_token
        )
        response = await self.call(request)
        logger.info(f"Authorize response: {response}")
        return response

    async def send_start_transaction(self, id_token: dict, connector_id: int):
        """Send RequestStartTransaction to charging station"""
        request = call.RequestStartTransactionRequest(
            id_token=id_token,
            connector_id=connector_id
        )
        response = await self.call(request)
        logger.info(f"Start transaction response: {response}")
        return response

    async def send_stop_transaction(self, transaction_id: str):
        """Send RequestStopTransaction to charging station"""
        request = call.RequestStopTransactionRequest(
            transaction_id=transaction_id
        )
        response = await self.call(request)
        logger.info(f"Stop transaction response: {response}")
        return response

    def get_status(self):
        """Get current CSMS status"""
        return {
            "charging_stations": self.charging_stations,
            "transactions": self.transactions,
            "connected_stations": len(self.charging_stations)
        }


async def main():
    """Main function to start CSMS server"""
    server = None
    try:
        # Start WebSocket server
        server = await websockets.serve(
            lambda ws, path: CSMS(path.split('/')[-1], ws),
            '0.0.0.0',
            9000,
            subprotocols=['ocpp2.0.1']
        )
        
        logger.info("CSMS Server started on ws://0.0.0.0:9000")
        logger.info("Waiting for charging stations to connect...")
        
        await server.wait_closed()
        
    except KeyboardInterrupt:
        logger.info("Shutting down CSMS server...")
    finally:
        if server:
            server.close()
            await server.wait_closed()


if __name__ == "__main__":
    import websockets
    asyncio.run(main()) 