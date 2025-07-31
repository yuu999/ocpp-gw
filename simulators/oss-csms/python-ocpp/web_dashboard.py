#!/usr/bin/env python3
"""
OCPP CSMS Web Dashboard
Real-time monitoring dashboard for OCPP communication
"""

import asyncio
import json
import logging
from datetime import datetime
from flask import Flask, render_template, jsonify
from flask_socketio import SocketIO, emit
import threading
import queue
import time

# Configure logging
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

class WebDashboard:
    def __init__(self, port=8081):
        self.app = Flask(__name__)
        self.app.config['SECRET_KEY'] = 'ocpp-dashboard-secret'
        self.socketio = SocketIO(self.app, cors_allowed_origins="*")
        self.port = port
        
        # Data storage
        self.connections = {}
        self.messages = []
        self.statistics = {
            'total_messages': 0,
            'active_connections': 0,
            'uptime_start': datetime.now(),
            'last_activity': None
        }
        
        # Message queue for communication with CSMS
        self.message_queue = queue.Queue()
        
        self.setup_routes()
        self.setup_socketio_events()
    
    def setup_routes(self):
        @self.app.route('/')
        def index():
            return render_template('dashboard.html')
        
        @self.app.route('/api/status')
        def get_status():
            uptime = datetime.now() - self.statistics['uptime_start']
            return jsonify({
                'status': 'running',
                'uptime': str(uptime),
                'total_messages': self.statistics['total_messages'],
                'active_connections': self.statistics['active_connections'],
                'last_activity': self.statistics['last_activity']
            })
        
        @self.app.route('/api/connections')
        def get_connections():
            return jsonify(self.connections)
        
        @self.app.route('/api/messages')
        def get_messages():
            # Return last 50 messages
            return jsonify(self.messages[-50:])
    
    def setup_socketio_events(self):
        @self.socketio.on('connect')
        def handle_connect():
            logger.info('Dashboard client connected')
            emit('status_update', self.get_current_status())
        
        @self.socketio.on('disconnect')
        def handle_disconnect():
            logger.info('Dashboard client disconnected')
    
    def get_current_status(self):
        uptime = datetime.now() - self.statistics['uptime_start']
        return {
            'uptime': str(uptime),
            'total_messages': self.statistics['total_messages'],
            'active_connections': self.statistics['active_connections'],
            'connections': self.connections,
            'recent_messages': self.messages[-10:]
        }
    
    def add_connection(self, station_id, connection_info):
        """Add or update connection information"""
        self.connections[station_id] = {
            **connection_info,
            'last_updated': datetime.now().isoformat()
        }
        self.statistics['active_connections'] = len(self.connections)
        self.statistics['last_activity'] = datetime.now().isoformat()
        
        # Broadcast to connected dashboard clients
        self.socketio.emit('connection_update', {
            'station_id': station_id,
            'action': 'connected',
            'info': self.connections[station_id]
        })
    
    def remove_connection(self, station_id):
        """Remove connection"""
        if station_id in self.connections:
            del self.connections[station_id]
            self.statistics['active_connections'] = len(self.connections)
            self.statistics['last_activity'] = datetime.now().isoformat()
            
            # Broadcast to connected dashboard clients
            self.socketio.emit('connection_update', {
                'station_id': station_id,
                'action': 'disconnected'
            })
    
    def add_message(self, station_id, direction, message_type, payload, timestamp=None):
        """Add message to the log"""
        if timestamp is None:
            timestamp = datetime.now()
        
        message = {
            'id': len(self.messages) + 1,
            'timestamp': timestamp.isoformat(),
            'station_id': station_id,
            'direction': direction,  # 'sent' or 'received'
            'message_type': message_type,
            'payload': payload
        }
        
        self.messages.append(message)
        self.statistics['total_messages'] += 1
        self.statistics['last_activity'] = timestamp.isoformat()
        
        # Keep only last 1000 messages
        if len(self.messages) > 1000:
            self.messages = self.messages[-1000:]
        
        # Broadcast to connected dashboard clients
        self.socketio.emit('new_message', message)
    
    def run(self):
        """Start the web dashboard"""
        logger.info(f"Starting OCPP Web Dashboard on port {self.port}")
        self.socketio.run(self.app, host='0.0.0.0', port=self.port, debug=False)

# Global dashboard instance
dashboard = None

def start_dashboard(port=8081):
    """Start dashboard in a separate thread"""
    global dashboard
    dashboard = WebDashboard(port)
    
    def run_dashboard():
        dashboard.run()
    
    dashboard_thread = threading.Thread(target=run_dashboard, daemon=True)
    dashboard_thread.start()
    return dashboard

if __name__ == "__main__":
    dashboard = WebDashboard()
    dashboard.run()