App.registerPage('busmon', {
    _conf: null,
    _timer: null,
    _autoRefresh: false,

    render: function(container) {
        container.innerHTML =
            '<div class="card">' +
                '<h2>Bus Monitor</h2>' +
                '<div class="toolbar">' +
                    '<div>' +
                        '<button class="btn btn-primary" id="busmon-refresh">Refresh</button> ' +
                        '<label style="margin-left:12px;">' +
                            '<input type="checkbox" id="busmon-auto"> Auto-refresh (5s)' +
                        '</label>' +
                    '</div>' +
                '</div>' +
                '<div id="busmon-msg" style="display:none;"></div>' +
                '<div id="busmon-loading" class="loading">Loading...</div>' +
                '<div id="busmon-table"></div>' +
            '</div>' +
            '<div class="card" id="busmon-detail" style="display:none;">' +
                '<h2>Address Details</h2>' +
                '<table class="data-table">' +
                    '<tbody>' +
                        '<tr><th>Address</th><td id="bm-addr">--</td></tr>' +
                        '<tr><th>Type</th><td id="bm-type">--</td></tr>' +
                        '<tr><th>Last Seen</th><td id="bm-time">--</td></tr>' +
                        '<tr><th>Last Value</th><td id="bm-value" class="mono">--</td></tr>' +
                        '<tr><th>Total Count</th><td id="bm-count">--</td></tr>' +
                    '</tbody>' +
                '</table>' +
                '<div style="margin-top:12px;">' +
                    '<button class="btn btn-warning" id="bm-replay" disabled>Replay Last Packet</button>' +
                '</div>' +
                '<div id="bm-send-result" style="display:none;margin-top:8px;"></div>' +
            '</div>';

        var self = this;
        document.getElementById('busmon-refresh').addEventListener('click', function() { self.load(); });
        document.getElementById('busmon-auto').addEventListener('change', function() {
            self._autoRefresh = this.checked;
            if (self._autoRefresh) {
                self._timer = setInterval(function() { self.load(); }, 5000);
            } else if (self._timer) {
                clearInterval(self._timer);
                self._timer = null;
            }
        });
        document.getElementById('bm-replay').addEventListener('click', function() { self.replayPacket(); });

        // Event delegation for address selection
        document.getElementById('busmon-table').addEventListener('click', function(e) {
            var row = e.target.closest('tr[data-index]');
            if (row) {
                self.selectAddress(parseInt(row.getAttribute('data-index')));
            }
        });

        this.load();
    },

    load: function() {
        var self = this;
        var loading = document.getElementById('busmon-loading');
        if (loading) loading.style.display = 'block';

        App.api('GET', '/api/admin/busmon').then(function(data) {
            if (loading) loading.style.display = 'none';
            if (data.error) {
                self.showMsg(data.error, 'error-msg');
                return;
            }
            self._conf = data;
            self.renderTable();
        }).catch(function(err) {
            if (loading) loading.style.display = 'none';
            self.showMsg(err.message, 'error-msg');
        });
    },

    getAddressList: function() {
        if (!this._conf) return [];
        var list = this._conf.EIB_BUS_MON_ADDRESSES_LIST;
        if (!list) return [];
        var addrs = list.EIB_BUS_MON_ADDRESS;
        if (!addrs) return [];
        return Array.isArray(addrs) ? addrs : [addrs];
    },

    renderTable: function() {
        var tableDiv = document.getElementById('busmon-table');
        if (!tableDiv) return;

        var addresses = this.getAddressList();

        if (addresses.length === 0) {
            tableDiv.innerHTML = '<div class="info-msg">No addresses found. The bus monitor will populate as EIB traffic is detected.</div>';
            return;
        }

        var html = '<table class="data-table"><thead><tr>' +
            '<th>Address</th><th>Type</th><th>Last Seen</th><th>Last Value</th><th>Count</th>' +
            '</tr></thead><tbody>';

        for (var i = 0; i < addresses.length; i++) {
            var a = addresses[i];
            var addr = a.EIB_BUS_MON_ADDRESS_STR || '--';
            var isGroup = a.EIB_BUS_MON_IS_ADDRESS_LOGICAL === 'true' || a.EIB_BUS_MON_IS_ADDRESS_LOGICAL === 'True';
            var time = a.EIB_BUS_MON_ADDR_LAST_RECVED_TIME || 'Never';
            var value = a.EIB_BUS_MON_LAST_ADDR_VALUE || '0x0';
            var count = a.EIB_BUS_MON_ADDRESSES_COUNT || '0';

            html += '<tr data-index="' + i + '" style="cursor:pointer;">' +
                '<td class="mono">' + addr + '</td>' +
                '<td>' + (isGroup ? 'Group' : 'Physical') + '</td>' +
                '<td>' + time + '</td>' +
                '<td class="mono">' + value + '</td>' +
                '<td>' + count + '</td>' +
                '</tr>';
        }
        html += '</tbody></table>';
        tableDiv.innerHTML = html;
    },

    selectAddress: function(index) {
        var addresses = this.getAddressList();
        if (index >= addresses.length) return;
        var a = addresses[index];

        this._selectedIndex = index;
        var detailDiv = document.getElementById('busmon-detail');
        if (detailDiv) detailDiv.style.display = 'block';

        this.setText('bm-addr', a.EIB_BUS_MON_ADDRESS_STR || '--');
        this.setText('bm-type', (a.EIB_BUS_MON_IS_ADDRESS_LOGICAL === 'true' || a.EIB_BUS_MON_IS_ADDRESS_LOGICAL === 'True') ? 'Group' : 'Physical');
        this.setText('bm-time', a.EIB_BUS_MON_ADDR_LAST_RECVED_TIME || 'Never');
        this.setText('bm-value', a.EIB_BUS_MON_LAST_ADDR_VALUE || '0x0');
        this.setText('bm-count', a.EIB_BUS_MON_ADDRESSES_COUNT || '0');

        var replayBtn = document.getElementById('bm-replay');
        if (replayBtn) {
            if (!App.state.admin) {
                replayBtn.style.display = 'none';
            } else {
                replayBtn.style.display = '';
                replayBtn.disabled = false;
            }
        }

        // Highlight selected row
        var rows = document.querySelectorAll('#busmon-table tr[data-index]');
        for (var i = 0; i < rows.length; i++) {
            rows[i].style.background = i === index ? '#e8f4fd' : '';
        }
    },

    replayPacket: function() {
        var addresses = this.getAddressList();
        if (this._selectedIndex === undefined || this._selectedIndex >= addresses.length) return;
        var a = addresses[this._selectedIndex];

        if (!confirm('Replay last packet to ' + (a.EIB_BUS_MON_ADDRESS_STR || '') + '?')) return;

        var self = this;
        var resultEl = document.getElementById('bm-send-result');

        App.api('POST', '/api/admin/busmon/send', {
            address: a.EIB_BUS_MON_ADDRESS_STR || '',
            value: a.EIB_BUS_MON_LAST_ADDR_VALUE || '0x0',
            mode: '3'
        }).then(function(data) {
            if (data.error) {
                if (resultEl) { resultEl.className = 'error-msg'; resultEl.textContent = data.error; resultEl.style.display = 'block'; }
            } else {
                if (resultEl) { resultEl.className = 'success-msg'; resultEl.textContent = 'Packet sent.'; resultEl.style.display = 'block'; }
            }
            setTimeout(function() { if (resultEl) resultEl.style.display = 'none'; }, 3000);
        }).catch(function(err) {
            if (resultEl) { resultEl.className = 'error-msg'; resultEl.textContent = err.message; resultEl.style.display = 'block'; }
        });
    },

    setText: function(id, text) {
        var el = document.getElementById(id);
        if (el) el.textContent = text;
    },

    showMsg: function(msg, cls) {
        var el = document.getElementById('busmon-msg');
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
