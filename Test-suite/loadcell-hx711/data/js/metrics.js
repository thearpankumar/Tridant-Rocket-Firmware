// Metrics Display Handler
class MetricsDisplay {
    constructor() {
        this.elements = {
            currentThrust: document.getElementById('currentThrust'),
            peakThrust: document.getElementById('peakThrust'),
            totalImpulse: document.getElementById('totalImpulse'),
            burnTime: document.getElementById('burnTime'),
            avgThrust: document.getElementById('avgThrust'),
            sampleCount: document.getElementById('sampleCount')
        };

        this.lastValues = {
            currentThrust: 0,
            peakThrust: 0,
            totalImpulse: 0,
            burnTime: 0,
            avgThrust: 0,
            sampleCount: 0
        };
    }

    updateCurrentThrust(value) {
        if (this.elements.currentThrust) {
            this.elements.currentThrust.textContent = Math.abs(value).toFixed(2);
            this.lastValues.currentThrust = value;
        }
    }

    updateMetrics(metrics) {
        if (metrics.peak !== undefined && this.elements.peakThrust) {
            this.animateValue(this.elements.peakThrust, this.lastValues.peakThrust, metrics.peak, 2);
            this.lastValues.peakThrust = metrics.peak;
        }

        if (metrics.impulse !== undefined && this.elements.totalImpulse) {
            this.animateValue(this.elements.totalImpulse, this.lastValues.totalImpulse, metrics.impulse, 2);
            this.lastValues.totalImpulse = metrics.impulse;
        }

        if (metrics.burn !== undefined && this.elements.burnTime) {
            this.animateValue(this.elements.burnTime, this.lastValues.burnTime, metrics.burn, 2);
            this.lastValues.burnTime = metrics.burn;
        }

        if (metrics.avg !== undefined && this.elements.avgThrust) {
            this.animateValue(this.elements.avgThrust, this.lastValues.avgThrust, metrics.avg, 2);
            this.lastValues.avgThrust = metrics.avg;
        }

        if (metrics.samples !== undefined && this.elements.sampleCount) {
            this.elements.sampleCount.textContent = metrics.samples.toLocaleString();
            this.lastValues.sampleCount = metrics.samples;
        }
    }

    animateValue(element, start, end, decimals) {
        // Skip animation for small changes
        if (Math.abs(end - start) < 0.01) {
            element.textContent = end.toFixed(decimals);
            return;
        }

        const duration = 200;
        const startTime = performance.now();

        const update = (currentTime) => {
            const elapsed = currentTime - startTime;
            const progress = Math.min(elapsed / duration, 1);

            // Ease out quad
            const eased = 1 - (1 - progress) * (1 - progress);
            const current = start + (end - start) * eased;

            element.textContent = current.toFixed(decimals);

            if (progress < 1) {
                requestAnimationFrame(update);
            }
        };

        requestAnimationFrame(update);
    }

    reset() {
        Object.keys(this.elements).forEach(key => {
            if (this.elements[key]) {
                if (key === 'sampleCount') {
                    this.elements[key].textContent = '0';
                } else {
                    this.elements[key].textContent = '0.00';
                }
            }
        });

        this.lastValues = {
            currentThrust: 0,
            peakThrust: 0,
            totalImpulse: 0,
            burnTime: 0,
            avgThrust: 0,
            sampleCount: 0
        };
    }

    getValues() {
        return { ...this.lastValues };
    }
}

// Global instance
let metricsDisplay = null;
