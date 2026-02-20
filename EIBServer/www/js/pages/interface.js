App.registerPage('interface', {
    _conf: null,
    _timer: null,

    render: function(container) {
        container.innerHTML =
            '<div class="card">' +
                '<h2>EIBNet/IP Device Control</h2>' +
                '<div class="toolbar">' +
                    '<button class="btn btn-primary" id="iface-refresh">Refresh</button>' +
                '</div>' +
                '<div id="iface-msg" style="display:none;"></div>' +
                '<div id="iface-loading" class="loading">Loading...</div>' +
                '<div id="iface-content" style="display:none;">' +
                    '<div class="stats-grid">' +
                        '<div class="stat-card">' +
                            '<div class="stat-value" id="iface-status-val">--</div>' +
                            '<div class="stat-label">Status</div>' +
                        '</div>' +
                        '<div class="stat-card">' +
                            '<div class="stat-value" id="iface-mode-val">--</div>' +
                            '<div class="stat-label">Mode</div>' +
                        '</div>' +
                        '<div class="stat-card">' +
                            '<div class="stat-value" id="iface-sent-val">--</div>' +
                            '<div class="stat-label">Packets Sent</div>' +
                        '</div>' +
                        '<div class="stat-card">' +
                            '<div class="stat-value" id="iface-recv-val">--</div>' +
                            '<div class="stat-label">Packets Received</div>' +
                        '</div>' +
                    '</div>' +
                    '<table class="data-table" id="iface-details">' +
                        '<tbody>' +
                            '<tr><th>Address</th><td id="iface-addr">--</td></tr>' +
                            '<tr><th>Port</th><td id="iface-port">--</td></tr>' +
                            '<tr><th>Auto Detect</th><td id="iface-autodetect">--</td></tr>' +
                            '<tr><th>Last Packet Sent</th><td id="iface-last-sent">--</td></tr>' +
                            '<tr><th>Last Packet Received</th><td id="iface-last-recv">--</td></tr>' +
                        '</tbody>' +
                    '</table>' +
                    '<div id="iface-device" style="display:none;margin-top:20px;">' +
                        '<h3 style="margin-bottom:12px;">Device Information</h3>' +
                        '<table class="data-table">' +
                            '<tbody>' +
                                '<tr><th>Name</th><td id="dev-name">--</td></tr>' +
                                '<tr><th>MAC Address</th><td id="dev-mac">--</td></tr>' +
                                '<tr><th>Physical Address</th><td id="dev-phy">--</td></tr>' +
                                '<tr><th>Serial Number</th><td id="dev-serial">--</td></tr>' +
                                '<tr><th>Multicast Address</th><td id="dev-mcast">--</td></tr>' +
                            '</tbody>' +
                        '</table>' +
                    '</div>' +
                    '<div style="margin-top:20px;">' +
                        '<button class="btn btn-success" id="iface-start" disabled>Start Device</button> ' +
                        '<button class="btn btn-danger" id="iface-stop" disabled>Stop Device</button>' +
                    '</div>' +
                '</div>' +
            '</div>';

        var self = this;
        document.getElementById('iface-refresh').addEventListener('click', function() { self.load(); });
        document.getElementById('iface-start').addEventListener('click', function() { self.startInterface(); });
        document.getElementById('iface-stop').addEventListener('click', function() { self.stopInterface(); });

        this.load();
    },

    load: function() {
        var self = this;
        var loading = document.getElementById('iface-loading');
        var content = document.getElementById('iface-content');
        if (loading) loading.style.display = 'block';
        if (content) content.style.display = 'none';

        App.api('GET', '/api/admin/interface').then(function(data) {
            if (loading) loading.style.display = 'none';
            if (data.error) {
                self.showMsg(data.error, 'error-msg');
                return;
            }
            self._conf = data;
            self.updateView();
        }).catch(function(err) {
            if (loading) loading.style.display = 'none';
            self.showMsg(err.message, 'error-msg');
        });
    },

    updateView: function() {
        var d = this._conf;
        if (!d) return;

        var content = document.getElementById('iface-content');
        if (content) content.style.display = 'block';

        var running = d.EIB_INTERFACE_RUNNING_STATUS === 'true' || d.EIB_INTERFACE_RUNNING_STATUS === 'True';
        var el = document.getElementById('iface-status-val');
        if (el) {
            el.textContent = running ? 'Running' : 'Stopped';
            el.style.color = running ? '#27ae60' : '#e74c3c';
        }

        var modes = { '0': 'Routing', '1': 'Tunneling', 'MODE_ROUTING': 'Routing', 'MODE_TUNNELING': 'Tunneling' };
        var modeEl = document.getElementById('iface-mode-val');
        if (modeEl) modeEl.textContent = modes[d.EIB_INTERFACE_MODE] || d.EIB_INTERFACE_MODE || '--';

        this.setText('iface-sent-val', d.EIB_INTERFACE_TOTAL_PACKETS_SENT || '0');
        this.setText('iface-recv-val', d.EIB_INTERFACE_TOTAL_PACKETS_RECEIVED || '0');
        this.setText('iface-addr', d.EIB_INTERFACE_ADDRESS || '--');
        this.setText('iface-port', d.EIB_INTERFACE_PORT || '--');
        this.setText('iface-autodetect', d.EIB_INTERFACE_AUTO_DETECT === 'true' || d.EIB_INTERFACE_AUTO_DETECT === 'True' ? 'Yes' : 'No');
        this.setText('iface-last-sent', d.EIB_INTERFACE_LAST_TIME_PACKET_SENT || 'Never');
        this.setText('iface-last-recv', d.EIB_INTERFACE_LAST_TIME_PACKET_RECEIVED || 'Never');

        var startBtn = document.getElementById('iface-start');
        var stopBtn = document.getElementById('iface-stop');
        if (!App.state.admin) {
            if (startBtn) startBtn.style.display = 'none';
            if (stopBtn) stopBtn.style.display = 'none';
        } else {
            if (startBtn) { startBtn.style.display = ''; startBtn.disabled = running; }
            if (stopBtn) { stopBtn.style.display = ''; stopBtn.disabled = !running; }
        }

        // Device info
        var devInfo = d.EIB_INTERFACE_DEV_DESCRIPTION;
        if (devInfo && typeof devInfo === 'object') {
            var devDiv = document.getElementById('iface-device');
            if (devDiv) devDiv.style.display = 'block';
            this.setText('dev-name', devInfo.EIB_INTERFACE_DEV_NAME || '--');
            this.setText('dev-mac', devInfo.EIB_INTERFACE_DEV_MAC_ADDRESS || '--');
            this.setText('dev-phy', devInfo.EIB_INTERFACE_DEV_PHY_ADDRESS || '--');
            this.setText('dev-serial', devInfo.EIB_INTERFACE_DEV_SERIAL_NUMBER || '--');
            this.setText('dev-mcast', devInfo.EIB_INTERFACE_DEV_MULTICAST_ADDRESS || '--');
        }
    },

    setText: function(id, text) {
        var el = document.getElementById(id);
        if (el) el.textContent = text;
    },

    startInterface: function() {
        if (!confirm('Start EIBNet/IP Device?')) return;
        var self = this;
        App.api('POST', '/api/admin/interface/start').then(function(data) {
            if (data.error) {
                self.showMsg(data.error, 'error-msg');
            } else {
                self._conf = data;
                self.updateView();
                self.showMsg('Device started.', 'success-msg');
            }
        }).catch(function(err) {
            self.showMsg(err.message, 'error-msg');
        });
    },

    stopInterface: function() {
        if (!confirm('Stop EIBNet/IP Device?')) return;
        var self = this;
        App.api('POST', '/api/admin/interface/stop').then(function(data) {
            if (data.error) {
                self.showMsg(data.error, 'error-msg');
            } else {
                self._conf = data;
                self.updateView();
                self.showMsg('Device stopped.', 'success-msg');
            }
        }).catch(function(err) {
            self.showMsg(err.message, 'error-msg');
        });
    },

    showMsg: function(msg, cls) {
        var el = document.getElementById('iface-msg');
        if (el) {
            el.className = cls;
            el.textContent = msg;
            el.style.display = 'block';
            setTimeout(function() { el.style.display = 'none'; }, 5000);
        }
    },

    destroy: function() {
        if (this._timer) { clearInterval(this._timer); this._timer = null; }
    }
});
