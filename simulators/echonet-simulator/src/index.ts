/**
 * ECHONET Lite Simulator Main Entry Point
 */

import { EchonetLiteServer } from './core/EchonetLiteServer';
import { EVCharger } from './classes/EVCharger';

class Simulator {
  private server: EchonetLiteServer;
  private devices: Map<string, EVCharger> = new Map();

  constructor() {
    this.server = new EchonetLiteServer(3610, '0.0.0.0');
    this.initializeDevices();
  }

  private initializeDevices(): void {
    // EV Charger (0x02A1) instance 1
    const evCharger1 = new EVCharger('0x02A1', 1, 'EV Charger 1');
    this.devices.set('0x02A1:1', evCharger1);

    // Start simulation for cumulative electricity
    evCharger1.startSimulation({
      type: 'increment',
      interval: 5000, // 5 seconds
      step: 1000,     // 1 kWh
      min: 0,
      max: 10000000
    });

    console.log('Initialized devices:');
    this.devices.forEach((device, key) => {
      console.log(`  - ${key}: ${device.getInstance().name}`);
    });
  }

  async start(): Promise<void> {
    try {
      // Start ECHONET Lite server
      await this.server.start();

      // Set up message handling
      this.server.on('message', (frame, rinfo) => {
        this.handleMessage(frame, rinfo);
      });

      console.log('ECHONET Lite Simulator started successfully');
      console.log('Listening on port 3610');
      console.log('Press Ctrl+C to stop');

    } catch (error) {
      console.error('Failed to start simulator:', error);
      process.exit(1);
    }
  }

  private handleMessage(frame: any, rinfo: any): void {
    console.log(`Received message from ${rinfo.address}:${rinfo.port}`);
    console.log(`Frame: ${JSON.stringify(frame, null, 2)}`);

    // Handle different message types
    switch (frame.esv) {
      case 0x62: // Get
        this.handleGetRequest(frame, rinfo);
        break;
      case 0x60: // Set
        this.handleSetRequest(frame, rinfo);
        break;
      case 0x61: // Get_Res
        this.handleGetResponse(frame);
        break;
      case 0x50: // Set_Res
        this.handleSetResponse(frame);
        break;
      default:
        console.log(`Unhandled ESV: 0x${frame.esv.toString(16)}`);
    }
  }

  private handleGetRequest(frame: any, rinfo: any): void {
    const targetEOJ = frame.deoj;
    const device = this.devices.get(targetEOJ);

    if (!device) {
      console.log(`Device not found: ${targetEOJ}`);
      return;
    }

    // Process each property in the request
    for (const prop of frame.properties) {
      const property = device.getProperty(prop.epc);
      if (property) {
        console.log(`Get property 0x${prop.epc.toString(16)}: ${property.value.toString('hex')}`);
        
        // Send response
        const response = {
          ehd1: 0x10,
          ehd2: 0x81,
          tid: frame.tid,
          seoj: targetEOJ,
          deoj: frame.seoj,
          esv: 0x72, // Get_Res
          opc: 1,
          properties: [{
            epc: prop.epc,
            pdc: property.value.length,
            edt: property.value
          }]
        };

        this.server.sendFrame(response, rinfo.address, rinfo.port);
      } else {
        console.log(`Property not found: 0x${prop.epc.toString(16)}`);
      }
    }
  }

  private handleSetRequest(frame: any, rinfo: any): void {
    const targetEOJ = frame.deoj;
    const device = this.devices.get(targetEOJ);

    if (!device) {
      console.log(`Device not found: ${targetEOJ}`);
      return;
    }

    // Process each property in the request
    for (const prop of frame.properties) {
      console.log(`Set property 0x${prop.epc.toString(16)}: ${prop.edt.toString('hex')}`);
      device.setProperty(prop.epc, prop.edt);
    }

    // Send response
    const response = {
      ehd1: 0x10,
      ehd2: 0x81,
      tid: frame.tid,
      seoj: targetEOJ,
      deoj: frame.seoj,
      esv: 0x71, // Set_Res
      opc: 0,
      properties: []
    };

    this.server.sendFrame(response, rinfo.address, rinfo.port);
  }

  private handleGetResponse(frame: any): void {
    console.log('Received Get_Res:', frame);
  }

  private handleSetResponse(frame: any): void {
    console.log('Received Set_Res:', frame);
  }

  async stop(): Promise<void> {
    // Stop all device simulations
    this.devices.forEach(device => {
      device.stopSimulation();
    });

    // Stop server
    await this.server.stop();
    console.log('Simulator stopped');
  }
}

// Handle graceful shutdown
process.on('SIGINT', async () => {
  console.log('\nShutting down...');
  if (simulator) {
    await simulator.stop();
  }
  process.exit(0);
});

process.on('SIGTERM', async () => {
  console.log('\nShutting down...');
  if (simulator) {
    await simulator.stop();
  }
  process.exit(0);
});

// Start simulator
const simulator = new Simulator();
simulator.start().catch(error => {
  console.error('Failed to start simulator:', error);
  process.exit(1);
}); 