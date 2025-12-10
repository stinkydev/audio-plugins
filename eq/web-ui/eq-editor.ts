/**
 * Sesame EQ - Interactive Curve Editor
 * TypeScript reference implementation for EQ curve visualization with draggable bands
 */

// Match the plugin's FilterType enum
enum FilterType {
    kLowCut = 0,    // Only available on Band 1 (high-pass)
    kBell = 1,      // Available on all bands
    kLowShelf = 2,  // Available on all bands
    kHighShelf = 3, // Available on all bands
    kHighCut = 4    // Only available on Band 4 (low-pass)
}

interface EQBand {
    type: FilterType;
    frequency: number;  // Hz
    gain: number;       // dB
    q: number;          // Q factor (0.707 default)
    enabled: boolean;
}

interface Point2D {
    x: number;
    y: number;
}

interface EQSettings {
    bands: EQBand[];
    outputGain: number;  // dB
    bypass: boolean;
}

class EQEditor {
    private canvas: HTMLCanvasElement;
    private ctx: CanvasRenderingContext2D;
    private bands: EQBand[];
    private outputGain: number = 0.0;
    private bypass: boolean = false;
    private activeBandIndex: number = -1;
    private isDragging: boolean = false;
    private dragBandIndex: number = -1;

    // Canvas dimensions
    private width: number = 0;
    private height: number = 0;
    private padding = { top: 20, right: 40, bottom: 40, left: 50 };

    // Frequency range (logarithmic scale)
    private minFreq = 20;    // Hz
    private maxFreq = 20000; // Hz

    // Gain range
    private minGain = -24;   // dB
    private maxGain = 24;    // dB

    // Colors for each band
    private bandColors = [
        '#FF6B6B', '#4ECDC4', '#45B7D1', '#FFA07A', 
        '#98D8C8', '#F7DC6F', '#BB8FCE', '#85C1E2'
    ];

    constructor(canvasId: string, numBands: number = 4) {
        this.canvas = document.getElementById(canvasId) as HTMLCanvasElement;
        if (!this.canvas) {
            throw new Error(`Canvas element with id "${canvasId}" not found`);
        }

        const context = this.canvas.getContext('2d');
        if (!context) {
            throw new Error('Could not get 2D context from canvas');
        }
        this.ctx = context;

        // Initialize bands with default values matching the plugin
        this.bands = [];
        const defaultConfigs = [
            { type: FilterType.kLowShelf, freq: 100, q: 0.707 },
            { type: FilterType.kBell, freq: 500, q: 1.0 },
            { type: FilterType.kBell, freq: 2000, q: 1.0 },
            { type: FilterType.kHighShelf, freq: 8000, q: 0.707 }
        ];
        
        for (let i = 0; i < numBands && i < defaultConfigs.length; i++) {
            this.bands.push({
                type: defaultConfigs[i].type,
                frequency: defaultConfigs[i].freq,
                gain: 0,
                q: defaultConfigs[i].q,
                enabled: true
            });
        }

        this.setupCanvas();
        this.setupEventListeners();
        this.createControls();
        this.render();
    }

    private setupCanvas(): void {
        // Set canvas size based on container
        const rect = this.canvas.getBoundingClientRect();
        const dpr = window.devicePixelRatio || 1;
        
        this.canvas.width = rect.width * dpr;
        this.canvas.height = rect.height * dpr;
        this.width = this.canvas.width;
        this.height = this.canvas.height;

        this.ctx.scale(dpr, dpr);
        
        // Handle window resize
        window.addEventListener('resize', () => {
            this.setupCanvas();
            this.render();
        });
    }

