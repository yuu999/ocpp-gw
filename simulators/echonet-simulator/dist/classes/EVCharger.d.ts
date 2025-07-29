/**
 * EV Charger Class (0x02A1) Implementation
 */
import { DeviceInstance, PropertyValue, SimulationConfig } from '../types/echonet';
export declare class EVCharger {
    private instance;
    private simulationTimer?;
    constructor(eoj: string, instance: number, name: string);
    private initializeProperties;
    getInstance(): DeviceInstance;
    getProperty(epc: number): PropertyValue | undefined;
    setProperty(epc: number, value: Buffer): void;
    private resetCumulativeElectricity;
    startSimulation(config: SimulationConfig): void;
    stopSimulation(): void;
    private updateSimulatedValues;
}
//# sourceMappingURL=EVCharger.d.ts.map