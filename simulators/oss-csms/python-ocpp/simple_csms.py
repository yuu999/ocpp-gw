#!/usr/bin/env python3
"""
Simple OCPP 2.0.1 CSMS Server with Enhanced Visualization
"""

import asyncio
import json
import logging
import websockets
from datetime import datetime
from typing import Dict, Set
import colorama
from colorama import Fore, Back, Style
import threading
import time
from web_dashboard import start_dashboard

# Initialize colorama for colored output
colorama.init()

# Custom formatter for colored logs
class ColoredFormatter(logging.Formatter):
    def format(self, record):
        if record.levelname == 'INFO':
            record.levelname = f"{Fore.GREEN}INFO{Style.RESET_ALL}"
        elif record.levelname == 'DEBUG':
            record.levelname = f"{Fore.CYAN}DEBUG{Style.RESET_ALL}"
        elif record.levelname == 'ERROR':
            record.levelname = f"{Fore.RED}ERROR{Style.RESET_ALL}"
        elif record.levelname == 'WARNING':
            record.levelname = f"{Fore.YELLOW}WARNING{Style.RESET_ALL}"
        return super().format(record)

# Configure logging with colors
handler = logging.StreamHandler()
handler.setFormatter(ColoredFormatter())
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
    handlers=[handler]
)
logger = logging.getLogger(__name__)

class SimpleCSMS:
    def __init__(self, enable_web_dashboard=True):
        self.charging_stations: Dict[str, Dict] = {}
        self.connections: Set[websockets.WebSocketServerProtocol] = set()
        self.message_count = 0
        self.start_time = datetime.now()
        self.last_activity = {}
        self.web_dashboard = None
        
        # Start web dashboard if enabled
        if enable_web_dashboard:
            try:
                self.web_dashboard = start_dashboard(port=8081)
                print(f"{Fore.GREEN}🌐 Web Dashboard started at http://localhost:8081{Style.RESET_ALL}")
            except Exception as e:
                print(f"{Fore.YELLOW}⚠️  Web Dashboard failed to start: {e}{Style.RESET_ALL}")
        
    def print_header(self):
        """Print colorful header"""
        print(f"\n{Back.BLUE}{Fore.WHITE}{'='*80}{Style.RESET_ALL}")
        print(f"{Back.BLUE}{Fore.WHITE}  OCPP 2.0.1 CSMS Simulator - Communication Monitor  {Style.RESET_ALL}")
        print(f"{Back.BLUE}{Fore.WHITE}{'='*80}{Style.RESET_ALL}")
        print(f"{Fore.CYAN}🚀 Server Status: {Fore.GREEN}RUNNING{Style.RESET_ALL}")
        print(f"{Fore.CYAN}📡 WebSocket Port: {Fore.YELLOW}9000{Style.RESET_ALL}")
        print(f"{Fore.CYAN}🔗 Protocol: {Fore.YELLOW}OCPP 2.0.1{Style.RESET_ALL}")
        print(f"{Fore.CYAN}⏰ Started: {Fore.YELLOW}{self.start_time.strftime('%Y-%m-%d %H:%M:%S')}{Style.RESET_ALL}")
        print(f"{Back.BLUE}{Fore.WHITE}{'='*80}{Style.RESET_ALL}\n")
    
    def print_connection_status(self, charging_station_id: str, action: str):
        """Print connection status with colors"""
        timestamp = datetime.now().strftime('%H:%M:%S')
        if action == "CONNECTED":
            print(f"{Fore.GREEN}[{timestamp}] 🔌 {charging_station_id} CONNECTED{Style.RESET_ALL}")
        elif action == "DISCONNECTED":
            print(f"{Fore.RED}[{timestamp}] 🔌 {charging_station_id} DISCONNECTED{Style.RESET_ALL}")
    
    def print_message_exchange(self, charging_station_id: str, direction: str, message_type: str, payload: dict):
        """Print detailed message exchange"""
        timestamp = datetime.now().strftime('%H:%M:%S')
        self.message_count += 1
        
        # Direction icons and colors
        if direction == "RECEIVED":
            icon = "📥"
            color = Fore.CYAN
        else:  # SENT
            icon = "📤"
            color = Fore.MAGENTA
            
        print(f"\n{color}[{timestamp}] {icon} {direction} from/to {charging_station_id}{Style.RESET_ALL}")
        print(f"{color}└─ Message Type: {Fore.YELLOW}{message_type}{Style.RESET_ALL}")
        
        # Pretty print payload
        if payload:
            print(f"{color}└─ Payload:{Style.RESET_ALL}")
            for key, value in payload.items():
                if isinstance(value, dict):
                    print(f"{color}   ├─ {key}:{Style.RESET_ALL}")
                    for k, v in value.items():
                        print(f"{color}   │  └─ {k}: {Fore.WHITE}{v}{Style.RESET_ALL}")
                else:
                    print(f"{color}   └─ {key}: {Fore.WHITE}{value}{Style.RESET_ALL}")
        
        print(f"{Fore.GREEN}💬 Total Messages: {self.message_count}{Style.RESET_ALL}")
        print("-" * 60)
    
    def print_statistics(self):
        """Print current statistics"""
        uptime = datetime.now() - self.start_time
        print(f"\n{Back.GREEN}{Fore.BLACK} LIVE STATISTICS {Style.RESET_ALL}")
        print(f"⏱️  Uptime: {uptime}")
        print(f"🔗 Active Connections: {len(self.connections)}")
        print(f"💬 Total Messages: {self.message_count}")
        print(f"📊 Connected Stations: {len(self.charging_stations)}")
        if self.charging_stations:
            for station_id, info in self.charging_stations.items():
                last_seen = info.get('last_seen', 'Never')
                print(f"   └─ {station_id}: {last_seen}")
        print()

    async def handle_connection(self, websocket, path=None):
        """Handle WebSocket connection"""
        # Handle both old and new websockets library signatures
        if path is None:
            path = getattr(websocket, 'path', '/CP001')
        charging_station_id = path.split('/')[-1] if path else 'unknown'
        
        # Visual connection notification
        self.print_connection_status(charging_station_id, "CONNECTED")
        
        # Update charging station info
        connection_info = {
            'connected_at': datetime.now().strftime('%Y-%m-%d %H:%M:%S'),
            'last_seen': datetime.now().strftime('%Y-%m-%d %H:%M:%S'),
            'message_count': 0
        }
        self.charging_stations[charging_station_id] = connection_info
        
        # Update web dashboard
        if self.web_dashboard:
            self.web_dashboard.add_connection(charging_station_id, connection_info)
        
        self.connections.add(websocket)
        
        try:
            async for message in websocket:
                try:
                    data = json.loads(message)
                    
                    # Extract message info for visualization
                    message_type = data[2] if len(data) > 2 else "Unknown"
                    payload = data[3] if len(data) > 3 else {}
                    
                    # Visual display of received message
                    self.print_message_exchange(charging_station_id, "RECEIVED", message_type, payload)
                    
                    # Update web dashboard with received message
                    if self.web_dashboard:
                        self.web_dashboard.add_message(charging_station_id, "received", message_type, payload)
                    
                    # Update station info
                    if charging_station_id in self.charging_stations:
                        self.charging_stations[charging_station_id]['last_seen'] = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
                        self.charging_stations[charging_station_id]['message_count'] += 1
                    
                    response = await self.handle_message(charging_station_id, data)
                    if response:
                        # Visual display of sent response
                        response_type = "Response"
                        response_payload = response[2] if len(response) > 2 else {}
                        self.print_message_exchange(charging_station_id, "SENT", response_type, response_payload)
                        
                        # Update web dashboard with sent message
                        if self.web_dashboard:
                            self.web_dashboard.add_message(charging_station_id, "sent", response_type, response_payload)
                        
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
            self.print_connection_status(charging_station_id, "DISCONNECTED")
        except Exception as e:
            logger.error(f"Connection error: {e}")
        finally:
            self.connections.discard(websocket)
            if charging_station_id in self.charging_stations:
                del self.charging_stations[charging_station_id]
            
            # Update web dashboard
            if self.web_dashboard:
                self.web_dashboard.remove_connection(charging_station_id)
            
            self.print_statistics()

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

