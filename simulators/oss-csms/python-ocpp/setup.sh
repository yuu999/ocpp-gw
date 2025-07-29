#!/bin/bash

# OCPP CSMS Simulator Setup Script

echo "Setting up OCPP CSMS Simulator..."

# Check if Python 3 is installed
if ! command -v python3 &> /dev/null; then
    echo "Python 3 is not installed. Please install Python 3.8 or later."
    exit 1
fi

# Check if pip is installed
if ! command -v pip3 &> /dev/null; then
    echo "pip3 is not installed. Please install pip3."
    exit 1
fi

# Create virtual environment
echo "Creating virtual environment..."
python3 -m venv venv

# Activate virtual environment
echo "Activating virtual environment..."
source venv/bin/activate

# Upgrade pip
echo "Upgrading pip..."
pip install --upgrade pip

# Install dependencies
echo "Installing dependencies..."
pip install -r requirements.txt

echo "Setup completed successfully!"
echo ""
echo "To start the CSMS server:"
echo "  source venv/bin/activate"
echo "  python csms_server.py"
echo ""
echo "To start the charge point emulator:"
echo "  source venv/bin/activate"
echo "  python charge_point.py"
echo ""
echo "To deactivate virtual environment:"
echo "  deactivate" 