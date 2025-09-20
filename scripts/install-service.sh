#!/bin/bash

# RPi IAQ Monitor - System Service Installation Script
# This script installs the air quality monitor as a systemd service

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
SERVICE_NAME="iaq-monitor"
INSTALL_DIR="/opt/rpi-iaq-monitor"
SERVICE_USER="pi"

print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

check_root() {
    if [[ $EUID -ne 0 ]]; then
        print_error "This script must be run as root (use sudo)"
        exit 1
    fi
}

check_binary() {
    if [[ ! -f "./build/air-quality-monitor" ]]; then
        print_error "Binary not found. Please build the project first:"
        echo "  mkdir -p build && cd build"
        echo "  cmake .. && make"
        exit 1
    fi
}

create_install_directory() {
    print_status "Creating installation directory: $INSTALL_DIR"
    mkdir -p "$INSTALL_DIR"
    chown "$SERVICE_USER:$SERVICE_USER" "$INSTALL_DIR"
}

install_binary() {
    print_status "Installing binary and configuration files"
    
    # Copy binary
    cp ./build/air-quality-monitor "$INSTALL_DIR/"
    chmod +x "$INSTALL_DIR/air-quality-monitor"
    
    # Copy or create default config
    if [[ -f "./config.yaml" ]]; then
        cp ./config.yaml "$INSTALL_DIR/"
        print_status "Copied existing config.yaml"
    elif [[ -f "./example-config.yaml" ]]; then
        cp ./example-config.yaml "$INSTALL_DIR/config.yaml"
        print_status "Copied example-config.yaml as config.yaml"
    else
        print_warning "No config file found. Service will create default config.yaml on first run."
    fi
    
    # Create saved_state directory
    mkdir -p "$INSTALL_DIR/saved_state"
    
    # Set ownership
    chown -R "$SERVICE_USER:$SERVICE_USER" "$INSTALL_DIR"
    
    print_success "Binary and configuration installed to $INSTALL_DIR"
}

install_service() {
    print_status "Installing systemd service"
    
    # Copy service file
    cp ./iaq-monitor.service /etc/systemd/system/
    
    # Reload systemd
    systemctl daemon-reload
    
    print_success "Service file installed"
}

configure_user() {
    # Add user to i2c group if exists
    if getent group i2c > /dev/null 2>&1; then
        usermod -a -G i2c "$SERVICE_USER"
        print_status "Added $SERVICE_USER to i2c group"
    fi
}

show_usage() {
    cat << 'USAGE'

Installation complete! You can now:

1. Edit the configuration file:
   sudo nano /opt/rpi-iaq-monitor/config.yaml

2. Enable and start the service:
   sudo systemctl enable iaq-monitor.service
   sudo systemctl start iaq-monitor.service

3. Check service status:
   sudo systemctl status iaq-monitor.service

4. View logs:
   sudo journalctl -u iaq-monitor.service -f

5. To update configuration without restarting:
   # Edit config file, then restart service
   sudo systemctl restart iaq-monitor.service

Configuration file supports:
- homebridge_url: Set to enable Homebridge publishing
- homebridge_publish_interval_seconds: Publishing interval
- iaq_temp_offset: Temperature calibration offset
- iaq_i2c_bus_device: I2C device path (usually /dev/i2c-1)
- iaq_saved_state_dir: BSEC state persistence directory
- iaq_saved_state_file: BSEC state filename

USAGE
}

main() {
    print_status "Installing RPi IAQ Monitor as system service"
    
    check_root
    check_binary
    create_install_directory
    install_binary
    install_service
    configure_user
    
    print_success "Installation completed successfully!"
    show_usage
}

main "$@"