async def statistics_monitor(csms):
    """Background task to show periodic statistics"""
    while True:
        await asyncio.sleep(30)  # Show stats every 30 seconds
        if csms.connections:
            csms.print_statistics()

async def main():
    """Main function to start CSMS server with enhanced visualization"""
    csms = SimpleCSMS()
    
    # Print the colorful header
    csms.print_header()
    
    try:
        server = await websockets.serve(
            csms.handle_connection,
            '0.0.0.0',
            9000,
            subprotocols=['ocpp2.0.1']
        )
        
        print(f"{Fore.GREEN}✅ CSMS Server successfully started!{Style.RESET_ALL}")
        print(f"{Fore.CYAN}🌐 OCPP WebSocket: ws://0.0.0.0:9000{Style.RESET_ALL}")
        print(f"{Fore.MAGENTA}📊 Web Dashboard: http://localhost:8081{Style.RESET_ALL}")
        print(f"{Fore.YELLOW}⏳ Waiting for OCPP charging stations to connect...{Style.RESET_ALL}")
        print(f"{Fore.WHITE}{'='*60}{Style.RESET_ALL}\n")
        
        # Start background statistics monitor
        asyncio.create_task(statistics_monitor(csms))
        
        await server.wait_closed()
    except Exception as e:
        logger.error(f"Server error: {e}")

if __name__ == "__main__":
    asyncio.run(main()) 