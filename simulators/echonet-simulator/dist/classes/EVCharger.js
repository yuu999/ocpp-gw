"use strict";
/**
 * EV Charger Class (0x02A1) Implementation
 */
Object.defineProperty(exports, "__esModule", { value: true });
exports.EVCharger = void 0;
class EVCharger {
    constructor(eoj, instance, name) {
        this.instance = {
            eoj,
            instance,
            name,
            properties: new Map()
        };
        this.initializeProperties();
    }
    initializeProperties() {
        // 基本プロパティの初期化
        const properties = [
            { epc: 0x80, name: 'Operating status', value: Buffer.from([0x30]) }, // 運転状態
            { epc: 0x81, name: 'Installation location', value: Buffer.from([0x00]) }, // 設置場所
            { epc: 0x82, name: 'Standard version information', value: Buffer.from([0x61]) }, // 規格Version情報
            { epc: 0x83, name: 'Identification number', value: Buffer.from([0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00]) }, // 識別番号
            { epc: 0x84, name: 'Measured instantaneous amount of electricity', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 瞬時電力計測値
            { epc: 0x85, name: 'Measured cumulative amount of electricity', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 積算電力量計測値
            { epc: 0x86, name: 'Manufacturer\'s fault code', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // メーカ異常コード
            { epc: 0x87, name: 'Current limit setting', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 電流制限設定
            { epc: 0x88, name: 'Fault status', value: Buffer.from([0x42]) }, // 異常状態
            { epc: 0x89, name: 'Fault description', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 異常内容
            { epc: 0x8A, name: 'Manufacturer\'s fault code', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // メーカ異常コード
            { epc: 0x8B, name: 'Fault status', value: Buffer.from([0x42]) }, // 異常状態
            { epc: 0x8C, name: 'Fault description', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 異常内容
            { epc: 0x8D, name: 'Manufacturer\'s fault code', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // メーカ異常コード
            { epc: 0x8E, name: 'Fault status', value: Buffer.from([0x42]) }, // 異常状態
            { epc: 0x8F, name: 'Fault description', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 異常内容
            { epc: 0x90, name: 'Manufacturer\'s fault code', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // メーカ異常コード
            { epc: 0x91, name: 'Fault status', value: Buffer.from([0x42]) }, // 異常状態
            { epc: 0x92, name: 'Fault description', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 異常内容
            { epc: 0x93, name: 'Manufacturer\'s fault code', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // メーカ異常コード
            { epc: 0x94, name: 'Fault status', value: Buffer.from([0x42]) }, // 異常状態
            { epc: 0x95, name: 'Fault description', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 異常内容
            { epc: 0x96, name: 'Manufacturer\'s fault code', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // メーカ異常コード
            { epc: 0x97, name: 'Current time setting', value: Buffer.from([0x00, 0x00]) }, // 現在時刻設定
            { epc: 0x98, name: 'Current date setting', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 現在年月日設定
            { epc: 0x99, name: 'Power limit setting', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 電力制限設定
            { epc: 0x9A, name: 'Power limit setting', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 電力制限設定
            { epc: 0x9B, name: 'Power limit setting', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 電力制限設定
            { epc: 0x9C, name: 'Power limit setting', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 電力制限設定
            { epc: 0x9D, name: 'Power limit setting', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 電力制限設定
            { epc: 0x9E, name: 'Power limit setting', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 電力制限設定
            { epc: 0x9F, name: 'Power limit setting', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 電力制限設定
            { epc: 0xA0, name: 'Power limit setting', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 電力制限設定
            { epc: 0xA1, name: 'Power limit setting', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 電力制限設定
            { epc: 0xA2, name: 'Power limit setting', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 電力制限設定
            { epc: 0xA3, name: 'Power limit setting', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 電力制限設定
            { epc: 0xA4, name: 'Power limit setting', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 電力制限設定
            { epc: 0xA5, name: 'Power limit setting', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 電力制限設定
            { epc: 0xA6, name: 'Power limit setting', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 電力制限設定
            { epc: 0xA7, name: 'Power limit setting', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 電力制限設定
            { epc: 0xA8, name: 'Power limit setting', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 電力制限設定
            { epc: 0xA9, name: 'Power limit setting', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 電力制限設定
            { epc: 0xAA, name: 'Power limit setting', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 電力制限設定
            { epc: 0xAB, name: 'Power limit setting', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 電力制限設定
            { epc: 0xAC, name: 'Power limit setting', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 電力制限設定
            { epc: 0xAD, name: 'Power limit setting', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 電力制限設定
            { epc: 0xAE, name: 'Power limit setting', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 電力制限設定
            { epc: 0xAF, name: 'Power limit setting', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 電力制限設定
            { epc: 0xB0, name: 'Power limit setting', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 電力制限設定
            { epc: 0xB1, name: 'Power limit setting', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 電力制限設定
            { epc: 0xB2, name: 'Power limit setting', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 電力制限設定
            { epc: 0xB3, name: 'Power limit setting', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 電力制限設定
            { epc: 0xB4, name: 'Power limit setting', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 電力制限設定
            { epc: 0xB5, name: 'Power limit setting', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 電力制限設定
            { epc: 0xB6, name: 'Power limit setting', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 電力制限設定
            { epc: 0xB7, name: 'Power limit setting', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 電力制限設定
            { epc: 0xB8, name: 'Power limit setting', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 電力制限設定
            { epc: 0xB9, name: 'Power limit setting', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 電力制限設定
            { epc: 0xBA, name: 'Power limit setting', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 電力制限設定
            { epc: 0xBB, name: 'Power limit setting', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 電力制限設定
            { epc: 0xBC, name: 'Power limit setting', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 電力制限設定
            { epc: 0xBD, name: 'Power limit setting', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 電力制限設定
            { epc: 0xBE, name: 'Power limit setting', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 電力制限設定
            { epc: 0xBF, name: 'Power limit setting', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 電力制限設定
            { epc: 0xC0, name: 'Power limit setting', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 電力制限設定
            { epc: 0xC1, name: 'Power limit setting', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 電力制限設定
            { epc: 0xC2, name: 'Power limit setting', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 電力制限設定
            { epc: 0xC3, name: 'Power limit setting', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 電力制限設定
            { epc: 0xC4, name: 'Power limit setting', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 電力制限設定
            { epc: 0xC5, name: 'Power limit setting', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 電力制限設定
            { epc: 0xC6, name: 'Power limit setting', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 電力制限設定
            { epc: 0xC7, name: 'Power limit setting', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 電力制限設定
            { epc: 0xC8, name: 'Power limit setting', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 電力制限設定
            { epc: 0xC9, name: 'Power limit setting', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 電力制限設定
            { epc: 0xCA, name: 'Power limit setting', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 電力制限設定
            { epc: 0xCB, name: 'Power limit setting', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 電力制限設定
            { epc: 0xCC, name: 'Power limit setting', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 電力制限設定
            { epc: 0xCD, name: 'Power limit setting', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 電力制限設定
            { epc: 0xCE, name: 'Power limit setting', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 電力制限設定
            { epc: 0xCF, name: 'Power limit setting', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 電力制限設定
            { epc: 0xD0, name: 'Power limit setting', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 電力制限設定
            { epc: 0xD1, name: 'Power limit setting', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 電力制限設定
            { epc: 0xD2, name: 'Power limit setting', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 電力制限設定
            { epc: 0xD3, name: 'Power limit setting', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 電力制限設定
            { epc: 0xD4, name: 'Power limit setting', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 電力制限設定
            { epc: 0xD5, name: 'Power limit setting', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 電力制限設定
            { epc: 0xD6, name: 'Power limit setting', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 電力制限設定
            { epc: 0xD7, name: 'Power limit setting', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 電力制限設定
            { epc: 0xD8, name: 'Measured cumulative amount of electricity', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 積算充電電力量計測値
            { epc: 0xD9, name: 'Cumulative amount of electricity reset setting', value: Buffer.from([0x00]) }, // 積算充電電力量リセット設定
            { epc: 0xDA, name: 'Power limit setting', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 電力制限設定
            { epc: 0xDB, name: 'Power limit setting', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 電力制限設定
            { epc: 0xDC, name: 'Power limit setting', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 電力制限設定
            { epc: 0xDD, name: 'Power limit setting', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 電力制限設定
            { epc: 0xDE, name: 'Power limit setting', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 電力制限設定
            { epc: 0xDF, name: 'Power limit setting', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 電力制限設定
            { epc: 0xE0, name: 'Power limit setting', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 電力制限設定
            { epc: 0xE1, name: 'Power limit setting', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 電力制限設定
            { epc: 0xE2, name: 'Power limit setting', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 電力制限設定
            { epc: 0xE3, name: 'Power limit setting', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 電力制限設定
            { epc: 0xE4, name: 'Power limit setting', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 電力制限設定
            { epc: 0xE5, name: 'Power limit setting', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 電力制限設定
            { epc: 0xE6, name: 'Vehicle ID', value: Buffer.from([0x00]) }, // 車両ID
            { epc: 0xE7, name: 'Power limit setting', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 電力制限設定
            { epc: 0xE8, name: 'Power limit setting', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 電力制限設定
            { epc: 0xE9, name: 'Power limit setting', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 電力制限設定
            { epc: 0xEA, name: 'Power limit setting', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 電力制限設定
            { epc: 0xEB, name: 'Power limit setting', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 電力制限設定
            { epc: 0xEC, name: 'Power limit setting', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 電力制限設定
            { epc: 0xED, name: 'Power limit setting', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 電力制限設定
            { epc: 0xEE, name: 'Power limit setting', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 電力制限設定
            { epc: 0xEF, name: 'Power limit setting', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 電力制限設定
            { epc: 0xF0, name: 'Power limit setting', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 電力制限設定
            { epc: 0xF1, name: 'Power limit setting', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 電力制限設定
            { epc: 0xF2, name: 'Power limit setting', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 電力制限設定
            { epc: 0xF3, name: 'Power limit setting', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 電力制限設定
            { epc: 0xF4, name: 'Power limit setting', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 電力制限設定
            { epc: 0xF5, name: 'Power limit setting', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 電力制限設定
            { epc: 0xF6, name: 'Power limit setting', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 電力制限設定
            { epc: 0xF7, name: 'Power limit setting', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 電力制限設定
            { epc: 0xF8, name: 'Power limit setting', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 電力制限設定
            { epc: 0xF9, name: 'Power limit setting', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 電力制限設定
            { epc: 0xFA, name: 'Power limit setting', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 電力制限設定
            { epc: 0xFB, name: 'Power limit setting', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 電力制限設定
            { epc: 0xFC, name: 'Power limit setting', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 電力制限設定
            { epc: 0xFD, name: 'Power limit setting', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 電力制限設定
            { epc: 0xFE, name: 'Power limit setting', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) }, // 電力制限設定
            { epc: 0xFF, name: 'Power limit setting', value: Buffer.from([0x00, 0x00, 0x00, 0x00]) } // 電力制限設定
        ];
        for (const prop of properties) {
            this.instance.properties.set(prop.epc, {
                epc: prop.epc,
                value: prop.value,
                timestamp: new Date()
            });
        }
    }
    getInstance() {
        return this.instance;
    }
    getProperty(epc) {
        return this.instance.properties.get(epc);
    }
    setProperty(epc, value) {
        const property = this.instance.properties.get(epc);
        if (property) {
            property.value = value;
            property.timestamp = new Date();
        }
        // 積算充電電力量リセット処理
        if (epc === 0xD9) {
            this.resetCumulativeElectricity();
        }
    }
    resetCumulativeElectricity() {
        // 積算充電電力量（0xD8）を0にリセット
        const resetValue = Buffer.from([0x00, 0x00, 0x00, 0x00]);
        this.setProperty(0xD8, resetValue);
    }
    startSimulation(config) {
        this.instance.simulation = config;
        if (this.simulationTimer) {
            clearInterval(this.simulationTimer);
        }
        this.simulationTimer = setInterval(() => {
            this.updateSimulatedValues();
        }, config.interval);
    }
    stopSimulation() {
        if (this.simulationTimer) {
            clearInterval(this.simulationTimer);
            this.simulationTimer = null;
        }
        delete this.instance.simulation;
    }
    updateSimulatedValues() {
        if (!this.instance.simulation)
            return;
        const { type, step = 1, min = 0, max = 1000000 } = this.instance.simulation;
        const currentValue = this.getProperty(0xD8); // 積算充電電力量
        if (currentValue) {
            let newValue = currentValue.value.readUInt32BE(0);
            switch (type) {
                case 'increment':
                    newValue += step;
                    break;
                case 'decrement':
                    newValue -= step;
                    break;
                case 'random':
                    newValue = Math.floor(Math.random() * (max - min + 1)) + min;
                    break;
                case 'sine':
                    const time = Date.now() / 1000;
                    const amplitude = this.instance.simulation.amplitude || 1000;
                    const frequency = this.instance.simulation.frequency || 1;
                    newValue = Math.floor(amplitude * Math.sin(2 * Math.PI * frequency * time) + amplitude);
                    break;
                case 'step':
                    newValue = (newValue + step) % (max + 1);
                    break;
            }
            // 範囲制限
            newValue = Math.max(min, Math.min(max, newValue));
            const newBuffer = Buffer.alloc(4);
            newBuffer.writeUInt32BE(newValue, 0);
            this.setProperty(0xD8, newBuffer);
        }
    }
}
exports.EVCharger = EVCharger;
//# sourceMappingURL=EVCharger.js.map