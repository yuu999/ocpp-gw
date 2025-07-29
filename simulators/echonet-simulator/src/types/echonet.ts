/**
 * ECHONET Lite Protocol Types
 */

export interface EchonetFrame {
  ehd1: number;  // ECHONET Lite Header 1
  ehd2: number;  // ECHONET Lite Header 2
  tid: number;   // Transaction ID
  seoj: string;  // Source EOJ
  deoj: string;  // Destination EOJ
  esv: number;   // ECHONET Lite Service
  opc: number;   // Number of OPC
  properties: EchonetProperty[];
}

export interface EchonetProperty {
  epc: number;   // ECHONET Property Code
  pdc: number;   // Property Data Counter
  edt: Buffer;   // Property Data
}

export interface EOJ {
  classGroupCode: number;
  classCode: number;
  instanceCode: number;
}

export interface DeviceClass {
  eoj: string;
  name: string;
  properties: DeviceProperty[];
}

export interface DeviceProperty {
  epc: number;
  name: string;
  accessRule: 'get' | 'set' | 'inf' | 'getset' | 'getinf' | 'setinf' | 'getsetinf';
  dataType: 'unsigned char' | 'unsigned short' | 'unsigned long' | 'signed long' | 'string' | 'array';
  size: number;
  unit?: string;
  description?: string;
}

export interface DeviceInstance {
  eoj: string;
  instance: number;
  name: string;
  properties: Map<number, PropertyValue>;
  simulation?: SimulationConfig;
}

export interface PropertyValue {
  epc: number;
  value: Buffer;
  timestamp: Date;
}

export interface SimulationConfig {
  type: 'increment' | 'decrement' | 'random' | 'sine' | 'step';
  interval: number;  // milliseconds
  step?: number;
  min?: number;
  max?: number;
  amplitude?: number;
  frequency?: number;
}

export interface DeviceConfig {
  eoj: string;
  instance: number;
  name: string;
  properties: PropertyConfig[];
}

export interface PropertyConfig {
  epc: number;
  initialValue: number | string;
  simulation?: SimulationConfig;
  writable?: boolean;
}

export interface SimulatorConfig {
  port: number;
  host: string;
  devices: DeviceConfig[];
  logging: {
    level: 'error' | 'warn' | 'info' | 'debug';
    file?: string;
    console: boolean;
  };
  api: {
    port: number;
    host: string;
  };
} 