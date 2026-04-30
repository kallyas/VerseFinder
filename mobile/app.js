// VerseFinder Mobile Companion App
class VerseFinderMobile {
    constructor() {
        this.serverAddress = '';
        this.websocket = null;
        this.apiToken = null;
        this.deviceId = null;
        this.sessionId = null;
        this.isConnected = false;
        this.currentTab = 'search';
        this.settings = {
            theme: 'dark',
            fontSize: 'large',
            vibration: true,
            sound: true,
            showPreview: true
        };
        
        this.init();
    }
    
    init() {
        this.loadSettings();
        this.setupEventListeners();
        this.setupServiceWorker();
        this.checkExistingConnection();
    }
    
    setupEventListeners() {
        // Pairing screen
        document.getElementById('connectBtn').addEventListener('click', () => this.initiateConnection());
        document.getElementById('validatePinBtn').addEventListener('click', () => this.validatePin());
        
        // Search
        document.getElementById('searchBtn').addEventListener('click', () => this.performSearch());
        document.getElementById('searchInput').addEventListener('keypress', (e) => {
            if (e.key === 'Enter') this.performSearch();
        });
        
        // Control buttons
        document.getElementById('togglePresentationBtn').addEventListener('click', () => this.togglePresentation());
        document.getElementById('blankScreenBtn').addEventListener('click', () => this.blankScreen());
        document.getElementById('previousVerseBtn').addEventListener('click', () => this.navigateVerse(-1));
        document.getElementById('nextVerseBtn').addEventListener('click', () => this.navigateVerse(1));
        document.getElementById('showLogoBtn').addEventListener('click', () => this.showLogo());
        document.getElementById('emergencyBtn').addEventListener('click', () => this.showEmergencyVerses());
        
        // Settings
        document.getElementById('textSize').addEventListener('input', (e) => this.adjustTextSize(e.target.value));
        document.getElementById('backgroundTheme').addEventListener('change', (e) => this.changeTheme(e.target.value));
        document.getElementById('disconnectBtn').addEventListener('click', () => this.disconnect());
        document.getElementById('resetAppBtn').addEventListener('click', () => this.resetApp());
        
        // Tabs
        document.querySelectorAll('.tab-btn').forEach(btn => {
            btn.addEventListener('click', (e) => this.switchTab(e.target.dataset.tab));
        });
        
        // Settings checkboxes
        document.getElementById('vibrationEnabled').addEventListener('change', (e) => {
            this.settings.vibration = e.target.checked;
            this.saveSettings();
        });
        
        document.getElementById('soundEnabled').addEventListener('change', (e) => {
            this.settings.sound = e.target.checked;
            this.saveSettings();
        });
        
        document.getElementById('showPreview').addEventListener('change', (e) => {
            this.settings.showPreview = e.target.checked;
            this.saveSettings();
        });
    }
    
    async setupServiceWorker() {
        if ('serviceWorker' in navigator) {
            try {
                await navigator.serviceWorker.register('sw.js');
                console.log('Service Worker registered');
            } catch (error) {
                console.log('Service Worker registration failed:', error);
            }
        }
    }
    
    checkExistingConnection() {
        const savedToken = localStorage.getItem('vf_auth_token');
        const savedServer = localStorage.getItem('vf_server_address');
        const savedDeviceId = localStorage.getItem('vf_device_id');
        
        if (savedToken && savedServer && savedDeviceId) {
            this.apiToken = savedToken;
            this.serverAddress = savedServer;
            this.deviceId = savedDeviceId;
            this.connectWebSocket();
        }
    }
    
    async initiateConnection() {
        const serverAddress = document.getElementById('serverAddress').value.trim();
        const deviceName = document.getElementById('deviceName').value.trim() || this.getDefaultDeviceName();
        
        if (!serverAddress) {
            this.showToast('Please enter server address', 'error');
            return;
        }
        
        this.serverAddress = serverAddress;
        this.showStatus('Connecting to server...', 'info');
        
        try {
            const response = await fetch(`http://${serverAddress}/api/mobile/pair`, {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify({
                    device_name: deviceName
                })
            });
            
            if (response.ok) {
                const data = await response.json();
                this.sessionId = data.session_id;
                
                // Show PIN to user
                this.showStatus(`Enter PIN: ${data.pin}`, 'success');
                document.getElementById('pinEntry').classList.remove('hidden');
                document.getElementById('pinCode').focus();
            } else {
                this.showStatus('Failed to connect to server', 'error');
            }
        } catch (error) {
            this.showStatus('Could not reach server. Check address and network.', 'error');
            console.error('Connection error:', error);
        }
    }
    