    private setupEventListeners(): void {
        this.canvas.addEventListener('mousedown', (e) => this.onMouseDown(e));
        this.canvas.addEventListener('mousemove', (e) => this.onMouseMove(e));
        this.canvas.addEventListener('mouseup', () => this.onMouseUp());
        this.canvas.addEventListener('mouseleave', () => this.onMouseUp());

        // Touch support
        this.canvas.addEventListener('touchstart', (e) => {
            e.preventDefault();
            const touch = e.touches[0];
            const mouseEvent = new MouseEvent('mousedown', {
                clientX: touch.clientX,
                clientY: touch.clientY
            });
            this.canvas.dispatchEvent(mouseEvent);
        });

        this.canvas.addEventListener('touchmove', (e) => {
            e.preventDefault();
            const touch = e.touches[0];
            const mouseEvent = new MouseEvent('mousemove', {
                clientX: touch.clientX,
                clientY: touch.clientY
            });
            this.canvas.dispatchEvent(mouseEvent);
        });

        this.canvas.addEventListener('touchend', (e) => {
            e.preventDefault();
            this.onMouseUp();
        });
    }

    private getCanvasPoint(e: MouseEvent): Point2D {
        const rect = this.canvas.getBoundingClientRect();
        const scaleX = this.canvas.width / rect.width;
        const scaleY = this.canvas.height / rect.height;
        
        return {
            x: (e.clientX - rect.left) * scaleX,
            y: (e.clientY - rect.top) * scaleY
        };
    }

    private onMouseDown(e: MouseEvent): void {
        const point = this.getCanvasPoint(e);
        const dpr = window.devicePixelRatio || 1;
        
        // Check if clicking on a band handle
        for (let i = 0; i < this.bands.length; i++) {
            if (!this.bands[i].enabled) continue;
            
            const handlePos = this.bandToCanvas(this.bands[i]);
            const distance = Math.sqrt(
                Math.pow(point.x - handlePos.x * dpr, 2) + 
                Math.pow(point.y - handlePos.y * dpr, 2)
            );
            
            if (distance < 15 * dpr) {
                this.isDragging = true;
                this.dragBandIndex = i;
                this.setActiveBand(i);
                return;
            }
        }
    }

    private onMouseMove(e: MouseEvent): void {
        if (!this.isDragging || this.dragBandIndex === -1) return;

        const point = this.getCanvasPoint(e);
        const dpr = window.devicePixelRatio || 1;
        const band = this.bands[this.dragBandIndex];

        // Convert canvas coordinates to frequency and gain
        const plotWidth = this.width / dpr - this.padding.left - this.padding.right;
        const plotHeight = this.height / dpr - this.padding.top - this.padding.bottom;

        const x = (point.x / dpr - this.padding.left) / plotWidth;
        const y = (point.y / dpr - this.padding.top) / plotHeight;

        // Update frequency (logarithmic scale)
        const logMin = Math.log10(this.minFreq);
        const logMax = Math.log10(this.maxFreq);
        band.frequency = Math.pow(10, logMin + x * (logMax - logMin));
        band.frequency = Math.max(this.minFreq, Math.min(this.maxFreq, band.frequency));

        // Update gain (linear scale, inverted Y)
        band.gain = this.maxGain - y * (this.maxGain - this.minGain);
        band.gain = Math.max(this.minGain, Math.min(this.maxGain, band.gain));

        this.updateControlValues(this.dragBandIndex);
        this.render();
    }

    private onMouseUp(): void {
        this.isDragging = false;
        this.dragBandIndex = -1;
    }

    private bandToCanvas(band: EQBand): Point2D {
        const plotWidth = this.width / (window.devicePixelRatio || 1) - this.padding.left - this.padding.right;
        const plotHeight = this.height / (window.devicePixelRatio || 1) - this.padding.top - this.padding.bottom;

        // Frequency to X (logarithmic)
        const logMin = Math.log10(this.minFreq);
        const logMax = Math.log10(this.maxFreq);
        const logFreq = Math.log10(band.frequency);
        const x = this.padding.left + ((logFreq - logMin) / (logMax - logMin)) * plotWidth;

        // Gain to Y (linear, inverted)
        const y = this.padding.top + ((this.maxGain - band.gain) / (this.maxGain - this.minGain)) * plotHeight;

        return { x, y };
    }

