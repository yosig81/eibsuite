App.registerPage('dashboard', {
    _timer: null,

    render: function(container) {
        container.innerHTML =
            '<h2>Dashboard</h2>' +
            '<div class="stats-grid" id="dash-stats">' +
                '<div class="stat-card"><div class="stat-value" id="dash-eib-status">--</div><div class="stat-label">EIB Server</div></div>' +
                '<div class="stat-card"><div class="stat-value" id="dash-addresses">--</div><div class="stat-label">Known Addresses</div></div>' +
                '<div class="stat-card"><div class="stat-value" id="dash-iface-status">--</div><div class="stat-label">EIBNet/IP Device</div></div>' +
            '</div>' +
            '<div class="card">' +
                '<h2>Quick Actions</h2>' +
                '<div style="display:flex;gap:12px;flex-wrap:wrap;">' +
                    '<a href="#/history" class="btn btn-primary">View History</a>' +
                    '<a href="#/send" class="btn btn-success">Send Command</a>' +
                    '<a href="#/busmon" class="btn btn-warning">Bus Monitor</a>' +
                '</div>' +
            '</div>';

        this.refresh();
    },

    refresh: function() {
        // Get interface status
        App.api('GET', '/api/admin/interface').then(function(data) {
            var el = document.getElementById('dash-iface-status');
            if (el) {
                if (data.error) {
                    el.textContent = '?';
                } else {
                    var running = data.EIB_INTERFACE_RUNNING_STATUS;
                    el.textContent = running === 'true' || running === 'True' ? 'Running' : 'Stopped';
                    el.style.color = running === 'true' || running === 'True' ? '#27ae60' : '#e74c3c';
                }
            }
        }).catch(function() {});

        // Get history to count addresses
        App.api('GET', '/api/history').then(function(data) {
            var el = document.getElementById('dash-addresses');
            var sel = document.getElementById('dash-eib-status');
            if (el && data.records) {
                el.textContent = data.records.length;
            }
            if (sel) {
                if (data.error) {
                    sel.textContent = 'Offline';
                    sel.style.color = '#e74c3c';
                } else {
                    sel.textContent = 'Online';
                    sel.style.color = '#27ae60';
                }
            }
        }).catch(function() {
            var sel = document.getElementById('dash-eib-status');
            if (sel) { sel.textContent = 'Offline'; sel.style.color = '#e74c3c'; }
        });
    },

    destroy: function() {
        if (this._timer) { clearInterval(this._timer); this._timer = null; }
    }
});