    async validatePin() {
        const pin = document.getElementById('pinCode').value.trim();
        
        if (!pin || pin.length !== 4) {
            this.showToast('Please enter a 4-digit PIN', 'error');
            return;
        }
        
        this.showStatus('Validating PIN...', 'info');
        
        try {
            const response = await fetch(`http://${this.serverAddress}/api/mobile/pair/validate`, {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify({
                    session_id: this.sessionId,
                    pin: pin
                })
            });
            
            if (response.ok) {
                const data = await response.json();
                this.deviceId = data.device_id;
                
                // Get auth token
                await this.getAuthToken();
                
                // Save connection info
                localStorage.setItem('vf_device_id', this.deviceId);
                localStorage.setItem('vf_server_address', this.serverAddress);
                
                this.showStatus('Connected successfully!', 'success');
                this.switchToControlScreen();
                this.connectWebSocket();
                
            } else {
                this.showStatus('Invalid PIN. Please try again.', 'error');
                document.getElementById('pinCode').value = '';
                document.getElementById('pinCode').focus();
            }
        } catch (error) {
            this.showStatus('Validation failed. Please try again.', 'error');
            console.error('Validation error:', error);
        }
    }
    
    async getAuthToken() {
        try {
            const response = await fetch(`http://${this.serverAddress}/api/mobile/auth`, {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify({
                    device_id: this.deviceId,
                    user_id: this.deviceId // Using device_id as user_id for simplicity
                })
            });
            
            if (response.ok) {
                const data = await response.json();
                this.apiToken = data.token;
                localStorage.setItem('vf_auth_token', this.apiToken);
            }
        } catch (error) {
            console.error('Auth token error:', error);
        }
    }
    
