#!/bin/bash
# Run the FastAPI application with log monitoring

# Change to the API directory
cd /home/raspberry/bondpaper_machine/api

# Activate virtual environment
source env/bin/activate

# Start the API server in the background
echo "Starting API server..."
python3 main.py > api_stdout.log 2>&1 &
API_PID=$!

# Wait a moment for the log file to be created
sleep 2

# Display the tail of the log file
echo "Monitoring serial_communication.log"
echo "Press Ctrl+C to stop monitoring (API will continue running)"
echo "---------------------------------------------------"
tail -f serial_communication.log

# Note: The API will continue running in the background after you exit this script
echo "API server is still running with PID: $API_PID"
echo "To stop the API server, run: kill $API_PID"