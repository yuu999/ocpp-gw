#!/usr/bin/env python3
"""
Simple test Charge Point for OCPP 2.0.1
"""
import asyncio
import json
import logging
import uuid
from datetime import datetime
import websockets

logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

def call_msg(action: str, payload: dict) -> list:
    return [2, str(uuid.uuid4()), action, payload]

async def test_connection():
    """Test basic connection and BootNotification"""
    try:
        async with websockets.connect("ws://localhost:9000/CP001", subprotocols=["ocpp2.0.1"]) as ws:
            logger.info("Connected to CSMS")
            
            # Send BootNotification
            boot_msg = call_msg("BootNotification", {
                "chargingStation": {
                    "serialNumber": "CP001",
                    "model": "Test CP",
                    "vendorName": "TestVendor"
                },
                "reason": "PowerUp"
            })
            
            logger.info("Sending BootNotification: %s", boot_msg)
            await ws.send(json.dumps(boot_msg))
            
            # Wait for response
            response = await asyncio.wait_for(ws.recv(), timeout=5.0)
            logger.info("Received response: %s", response)
            
            # Send Heartbeat
            heartbeat_msg = call_msg("Heartbeat", {})
            logger.info("Sending Heartbeat: %s", heartbeat_msg)
            await ws.send(json.dumps(heartbeat_msg))
            
            response = await asyncio.wait_for(ws.recv(), timeout=5.0)
            logger.info("Received heartbeat response: %s", response)
            
            logger.info("Test completed successfully!")
            
    except Exception as e:
        logger.error("Test failed: %s", e)

if __name__ == "__main__":
    asyncio.run(test_connection()) 