module.exports = async () => {
    const customTitlebar = require('custom-electron-titlebar');
    const AdmZip = require('adm-zip');
    const fs = require('fs');
    const { spawn, ChildProcess } = require('child_process');
    const { ipcRenderer, remote, shell } = require('electron');
    const getJSON = require('get-json');
    const semver = require('semver');
    const os = require('os');


    new customTitlebar.Titlebar({
        backgroundColor: customTitlebar.Color.fromHex('#000000'),
        menu: null
    });

    let scrollSwitch = true;

    function check() {
        if (scrollSwitch)
            document.getElementById('log').scrollTop = document.getElementById('log').scrollHeight;
        if (document.getElementById('log').childNodes.length <= 200)
            return;
        document.getElementById('log').removeChild(document.getElementById('log').firstChild);
    }

    function log(str) {
        document.getElementById('log').insertAdjacentHTML('beforeend', '<div class="pre-wrap">' + str + '</div>');
        check();
    }

    function extendedLog(entity, str) {
        document.getElementById('log')
            .insertAdjacentHTML('beforeend', `<div class="pre-wrap"><div class="entity">${entity}, ${new Date().toLocaleTimeString()}</div>${str}</div>`);
        check();
    }

    async function dialog(description) {
        document.querySelector('#input>label').textContent = description;
        //document.getElementById('buttons').setAttribute('hidden', '');
        document.querySelectorAll('.big-button').forEach(e => e.style.display = 'none');
        //document.getElementById('input').removeAttribute('hidden');
        document.getElementById('input').style.display = '';
        return await new Promise((resolve, reject) => {
            document.getElementById('ok').addEventListener('click', () => {
                resolve(document.getElementById('input-text').value);
                document.getElementById('input-text').value = '';
                //document.getElementById('input').setAttribute('hidden', '');
                document.getElementById('input').style.display = 'none';
                //document.getElementById('buttons').removeAttribute('hidden');
                document.querySelectorAll('.big-button').forEach(e => e.style.display = '');
            }, { once: true });
        });
    }

    function download() {
        log('Starting download.');
        ipcRenderer.send('download', {
            url: 'https://ci.appveyor.com/api/projects/antollo/starship-battle/artifacts/src/artifacts.zip?branch=master&job=Image%3A%20Visual%20Studio%202017',
            properties: {
                directory: __dirname,
                allowOverwrite: true
            }
        });
    }

    ipcRenderer.on('download progress', (event, progress) => {
        if (progress > 0 && progress < 1) {
            log(`Downloading: ${Math.round(progress * 100)}%`);
        }
    });

    ipcRenderer.on('download complete', (event, file) => {
        log('Downloaded.');
        new AdmZip(file).extractAllTo(__dirname, true);
        fs.unlink(file, err => console.log(err));
        delete require.cache[require.resolve(__dirname + '/config.json')]
        log('Game is ready.');
    });

    ipcRenderer.on('log', (_event, message) => {
        log(message);
    });

    let client, server;

    /*document.getElementById('server').addEventListener('click', async () => {
        if (server instanceof ChildProcess)
            server.kill();
        log('Server ip: ' + ip.address());
        server = spawn(__dirname + '/' + require(__dirname + '/config.json').executable, ['server'], {
            cwd: __dirname + '/'
        });

        server.stdout.on('data', (data) => {
            extendedLog('Server', data);
        });
        server.stderr.on('data', (data) => {
            extendedLog('Server error', data);
        });
    });*/

    document.getElementById('input').style.display = 'none';

    document.getElementById('client').addEventListener('click', async () => {
        if (client instanceof ChildProcess)
            client.kill();
        const ip = await dialog('Server ip:');
        client = spawn(__dirname + '/' + require(__dirname + '/config.json').executable, [
            'ip', ip,
            'port', '1717'
        ], {
            cwd: __dirname + '/'
        });

        client.stdout.on('data', (data) => {
            extendedLog('Client', data);
        });
        client.stderr.on('data', (data) => {
            extendedLog('Client error', data);
        });
    });

    document.getElementById('local').addEventListener('click', () => {

        if (server instanceof ChildProcess)
            server.kill();
        if (client instanceof ChildProcess)
            client.kill();

        server = spawn(__dirname + '/' + require(__dirname + '/config.json').executable, [
            'server', 'normal',
            'port', '1717'
        ], {
            cwd: __dirname + '/'
        });

        client = spawn(__dirname + '/' + require(__dirname + '/config.json').executable, [
            'ip', '127.0.0.1',
            'port', '1717'
        ], {
            cwd: __dirname + '/'
        });

        server.stdout.on('data', (data) => {
            extendedLog('Server', data);
        });
        server.stderr.on('data', (data) => {
            extendedLog('Server error', data);
        });

        client.stdout.on('data', (data) => {
            extendedLog('Client', data);
        });
        client.stderr.on('data', (data) => {
            extendedLog('Client error', data);
        });
    });

    document.getElementById('ranked').addEventListener('click', () => {

        if (server instanceof ChildProcess)
            server.kill();
        if (client instanceof ChildProcess)
            client.kill();

        server = spawn(__dirname + '/' + require(__dirname + '/config.json').executable, [
            'server', 'ranked',
            'port', '1717'
        ], {
            cwd: __dirname + '/'
        });

        client = spawn(__dirname + '/' + require(__dirname + '/config.json').executable, [
            'ip', '127.0.0.1',
            'port', '1717',
            'command', 'ranked'
        ], {
            cwd: __dirname + '/'
        });

        server.stdout.on('data', (data) => {
            extendedLog('Server', data);
        });
        server.stderr.on('data', (data) => {
            extendedLog('Server error', data);
        });

        client.stdout.on('data', (data) => {
            extendedLog('Client', data);
        });
        client.stderr.on('data', (data) => {
            extendedLog('Client error', data);
        });
    });

    document.getElementById('help').addEventListener('click', () => {
        shell.openExternal('https://starship-battle.herokuapp.com/');
    });

    /*document.getElementById('spaceships').addEventListener('click', async () => {
        for (spaceshipName of require(__dirname + '/config.json').spaceships) {
            const spaceship = { type: 'spaceship', name: spaceshipName, ...require(`${__dirname}/${spaceshipName}.json`) };
            extendedLog(`${__dirname}\\${spaceshipName}.json`, tableify(spaceship));
            const turretName = spaceship.turrets[0].type;
            const turret = { type: 'turret', name: turretName, ...require(`${__dirname}/${turretName}.json`) };
            extendedLog(`${__dirname}\\${turretName}.json`, tableify(turret));
        }
    });*/

    /*document.getElementById('clear').addEventListener('click', async () => {
        document.getElementById('log').innerHTML = '';
    });*/

    document.getElementById('scroll').addEventListener('click', async () => {
        scrollSwitch = !scrollSwitch;
        document.getElementById('scroll').setAttribute('checked', scrollSwitch);
    });

    new ResizeObserver(() => {
        document.getElementById('log').style.height = `calc(100% - ${document.getElementById('controls').offsetHeight + 16}px)`;
    }).observe(document.getElementById('controls'));
    document.getElementById('log').style.height = `calc(100% - ${document.getElementById('controls').offsetHeight + 16}px)`;

    log(`Launcher version: ${remote.app.getVersion()}`);

    if (fs.existsSync(__dirname + '/config.json')) {
        const local = require(__dirname + '/config.json').version;
        log(`Game version: ${local}`);
        const remote = (await getJSON('https://raw.githubusercontent.com/Antollo/Starship-battle/master/src/Release/config.json')).version;
        log(`Latest game version: ${remote}`);
        log('\n');
        if (local == undefined || remote == undefined || semver.gt(remote, local)) {
            log('Need to update the game.');
            download();
        } else {
            log('Game is up to date.');
        }
    } else {
        log('Need to download the game.');
        download();
    }

    log('\nLocal Ip:')
    const networkInterfaces = Object.entries(os.networkInterfaces())
    for (const [name, infos] of networkInterfaces) {
        for (const info of infos) {
            if ('IPv4' !== info.family || info.internal !== false) continue;
            log(name + '  ' + info.address);
            break;
        }
    }
};