    private calculateResponse(frequency: number): number {
        let totalGain = 0;

        for (const band of this.bands) {
            if (!band.enabled) continue;

            const gain = this.calculateBandResponse(frequency, band);
            totalGain += gain;
        }

        return totalGain;
    }

    private calculateBandResponse(frequency: number, band: EQBand): number {
        const sampleRate = 48000; // Assumed sample rate for visualization
        const omega = 2 * Math.PI * band.frequency / sampleRate;
        const cosOmega = Math.cos(omega);
        const sinOmega = Math.sin(omega);
        const alpha = sinOmega / (2 * band.q);
        const A = Math.pow(10, band.gain / 40);
        
        let b0 = 1, b1 = 0, b2 = 0;
        let a0 = 1, a1 = 0, a2 = 0;
        
        switch (band.type) {
            case FilterType.kBell: {
                // Bell/Peaking filter (RBJ Audio EQ Cookbook)
                b0 = 1 + alpha * A;
                b1 = -2 * cosOmega;
                b2 = 1 - alpha * A;
                a0 = 1 + alpha / A;
                a1 = -2 * cosOmega;
                a2 = 1 - alpha / A;
                break;
            }

            case FilterType.kLowShelf: {
                // Low shelf filter (RBJ Audio EQ Cookbook)
                const beta = Math.sqrt(A) / band.q;
                b0 = A * ((A + 1) - (A - 1) * cosOmega + beta * sinOmega);
                b1 = 2 * A * ((A - 1) - (A + 1) * cosOmega);
                b2 = A * ((A + 1) - (A - 1) * cosOmega - beta * sinOmega);
                a0 = (A + 1) + (A - 1) * cosOmega + beta * sinOmega;
                a1 = -2 * ((A - 1) + (A + 1) * cosOmega);
                a2 = (A + 1) + (A - 1) * cosOmega - beta * sinOmega;
                break;
            }

            case FilterType.kHighShelf: {
                // High shelf filter (RBJ Audio EQ Cookbook)
                const beta = Math.sqrt(A) / band.q;
                b0 = A * ((A + 1) + (A - 1) * cosOmega + beta * sinOmega);
                b1 = -2 * A * ((A - 1) + (A + 1) * cosOmega);
                b2 = A * ((A + 1) + (A - 1) * cosOmega - beta * sinOmega);
                a0 = (A + 1) - (A - 1) * cosOmega + beta * sinOmega;
                a1 = 2 * ((A - 1) - (A + 1) * cosOmega);
                a2 = (A + 1) - (A - 1) * cosOmega - beta * sinOmega;
                break;
            }

            case FilterType.kLowCut: {
                // Low cut (high-pass) filter (RBJ Audio EQ Cookbook)
                b0 = (1 + cosOmega) / 2;
                b1 = -(1 + cosOmega);
                b2 = (1 + cosOmega) / 2;
                a0 = 1 + alpha;
                a1 = -2 * cosOmega;
                a2 = 1 - alpha;
                break;
            }

            case FilterType.kHighCut: {
                // High cut (low-pass) filter (RBJ Audio EQ Cookbook)
                b0 = (1 - cosOmega) / 2;
                b1 = 1 - cosOmega;
                b2 = (1 - cosOmega) / 2;
                a0 = 1 + alpha;
                a1 = -2 * cosOmega;
                a2 = 1 - alpha;
                break;
            }

            default:
                return 0;
        }
        
        // Normalize coefficients by a0
        b0 /= a0;
        b1 /= a0;
        b2 /= a0;
        a1 /= a0;
        a2 /= a0;
        
        // Calculate frequency response magnitude at the given frequency
        const w = 2 * Math.PI * frequency / sampleRate;
        const phi = Math.sin(w / 2);
        const phi2 = phi * phi;
        
        // H(z) = (b0 + b1*z^-1 + b2*z^-2) / (1 + a1*z^-1 + a2*z^-2)
        // |H(w)| = sqrt[(b0 + b1*cos(w) + b2*cos(2w))^2 + (b1*sin(w) + b2*sin(2w))^2] /
        //          sqrt[(1 + a1*cos(w) + a2*cos(2w))^2 + (a1*sin(w) + a2*sin(2w))^2]
        
        const cosW = Math.cos(w);
        const sinW = Math.sin(w);
        const cos2W = Math.cos(2 * w);
        const sin2W = Math.sin(2 * w);
        
        const numReal = b0 + b1 * cosW + b2 * cos2W;
        const numImag = b1 * sinW + b2 * sin2W;
        const denReal = 1 + a1 * cosW + a2 * cos2W;
        const denImag = a1 * sinW + a2 * sin2W;
        
        const numMag = Math.sqrt(numReal * numReal + numImag * numImag);
        const denMag = Math.sqrt(denReal * denReal + denImag * denImag);
        
        const magnitude = numMag / denMag;
        return 20 * Math.log10(Math.max(magnitude, 1e-10));
    }

