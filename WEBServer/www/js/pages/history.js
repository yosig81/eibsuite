App.registerPage('history', {
    render: function(container, hash) {
        // Check if specific address requested: #/history/1/2/3
        var parts = hash.replace('#/history', '').replace(/^\//, '');

        if (parts.length > 0) {
            this.renderFunction(container, decodeURIComponent(parts));
            return;
        }

        container.innerHTML =
            '<div class="card">' +
                '<h2>Global History</h2>' +
                '<div class="toolbar">' +
                    '<div class="form-inline">' +
                        '<div class="form-group">' +
                            '<label>Lookup Address</label>' +
                            '<input type="text" id="hist-addr" placeholder="e.g. 1/2/3" style="width:150px;">' +
                        '</div>' +
                        '<button class="btn btn-primary" id="hist-lookup">Lookup</button>' +
                    '</div>' +
                    '<button class="btn btn-primary" id="hist-refresh">Refresh</button>' +
                '</div>' +
                '<div id="hist-loading" class="loading">Loading...</div>' +
                '<div id="hist-table"></div>' +
            '</div>';

        var self = this;
        document.getElementById('hist-refresh').addEventListener('click', function() { self.loadGlobal(); });
        document.getElementById('hist-lookup').addEventListener('click', function() {
            var addr = document.getElementById('hist-addr').value.trim();
            if (addr) window.location.hash = '#/history/' + encodeURIComponent(addr);
        });

        this.loadGlobal();
    },

    loadGlobal: function() {
        var loading = document.getElementById('hist-loading');
        var tableDiv = document.getElementById('hist-table');
        if (loading) loading.style.display = 'block';

        App.api('GET', '/api/history').then(function(data) {
            if (loading) loading.style.display = 'none';
            if (!tableDiv) return;

            if (data.error) {
                tableDiv.innerHTML = '<div class="error-msg">' + data.error + '</div>';
                return;
            }

            if (!data.records || data.records.length === 0) {
                tableDiv.innerHTML = '<div class="info-msg">No history data available.</div>';
                return;
            }

            var html = '<table class="data-table"><thead><tr>' +
                '<th>Address</th><th>Last Value</th><th>Last Time</th><th>Entries</th><th></th>' +
                '</tr></thead><tbody>';

            for (var i = 0; i < data.records.length; i++) {
                var r = data.records[i];
                var lastEntry = r.entries && r.entries.length > 0 ? r.entries[r.entries.length - 1] : null;
                html += '<tr>' +
                    '<td class="mono">' + r.address + '</td>' +
                    '<td class="mono">' + (lastEntry ? lastEntry.value : '-') + '</td>' +
                    '<td>' + (lastEntry ? lastEntry.time : '-') + '</td>' +
                    '<td>' + (r.entries ? r.entries.length : 0) + '</td>' +
                    '<td><a href="#/history/' + encodeURIComponent(r.address) + '" class="btn btn-small btn-primary">Details</a></td>' +
                    '</tr>';
            }
            html += '</tbody></table>';
            tableDiv.innerHTML = html;
        }).catch(function(err) {
            if (loading) loading.style.display = 'none';
            if (tableDiv) tableDiv.innerHTML = '<div class="error-msg">' + err.message + '</div>';
        });
    },

    renderFunction: function(container, address) {
        container.innerHTML =
            '<div class="card">' +
                '<h2>History: ' + address + '</h2>' +
                '<div class="toolbar">' +
                    '<a href="#/history" class="btn btn-small">&larr; Back to Global</a>' +
                    '<button class="btn btn-primary" id="fhist-refresh">Refresh</button>' +
                '</div>' +
                '<div id="fhist-loading" class="loading">Loading...</div>' +
                '<div id="fhist-table"></div>' +
            '</div>';

        var self = this;
        document.getElementById('fhist-refresh').addEventListener('click', function() { self.loadFunction(address); });
        this.loadFunction(address);
    },

    loadFunction: function(address) {
        var loading = document.getElementById('fhist-loading');
        var tableDiv = document.getElementById('fhist-table');
        if (loading) loading.style.display = 'block';

        App.api('GET', '/api/history/' + encodeURIComponent(address)).then(function(data) {
            if (loading) loading.style.display = 'none';
            if (!tableDiv) return;

            if (data.error) {
                tableDiv.innerHTML = '<div class="error-msg">' + data.error + '</div>';
                return;
            }

            if (!data.entries || data.entries.length === 0) {
                tableDiv.innerHTML = '<div class="info-msg">No entries found for this address.</div>';
                return;
            }

            var html = '<table class="data-table"><thead><tr>' +
                '<th>Time</th><th>Value</th>' +
                '</tr></thead><tbody>';

            for (var i = data.entries.length - 1; i >= 0; i--) {
                html += '<tr>' +
                    '<td>' + data.entries[i].time + '</td>' +
                    '<td class="mono">' + data.entries[i].value + '</td>' +
                    '</tr>';
            }
            html += '</tbody></table>';
            tableDiv.innerHTML = html;
        }).catch(function(err) {
            if (loading) loading.style.display = 'none';
            if (tableDiv) tableDiv.innerHTML = '<div class="error-msg">' + err.message + '</div>';
        });
    }
});