    connectWebSocket() {
        const wsProtocol = this.serverAddress.startsWith('https') ? 'wss' : 'ws';
        const wsAddress = this.serverAddress.replace(/^https?:\/\//, '').replace(/:\d+$/, ':8081');
        
        try {
            this.websocket = new WebSocket(`${wsProtocol}://${wsAddress}`);
            
            this.websocket.onopen = () => {
                console.log('WebSocket connected');
                this.isConnected = true;
                this.updateConnectionStatus(true);
                
                // Authenticate WebSocket connection
                this.sendWebSocketMessage('auth', {
                    token: this.apiToken
                });
                
                // Subscribe to events
                this.sendWebSocketMessage('subscribe', {
                    event: 'presentation_state_changed'
                });
                
                this.sendWebSocketMessage('subscribe', {
                    event: 'verse_changed'
                });
            };
            
            this.websocket.onmessage = (event) => {
                this.handleWebSocketMessage(JSON.parse(event.data));
            };
            
            this.websocket.onclose = () => {
                console.log('WebSocket disconnected');
                this.isConnected = false;
                this.updateConnectionStatus(false);
                
                // Attempt to reconnect after 5 seconds
                setTimeout(() => {
                    if (!this.isConnected) {
                        this.connectWebSocket();
                    }
                }, 5000);
            };
            
            this.websocket.onerror = (error) => {
                console.error('WebSocket error:', error);
                this.showToast('Connection error', 'error');
            };
            
        } catch (error) {
            console.error('WebSocket connection failed:', error);
            this.showToast('Failed to establish real-time connection', 'error');
        }
    }
    
    sendWebSocketMessage(event, data) {
        if (this.websocket && this.websocket.readyState === WebSocket.OPEN) {
            this.websocket.send(JSON.stringify({
                type: 'event',
                event: event,
                data: JSON.stringify(data)
            }));
        }
    }
    
    handleWebSocketMessage(message) {
        console.log('WebSocket message:', message);
        
        switch (message.event) {
            case 'auth_success':
                console.log('WebSocket authenticated');
                break;
                
            case 'presentation_state_changed':
                this.updatePresentationPreview(JSON.parse(message.data));
                break;
                
            case 'verse_changed':
                const verseData = JSON.parse(message.data);
                this.updateCurrentVerse(verseData.verse, verseData.reference);
                break;
                
            default:
                console.log('Unknown WebSocket event:', message.event);
        }
    }
    
    async performSearch() {
        const query = document.getElementById('searchInput').value.trim();
        const translation = document.getElementById('translationSelect').value;
        
        if (!query) {
            this.showToast('Please enter a search term', 'error');
            return;
        }
        
        try {
            const response = await fetch(`http://${this.serverAddress}/api/mobile/search?q=${encodeURIComponent(query)}&translation=${translation}`, {
                headers: {
                    'Authorization': `Bearer ${this.apiToken}`
                }
            });
            
            if (response.ok) {
                const results = await response.json();
                this.displaySearchResults(results);
            } else {
                this.showToast('Search failed', 'error');
            }
        } catch (error) {
            this.showToast('Search error', 'error');
            console.error('Search error:', error);
        }
    }
    
    displaySearchResults(results) {
        const container = document.getElementById('searchResults');
        container.innerHTML = '';
        
        if (results.length === 0) {
            container.innerHTML = '<div class="result-item">No results found</div>';
            return;
        }
        
        results.forEach(result => {
            const item = document.createElement('div');
            item.className = 'result-item';
            
            // Parse result (assuming format "Book Chapter:Verse - Text")
            const parts = result.split(' - ');
            const reference = parts[0];
            const text = parts[1] || 'Loading...';
            
            item.innerHTML = `
                <div class="result-reference">${reference}</div>
                <div class="result-text">${text}</div>
            `;
            
            item.addEventListener('click', () => {
                this.displayVerse(text, reference);
                this.hapticFeedback();
            });
            
            container.appendChild(item);
        });
    }
    
    async displayVerse(verseText, reference) {
        try {
            const response = await fetch(`http://${this.serverAddress}/api/mobile/display`, {
                method: 'POST',
                headers: {
                    'Authorization': `Bearer ${this.apiToken}`,
                    'Content-Type': 'application/json'
                },
                body: JSON.stringify({
                    verse_text: verseText,
                    reference: reference
                })
            });
            
            if (response.ok) {
                this.showToast('Verse displayed', 'success');
                this.updateCurrentVerse(verseText, reference);
            } else {
                this.showToast('Failed to display verse', 'error');
            }
        } catch (error) {
            this.showToast('Display error', 'error');
            console.error('Display error:', error);
        }
    }
    
    async togglePresentation() {
        this.sendPresentationCommand('toggle');
    }
    
    async blankScreen() {
        this.sendPresentationCommand('blank');
    }
    
    async navigateVerse(direction) {
        this.sendWebSocketMessage('presentation_command', {
            command: 'navigate',
            direction: direction
        });
        this.hapticFeedback();
    }
    
    async showLogo() {
        this.sendPresentationCommand('logo');
    }
    
    async sendPresentationCommand(command) {
        try {
            const response = await fetch(`http://${this.serverAddress}/api/mobile/presentation/${command}`, {
                method: 'POST',
                headers: {
                    'Authorization': `Bearer ${this.apiToken}`,
                    'Content-Type': 'application/json'
                }
            });
            
            if (response.ok) {
                this.hapticFeedback();
            } else {
                this.showToast('Command failed', 'error');
            }
        } catch (error) {
            this.showToast('Command error', 'error');
            console.error('Command error:', error);
        }
    }
    
    adjustTextSize(size) {
        this.sendWebSocketMessage('presentation_command', {
            command: 'text_size',
            size: parseFloat(size)
        });
    }
    
    changeTheme(theme) {
        this.sendWebSocketMessage('presentation_command', {
            command: 'theme',
            theme: theme
        });
    }
    
    showEmergencyVerses() {
        // Switch to favorites tab and highlight emergency section
        this.switchTab('favorites');
        this.loadEmergencyVerses();
    }
    
    async loadEmergencyVerses() {
        try {
            const response = await fetch(`http://${this.serverAddress}/api/mobile/emergency`, {
                headers: {
                    'Authorization': `Bearer ${this.apiToken}`
                }
            });
            
            if (response.ok) {
                const verses = await response.json();
                this.displayEmergencyVerses(verses);
            }
        } catch (error) {
            console.error('Emergency verses error:', error);
        }
    }
    
    displayEmergencyVerses(verses) {
        const container = document.getElementById('emergencyVerses');
        container.innerHTML = '';
        
        verses.forEach(verse => {
            const btn = document.createElement('button');
            btn.className = 'verse-btn';
            btn.textContent = verse;
            btn.addEventListener('click', () => {
                this.displayVerseByReference(verse);
                this.hapticFeedback();
            });
            container.appendChild(btn);
        });
    }
    
    async displayVerseByReference(reference) {
        try {
            const response = await fetch(`http://${this.serverAddress}/api/mobile/search?q=${encodeURIComponent(reference)}`, {
                headers: {
                    'Authorization': `Bearer ${this.apiToken}`
                }
            });
            
            if (response.ok) {
                const results = await response.json();
                if (results.length > 0) {
                    const parts = results[0].split(' - ');
                    this.displayVerse(parts[1] || 'Loading...', parts[0]);
                }
            }
        } catch (error) {
            console.error('Display by reference error:', error);
        }
    }
    
    switchTab(tabName) {
        // Update tab buttons
        document.querySelectorAll('.tab-btn').forEach(btn => {
            btn.classList.toggle('active', btn.dataset.tab === tabName);
        });
        
        // Update tab content
        document.querySelectorAll('.tab-content').forEach(content => {
            content.classList.toggle('active', content.id === `${tabName}Tab`);
        });
        
        this.currentTab = tabName;
        
        // Load tab-specific data
        if (tabName === 'favorites') {
            this.loadFavorites();
            this.loadEmergencyVerses();
        }
    }
    
    async loadFavorites() {
        try {
            const response = await fetch(`http://${this.serverAddress}/api/mobile/favorites`, {
                headers: {
                    'Authorization': `Bearer ${this.apiToken}`
                }
            });
            
            if (response.ok) {
                const favorites = await response.json();
                this.displayFavorites(favorites);
            }
        } catch (error) {
            console.error('Favorites error:', error);
        }
    }
    
    displayFavorites(favorites) {
        const container = document.getElementById('favoriteVerses');
        container.innerHTML = '';
        
        if (favorites.length === 0) {
            container.innerHTML = '<div class="verse-item">No favorites yet</div>';
            return;
        }
        
        favorites.forEach(verse => {
            const item = document.createElement('div');
            item.className = 'verse-item';
            item.innerHTML = `
                <span class="verse-ref">${verse}</span>
                <button class="remove-btn">❌</button>
            `;
            
            item.querySelector('.verse-ref').addEventListener('click', () => {
                this.displayVerseByReference(verse);
            });
            
            item.querySelector('.remove-btn').addEventListener('click', () => {
                this.removeFromFavorites(verse);
            });
            
            container.appendChild(item);
        });
    }
    
    async removeFromFavorites(verse) {
        try {
            const response = await fetch(`http://${this.serverAddress}/api/mobile/favorites`, {
                method: 'DELETE',
                headers: {
                    'Authorization': `Bearer ${this.apiToken}`,
                    'Content-Type': 'application/json'
                },
                body: JSON.stringify({
                    verse_reference: verse
                })
            });
            
            if (response.ok) {
                this.loadFavorites();
                this.showToast('Removed from favorites', 'success');
            }
        } catch (error) {
            console.error('Remove favorite error:', error);
        }
    }
    
    updatePresentationPreview(state) {
        if (this.settings.showPreview) {
            this.updateCurrentVerse(state.current_verse, state.current_reference);
        }
    }
    
    updateCurrentVerse(verseText, reference) {
        document.querySelector('.verse-text').textContent = verseText || 'No verse displayed';
        document.querySelector('.verse-reference').textContent = reference || '';
    }
    
    updateConnectionStatus(connected) {
        const indicator = document.getElementById('connectionIndicator');
        const deviceInfo = document.getElementById('deviceInfo');
        
        if (connected) {
            indicator.classList.add('connected');
            deviceInfo.textContent = 'Connected';
        } else {
            indicator.classList.remove('connected');
            deviceInfo.textContent = 'Disconnected';
        }
        
        // Update device info in settings
        document.getElementById('deviceId').textContent = this.deviceId || 'Not connected';
        document.getElementById('serverInfo').textContent = this.serverAddress || 'Not connected';
    }
    
    switchToControlScreen() {
        document.getElementById('pairingScreen').classList.remove('active');
        document.getElementById('controlScreen').classList.add('active');
    }
    
    disconnect() {
        if (this.websocket) {
            this.websocket.close();
        }
        
        localStorage.removeItem('vf_auth_token');
        localStorage.removeItem('vf_device_id');
        localStorage.removeItem('vf_server_address');
        
        this.apiToken = null;
        this.deviceId = null;
        this.serverAddress = '';
        this.isConnected = false;
        
        document.getElementById('controlScreen').classList.remove('active');
        document.getElementById('pairingScreen').classList.add('active');
        
        this.showToast('Disconnected', 'success');
    }
    
    resetApp() {
        if (confirm('This will reset all app data. Continue?')) {
            localStorage.clear();
            location.reload();
        }
    }
    
    showStatus(message, type) {
        const statusEl = document.getElementById('statusMessage');
        statusEl.textContent = message;
        statusEl.className = `status-message ${type}`;
    }
    
    showToast(message, type = 'info') {
        const toast = document.createElement('div');
        toast.className = `toast ${type}`;
        toast.textContent = message;
        
        document.getElementById('toastContainer').appendChild(toast);
        
        setTimeout(() => {
            toast.remove();
        }, 3000);
    }
    
    hapticFeedback() {
        if (this.settings.vibration && navigator.vibrate) {
            navigator.vibrate(50);
        }
    }
    
    getDefaultDeviceName() {
        const userAgent = navigator.userAgent;
        if (/iPhone/i.test(userAgent)) return "iPhone";
        if (/iPad/i.test(userAgent)) return "iPad";
        if (/Android/i.test(userAgent)) return "Android Device";
        return "Mobile Device";
    }
    
    loadSettings() {
        const saved = localStorage.getItem('vf_mobile_settings');
        if (saved) {
            this.settings = { ...this.settings, ...JSON.parse(saved) };
        }
        this.applySettings();
    }
    
    saveSettings() {
        localStorage.setItem('vf_mobile_settings', JSON.stringify(this.settings));
        this.applySettings();
    }
    
    applySettings() {
        // Apply theme
        document.body.className = `theme-${this.settings.theme}`;
        
        // Apply font size
        document.body.style.fontSize = {
            'small': '14px',
            'medium': '16px',
            'large': '18px'
        }[this.settings.fontSize];
        
        // Update UI elements
        document.getElementById('vibrationEnabled').checked = this.settings.vibration;
        document.getElementById('soundEnabled').checked = this.settings.sound;
        document.getElementById('showPreview').checked = this.settings.showPreview;
    }
}

// Initialize app when DOM is loaded
document.addEventListener('DOMContentLoaded', () => {
    window.verseFinderMobile = new VerseFinderMobile();
});

// Handle PWA installation
let deferredPrompt;
window.addEventListener('beforeinstallprompt', (e) => {
    e.preventDefault();
    deferredPrompt = e;
    
    // Show install button if needed
    const installBtn = document.createElement('button');
    installBtn.textContent = 'Install App';
    installBtn.onclick = async () => {
        if (deferredPrompt) {
            deferredPrompt.prompt();
            const { outcome } = await deferredPrompt.userChoice;
            console.log(`User response to the install prompt: ${outcome}`);
            deferredPrompt = null;
            installBtn.remove();
        }
    };
    
    // Add to header or settings
    document.querySelector('.header').appendChild(installBtn);
});

// Handle network status
window.addEventListener('online', () => {
    if (window.verseFinderMobile && window.verseFinderMobile.websocket) {
        window.verseFinderMobile.connectWebSocket();
    }
});

window.addEventListener('offline', () => {
    if (window.verseFinderMobile) {
        window.verseFinderMobile.showToast('You are offline', 'warning');
    }
});