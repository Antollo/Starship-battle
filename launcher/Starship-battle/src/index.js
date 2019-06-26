const { app, BrowserWindow, ipcMain, autoUpdater } = require('electron');
const { download } = require('electron-dl');

// Handle creating/removing shortcuts on Windows when installing/uninstalling.
if (require('electron-squirrel-startup')) { // eslint-disable-line global-require
    app.quit();
}

// Keep a global reference of the window object, if you don't, the window will
// be closed automatically when the JavaScript object is garbage collected.
let mainWindow;

const createWindow = () => {
    // Create the browser window.
    mainWindow = new BrowserWindow({
        width: 800,
        height: 600,
        frame: false,
        show: false,
        webPreferences: {
            nodeIntegration: true
        }
    });

    mainWindow.setMenu(null);

    // and load the index.html of the app.
    mainWindow.loadURL(`file://${__dirname}/index.html`);

    // Open the DevTools.
    //mainWindow.webContents.openDevTools();

    // Emitted when the window is closed.
    mainWindow.on('closed', () => {
        // Dereference the window object, usually you would store windows
        // in an array if your app supports multi windows, this is the time
        // when you should delete the corresponding element.
        mainWindow = null;
    });

    mainWindow.once('ready-to-show', () => {
        mainWindow.show();
    })
};

// This method will be called when Electron has finished
// initialization and is ready to create browser windows.
// Some APIs can only be used after this event occurs.
app.on('ready', createWindow);

// Quit when all windows are closed.
app.on('window-all-closed', () => {
    // On OS X it is common for applications and their menu bar
    // to stay active until the user quits explicitly with Cmd + Q
    if (process.platform !== 'darwin') {
        app.quit();
    }
});

app.on('activate', () => {
    // On OS X it's common to re-create a window in the app when the
    // dock icon is clicked and there are no other windows open.
    if (mainWindow === null) {
        createWindow();
    }
});

// In this file you can include the rest of your app's specific main process
// code. You can also put them in separate files and import them here.

ipcMain.on('download', (event, info) => {
    info.properties.onProgress = function (status) {
        if (!Number.isFinite(status))
            return;
        mainWindow.webContents.send('download progress', status);
    }
    download(BrowserWindow.getFocusedWindow(), info.url, info.properties)
        .then(dl => mainWindow.webContents.send('download complete', dl.getSavePath()));
});


const server = 'https://update.electronjs.org';
let feed;
if (process.platform == 'win32')
    feed = `${server}/Antollo/Starship-battle/${process.platform}/${app.getVersion()}`;
else
    feed = `${server}/Antollo/Starship-battle/${process.platform}-${process.arch}/${app.getVersion()}`

autoUpdater.setFeedURL(feed);

autoUpdater.on('error', (err) => {
    mainWindow.webContents.send('log', 'Error while checking launcher update.');
    console.log('error', err);
})

autoUpdater.on('checking-for-update', () => {
    console.log('checking-for-update');
});

autoUpdater.on('update-available', () => {
    mainWindow.webContents.send('log', 'Launcher update is available, starting download.');
    console.log('update-is-available');
    autoUpdater.on('update-downloaded', () => {
        console.log('update-downloaded');
        autoUpdater.quitAndInstall();
    });

});

autoUpdater.on('update-not-available', () => {
    console.log('update not available');
});

setTimeout(() => {
    autoUpdater.checkForUpdates();
}, 1000);

setInterval(() => {
    autoUpdater.checkForUpdates();
}, 10 * 60 * 1000);
