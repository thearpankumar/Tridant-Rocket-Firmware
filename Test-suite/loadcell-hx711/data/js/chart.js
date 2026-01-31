// Real-time Thrust Chart using ApexCharts
class ThrustChart {
    constructor(containerId) {
        this.containerId = containerId;
        this.chart = null;
        this.data = [];
        this.maxDataPoints = 1600; // ~20 seconds at 80Hz
        this.peakValue = 0;
        this.peakTime = 0;
        this.updateCounter = 0;

        this.initChart();
    }

    initChart() {
        const options = {
            series: [{
                name: 'Thrust',
                data: []
            }],
            chart: {
                type: 'area',
                height: '100%',
                animations: {
                    enabled: true,
                    easing: 'linear',
                    dynamicAnimation: {
                        speed: 100
                    }
                },
                toolbar: {
                    show: false
                },
                zoom: {
                    enabled: false
                },
                background: 'transparent',
                fontFamily: '-apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif'
            },
            colors: ['#00d4ff'],
            fill: {
                type: 'gradient',
                gradient: {
                    shadeIntensity: 1,
                    opacityFrom: 0.5,
                    opacityTo: 0.1,
                    stops: [0, 90, 100],
                    colorStops: [
                        {
                            offset: 0,
                            color: '#00d4ff',
                            opacity: 0.5
                        },
                        {
                            offset: 100,
                            color: '#00d4ff',
                            opacity: 0.05
                        }
                    ]
                }
            },
            stroke: {
                curve: 'smooth',
                width: 2
            },
            dataLabels: {
                enabled: false
            },
            xaxis: {
                type: 'numeric',
                labels: {
                    style: {
                        colors: '#8888aa',
                        fontSize: '11px'
                    },
                    formatter: function(value) {
                        return (value / 1000).toFixed(1) + 's';
                    }
                },
                axisBorder: {
                    show: false
                },
                axisTicks: {
                    show: false
                }
            },
            yaxis: {
                labels: {
                    style: {
                        colors: '#8888aa',
                        fontSize: '11px'
                    },
                    formatter: function(value) {
                        if (value === undefined || value === null) return '0 N';
                        return value.toFixed(0) + ' N';
                    }
                },
                min: 0
            },
            grid: {
                borderColor: 'rgba(100, 100, 150, 0.15)',
                strokeDashArray: 3,
                xaxis: {
                    lines: {
                        show: true
                    }
                },
                yaxis: {
                    lines: {
                        show: true
                    }
                }
            },
            tooltip: {
                enabled: true,
                theme: 'dark',
                x: {
                    formatter: function(value) {
                        return (value / 1000).toFixed(2) + 's';
                    }
                },
                y: {
                    formatter: function(value) {
                        if (value === undefined || value === null) return '0.00 N';
                        return value.toFixed(2) + ' N';
                    }
                },
                marker: {
                    show: true
                }
            },
            annotations: {
                points: []
            },
            noData: {
                text: 'Waiting for data...',
                align: 'center',
                verticalAlign: 'middle',
                style: {
                    color: '#8888aa',
                    fontSize: '16px'
                }
            }
        };

        this.chart = new ApexCharts(document.querySelector('#' + this.containerId), options);
        this.chart.render();
    }

    addDataPoint(timestamp, force) {
        const absForce = Math.abs(force);

        this.data.push({
            x: timestamp,
            y: absForce
        });

        // Track peak
        if (absForce > this.peakValue) {
            this.peakValue = absForce;
            this.peakTime = timestamp;
            this.updatePeakAnnotation();
        }

        // Limit data points
        while (this.data.length > this.maxDataPoints) {
            this.data.shift();
        }

        // Batch update chart every 8 samples (~10Hz visual update at 80Hz data rate)
        // All data points are stored for accuracy, chart renders less frequently
        this.updateCounter++;
        if (this.updateCounter >= 8) {
            this.updateCounter = 0;
            this.updateChart();
        }
    }

    updateChart() {
        if (!this.chart) return;

        this.chart.updateSeries([{
            name: 'Thrust',
            data: this.data
        }], false);
    }

    updatePeakAnnotation() {
        if (!this.chart || this.peakValue < 0.5) return;

        this.chart.updateOptions({
            annotations: {
                points: [{
                    x: this.peakTime,
                    y: this.peakValue,
                    marker: {
                        size: 6,
                        fillColor: '#ff4444',
                        strokeColor: '#ff4444',
                        radius: 2
                    },
                    label: {
                        borderColor: '#ff4444',
                        offsetY: -10,
                        style: {
                            color: '#fff',
                            background: '#ff4444',
                            fontSize: '10px',
                            fontWeight: 600
                        },
                        text: 'Peak: ' + this.peakValue.toFixed(1) + ' N'
                    }
                }]
            }
        }, false, false);
    }

    reset() {
        this.data = [];
        this.peakValue = 0;
        this.peakTime = 0;
        this.updateCounter = 0;

        if (this.chart) {
            this.chart.updateSeries([{
                name: 'Thrust',
                data: []
            }]);
            this.chart.updateOptions({
                annotations: {
                    points: []
                }
            });
        }
    }

    getData() {
        return this.data.slice();
    }

    getPeak() {
        return {
            value: this.peakValue,
            time: this.peakTime
        };
    }
}

// Global instance
let thrustChart = null;