    private render(): void {
        const dpr = window.devicePixelRatio || 1;
        
        // Clear canvas with bypass indicator color
        this.ctx.fillStyle = this.bypass ? '#3a2525' : '#2a2a2a';
        this.ctx.fillRect(0, 0, this.width, this.height);
        
        // Draw grid and axes
        this.drawGrid();
        
        // Draw frequency response curve
        this.drawResponseCurve();
        
        // Draw individual band responses (faded)
        this.drawIndividualBands();
        
        // Draw band handles
        this.drawBandHandles();
    }

    private drawGrid(): void {
        const dpr = window.devicePixelRatio || 1;
        const w = this.width / dpr;
        const h = this.height / dpr;
        const plotWidth = w - this.padding.left - this.padding.right;
        const plotHeight = h - this.padding.top - this.padding.bottom;

        this.ctx.strokeStyle = '#404040';
        this.ctx.fillStyle = '#b0b0b0';
        this.ctx.font = '11px sans-serif';
        this.ctx.lineWidth = 1;

        // Horizontal grid lines (gain)
        const gainSteps = [-24, -18, -12, -6, 0, 6, 12, 18, 24];
        gainSteps.forEach(gain => {
            const y = this.padding.top + ((this.maxGain - gain) / (this.maxGain - this.minGain)) * plotHeight;
            
            this.ctx.beginPath();
            this.ctx.moveTo(this.padding.left, y);
            this.ctx.lineTo(w - this.padding.right, y);
            this.ctx.strokeStyle = gain === 0 ? '#606060' : '#404040';
            this.ctx.stroke();

            // Label
            this.ctx.textAlign = 'right';
            this.ctx.textBaseline = 'middle';
            this.ctx.fillText(`${gain > 0 ? '+' : ''}${gain} dB`, this.padding.left - 5, y);
        });

        // Vertical grid lines (frequency)
        const freqSteps = [20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000];
        freqSteps.forEach(freq => {
            const logMin = Math.log10(this.minFreq);
            const logMax = Math.log10(this.maxFreq);
            const logFreq = Math.log10(freq);
            const x = this.padding.left + ((logFreq - logMin) / (logMax - logMin)) * plotWidth;

            this.ctx.beginPath();
            this.ctx.moveTo(x, this.padding.top);
            this.ctx.lineTo(x, h - this.padding.bottom);
            this.ctx.strokeStyle = '#404040';
            this.ctx.stroke();

            // Label
            this.ctx.textAlign = 'center';
            this.ctx.textBaseline = 'top';
            const label = freq >= 1000 ? `${freq / 1000}k` : `${freq}`;
            this.ctx.fillText(label, x, h - this.padding.bottom + 5);
        });

        // Axes labels
        this.ctx.fillStyle = '#4CAF50';
        this.ctx.font = '12px sans-serif';
        this.ctx.textAlign = 'center';
        this.ctx.fillText('Frequency (Hz)', w / 2, h - 10);
        
        this.ctx.save();
        this.ctx.translate(15, h / 2);
        this.ctx.rotate(-Math.PI / 2);
        this.ctx.fillText('Gain (dB)', 0, 0);
        this.ctx.restore();
    }

