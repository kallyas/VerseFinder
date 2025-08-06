# VerseFinder Mobile Companion App

The VerseFinder Mobile Companion App allows worship leaders and pastors to remotely control VerseFinder presentations from their mobile devices.

## Features

### Remote Presentation Control
- Toggle presentation mode on/off
- Navigate between verses (previous/next)
- Blank screen for seamless transitions
- Show logo/home screen
- Emergency verse quick access

### Search and Display
- Full Bible search with keyword and reference support
- Multiple translation support
- Real-time verse display synchronization
- Quick access to favorite verses
- Emergency verse collection for unplanned moments

### Device Management
- Secure device pairing with PIN codes
- Multiple device support with role-based permissions
- Real-time connection status monitoring
- Automatic reconnection handling

### User Experience
- Progressive Web App (PWA) - works like a native app
- Responsive design optimized for mobile devices
- Dark theme for low-light church environments
- Touch-friendly controls with haptic feedback
- Offline support for cached content

## Setup Instructions

### 1. Enable API Server in VerseFinder
1. Open VerseFinder on your main computer
2. Go to Settings → Integrations
3. Enable "API Server" (this starts the HTTP API on port 8080)
4. The WebSocket server will automatically start on port 8081

### 2. Connect Mobile Device
1. Ensure your mobile device is on the same WiFi network as VerseFinder
2. Open a web browser on your mobile device (Safari on iOS, Chrome on Android)
3. Navigate to: `http://[COMPUTER-IP]:8080/mobile/index.html`
   - Replace `[COMPUTER-IP]` with your VerseFinder computer's IP address
   - Example: `http://192.168.1.100:8080/mobile/index.html`

### 3. Pair Your Device
1. Enter your computer's IP address (e.g., `192.168.1.100:8080`)
2. Enter a device name (e.g., "Pastor's iPhone")
3. Click "Connect"
4. A PIN will be displayed - enter this PIN in VerseFinder when prompted
5. Click "Validate PIN"
6. Your device is now connected!

### 4. Add to Home Screen (Optional)
- **iOS**: Tap the Share button → "Add to Home Screen"
- **Android**: Tap the menu button → "Add to Home Screen" or "Install App"

## Using the Mobile Companion

### Search Tab
- **Search Box**: Enter verse references (e.g., "John 3:16") or keywords (e.g., "love")
- **Translation Selector**: Choose between available Bible translations
- **Quick Access**: Tap frequently used verses for instant display
- **Search Results**: Tap any result to display it on the main screen

### Control Tab
- **Presentation Preview**: See what's currently displayed (if enabled in settings)
- **Toggle Presentation**: Turn presentation mode on/off
- **Blank Screen**: Hide content temporarily
- **Previous/Next**: Navigate between verses
- **Show Logo**: Display church logo or home screen
- **Emergency**: Quick access to emergency verses
- **Text Size**: Adjust presentation text size
- **Theme**: Change presentation background

### Favorites Tab
- **Your Favorites**: Verses you've bookmarked
- **Emergency Verses**: Pre-configured verses for urgent situations
- Tap any verse to display it immediately

### Settings Tab
- **App Settings**: Customize vibration, sound, theme, font size
- **Device Info**: View connection status and permissions
- **Disconnect**: Safely disconnect from VerseFinder

## Technical Details

### Network Requirements
- VerseFinder computer and mobile device must be on same WiFi network
- Firewall should allow connections on ports 8080 (HTTP) and 8081 (WebSocket)
- No internet connection required once connected

### Security Features
- Device pairing with time-limited PIN codes (10-minute expiry)
- Encrypted WebSocket communication
- Authentication tokens with session management
- Permission-based access control

### API Endpoints

#### Device Pairing
- `POST /api/mobile/pair` - Initiate device pairing
- `POST /api/mobile/pair/validate` - Validate PIN code
- `POST /api/mobile/auth` - Get authentication token

#### Presentation Control
- `GET /api/mobile/presentation/status` - Get current presentation state
- `POST /api/mobile/presentation/toggle` - Toggle presentation mode
- `POST /api/mobile/presentation/blank` - Toggle blank screen
- `POST /api/mobile/presentation/navigate` - Navigate verses

#### Search and Content
- `GET /api/mobile/search?q=query&translation=KJV` - Search verses
- `POST /api/mobile/display` - Display specific verse
- `GET /api/mobile/translations` - List available translations

#### User Data
- `GET /api/mobile/favorites` - Get user's favorite verses
- `POST /api/mobile/favorites` - Add to favorites
- `DELETE /api/mobile/favorites` - Remove from favorites
- `GET /api/mobile/emergency` - Get emergency verses

### WebSocket Events
- `verse_changed` - Notifies when displayed verse changes
- `presentation_state_changed` - Notifies of presentation state changes
- `device_connected/disconnected` - Device status updates

## Troubleshooting

### Connection Issues
- **Can't find server**: Verify IP address and ensure both devices are on same network
- **PIN expired**: PIN codes expire after 10 minutes, restart pairing process
- **Connection lost**: App will automatically attempt to reconnect

### Performance Tips
- Keep VerseFinder computer connected to power
- Use strong WiFi signal for best performance
- Close other apps on mobile device for optimal performance

### Supported Devices
- **iOS**: Safari on iOS 12+
- **Android**: Chrome on Android 8+
- **Other**: Modern browsers with WebSocket support

## Advanced Features

### Multiple Device Support
- Multiple mobile devices can connect simultaneously
- Role-based permissions (admin, presenter, user)
- Activity logging and conflict resolution

### Voice Commands (Future)
- Voice search for verses
- Voice control of presentation

### Integration Features
- Service plan synchronization
- Calendar integration for scheduled verses
- Remote translation management

## Support

For technical support or feature requests, please visit the VerseFinder repository on GitHub.

## License

The VerseFinder Mobile Companion App is part of the VerseFinder project and is released under the same license.