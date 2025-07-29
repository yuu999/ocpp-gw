#!/usr/bin/env python3
"""
Simple OCPP 2.0.1 CSMS Server
"""

import asyncio
import json
import logging
import websockets
from datetime import datetime
from typing import Dict, Set

logging.basicConfig(level=logging.DEBUG, format='%(asctime)s - %(name)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

class SimpleCSMS:
    def __init__(self):
        self.charging_stations: Dict[str, Dict] = {}
        self.connections: Set[websockets.WebSocketServerProtocol] = set()

    async def handle_connection(self, websocket, path):
        """Handle WebSocket connection"""
        charging_station_id = path.split('/')[-1] if path else 'unknown'
        logger.info(f"Charging station connected: {charging_station_id}")
        logger.debug(f"Connection path: {path}")
        
        self.connections.add(websocket)
        
        try:
            async for message in websocket:
                logger.debug(f"Received raw message: {message}")
                try:
                    data = json.loads(message)
                    logger.debug(f"Parsed JSON: {data}")
                    response = await self.handle_message(charging_station_id, data)
                    if response:
                        logger.debug(f"Sending response: {response}")
                        await websocket.send(json.dumps(response))
                except json.JSONDecodeError as e:
                    logger.error(f"Invalid JSON: {message}, error: {e}")
                    # Send error response
                    error_response = [3, "error", {"errorCode": "FormatViolation"}]
                    logger.debug(f"Sending error response: {error_response}")
                    await websocket.send(json.dumps(error_response))
                except Exception as e:
                    logger.error(f"Error handling message: {e}")
                    # Send error response
                    error_response = [3, "error", {"errorCode": "InternalError"}]
                    logger.debug(f"Sending error response: {error_response}")
                    await websocket.send(json.dumps(error_response))
        except websockets.exceptions.ConnectionClosed:
            logger.info(f"Charging station disconnected: {charging_station_id}")
        except Exception as e:
            logger.error(f"Connection error: {e}")
        finally:
            self.connections.discard(websocket)

    async def handle_message(self, charging_station_id: str, message: list):
        """Handle OCPP message"""
        try:
            if not isinstance(message, list) or len(message) < 3:
                logger.error(f"Invalid message format: {message}")
                return [3, "error", {"errorCode": "FormatViolation"}]
            
            message_type_id, message_id, action, *payload = message
            logger.debug(f"Message type: {message_type_id}, ID: {message_id}, Action: {action}, Payload: {payload}")
            
            logger.info(f"Received {action} from {charging_station_id}")
            
            if message_type_id == 2:  # Call
                return await self.handle_call(charging_station_id, message_id, action, payload[0] if payload else {})
            else:
                logger.warning(f"Unsupported message type: {message_type_id}")
                return [3, message_id, {"errorCode": "NotSupported"}]
                
        except Exception as e:
            logger.error(f"Error in handle_message: {e}")
            return [3, "error", {"errorCode": "InternalError"}]

    async def handle_call(self, charging_station_id: str, message_id: str, action: str, payload: dict):
        """Handle OCPP call"""
        try:
            logger.debug(f"Handling call: {action} with payload: {payload}")
            if action == "BootNotification":
                return self.handle_boot_notification(charging_station_id, message_id, payload)
            elif action == "StatusNotification":
                return self.handle_status_notification(charging_station_id, message_id, payload)
            elif action == "Authorize":
                return self.handle_authorize(charging_station_id, message_id, payload)
            elif action == "TransactionEvent":
                return self.handle_transaction_event(charging_station_id, message_id, payload)
            elif action == "MeterValues":
                return self.handle_meter_values(charging_station_id, message_id, payload)
            elif action == "Heartbeat":
                return self.handle_heartbeat(charging_station_id, message_id, payload)
            elif action == "DataTransfer":
                return self.handle_data_transfer(charging_station_id, message_id, payload)
            else:
                logger.warning(f"Unhandled action: {action}")
                return [3, message_id, {"errorCode": "NotSupported"}]
        except Exception as e:
            logger.error(f"Error in handle_call: {e}")
            return [3, message_id, {"errorCode": "InternalError"}]

    def handle_boot_notification(self, charging_station_id: str, message_id: str, payload: dict):
        """Handle BootNotification"""
        logger.info(f"Boot notification from {charging_station_id}: {payload}")
        
        # Store charging station info
        self.charging_stations[charging_station_id] = {
            "charging_station": payload.get("charging_station", {}),
            "reason": payload.get("reason", "Unknown"),
            "connected_at": datetime.utcnow().isoformat(),
            "status": "connected"
        }
        
        response = [3, message_id, {
            "status": "Accepted",
            "currentTime": datetime.utcnow().isoformat(),
            "interval": 300
        }]
        logger.debug(f"BootNotification response: {response}")
        return response

    def handle_status_notification(self, charging_station_id: str, message_id: str, payload: dict):
        """Handle StatusNotification"""
        logger.info(f"Status notification from {charging_station_id}: {payload}")
        
        if charging_station_id not in self.charging_stations:
            self.charging_stations[charging_station_id] = {}
        
        if "connectors" not in self.charging_stations[charging_station_id]:
            self.charging_stations[charging_station_id]["connectors"] = {}
        
        connector_id = payload.get("connectorId", 0)
        self.charging_stations[charging_station_id]["connectors"][connector_id] = {
            "status": payload.get("connectorStatus", "Unknown"),
            "evse_id": payload.get("evseId", 0),
            "timestamp": payload.get("timestamp", datetime.utcnow().isoformat())
        }
        
        response = [3, message_id, {}]
        logger.debug(f"StatusNotification response: {response}")
        return response

    def handle_authorize(self, charging_station_id: str, message_id: str, payload: dict):
        """Handle Authorize"""
        logger.info(f"Authorize request from {charging_station_id}: {payload}")
        
        response = [3, message_id, {
            "idTokenInfo": {
                "status": "Accepted"
            }
        }]
        logger.debug(f"Authorize response: {response}")
        return response

    def handle_transaction_event(self, charging_station_id: str, message_id: str, payload: dict):
        """Handle TransactionEvent"""
        logger.info(f"Transaction event from {charging_station_id}: {payload}")
        
        response = [3, message_id, {
            "totalCost": 0,
            "chargingPriority": 0,
            "idTokenInfo": {
                "status": "Accepted"
            },
            "updatedPersonalMessage": {
                "format": "UTF8",
                "code": "OK"
            }
        }]
        logger.debug(f"TransactionEvent response: {response}")
        return response

    def handle_meter_values(self, charging_station_id: str, message_id: str, payload: dict):
        """Handle MeterValues"""
        logger.info(f"Meter values from {charging_station_id}: {payload}")
        
        response = [3, message_id, {}]
        logger.debug(f"MeterValues response: {response}")
        return response

    def handle_heartbeat(self, charging_station_id: str, message_id: str, payload: dict):
        """Handle Heartbeat"""
        logger.info(f"Heartbeat from {charging_station_id}")
        
        response = [3, message_id, {
            "currentTime": datetime.utcnow().isoformat()
        }]
        logger.debug(f"Heartbeat response: {response}")
        return response

    def handle_data_transfer(self, charging_station_id: str, message_id: str, payload: dict):
        """Handle DataTransfer"""
        logger.info(f"Data transfer from {charging_station_id}: {payload}")
        
        response = [3, message_id, {
            "status": "Accepted"
        }]
        logger.debug(f"DataTransfer response: {response}")
        return response

    def get_status(self):
        """Get current CSMS status"""
        return {
            "charging_stations": self.charging_stations,
            "connected_stations": len(self.charging_stations),
            "active_connections": len(self.connections)
        }

async def main():
    """Main function to start CSMS server"""
    csms = SimpleCSMS()
    
    try:
        server = await websockets.serve(
            csms.handle_connection,
            '0.0.0.0',
            9000,
            subprotocols=['ocpp2.0.1']
        )
        
        logger.info("Simple CSMS Server started on ws://0.0.0.0:9000")
        logger.info("Waiting for charging stations to connect...")
        
        await server.wait_closed()
    except Exception as e:
        logger.error(f"Server error: {e}")

if __name__ == "__main__":
    asyncio.run(main()) 