    private drawResponseCurve(): void {
        const dpr = window.devicePixelRatio || 1;
        const plotWidth = this.width / dpr - this.padding.left - this.padding.right;
        const plotHeight = this.height / dpr - this.padding.top - this.padding.bottom;

        if (this.bypass) {
            // Draw flat line at output gain when bypassed
            this.ctx.setLineDash([5, 5]);
            this.ctx.strokeStyle = '#777777';
            this.ctx.lineWidth = 2;
            this.ctx.beginPath();
            const canvasY = this.padding.top + ((this.maxGain - this.outputGain) / (this.maxGain - this.minGain)) * plotHeight;
            this.ctx.moveTo(this.padding.left, canvasY);
            this.ctx.lineTo(this.padding.left + plotWidth, canvasY);
            this.ctx.stroke();
            this.ctx.setLineDash([]);
            return;
        }

        this.ctx.beginPath();
        this.ctx.strokeStyle = '#4CAF50';
        this.ctx.lineWidth = 3;

        const points = 500;
        for (let i = 0; i <= points; i++) {
            const x = i / points;
            const logMin = Math.log10(this.minFreq);
            const logMax = Math.log10(this.maxFreq);
            const freq = Math.pow(10, logMin + x * (logMax - logMin));
            
            const gain = this.calculateResponse(freq) + this.outputGain;
            const canvasX = this.padding.left + x * plotWidth;
            const canvasY = this.padding.top + ((this.maxGain - gain) / (this.maxGain - this.minGain)) * plotHeight;

            if (i === 0) {
                this.ctx.moveTo(canvasX, canvasY);
            } else {
                this.ctx.lineTo(canvasX, canvasY);
            }
        }

        this.ctx.stroke();
    }

    private drawIndividualBands(): void {
        if (this.bypass) return;
        
        const dpr = window.devicePixelRatio || 1;
        const plotWidth = this.width / dpr - this.padding.left - this.padding.right;
        const plotHeight = this.height / dpr - this.padding.top - this.padding.bottom;

        this.bands.forEach((band, index) => {
            if (!band.enabled) return;

            this.ctx.beginPath();
            this.ctx.strokeStyle = this.bandColors[index % this.bandColors.length] + '40'; // 25% opacity
            this.ctx.lineWidth = 1.5;

            const points = 200;
            for (let i = 0; i <= points; i++) {
                const x = i / points;
                const logMin = Math.log10(this.minFreq);
                const logMax = Math.log10(this.maxFreq);
                const freq = Math.pow(10, logMin + x * (logMax - logMin));
                
                const gain = this.calculateBandResponse(freq, band);
                const canvasX = this.padding.left + x * plotWidth;
                const canvasY = this.padding.top + ((this.maxGain - gain) / (this.maxGain - this.minGain)) * plotHeight;

                if (i === 0) {
                    this.ctx.moveTo(canvasX, canvasY);
                } else {
                    this.ctx.lineTo(canvasX, canvasY);
                }
            }

            this.ctx.stroke();
        });
    }

