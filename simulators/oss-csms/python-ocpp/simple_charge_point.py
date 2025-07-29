#!/usr/bin/env python3
"""
Simple OCPP 2.0.1 Charge Point Emulator
Sends BootNotification, Heartbeat, and MeterValues to CSMS.
"""
import asyncio
import json
import logging
import uuid
from datetime import datetime
import random
import websockets
import sys

logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

CSMS_URL = "ws://localhost:9000/CP001"

# Utility to build OCPP CALL message

def call_msg(action: str, payload: dict) -> list:
    return [2, str(uuid.uuid4()), action, payload]

async def send_message_with_retry(ws, message, max_retries=3):
    """Send message with retry logic"""
    for attempt in range(max_retries):
        try:
            await ws.send(json.dumps(message))
            response = await asyncio.wait_for(ws.recv(), timeout=10.0)
            logger.info("Message sent successfully, response: %s", response)
            return response
        except asyncio.TimeoutError:
            logger.warning("Timeout on attempt %d", attempt + 1)
            if attempt == max_retries - 1:
                raise
        except websockets.exceptions.ConnectionClosedError as e:
            logger.error("Connection closed: %s", e)
            raise
        except Exception as e:
            logger.error("Error on attempt %d: %s", attempt + 1, e)
            if attempt == max_retries - 1:
                raise
        await asyncio.sleep(1)

async def main():
    try:
        async with websockets.connect(CSMS_URL, subprotocols=["ocpp2.0.1"]) as ws:
            logger.info("Connected to CSMS at %s", CSMS_URL)

            # BootNotification
            boot_payload = {
                "chargingStation": {
                    "serialNumber": "CP001",
                    "model": "Test CP",
                    "vendorName": "TestVendor"
                },
                "reason": "PowerUp"
            }
            await send_message_with_retry(ws, call_msg("BootNotification", boot_payload))
            logger.info("BootNotification completed")

            # Initial status notification
            status_payload = {
                "timestamp": datetime.utcnow().isoformat(),
                "connectorStatus": "Available",
                "evseId": 1,
                "connectorId": 1
            }
            await send_message_with_retry(ws, call_msg("StatusNotification", status_payload))
            logger.info("StatusNotification completed")

            # Simple test - send a few messages and exit
            for i in range(3):
                await asyncio.sleep(5)
                
                # Heartbeat
                await send_message_with_retry(ws, call_msg("Heartbeat", {}))
                logger.info("Heartbeat %d completed", i + 1)
                
                # MeterValues
                energy = round(random.uniform(0.1, 0.5) * (i + 1), 3)
                meter_payload = {
                    "evseId": 1,
                    "meterValue": [{
                        "timestamp": datetime.utcnow().isoformat(),
                        "sampledValue": [{
                            "value": energy,
                            "measurand": "Energy.Active.Import.Register",
                            "unitOfMeasure": {"unit": "kWh"}
                        }]
                    }]
                }
                await send_message_with_retry(ws, call_msg("MeterValues", meter_payload))
                logger.info("MeterValues %d completed (energy: %s kWh)", i + 1, energy)

            logger.info("Test completed successfully!")
            
    except websockets.exceptions.ConnectionClosedError as e:
        logger.error("Connection closed: %s", e)
        sys.exit(1)
    except Exception as e:
        logger.error("Unexpected error: %s", e)
        sys.exit(1)

if __name__ == "__main__":
    asyncio.run(main()) 