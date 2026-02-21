App.registerPage('users', {
    _users: null,
    _editIndex: -1,

    render: function(container) {
        container.innerHTML =
            '<div class="card">' +
                '<h2>User Management</h2>' +
                '<div class="toolbar">' +
                    '<div>' +
                        '<button class="btn btn-primary" id="users-refresh">Refresh</button> ' +
                        '<button class="btn btn-success" id="users-add">Add User</button>' +
                    '</div>' +
                '</div>' +
                '<div id="users-msg" style="display:none;"></div>' +
                '<div id="users-loading" class="loading">Loading...</div>' +
                '<div id="users-table"></div>' +
            '</div>' +
            '<div id="user-dialog" style="display:none;position:fixed;top:0;left:0;right:0;bottom:0;background:rgba(0,0,0,0.5);z-index:100;">' +
                '<div class="card" style="max-width:400px;margin:60px auto;">' +
                    '<h2 id="user-dlg-title">Add User</h2>' +
                    '<form id="user-form">' +
                        '<div class="form-group">' +
                            '<label>Username</label>' +
                            '<input type="text" id="user-name" required>' +
                        '</div>' +
                        '<div class="form-group">' +
                            '<label>Password</label>' +
                            '<input type="password" id="user-pass">' +
                        '</div>' +
                        '<div class="form-group">' +
                            '<label><input type="checkbox" id="user-read"> Read Access</label>' +
                        '</div>' +
                        '<div class="form-group">' +
                            '<label><input type="checkbox" id="user-write"> Write Access</label>' +
                        '</div>' +
                        '<div class="form-group">' +
                            '<label><input type="checkbox" id="user-web"> Web Access</label>' +
                        '</div>' +
                        '<div class="form-group">' +
                            '<label><input type="checkbox" id="user-admin"> Admin Access</label>' +
                        '</div>' +
                        '<div class="form-group">' +
                            '<label>Source Mask (hex)</label>' +
                            '<input type="text" id="user-srcmask" value="0xFFFF">' +
                        '</div>' +
                        '<div class="form-group">' +
                            '<label>Destination Mask (hex)</label>' +
                            '<input type="text" id="user-dstmask" value="0xFFFF">' +
                        '</div>' +
                        '<button type="submit" class="btn btn-primary">OK</button> ' +
                        '<button type="button" class="btn" id="user-cancel">Cancel</button>' +
                    '</form>' +
                '</div>' +
            '</div>';

        var self = this;
        document.getElementById('users-refresh').addEventListener('click', function() { self.load(); });
        document.getElementById('users-add').addEventListener('click', function() { self.showAddDialog(); });
        document.getElementById('user-cancel').addEventListener('click', function() { self.hideDialog(); });
        document.getElementById('user-form').addEventListener('submit', function(e) {
            e.preventDefault();
            self.onDialogSubmit();
        });

        // Event delegation for edit/delete buttons
        document.getElementById('users-table').addEventListener('click', function(e) {
            var btn = e.target;
            if (btn.getAttribute('data-action') === 'edit') {
                self.editUser(parseInt(btn.getAttribute('data-index')));
            } else if (btn.getAttribute('data-action') === 'delete') {
                self.deleteUser(parseInt(btn.getAttribute('data-index')));
            }
        });

        this.load();
    },

    load: function() {
        var self = this;
        var loading = document.getElementById('users-loading');
        if (loading) loading.style.display = 'block';
        var tableDiv = document.getElementById('users-table');
        if (tableDiv) tableDiv.innerHTML = '';

        App.api('GET', '/api/admin/users').then(function(data) {
            if (loading) loading.style.display = 'none';
            if (data.error) {
                self.showMsg(data.error, 'error-msg');
                return;
            }
            self._users = data;
            self.renderTable();
        }).catch(function(err) {
            if (loading) loading.style.display = 'none';
            self.showMsg(err.message, 'error-msg');
        });
    },

    getUserList: function() {
        if (!this._users) this._users = {};
        if (!this._users.EIB_SERVER_USERS_LIST) this._users.EIB_SERVER_USERS_LIST = {};
        var list = this._users.EIB_SERVER_USERS_LIST;
        if (!list.EIB_SERVER_USER) list.EIB_SERVER_USER = [];
        if (!Array.isArray(list.EIB_SERVER_USER)) {
            list.EIB_SERVER_USER = [list.EIB_SERVER_USER];
        }
        return list.EIB_SERVER_USER;
    },

    renderTable: function() {
        var tableDiv = document.getElementById('users-table');
        if (!tableDiv) return;

        var userList = this.getUserList();

        if (userList.length === 0) {
            tableDiv.innerHTML = '<div class="info-msg">No users found.</div>';
            return;
        }

        var html = '<table class="data-table"><thead><tr>' +
            '<th>Name</th><th>IP Address</th><th>Connected</th><th>Privileges</th><th>Actions</th>' +
            '</tr></thead><tbody>';

        for (var i = 0; i < userList.length; i++) {
            var u = userList[i];
            var name = u.EIB_SERVER_USER_NAME || '';
            var ip = u.EIB_SERVER_USER_IP_ADDRESS || '0.0.0.0';
            var connected = u.EIB_SERVER_USER_IS_CONNECTED || 'false';
            var privs = parseInt(u.EIB_SERVER_USER_PRIVILIGES || '0');
            var privStr = [];
            if (privs & 1) privStr.push('Read');
            if (privs & 2) privStr.push('Write');
            if (privs & 4) privStr.push('Web');
            if (privs & 8) privStr.push('Admin');

            var isConnected = connected === 'true' || connected === 'True';
            html += '<tr>' +
                '<td>' + name + '</td>' +
                '<td class="mono">' + ip + '</td>' +
                '<td><span class="status-badge ' + (isConnected ? 'status-connected' : 'status-disconnected') + '">' +
                    (isConnected ? 'Yes' : 'No') + '</span></td>' +
                '<td>' + (privStr.length > 0 ? privStr.join(', ') : 'None') + '</td>' +
                '<td>' +
                    '<button class="btn btn-small btn-primary" data-action="edit" data-index="' + i + '">Edit</button> ' +
                    '<button class="btn btn-small btn-danger" data-action="delete" data-index="' + i + '">Delete</button>' +
                '</td>' +
                '</tr>';
        }
        html += '</tbody></table>';
        tableDiv.innerHTML = html;
    },

    showAddDialog: function() {
        this._editIndex = -1;
        document.getElementById('user-dlg-title').textContent = 'Add User';
        document.getElementById('user-name').value = '';
        document.getElementById('user-pass').value = '';
        document.getElementById('user-read').checked = true;
        document.getElementById('user-write').checked = false;
        document.getElementById('user-web').checked = false;
        document.getElementById('user-admin').checked = false;
        document.getElementById('user-srcmask').value = '0xFFFF';
        document.getElementById('user-dstmask').value = '0xFFFF';
        document.getElementById('user-name').disabled = false;
        document.getElementById('user-dialog').style.display = 'block';
    },

    editUser: function(index) {
        var userList = this.getUserList();
        if (index >= userList.length) return;
        var u = userList[index];
        this._editIndex = index;
        document.getElementById('user-dlg-title').textContent = 'Edit User';
        document.getElementById('user-name').value = u.EIB_SERVER_USER_NAME || '';
        document.getElementById('user-pass').value = u.EIB_SERVER_USER_PASSWORD || '';
        document.getElementById('user-name').disabled = false;
        var privs = parseInt(u.EIB_SERVER_USER_PRIVILIGES || '0');
        document.getElementById('user-read').checked = (privs & 1) !== 0;
        document.getElementById('user-write').checked = (privs & 2) !== 0;
        document.getElementById('user-web').checked = (privs & 4) !== 0;
        document.getElementById('user-admin').checked = (privs & 8) !== 0;
        document.getElementById('user-srcmask').value = '0x' + (parseInt(u.EIB_SERVER_USER_SOURCE_ADDR_MASK || '65535')).toString(16).toUpperCase();
        document.getElementById('user-dstmask').value = '0x' + (parseInt(u.EIB_SERVER_USER_DST_ADDR_MASK || '65535')).toString(16).toUpperCase();
        document.getElementById('user-dialog').style.display = 'block';
    },

    deleteUser: function(index) {
        if (!confirm('Are you sure you want to delete this user?')) return;
        var userList = this.getUserList();
        if (index >= userList.length) return;
        userList.splice(index, 1);
        this.save();
    },

    hideDialog: function() {
        document.getElementById('user-dialog').style.display = 'none';
    },

    onDialogSubmit: function() {
        var name = document.getElementById('user-name').value.trim();
        var pass = document.getElementById('user-pass').value;
        var privs = 0;
        if (document.getElementById('user-read').checked) privs |= 1;
        if (document.getElementById('user-write').checked) privs |= 2;
        if (document.getElementById('user-web').checked) privs |= 4;
        if (document.getElementById('user-admin').checked) privs |= 8;
        var srcMask = parseInt(document.getElementById('user-srcmask').value) || 65535;
        var dstMask = parseInt(document.getElementById('user-dstmask').value) || 65535;

        var user = {
            EIB_SERVER_USER_NAME: name,
            EIB_SERVER_USER_PASSWORD: pass,
            EIB_SERVER_USER_PRIVILIGES: privs.toString(),
            EIB_SERVER_USER_IP_ADDRESS: '0.0.0.0',
            EIB_SERVER_USER_IS_CONNECTED: 'false',
            EIB_SERVER_USER_SESSION_ID: '0',
            EIB_SERVER_USER_SOURCE_ADDR_MASK: srcMask.toString(),
            EIB_SERVER_USER_DST_ADDR_MASK: dstMask.toString()
        };

        var userList = this.getUserList();
        if (this._editIndex >= 0 && this._editIndex < userList.length) {
            user.EIB_SERVER_USER_IS_CONNECTED = userList[this._editIndex].EIB_SERVER_USER_IS_CONNECTED;
            user.EIB_SERVER_USER_SESSION_ID = userList[this._editIndex].EIB_SERVER_USER_SESSION_ID;
            user.EIB_SERVER_USER_IP_ADDRESS = userList[this._editIndex].EIB_SERVER_USER_IP_ADDRESS;
            userList[this._editIndex] = user;
        } else {
            userList.push(user);
        }

        this.hideDialog();
        this.save();
    },

    save: function() {
        var self = this;
        var userList = this.getUserList();
        var xml = '<?xml version="1.0" encoding="utf-8"?><Root><EIB_SERVER_USERS_LIST>';
        for (var i = 0; i < userList.length; i++) {
            var u = userList[i];
            xml += '<EIB_SERVER_USER>';
            xml += '<EIB_SERVER_USER_IP_ADDRESS>' + (u.EIB_SERVER_USER_IP_ADDRESS || '0.0.0.0') + '</EIB_SERVER_USER_IP_ADDRESS>';
            xml += '<EIB_SERVER_USER_NAME>' + (u.EIB_SERVER_USER_NAME || '') + '</EIB_SERVER_USER_NAME>';
            xml += '<EIB_SERVER_USER_PASSWORD>' + (u.EIB_SERVER_USER_PASSWORD || '') + '</EIB_SERVER_USER_PASSWORD>';
            xml += '<EIB_SERVER_USER_IS_CONNECTED>' + (u.EIB_SERVER_USER_IS_CONNECTED || 'false') + '</EIB_SERVER_USER_IS_CONNECTED>';
            xml += '<EIB_SERVER_USER_SESSION_ID>' + (u.EIB_SERVER_USER_SESSION_ID || '0') + '</EIB_SERVER_USER_SESSION_ID>';
            xml += '<EIB_SERVER_USER_PRIVILIGES>' + (u.EIB_SERVER_USER_PRIVILIGES || '0') + '</EIB_SERVER_USER_PRIVILIGES>';
            xml += '<EIB_SERVER_USER_SOURCE_ADDR_MASK>' + (u.EIB_SERVER_USER_SOURCE_ADDR_MASK || '65535') + '</EIB_SERVER_USER_SOURCE_ADDR_MASK>';
            xml += '<EIB_SERVER_USER_DST_ADDR_MASK>' + (u.EIB_SERVER_USER_DST_ADDR_MASK || '65535') + '</EIB_SERVER_USER_DST_ADDR_MASK>';
            xml += '</EIB_SERVER_USER>';
        }
        xml += '</EIB_SERVER_USERS_LIST></Root>';

        fetch('/api/admin/users', {
            method: 'POST',
            credentials: 'same-origin',
            headers: { 'Content-Type': 'text/xml' },
            body: xml
        }).then(function(resp) { return resp.json(); }).then(function(data) {
            if (data.error) {
                self.showMsg(data.error, 'error-msg');
            } else {
                self.showMsg('Users saved successfully.', 'success-msg');
                self.renderTable();
            }
        }).catch(function(err) {
            self.showMsg(err.message, 'error-msg');
        });
    },

    showMsg: function(msg, cls) {
        var el = document.getElementById('users-msg');
        if (el) {
            el.className = cls;
            el.textContent = msg;
            el.style.display = 'block';
            setTimeout(function() { el.style.display = 'none'; }, 5000);
        }
    }
});