    private drawBandHandles(): void {
        if (this.bypass) return;
        
        const dpr = window.devicePixelRatio || 1;
        
        this.bands.forEach((band, index) => {
            if (!band.enabled) return;

            const pos = this.bandToCanvas(band);
            const color = this.bandColors[index % this.bandColors.length];
            const isActive = index === this.activeBandIndex;
            const radius = isActive ? 10 : 8;

            // Outer ring for active band
            if (isActive) {
                this.ctx.beginPath();
                this.ctx.arc(pos.x, pos.y, radius + 4, 0, 2 * Math.PI);
                this.ctx.strokeStyle = color;
                this.ctx.lineWidth = 2;
                this.ctx.stroke();
            }

            // Main handle
            this.ctx.beginPath();
            this.ctx.arc(pos.x, pos.y, radius, 0, 2 * Math.PI);
            this.ctx.fillStyle = color;
            this.ctx.fill();
            this.ctx.strokeStyle = '#1a1a1a';
            this.ctx.lineWidth = 2;
            this.ctx.stroke();

            // Label
            this.ctx.fillStyle = '#e0e0e0';
            this.ctx.font = 'bold 11px sans-serif';
            this.ctx.textAlign = 'center';
            this.ctx.textBaseline = 'middle';
            this.ctx.fillText(`${index + 1}`, pos.x, pos.y);
        });
    }

