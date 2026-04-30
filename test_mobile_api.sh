#!/bin/bash

# Mobile API Test Script for VerseFinder
echo "Testing VerseFinder Mobile API..."

# Configuration
SERVER="localhost:8080"
WS_SERVER="localhost:8081"

echo "1. Testing API server health..."
curl -s "http://$SERVER/api/health" | head -c 200
echo -e "\n"

echo "2. Testing available translations..."
curl -s "http://$SERVER/api/translations" | head -c 200
echo -e "\n"

echo "3. Testing device pairing..."
PAIR_RESPONSE=$(curl -s -X POST "http://$SERVER/api/mobile/pair" \
  -H "Content-Type: application/json" \
  -d '{"device_name":"Test Mobile Device"}')

echo "Pairing response: $PAIR_RESPONSE"

# Extract session_id and pin from response (basic parsing)
SESSION_ID=$(echo $PAIR_RESPONSE | grep -o '"session_id":"[^"]*"' | cut -d'"' -f4)
PIN=$(echo $PAIR_RESPONSE | grep -o '"pin":"[^"]*"' | cut -d'"' -f4)

echo "Session ID: $SESSION_ID"
echo "PIN: $PIN"

if [ -n "$SESSION_ID" ] && [ -n "$PIN" ]; then
    echo "4. Testing PIN validation..."
    VALIDATE_RESPONSE=$(curl -s -X POST "http://$SERVER/api/mobile/pair/validate" \
      -H "Content-Type: application/json" \
      -d "{\"session_id\":\"$SESSION_ID\",\"pin\":\"$PIN\"}")
    
    echo "Validation response: $VALIDATE_RESPONSE"
    
    # Extract device_id
    DEVICE_ID=$(echo $VALIDATE_RESPONSE | grep -o '"device_id":"[^"]*"' | cut -d'"' -f4)
    echo "Device ID: $DEVICE_ID"
    
    if [ -n "$DEVICE_ID" ]; then
        echo "5. Testing auth token request..."
        TOKEN_RESPONSE=$(curl -s -X POST "http://$SERVER/api/mobile/auth" \
          -H "Content-Type: application/json" \
          -d "{\"device_id\":\"$DEVICE_ID\",\"user_id\":\"test_user\"}")
        
        echo "Token response: $TOKEN_RESPONSE"
        
        # Extract token
        TOKEN=$(echo $TOKEN_RESPONSE | grep -o '"token":"[^"]*"' | cut -d'"' -f4)
        echo "Auth Token: $TOKEN"
        
        if [ -n "$TOKEN" ]; then
            echo "6. Testing authenticated endpoints..."
            
            echo "- Testing search..."
            curl -s "http://$SERVER/api/mobile/search?q=John%203:16" \
              -H "Authorization: Bearer $TOKEN" | head -c 200
            echo -e "\n"
            
            echo "- Testing presentation status..."
            curl -s "http://$SERVER/api/mobile/presentation/status" \
              -H "Authorization: Bearer $TOKEN" | head -c 200
            echo -e "\n"
            
            echo "- Testing emergency verses..."
            curl -s "http://$SERVER/api/mobile/emergency" \
              -H "Authorization: Bearer $TOKEN" | head -c 200
            echo -e "\n"
            
            echo "7. Testing WebSocket connection..."
            # Simple WebSocket test using nc (if available)
            if command -v nc &> /dev/null; then
                echo "Attempting WebSocket handshake to $WS_SERVER..."
                timeout 2s nc $WS_SERVER || echo "WebSocket server may not be running or accessible"
            else
                echo "NetCat not available for WebSocket test"
            fi
        fi
    fi
fi

echo -e "\nMobile API test complete!"
echo "To test the full mobile companion:"
echo "1. Start VerseFinder with API server enabled"
echo "2. Open mobile/index.html in a mobile browser"
echo "3. Enter server address: $SERVER"
echo "4. Follow the pairing process"