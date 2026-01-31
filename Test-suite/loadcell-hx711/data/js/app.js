// Thrust Test Dashboard - Main Application
class ThrustDashboard {
    constructor() {
        this.recording = false;
        this.dataLog = [];

        // Initialize components
        this.initComponents();
        this.bindEvents();
        this.connectWebSocket();
    }

    initComponents() {
        // Initialize chart
        thrustChart = new ThrustChart('thrustChart');

        // Initialize metrics display
        metricsDisplay = new MetricsDisplay();

        // Cache DOM elements
        this.elements = {
            connectionStatus: document.getElementById('connectionStatus'),
            statusText: document.querySelector('.status-text'),
            btnTare: document.getElementById('btnTare'),
            btnStartStop: document.getElementById('btnStartStop'),
            btnReset: document.getElementById('btnReset'),
            btnExport: document.getElementById('btnExport'),
            btnCalibrate: document.getElementById('btnCalibrate'),
            calibrationWeight: document.getElementById('calibrationWeight'),
            ipAddress: document.getElementById('ipAddress')
        };

        // Set IP address display
        if (this.elements.ipAddress) {
            this.elements.ipAddress.textContent = window.location.host;
        }
    }

    bindEvents() {
        // Tare button
        if (this.elements.btnTare) {
            this.elements.btnTare.addEventListener('click', () => {
                wsHandler.tare();
                this.showFeedback(this.elements.btnTare, 'Taring...');
            });
        }

        // Start/Stop button
        if (this.elements.btnStartStop) {
            this.elements.btnStartStop.addEventListener('click', () => {
                if (this.recording) {
                    wsHandler.stop();
                } else {
                    wsHandler.start();
                }
            });
        }

        // Reset button
        if (this.elements.btnReset) {
            this.elements.btnReset.addEventListener('click', () => {
                wsHandler.reset();
                this.resetLocal();
            });
        }

        // Export button
        if (this.elements.btnExport) {
            this.elements.btnExport.addEventListener('click', () => {
                this.exportCSV();
            });
        }

        // Calibrate button
        if (this.elements.btnCalibrate) {
            this.elements.btnCalibrate.addEventListener('click', () => {
                const weight = parseFloat(this.elements.calibrationWeight.value);
                if (weight > 0) {
                    wsHandler.calibrate(weight);
                    this.showFeedback(this.elements.btnCalibrate, 'Calibrating...');
                } else {
                    alert('Please enter a valid weight in grams');
                }
            });
        }

        // Enter key on calibration input
        if (this.elements.calibrationWeight) {
            this.elements.calibrationWeight.addEventListener('keypress', (e) => {
                if (e.key === 'Enter') {
                    this.elements.btnCalibrate.click();
                }
            });
        }
    }

    connectWebSocket() {
        // Set up WebSocket callbacks
        wsHandler.onConnect(() => {
            this.updateConnectionStatus(true);
        });

        wsHandler.onDisconnect(() => {
            this.updateConnectionStatus(false);
        });

        wsHandler.onInit((msg) => {
            this.recording = msg.recording || false;
            this.updateRecordingUI();
        });

        wsHandler.onData((timestamp, force) => {
            // Update chart
            thrustChart.addDataPoint(timestamp, force);

            // Update current thrust display
            metricsDisplay.updateCurrentThrust(force);

            // Log data for export
            if (this.recording) {
                this.dataLog.push({ t: timestamp, f: force });
            }
        });

        wsHandler.onMetrics((metrics) => {
            metricsDisplay.updateMetrics(metrics);

            // Update recording state from server
            if (metrics.recording !== undefined && metrics.recording !== this.recording) {
                this.recording = metrics.recording;
                this.updateRecordingUI();
            }
        });

        wsHandler.onAck((cmd) => {
            switch (cmd) {
                case 'start':
                    this.recording = true;
                    this.updateRecordingUI();
                    break;
                case 'stop':
                    this.recording = false;
                    this.updateRecordingUI();
                    break;
                case 'reset':
                    this.resetLocal();
                    break;
                case 'tare':
                    this.resetLocal();
                    break;
            }
        });

        // Connect
        wsHandler.connect();
    }

    updateConnectionStatus(connected) {
        if (this.elements.connectionStatus) {
            if (connected) {
                this.elements.connectionStatus.classList.add('connected');
                this.elements.statusText.textContent = 'Connected';
            } else {
                this.elements.connectionStatus.classList.remove('connected');
                this.elements.statusText.textContent = 'Disconnected';
            }
        }
    }

    updateRecordingUI() {
        const btn = this.elements.btnStartStop;
        if (!btn) return;

        const textSpan = btn.querySelector('.btn-text');

        if (this.recording) {
            btn.classList.add('recording');
            if (textSpan) textSpan.textContent = 'STOP';
        } else {
            btn.classList.remove('recording');
            if (textSpan) textSpan.textContent = 'START';
        }
    }

    resetLocal() {
        thrustChart.reset();
        metricsDisplay.reset();
        this.dataLog = [];
    }

    showFeedback(button, text) {
        const originalText = button.textContent;
        button.disabled = true;
        button.style.opacity = '0.7';

        setTimeout(() => {
            button.disabled = false;
            button.style.opacity = '1';
        }, 1000);
    }

    exportCSV() {
        // Get data from chart if no logged data
        let data = this.dataLog.length > 0 ? this.dataLog : thrustChart.getData().map(d => ({
            t: d.x,
            f: d.y
        }));

        if (data.length === 0) {
            alert('No data to export');
            return;
        }

        // Create CSV content
        const metrics = metricsDisplay.getValues();
        let csv = '# Thrust Test Data Export\n';
        csv += `# Date: ${new Date().toISOString()}\n`;
        csv += `# Peak Thrust: ${metrics.peakThrust.toFixed(2)} N\n`;
        csv += `# Total Impulse: ${metrics.totalImpulse.toFixed(2)} Ns\n`;
        csv += `# Burn Time: ${metrics.burnTime.toFixed(2)} s\n`;
        csv += `# Average Thrust: ${metrics.avgThrust.toFixed(2)} N\n`;
        csv += `# Sample Count: ${metrics.sampleCount}\n`;
        csv += '#\n';
        csv += 'timestamp_ms,force_N\n';

        data.forEach(point => {
            csv += `${point.t},${point.f.toFixed(3)}\n`;
        });

        // Download
        const blob = new Blob([csv], { type: 'text/csv' });
        const url = URL.createObjectURL(blob);
        const a = document.createElement('a');
        a.href = url;
        a.download = `thrust_test_${Date.now()}.csv`;
        document.body.appendChild(a);
        a.click();
        document.body.removeChild(a);
        URL.revokeObjectURL(url);

        console.log(`Exported ${data.length} data points`);
    }
}

// Initialize on DOM ready
document.addEventListener('DOMContentLoaded', () => {
    window.dashboard = new ThrustDashboard();
});