    private createControls(): void {
        const container = document.getElementById('bandControls');
        if (!container) return;

        container.innerHTML = '';

        this.bands.forEach((band, index) => {
            const bandDiv = document.createElement('div');
            bandDiv.className = 'band-control';
            bandDiv.dataset.bandIndex = index.toString();

            const color = this.bandColors[index % this.bandColors.length];
            
            bandDiv.innerHTML = `
                <h3 style="color: ${color}">Band ${index + 1}</h3>
                <div class="param-group">
                    <div class="param">
                        <label>Enabled</label>
                        <div class="param-value">
                            <input type="checkbox" class="enable-checkbox" ${band.enabled ? 'checked' : ''} 
                                   style="width: 20px; height: 20px; cursor: pointer;">
                        </div>
                    </div>
                    <div class="param">
                        <label>Type</label>
                        <div class="param-value">
                            <select class="type-selector" style="padding: 4px; background: #454545; color: #e0e0e0; border: 1px solid #555; border-radius: 3px; cursor: pointer;">
                                ${index === 0 ? `<option value="${FilterType.kLowCut}" ${band.type === FilterType.kLowCut ? 'selected' : ''}>Low Cut</option>` : ''}
                                <option value="${FilterType.kBell}" ${band.type === FilterType.kBell ? 'selected' : ''}>Bell</option>
                                <option value="${FilterType.kLowShelf}" ${band.type === FilterType.kLowShelf ? 'selected' : ''}>Low Shelf</option>
                                <option value="${FilterType.kHighShelf}" ${band.type === FilterType.kHighShelf ? 'selected' : ''}>High Shelf</option>
                                ${index === 3 ? `<option value="${FilterType.kHighCut}" ${band.type === FilterType.kHighCut ? 'selected' : ''}>High Cut</option>` : ''}
                            </select>
                        </div>
                    </div>
                    <div class="param">
                        <label>Frequency</label>
                        <div class="param-value">
                            <input type="range" class="freq-slider" 
                                   min="${Math.log10(this.minFreq)}" 
                                   max="${Math.log10(this.maxFreq)}" 
                                   step="0.001" 
                                   value="${Math.log10(band.frequency)}">
                            <span class="param-display freq-display">${this.formatFrequency(band.frequency)}</span>
                        </div>
                    </div>
                    <div class="param">
                        <label>Gain</label>
                        <div class="param-value">
                            <input type="range" class="gain-slider" 
                                   min="${this.minGain}" 
                                   max="${this.maxGain}" 
                                   step="0.1" 
                                   value="${band.gain}">
                            <span class="param-display gain-display">${band.gain.toFixed(1)} dB</span>
                        </div>
                    </div>
                    <div class="param">
                        <label>Q</label>
                        <div class="param-value">
                            <input type="range" class="q-slider" 
                                   min="0.1" 
                                   max="10" 
                                   step="0.1" 
                                   value="${band.q}">
                            <span class="param-display q-display">${band.q.toFixed(1)}</span>
                        </div>
                    </div>
                </div>
            `;

            // Event listeners for controls
            const enableCheckbox = bandDiv.querySelector('.enable-checkbox') as HTMLInputElement;
            const typeSelector = bandDiv.querySelector('.type-selector') as HTMLSelectElement;
            const freqSlider = bandDiv.querySelector('.freq-slider') as HTMLInputElement;
            const gainSlider = bandDiv.querySelector('.gain-slider') as HTMLInputElement;
            const qSlider = bandDiv.querySelector('.q-slider') as HTMLInputElement;

            enableCheckbox.addEventListener('change', (e) => {
                this.bands[index].enabled = (e.target as HTMLInputElement).checked;
                this.render();
            });

            typeSelector.addEventListener('change', (e) => {
                this.bands[index].type = parseInt((e.target as HTMLSelectElement).value) as FilterType;
                this.updateControlVisibility(index);
                this.render();
            });

            freqSlider.addEventListener('input', (e) => {
                const value = parseFloat((e.target as HTMLInputElement).value);
                this.bands[index].frequency = Math.pow(10, value);
                this.updateControlValues(index);
                this.render();
            });

            gainSlider.addEventListener('input', (e) => {
                this.bands[index].gain = parseFloat((e.target as HTMLInputElement).value);
                this.updateControlValues(index);
                this.render();
            });

            qSlider.addEventListener('input', (e) => {
                this.bands[index].q = parseFloat((e.target as HTMLInputElement).value);
                this.updateControlValues(index);
                this.render();
            });

            // Click to highlight
            bandDiv.addEventListener('click', () => {
                this.setActiveBand(index);
            });

            // Update visibility based on initial type
            this.updateControlVisibility(index);

            container.appendChild(bandDiv);
        });

        // Global controls
        const globalControlsDiv = document.createElement('div');
        globalControlsDiv.style.cssText = 'background: #353535; padding: 15px; border-radius: 6px; margin-bottom: 15px;';
        globalControlsDiv.innerHTML = `
            <h3 style="color: #4CAF50; margin-bottom: 10px; font-size: 14px;">Global Controls</h3>
            <div style="display: flex; gap: 20px; align-items: center;">
                <div class="param" style="flex: 1;">
                    <label>Output Gain</label>
                    <div class="param-value">
                        <input type="range" id="outputGainSlider" 
                               min="-24" max="24" step="0.1" value="${this.outputGain}"
                               style="width: 150px;">
                        <span class="param-display" id="outputGainDisplay">${this.outputGain.toFixed(1)} dB</span>
                    </div>
                </div>
                <div class="param">
                    <label>Bypass</label>
                    <input type="checkbox" id="bypassCheckbox" ${this.bypass ? 'checked' : ''}
                           style="width: 20px; height: 20px; cursor: pointer;">
                </div>
            </div>
        `;
        
        container.parentElement?.insertBefore(globalControlsDiv, container);

        // Global control listeners
        document.getElementById('outputGainSlider')?.addEventListener('input', (e) => {
            this.outputGain = parseFloat((e.target as HTMLInputElement).value);
            const display = document.getElementById('outputGainDisplay');
            if (display) display.textContent = `${this.outputGain.toFixed(1)} dB`;
            this.render();
        });

        document.getElementById('bypassCheckbox')?.addEventListener('change', (e) => {
            this.bypass = (e.target as HTMLInputElement).checked;
            this.render();
        });

        // Global controls
        document.getElementById('resetBtn')?.addEventListener('click', () => {
            this.resetAllBands();
        });

        document.getElementById('exportBtn')?.addEventListener('click', () => {
            this.exportSettings();
        });
    }

