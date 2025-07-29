#!/bin/bash

# EVerest/ocpp-csms Setup Script

echo "Setting up EVerest/ocpp-csms..."

# Check if git is installed
if ! command -v git &> /dev/null; then
    echo "git is not installed. Please install git."
    exit 1
fi

# Check if Python 3 is installed
if ! command -v python3 &> /dev/null; then
    echo "Python 3 is not installed. Please install Python 3.8 or later."
    exit 1
fi

# Clone EVerest/ocpp-csms repository
echo "Cloning EVerest/ocpp-csms repository..."
if [ ! -d "ocpp-csms" ]; then
    git clone https://github.com/EVerest/ocpp-csms.git
else
    echo "Repository already exists. Updating..."
    cd ocpp-csms
    git pull
    cd ..
fi

# Create virtual environment
echo "Creating virtual environment..."
python3 -m venv venv

# Activate virtual environment
echo "Activating virtual environment..."
source venv/bin/activate

# Install dependencies
echo "Installing dependencies..."
cd ocpp-csms
pip install -r requirements.txt
cd ..

echo "Setup completed successfully!"
echo ""
echo "To start the EVerest CSMS:"
echo "  source venv/bin/activate"
echo "  cd ocpp-csms"
echo "  python main.py"
echo ""
echo "To deactivate virtual environment:"
echo "  deactivate" 