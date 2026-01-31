// WebSocket Connection Handler
class WebSocketHandler {
    constructor() {
        this.ws = null;
        this.reconnectInterval = 2000;
        this.reconnectAttempts = 0;
        this.maxReconnectAttempts = 50;
        this.callbacks = {
            onData: null,
            onMetrics: null,
            onConnect: null,
            onDisconnect: null,
            onAck: null,
            onInit: null,
            onClear: null
        };
    }

    connect() {
        const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
        const wsUrl = `${protocol}//${window.location.host}/ws`;

        console.log('Connecting to WebSocket:', wsUrl);

        try {
            this.ws = new WebSocket(wsUrl);

            this.ws.onopen = () => {
                console.log('WebSocket connected');
                this.reconnectAttempts = 0;
                if (this.callbacks.onConnect) {
                    this.callbacks.onConnect();
                }
            };

            this.ws.onclose = (event) => {
                console.log('WebSocket disconnected:', event.code);
                if (this.callbacks.onDisconnect) {
                    this.callbacks.onDisconnect();
                }
                this.scheduleReconnect();
            };

            this.ws.onerror = (error) => {
                console.error('WebSocket error:', error);
            };

            this.ws.onmessage = (event) => {
                this.handleMessage(event.data);
            };
        } catch (error) {
            console.error('WebSocket connection error:', error);
            this.scheduleReconnect();
        }
    }

    handleMessage(data) {
        try {
            const msg = JSON.parse(data);

            switch (msg.type) {
                case 'data':
                    if (this.callbacks.onData) {
                        this.callbacks.onData(msg.t, msg.f);
                    }
                    break;

                case 'metrics':
                    if (this.callbacks.onMetrics) {
                        this.callbacks.onMetrics(msg);
                    }
                    break;

                case 'init':
                    console.log('Received init:', msg);
                    if (this.callbacks.onInit) {
                        this.callbacks.onInit(msg);
                    }
                    break;

                case 'ack':
                    console.log('Command acknowledged:', msg.cmd);
                    if (this.callbacks.onAck) {
                        this.callbacks.onAck(msg.cmd);
                    }
                    break;

                case 'clear':
                    console.log('Clear signal received');
                    if (this.callbacks.onClear) {
                        this.callbacks.onClear();
                    }
                    break;

                default:
                    console.log('Unknown message type:', msg.type);
            }
        } catch (error) {
            console.error('Error parsing message:', error, data);
        }
    }

    scheduleReconnect() {
        if (this.reconnectAttempts >= this.maxReconnectAttempts) {
            console.log('Max reconnect attempts reached');
            return;
        }

        this.reconnectAttempts++;
        const delay = Math.min(this.reconnectInterval * this.reconnectAttempts, 10000);
        console.log(`Reconnecting in ${delay}ms (attempt ${this.reconnectAttempts})`);

        setTimeout(() => {
            this.connect();
        }, delay);
    }

    send(command, value = null) {
        if (!this.ws || this.ws.readyState !== WebSocket.OPEN) {
            console.warn('WebSocket not connected');
            return false;
        }

        const msg = { cmd: command };
        if (value !== null) {
            msg.value = value;
        }

        this.ws.send(JSON.stringify(msg));
        return true;
    }

    // Command shortcuts
    tare() {
        return this.send('tare');
    }

    start() {
        return this.send('start');
    }

    stop() {
        return this.send('stop');
    }

    reset() {
        return this.send('reset');
    }

    calibrate(weightGrams) {
        return this.send('calibrate', weightGrams);
    }

    // Event registration
    onData(callback) {
        this.callbacks.onData = callback;
    }

    onMetrics(callback) {
        this.callbacks.onMetrics = callback;
    }

    onConnect(callback) {
        this.callbacks.onConnect = callback;
    }

    onDisconnect(callback) {
        this.callbacks.onDisconnect = callback;
    }

    onInit(callback) {
        this.callbacks.onInit = callback;
    }

    onAck(callback) {
        this.callbacks.onAck = callback;
    }

    onClear(callback) {
        this.callbacks.onClear = callback;
    }

    isConnected() {
        return this.ws && this.ws.readyState === WebSocket.OPEN;
    }
}

// Global instance
const wsHandler = new WebSocketHandler();
