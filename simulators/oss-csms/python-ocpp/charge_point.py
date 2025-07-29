#!/usr/bin/env python3
"""
OCPP 2.0.1 Charge Point Emulator using python-ocpp
"""

import asyncio
import logging
import random
import time
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


class ChargePointEmulator(ChargePointV201):
    """Charge Point Emulator using python-ocpp"""

    def __init__(self, id: str, connection, response_timeout: int = 30):
        super().__init__(id, connection, response_timeout)
        self.connectors: Dict[int, Dict] = {
            1: {"status": ConnectorStatus.available, "transaction_id": None},
            2: {"status": ConnectorStatus.available, "transaction_id": None},
        }
        self.transactions: Dict[str, Dict] = {}
        self.meter_values: Dict[int, float] = {1: 0.0, 2: 0.0}
        self.simulation_task = None

    async def start_simulation(self):
        """Start simulation tasks"""
        self.simulation_task = asyncio.create_task(self._simulate_charging())
        logger.info(f"Started simulation for {self.id}")

    async def stop_simulation(self):
        """Stop simulation tasks"""
        if self.simulation_task:
            self.simulation_task.cancel()
            try:
                await self.simulation_task
            except asyncio.CancelledError:
                pass
        logger.info(f"Stopped simulation for {self.id}")

    async def _simulate_charging(self):
        """Simulate charging process"""
        while True:
            try:
                # Simulate meter value updates
                for connector_id in [1, 2]:
                    if self.connectors[connector_id]["transaction_id"]:
                        # Increment meter value during charging
                        self.meter_values[connector_id] += random.uniform(0.1, 0.5)
                        
                        # Send meter values
                        await self._send_meter_values(connector_id)
                
                await asyncio.sleep(5)  # Update every 5 seconds
                
            except asyncio.CancelledError:
                break
            except Exception as e:
                logger.error(f"Simulation error: {e}")
                await asyncio.sleep(1)

    async def _send_meter_values(self, connector_id: int):
        """Send meter values for connector"""
        meter_value = MeterValue(
            timestamp=datetime.utcnow().isoformat(),
            sampled_value=[
                SampledValue(
                    value=self.meter_values[connector_id],
                    measurand="Energy.Active.Import.Register",
                    unit_of_measure={"unit": "kWh"},
                ),
                SampledValue(
                    value=random.uniform(3.0, 22.0),  # kW
                    measurand="Power.Active.Import",
                    unit_of_measure={"unit": "kW"},
                ),
            ],
        )

        request = call.MeterValuesRequest(
            evse_id=connector_id,
            meter_value=[meter_value],
        )
        
        try:
            await self.call(request)
        except Exception as e:
            logger.error(f"Failed to send meter values: {e}")

    @on("RequestStartTransaction")
    def on_request_start_transaction(
        self,
        id_token: dict,
        connector_id: int,
        **kwargs,
    ):
        """Handle RequestStartTransaction from CSMS"""
        logger.info(f"Request start transaction for connector {connector_id}")
        
        if self.connectors[connector_id]["status"] != ConnectorStatus.available:
            return call_result.RequestStartTransactionResponse(
                status=RequestStartStopStatus.rejected,
            )
        
        # Start transaction
        transaction_id = f"tx_{int(time.time())}_{connector_id}"
        self.connectors[connector_id]["transaction_id"] = transaction_id
        self.connectors[connector_id]["status"] = ConnectorStatus.occupied
        
        self.transactions[transaction_id] = {
            "connector_id": connector_id,
            "id_token": id_token,
            "start_time": datetime.utcnow().isoformat(),
            "meter_start": self.meter_values[connector_id],
        }
        
        # Send TransactionEvent
        asyncio.create_task(self._send_transaction_event(
            TransactionEvent.started,
            transaction_id,
            connector_id
        ))
        
        return call_result.RequestStartTransactionResponse(
            status=RequestStartStopStatus.accepted,
            transaction_id=transaction_id,
        )

    @on("RequestStopTransaction")
    def on_request_stop_transaction(
        self,
        transaction_id: str,
        **kwargs,
    ):
        """Handle RequestStopTransaction from CSMS"""
        logger.info(f"Request stop transaction {transaction_id}")
        
        if transaction_id not in self.transactions:
            return call_result.RequestStopTransactionResponse(
                status=RequestStartStopStatus.rejected,
            )
        
        # Stop transaction
        transaction = self.transactions[transaction_id]
        connector_id = transaction["connector_id"]
        
        self.connectors[connector_id]["transaction_id"] = None
        self.connectors[connector_id]["status"] = ConnectorStatus.available
        
        transaction["stop_time"] = datetime.utcnow().isoformat()
        transaction["meter_stop"] = self.meter_values[connector_id]
        
        # Send TransactionEvent
        asyncio.create_task(self._send_transaction_event(
            TransactionEvent.ended,
            transaction_id,
            connector_id
        ))
        
        return call_result.RequestStopTransactionResponse(
            status=RequestStartStopStatus.accepted,
        )

    async def _send_transaction_event(self, event_type: TransactionEvent, transaction_id: str, connector_id: int):
        """Send TransactionEvent to CSMS"""
        transaction = self.transactions[transaction_id]
        
        request = call.TransactionEventRequest(
            event_type=event_type,
            timestamp=datetime.utcnow().isoformat(),
            transaction_info=Transaction(
                transaction_id=transaction_id,
                charging_state="Charging" if event_type == TransactionEvent.started else "EVConnected",
                time_spent_charging=0,
                stopped_reason="Remote" if event_type == TransactionEvent.ended else None,
            ),
            evse=connector_id,
            meter_value=[
                MeterValue(
                    timestamp=datetime.utcnow().isoformat(),
                    sampled_value=[
                        SampledValue(
                            value=self.meter_values[connector_id],
                            measurand="Energy.Active.Import.Register",
                            unit_of_measure={"unit": "kWh"},
                        ),
                    ],
                )
            ],
        )
        
        try:
            await self.call(request)
        except Exception as e:
            logger.error(f"Failed to send transaction event: {e}")

    async def send_boot_notification(self):
        """Send BootNotification to CSMS"""
        request = call.BootNotificationRequest(
            charging_station={
                "serial_number": f"CP{self.id}",
                "model": "OCPP Gateway Test CP",
                "vendor_name": "Test Vendor",
                "firmware_version": "1.0.0",
            },
            reason=BootReason.power_up,
        )
        
        try:
            response = await self.call(request)
            logger.info(f"Boot notification response: {response}")
            return response
        except Exception as e:
            logger.error(f"Failed to send boot notification: {e}")

    async def send_status_notification(self, connector_id: int, status: ConnectorStatus):
        """Send StatusNotification to CSMS"""
        request = call.StatusNotificationRequest(
            timestamp=datetime.utcnow().isoformat(),
            connector_status=status,
            evse_id=connector_id,
            connector_id=connector_id,
        )
        
        try:
            await self.call(request)
        except Exception as e:
            logger.error(f"Failed to send status notification: {e}")

    async def send_authorize(self, id_token: str):
        """Send Authorize request to CSMS"""
        request = call.AuthorizeRequest(
            id_token={
                "id_token": id_token,
                "type": "Central",
            }
        )
        
        try:
            response = await self.call(request)
            logger.info(f"Authorize response: {response}")
            return response
        except Exception as e:
            logger.error(f"Failed to send authorize: {e}")

    async def send_heartbeat(self):
        """Send Heartbeat to CSMS"""
        request = call.HeartbeatRequest()
        
        try:
            response = await self.call(request)
            logger.info(f"Heartbeat response: {response}")
            return response
        except Exception as e:
            logger.error(f"Failed to send heartbeat: {e}")


async def main():
    """Main function to start charge point emulator"""
    uri = "ws://localhost:9000/CP001"
    
    async for websocket in websockets.connect(
        uri,
        subprotocols=['ocpp2.0.1']
    ):
        try:
            charge_point = ChargePointEmulator("CP001", websocket)
            
            # Send boot notification
            await charge_point.send_boot_notification()
            
            # Start simulation
            await charge_point.start_simulation()
            
            # Send initial status notifications
            for connector_id in [1, 2]:
                await charge_point.send_status_notification(
                    connector_id, 
                    ConnectorStatus.available
                )
            
            # Keep connection alive
            await charge_point.start()
            
        except websockets.exceptions.ConnectionClosed:
            logger.info("Connection closed")
            break
        except Exception as e:
            logger.error(f"Error: {e}")
            break


if __name__ == "__main__":
    import websockets
    asyncio.run(main()) 