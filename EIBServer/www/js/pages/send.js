App.registerPage('send', {
    render: function(container) {
        container.innerHTML =
            '<div class="card">' +
                '<h2>Send EIB Command</h2>' +
                '<form id="send-form">' +
                    '<div class="form-group">' +
                        '<label for="send-addr">Destination Address</label>' +
                        '<input type="text" id="send-addr" placeholder="e.g. 1/2/3" required>' +
                    '</div>' +
                    '<div class="form-group">' +
                        '<label for="send-value">APCI Value (hex)</label>' +
                        '<input type="text" id="send-value" placeholder="0x01" value="0x" required>' +
                    '</div>' +
                    '<div id="send-result" style="display:none;"></div>' +
                    '<button type="submit" class="btn btn-success">Send</button>' +
                '</form>' +
            '</div>';

        document.getElementById('send-form').addEventListener('submit', function(e) {
            e.preventDefault();
            var addr = document.getElementById('send-addr').value.trim();
            var val = document.getElementById('send-value').value.trim();
            var resultEl = document.getElementById('send-result');

            resultEl.style.display = 'none';

            App.api('POST', '/api/eib/send', { address: addr, value: val }).then(function(data) {
                if (data.error) {
                    resultEl.className = 'error-msg';
                    resultEl.textContent = data.error;
                } else {
                    resultEl.className = 'success-msg';
                    resultEl.textContent = 'Command sent successfully to ' + addr;
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
