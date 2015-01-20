var DEFAULT_PORT = '';
var HTTP_PORT = 1337;

var serialport = require('serialport');
var SerialPort = serialport.SerialPort;

if (DEFAULT_PORT) {
    initSerialPort(null, DEFAULT_PORT)
} else {
    // list serial ports:
    serialport.list(function (err, ports) {
        ports.forEach(function(port) {
            if (port.comName.indexOf('/dev/cu.usbmodem') != -1) {
                initSerialPort(null, port.comName)
            }
        });
    });
}

var http = require('http');
var url = require('url');
var path = require('path');
var fs = require('fs');

var server = http.createServer(httpHandler);

function httpHandler(req, res) {
    var filename = path.join(process.cwd(), 'index.html');

    if (req.method == 'POST') {
        var store = '';
        res.writeHead(200, {'Content-Type': 'text/json'});
        req.on('data', function(data) {
            store += data;
        });
        req.on('end', function() {
            store = JSON.parse(store)
            res.end();

            if (arduinoSerial && store.cmd) {
                console.log(new Date() + ': Box ' + store.cmd + ' selected.');
                arduinoSerial.write(store.cmd);
                if (store.time) {
                    arduinoSerial.write(',' + store.time);
                }
                arduinoSerial.write('\n');
            }
        });
    } else {
        fs.readFile(filename, 'binary', function(err, file) {
            if (err) {
                res.writeHead(500, {'Content-Type': 'text/plain'});
                res.write(err + '\n');
                res.end();
                return;
            }

            res.writeHead(200, {'Content-Type': 'text/html; charset=UTF-8'});
            res.write(file, 'binary');
            res.end();
        });
    }
}

server.listen(HTTP_PORT, '127.0.0.1');
console.log(new Date() + ': Server running at http://127.0.0.1:' + HTTP_PORT + '/');

var io = require('socket.io').listen(server);
io.on('connection', function(socket) {
    var address = socket.handshake.address;
    console.log(new Date() + ': New connection from ' + socket.handshake.address + ' (' + socket.handshake.headers['user-agent'] + ')');
    socket.on('disconnect', function() {
        console.log(new Date() + ': ' + socket.handshake.address + ' (' + socket.handshake.headers['user-agent'] + ') disconnected');
    });
    socket.emit('notification', { status: oldStatus });
});

var arduinoSerial;
var oldStatus;

function initSerialPort(err, comName) {
    if (err) {
        console.error(err.stack || err.message);
        return;
    }

    arduinoSerial = new SerialPort(comName, {
        baudrate: 57600,
        parser: serialport.parsers.readline('\n')
    });

    arduinoSerial.on('open', function() {
        console.log(new Date() + ': Serial port ' + comName + ' opened.');
        arduinoSerial.on('data', function(data) {
            if (data.indexOf('?') != -1) {
                var status = data.substring(data.indexOf('?') + 1).replace(/\s/g, '');
                if (status != oldStatus) {
                    oldStatus = status;
                    io.sockets.emit('notification', { status: status });
                }
            }
        });
    });
}
