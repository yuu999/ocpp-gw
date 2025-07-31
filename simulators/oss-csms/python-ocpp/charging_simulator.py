#!/usr/bin/env python3
"""
OCPP 2.0.1 Charging Station Simulator
充電シミュレーション機能付き充電ポイントクライアント
"""

import asyncio
import json
import logging
import uuid
import random
import time
from datetime import datetime, timedelta
import websockets
import sys
from typing import Dict, Optional

# Configure logging
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

class ChargingStationSimulator:
    def __init__(self, station_id: str = "CP001", csms_url: str = "ws://localhost:9000/CP001"):
        self.station_id = station_id
        self.csms_url = csms_url
        self.websocket = None
        self.connected = False
        self.transaction_id = None
        self.charging_state = "Available"  # Available, Preparing, Charging, Finishing, Reserved, Unavailable
        self.connector_id = 1
        self.meter_values_list = []
        self.start_time = None
        self.charging_power = 0  # kW
        self.max_power = 22  # kW
        self.energy_consumed = 0  # kWh
        self.voltage = 230  # V
        self.current = 0  # A
        self.frequency = 50  # Hz
        
    def call_msg(self, action: str, payload: dict) -> list:
        """Create OCPP CALL message"""
        message_id = str(uuid.uuid4())
        return [2, message_id, action, payload]
    
    async def send_message_with_retry(self, message, max_retries=3):
        """Send message with retry logic"""
        for attempt in range(max_retries):
            try:
                await self.websocket.send(json.dumps(message))
                return True
            except Exception as e:
                logger.error(f"Send attempt {attempt + 1} failed: {e}")
                if attempt == max_retries - 1:
                    raise
                await asyncio.sleep(1)
        return False
    
    async def boot_notification(self):
        """Send BootNotification"""
        payload = {
            "chargingStation": {
                "serialNumber": self.station_id,
                "model": "Charging Station Simulator",
                "vendorName": "OCPP Simulator"
            },
            "reason": "PowerUp"
        }
        message = self.call_msg("BootNotification", payload)
        logger.info(f"Sending BootNotification: {message}")
        await self.send_message_with_retry(message)
    
    async def status_notification(self, status: str, error_code: str = None):
        """Send StatusNotification"""
        payload = {
            "connectorId": self.connector_id,
            "errorCode": error_code,
            "status": status
        }
        message = self.call_msg("StatusNotification", payload)
        logger.info(f"Sending StatusNotification: {status}")
        await self.send_message_with_retry(message)
    
    async def authorize(self, id_token: str = "test_token_123"):
        """Send Authorize request"""
        payload = {
            "idToken": {
                "idToken": id_token,
                "type": "Central"
            }
        }
        message = self.call_msg("Authorize", payload)
        logger.info(f"Sending Authorize: {message}")
        await self.send_message_with_retry(message)
    
    async def transaction_event(self, event_type: str, trigger_reason: str = "Authorized"):
        """Send TransactionEvent"""
        payload = {
            "eventType": event_type,
            "timestamp": datetime.utcnow().isoformat(),
            "triggerReason": trigger_reason,
            "seqNo": 1,
            "transactionInfo": {
                "transactionId": self.transaction_id or f"tx_{int(time.time())}",
                "chargingState": self.charging_state
            }
        }
        message = self.call_msg("TransactionEvent", payload)
        logger.info(f"Sending TransactionEvent: {event_type}")
        await self.send_message_with_retry(message)
    
    async def meter_values(self):
        """Send MeterValues with simulated charging data"""
        if self.charging_state == "Charging":
            # Simulate charging progress
            self.energy_consumed += random.uniform(0.1, 0.5)  # kWh
            self.charging_power = random.uniform(5, self.max_power)  # kW
            self.current = (self.charging_power * 1000) / self.voltage  # A
            self.voltage = random.uniform(220, 240)  # V
            self.frequency = random.uniform(49.8, 50.2)  # Hz
        
        payload = {
            "connectorId": self.connector_id,
            "transactionId": self.transaction_id,
            "meterValue": [
                {
                    "timestamp": datetime.utcnow().isoformat(),
                    "sampledValue": [
                        {
                            "value": f"{self.energy_consumed:.3f}",
                            "measurand": "Energy.Active.Import.Register",
                            "unit": "kWh"
                        },
                        {
                            "value": f"{self.charging_power:.2f}",
                            "measurand": "Power.Active.Import",
                            "unit": "W"
                        },
                        {
                            "value": f"{self.voltage:.1f}",
                            "measurand": "Voltage",
                            "unit": "V"
                        },
                        {
                            "value": f"{self.current:.2f}",
                            "measurand": "Current.Import",
                            "unit": "A"
                        },
                        {
                            "value": f"{self.frequency:.1f}",
                            "measurand": "Frequency",
                            "unit": "Hz"
                        }
                    ]
                }
            ]
        }
        message = self.call_msg("MeterValues", payload)
        logger.info(f"Sending MeterValues: {self.energy_consumed:.3f} kWh, {self.charging_power:.2f} kW")
        await self.send_message_with_retry(message)
    
    async def heartbeat(self):
        """Send Heartbeat"""
        payload = {}
        message = self.call_msg("Heartbeat", payload)
        await self.send_message_with_retry(message)
    
    async def simulate_charging_session(self):
        """Simulate a complete charging session"""
        logger.info("🚗 Starting charging session simulation...")
        
        # 1. Boot notification
        await self.boot_notification()
        await asyncio.sleep(2)
        
        # 2. Status: Available -> Preparing
        await self.status_notification("Preparing")
        await asyncio.sleep(3)
        
        # 3. Authorize
        await self.authorize()
        await asyncio.sleep(2)
        
        # 4. Start transaction
        self.transaction_id = f"tx_{int(time.time())}"
        self.charging_state = "Charging"
        await self.transaction_event("Started", "Authorized")
        await asyncio.sleep(2)
        
        # 5. Status: Charging
        await self.status_notification("Charging")
        await asyncio.sleep(2)
        
        # 6. Simulate charging for 30 seconds
        logger.info("⚡ Starting charging simulation (30 seconds)...")
        self.start_time = datetime.now()
        
        for i in range(30):
            await self.meter_values()
            await asyncio.sleep(1)
            
            # Show progress
            if i % 5 == 0:
                progress = (i + 1) / 30 * 100
                logger.info(f"🔋 Charging progress: {progress:.1f}% - {self.energy_consumed:.3f} kWh")
        
        # 7. Finish transaction
        self.charging_state = "Finishing"
        await self.transaction_event("Ended", "EVDisconnected")
        await asyncio.sleep(2)
        
        # 8. Status: Available
        self.charging_state = "Available"
        await self.status_notification("Available")
        
        logger.info(f"✅ Charging session completed! Total energy: {self.energy_consumed:.3f} kWh")
    
    async def continuous_heartbeat(self):
        """Send heartbeat every 30 seconds"""
        while self.connected:
            try:
                await self.heartbeat()
                await asyncio.sleep(30)
            except Exception as e:
                logger.error(f"Heartbeat error: {e}")
                break
    
    async def run(self):
        """Main simulation loop"""
        try:
            logger.info(f"🔌 Connecting to CSMS: {self.csms_url}")
            async with websockets.connect(self.csms_url, subprotocols=["ocpp2.0.1"]) as ws:
                self.websocket = ws
                self.connected = True
                logger.info("✅ Connected to CSMS")
                
                # Start heartbeat in background
                heartbeat_task = asyncio.create_task(self.continuous_heartbeat())
                
                # Start charging simulation
                await self.simulate_charging_session()
                
                # Keep connection alive for a while
                await asyncio.sleep(10)
                
                # Cancel heartbeat
                heartbeat_task.cancel()
                
        except websockets.exceptions.ConnectionClosedError as e:
            logger.error(f"Connection closed: {e}")
            sys.exit(1)
        except Exception as e:
            logger.error(f"Unexpected error: {e}")
            import traceback
            logger.error(f"Traceback: {traceback.format_exc()}")
            sys.exit(1)

async def main():
    """Main function"""
    import argparse
    
    parser = argparse.ArgumentParser(description="OCPP Charging Station Simulator")
    parser.add_argument("--station-id", default="CP001", help="Charging station ID")
    parser.add_argument("--csms-url", default="ws://localhost:9000/CP001", help="CSMS WebSocket URL")
    parser.add_argument("--sessions", type=int, default=1, help="Number of charging sessions to simulate")
    
    args = parser.parse_args()
    
    logger.info("🚀 Starting OCPP Charging Station Simulator")
    logger.info(f"📡 Station ID: {args.station_id}")
    logger.info(f"🌐 CSMS URL: {args.csms_url}")
    logger.info(f"🔋 Sessions: {args.sessions}")
    
    for session in range(args.sessions):
        if args.sessions > 1:
            logger.info(f"\n🔄 Starting session {session + 1}/{args.sessions}")
        
        simulator = ChargingStationSimulator(args.station_id, args.csms_url)
        await simulator.run()
        
        if session < args.sessions - 1:
            logger.info("⏳ Waiting 5 seconds before next session...")
            await asyncio.sleep(5)

if __name__ == "__main__":
    asyncio.run(main()) 