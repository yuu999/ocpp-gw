# Requirements Document

## Introduction

This document outlines the requirements for an OCPP 2.0.1 Gateway Middleware that integrates existing ECHONET Lite (EL) and Modbus (RTU/TCP) compatible charging stations into an OCPP 2.0.1 network. The middleware will enable centralized management of up to 100 charging stations without requiring firmware modifications to the charging equipment. The system will be developed in C++ for Linux-based embedded systems.

## Requirements

### Requirement 1: OCPP Protocol Support

**User Story:** As a charging network operator, I want the gateway to fully implement the OCPP 2.0.1 Core Profile, so that I can integrate legacy charging equipment into my modern charging network.

#### Acceptance Criteria

1. WHEN the gateway starts up THEN the system SHALL send BootNotification to the CSMS
2. WHEN the gateway is running THEN the system SHALL send periodic Heartbeat messages to maintain connection
3. WHEN a charging station's status changes THEN the system SHALL send StatusNotification to the CSMS
4. WHEN a charging transaction occurs THEN the system SHALL handle TransactionEvent messages
5. WHEN a user attempts to charge THEN the system SHALL handle Authorize requests
6. WHEN the CSMS sends Remote Start/Stop Transaction commands THEN the system SHALL translate and forward them to the appropriate charging station
7. WHEN the CSMS sends UnlockConnector or TriggerMessage commands THEN the system SHALL process them correctly
8. WHEN a charging transaction is in progress THEN the system SHALL send MeterValues at 1-minute intervals
9. WHEN a charging transaction ends THEN the system SHALL send final MeterValues
10. WHEN the CSMS sends SetChargingProfile THEN the system SHALL apply current limits to the charging station

### Requirement 2: Protocol Translation

**User Story:** As a system integrator, I want the gateway to dynamically map OCPP variables to device-specific registers/EPCs, so that I can easily add support for new charging station models.

#### Acceptance Criteria

1. WHEN configuring the gateway THEN the system SHALL allow mapping OCPP variables to Modbus registers or EL EPCs via external definition files
2. WHEN adding a new device model THEN the system SHALL support template-based configuration where only differences need to be edited
3. WHEN translating values THEN the system SHALL support scaling and unit conversion (e.g., 0.1 kWh → Wh)
4. WHEN mapping values THEN the system SHALL support enumeration mapping (e.g., {0:Available, 1:Charging})

### Requirement 3: Device Communication

**User Story:** As a charging station operator, I want the gateway to reliably communicate with various charging stations using different protocols, so that I can ensure consistent operation across my heterogeneous equipment.

#### Acceptance Criteria

1. WHEN communicating with ECHONET Lite devices THEN the system SHALL use UDP/IPv4 with multicast discovery and unicast control
2. WHEN communicating with ECHONET Lite devices THEN the system SHALL implement retransmission and timeout management
3. WHEN communicating with Modbus RTU devices THEN the system SHALL support up to 25 devices per RS-485 bus
4. WHEN communicating with Modbus RTU devices THEN the system SHALL manage polling cycles and CRC error retransmission
5. WHEN communicating with Modbus TCP devices THEN the system SHALL use asynchronous sockets and connection pools for parallel processing

### Requirement 4: Scalability

**User Story:** As a charging network operator, I want the gateway to handle up to 100 charging stations simultaneously, so that I can efficiently manage large charging facilities.

#### Acceptance Criteria

1. WHEN deployed THEN the system SHALL maintain connections to up to 100 charging stations
2. WHEN operating at full capacity THEN the system SHALL maintain up to 100 WebSocket connections to the CSMS
3. WHEN operating at full capacity THEN the system SHALL maintain CPU usage below 60% on a 4-core ARMv8 1.6 GHz processor

### Requirement 5: Security

**User Story:** As a system administrator, I want the gateway to implement robust security measures, so that I can protect the charging infrastructure from unauthorized access.

#### Acceptance Criteria

1. WHEN connecting to the CSMS THEN the system SHALL use TLS 1.2 or higher
2. WHEN establishing TLS connections THEN the system SHALL verify server certificates
3. WHEN configured THEN the system SHALL optionally support client certificates for mutual authentication
4. WHEN storing configuration files and certificates THEN the system SHALL use encrypted storage
5. WHEN accessing the management UI THEN the system SHALL enforce RBAC and HTTPS

### Requirement 6: Logging and Monitoring

**User Story:** As a system operator, I want comprehensive logging and monitoring capabilities, so that I can troubleshoot issues and monitor system health.

#### Acceptance Criteria

1. WHEN operating THEN the system SHALL log device communications, OCPP messages, and errors with appropriate severity levels
2. WHEN logging THEN the system SHALL implement log rotation and compression
3. WHEN monitored THEN the system SHALL expose health and statistics via REST API or Prometheus exporter

### Requirement 7: Fault Tolerance

**User Story:** As a charging station operator, I want the gateway to handle network and device failures gracefully, so that I can maintain service availability during disruptions.

#### Acceptance Criteria

1. WHEN the CSMS connection is lost THEN the system SHALL queue offline transactions
2. WHEN the CSMS connection is restored THEN the system SHALL automatically send queued transactions
3. WHEN a device is unresponsive THEN the system SHALL retry a configurable number of times
4. WHEN a device remains unresponsive after retries THEN the system SHALL notify the CSMS with Faulted status

### Requirement 8: Updates and Maintenance

**User Story:** As a system administrator, I want simple update and maintenance procedures, so that I can keep the gateway software current with minimal downtime.

#### Acceptance Criteria

1. WHEN updates are available THEN the system SHALL support Debian-based package (.deb) or OTA update scripts
2. WHEN configuration changes are made THEN the system SHALL apply them without requiring service restart (e.g., via HUP signal)
3. WHEN deployed THEN the system SHALL run on Debian/Ubuntu 22.04 or Yocto-based distributions

### Requirement 9: Internationalization

**User Story:** As a global solution provider, I want the gateway to support multiple languages, so that operators worldwide can use it in their preferred language.

#### Acceptance Criteria

1. WHEN configured THEN the system SHALL support switching between Japanese and English for logs and UI
2. WHEN installed THEN the system SHALL default to English with Japanese resources included