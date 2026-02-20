App.registerPage('schedule', {
    render: function(container) {
        container.innerHTML =
            '<div class="card">' +
                '<h2>Schedule EIB Command</h2>' +
                '<form id="sched-form">' +
                    '<div class="form-group">' +
                        '<label for="sched-addr">Destination Address</label>' +
                        '<input type="text" id="sched-addr" placeholder="e.g. 1/2/3" required>' +
                    '</div>' +
                    '<div class="form-group">' +
                        '<label for="sched-value">APCI Value (hex)</label>' +
                        '<input type="text" id="sched-value" placeholder="0x01" value="0x" required>' +
                    '</div>' +
                    '<div class="form-group">' +
                        '<label for="sched-datetime">Date/Time</label>' +
                        '<input type="datetime-local" id="sched-datetime" required>' +
                    '</div>' +
                    '<div id="sched-result" style="display:none;"></div>' +
                    '<button type="submit" class="btn btn-success">Schedule</button>' +
                '</form>' +
            '</div>';

        // Set default datetime to 1 hour from now
        var now = new Date();
        now.setHours(now.getHours() + 1);
        var dtLocal = now.toISOString().slice(0, 16);
        document.getElementById('sched-datetime').value = dtLocal;

        document.getElementById('sched-form').addEventListener('submit', function(e) {
            e.preventDefault();
            var addr = document.getElementById('sched-addr').value.trim();
            var val = document.getElementById('sched-value').value.trim();
            var dt = document.getElementById('sched-datetime').value;
            var resultEl = document.getElementById('sched-result');

            // Convert datetime-local to the format expected by CTime
            // CTime expects "dd MMM yyyy HH:mm:ss" format
            var d = new Date(dt);
            var months = ['Jan','Feb','Mar','Apr','May','Jun','Jul','Aug','Sep','Oct','Nov','Dec'];
            var formatted = ('0' + d.getDate()).slice(-2) + ' ' +
                           months[d.getMonth()] + ' ' +
                           d.getFullYear() + ' ' +
                           ('0' + d.getHours()).slice(-2) + ':' +
                           ('0' + d.getMinutes()).slice(-2) + ':' +
                           ('0' + d.getSeconds()).slice(-2);

            resultEl.style.display = 'none';

            App.api('POST', '/api/eib/schedule', { address: addr, value: val, datetime: formatted }).then(function(data) {
                if (data.error) {
                    resultEl.className = 'error-msg';
                    resultEl.textContent = data.error;
                } else {
                    resultEl.className = 'success-msg';
                    resultEl.textContent = 'Command scheduled for ' + formatted;
                }
                resultEl.style.display = 'block';
            }).catch(function(err) {
                resultEl.className = 'error-msg';
                resultEl.textContent = err.message;
                resultEl.style.display = 'block';
            });
        });
    }
});
