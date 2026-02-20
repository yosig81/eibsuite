// EIB Suite SPA - Main Application
var App = (function() {
    var state = {
        authenticated: false,
        user: '',
        read: false,
        write: false,
        admin: false
    };

    var pages = {};
    var currentPage = null;

    function api(method, url, body) {
        var opts = {
            method: method,
            credentials: 'same-origin',
            headers: {}
        };
        if (body) {
            opts.headers['Content-Type'] = 'application/json';
            opts.body = JSON.stringify(body);
        }
        return fetch(url, opts).then(function(resp) {
            if (resp.status === 401) {
                state.authenticated = false;
                showLogin();
                throw new Error('Not authenticated');
            }
            return resp.json();
        });
    }

    function registerPage(name, page) {
        pages[name] = page;
    }

    function navigate(hash) {
        if (!hash || hash === '#/' || hash === '#') hash = '#/dashboard';
        var route = hash.replace('#/', '').split('/')[0];

        // Update active nav link
        var links = document.querySelectorAll('.nav-link');
        for (var i = 0; i < links.length; i++) {
            links[i].classList.remove('active');
            if (links[i].getAttribute('data-page') === route) {
                links[i].classList.add('active');
            }
        }

        var container = document.getElementById('page-content');
        if (pages[route]) {
            if (currentPage && pages[currentPage] && pages[currentPage].destroy) {
                pages[currentPage].destroy();
            }
            currentPage = route;
            pages[route].render(container, hash);
        } else {
            container.innerHTML = '<div class="card"><h2>Page Not Found</h2><p>The page "' + route + '" does not exist.</p></div>';
        }
    }

    function showLogin() {
        document.getElementById('login-view').style.display = 'block';
        document.getElementById('main-view').style.display = 'none';
    }

    function showMain() {
        document.getElementById('login-view').style.display = 'none';
        document.getElementById('main-view').style.display = 'flex';
        document.getElementById('user-info').textContent = state.user;
        navigate(window.location.hash);
    }

    function checkSession() {
        api('GET', '/api/session').then(function(data) {
            if (data.authenticated) {
                state.authenticated = true;
                state.user = data.user;
                state.read = data.read;
                state.write = data.write;
                state.admin = data.admin;
                showMain();
            } else {
                showLogin();
            }
        }).catch(function() {
            showLogin();
        });
    }

    function login(user, pass) {
        return api('POST', '/api/login', { user: user, pass: pass }).then(function(data) {
            if (data.status === 'ok') {
                state.authenticated = true;
                state.user = data.user;
                state.read = data.read;
                state.write = data.write;
                state.admin = data.admin;
                showMain();
                return true;
            }
            return false;
        });
    }

    function logout() {
        api('POST', '/api/logout').then(function() {
            state.authenticated = false;
            state.user = '';
            showLogin();
        }).catch(function() {
            state.authenticated = false;
            showLogin();
        });
    }

    function init() {
        // Hash change routing
        window.addEventListener('hashchange', function() {
            if (state.authenticated) {
                navigate(window.location.hash);
            }
        });

        // Login form
        document.getElementById('login-form').addEventListener('submit', function(e) {
            e.preventDefault();
            var user = document.getElementById('login-user').value;
            var pass = document.getElementById('login-pass').value;
            var errEl = document.getElementById('login-error');
            errEl.style.display = 'none';

            login(user, pass).then(function(ok) {
                if (!ok) {
                    errEl.textContent = 'Invalid credentials';
                    errEl.style.display = 'block';
                }
            }).catch(function(err) {
                errEl.textContent = err.message || 'Login failed';
                errEl.style.display = 'block';
            });
        });

        // Logout button
        document.getElementById('btn-logout').addEventListener('click', logout);

        // Check existing session
        checkSession();
    }

    // Initialize when DOM is ready
    if (document.readyState === 'loading') {
        document.addEventListener('DOMContentLoaded', init);
    } else {
        init();
    }

    return {
        api: api,
        registerPage: registerPage,
        state: state
    };
})();