    private updateControlValues(index: number): void {
        const bandDiv = document.querySelector(`[data-band-index="${index}"]`);
        if (!bandDiv) return;

        const band = this.bands[index];

        const freqDisplay = bandDiv.querySelector('.freq-display');
        const gainDisplay = bandDiv.querySelector('.gain-display');
        const qDisplay = bandDiv.querySelector('.q-display');

        if (freqDisplay) freqDisplay.textContent = this.formatFrequency(band.frequency);
        if (gainDisplay) gainDisplay.textContent = `${band.gain.toFixed(1)} dB`;
        if (qDisplay) qDisplay.textContent = band.q.toFixed(1);

        const freqSlider = bandDiv.querySelector('.freq-slider') as HTMLInputElement;
        if (freqSlider) freqSlider.value = Math.log10(band.frequency).toString();

        const gainSlider = bandDiv.querySelector('.gain-slider') as HTMLInputElement;
        if (gainSlider) gainSlider.value = band.gain.toString();
    }

    private updateControlVisibility(index: number): void {
        const bandDiv = document.querySelector(`[data-band-index="${index}"]`);
        if (!bandDiv) return;

        const band = this.bands[index];
        const gainParam = bandDiv.querySelector('.gain-slider')?.closest('.param') as HTMLElement;
        const qParam = bandDiv.querySelector('.q-slider')?.closest('.param') as HTMLElement;
        const qLabel = qParam?.querySelector('label');

        // Gain is not used for low cut/high cut filters
        if (gainParam) {
            const noGainTypes = [
                FilterType.kHighCut,
                FilterType.kLowCut
            ];
            if (noGainTypes.includes(band.type)) {
                gainParam.style.display = 'none';
            } else {
                gainParam.style.display = 'flex';
            }
        }

        // Q parameter label changes meaning for different filter types
        if (qLabel) {
            if (band.type === FilterType.kHighCut || band.type === FilterType.kLowCut) {
                qLabel.textContent = 'Slope';
            } else if (band.type === FilterType.kLowShelf || band.type === FilterType.kHighShelf) {
                qLabel.textContent = 'Slope';
            } else {
                qLabel.textContent = 'Q';
            }
        }
    }

    private setActiveBand(index: number): void {
        this.activeBandIndex = index;
        
        // Update UI
        document.querySelectorAll('.band-control').forEach((el, i) => {
            if (i === index) {
                el.classList.add('active');
            } else {
                el.classList.remove('active');
            }
        });

        this.render();
    }

    private formatFrequency(freq: number): string {
        if (freq >= 1000) {
            return `${(freq / 1000).toFixed(2)} kHz`;
        }
        return `${freq.toFixed(0)} Hz`;
    }

    private resetAllBands(): void {
        this.bands.forEach(band => {
            band.gain = 0;
            band.q = 0.707; // Default Q matching plugin
        });

        document.querySelectorAll('.band-control').forEach((el, index) => {
            this.updateControlValues(index);
        });

        this.render();
    }

    private exportSettings(): void {
        const settings: EQSettings = {
            bands: this.bands.map(band => ({
                type: band.type,
                frequency: band.frequency,
                gain: band.gain,
                q: band.q,
                enabled: band.enabled
            })),
            outputGain: this.outputGain,
            bypass: this.bypass
        };

        const json = JSON.stringify(settings, null, 2);
        console.log('EQ Settings:', json);
        
        // Copy to clipboard if available
        if (navigator.clipboard) {
            navigator.clipboard.writeText(json).then(() => {
                alert('EQ settings copied to clipboard!');
            });
        } else {
            alert('EQ settings logged to console');
        }
    }

    public getBands(): EQBand[] {
        return this.bands;
    }

    public setBand(index: number, band: Partial<EQBand>): void {
        if (index >= 0 && index < this.bands.length) {
            Object.assign(this.bands[index], band);
            this.updateControlValues(index);
            this.render();
        }
    }
}

// Initialize the editor when DOM is loaded
document.addEventListener('DOMContentLoaded', () => {
    const editor = new EQEditor('eqCanvas', 4);
    
    // Make it globally accessible for debugging
    (window as any).eqEditor = editor;
});
