var WebSocketServer = require('websocket').server;
var http = require('http');
//require the express nodejs module
var express = require('express');
//set an instance of exress
var app = express();
//set an instance of parse for parsing the request header
var parser = require('ua-parser-js');

var viewImageResponseObject;
var firstFrameHeaderSent = false;
var getRequestTriggered = false;
var requestObject;

var left = 0
var right = 0
var isWorking = false;
var espClient;
var browserClient;
/*
 *   Server Creation
 */
var server = app.listen(8081, function() {
    var host = server.address().address
    var port = server.address().port
    console.log("Node Js app listening at http://" + host + port);
});

//-- serves static files
app.use('/', express.static("html"));

//Endpoint for getting the current values for engine 
app.get('/getValues', function(req, res) {
    if (isWorking)
        res.send({
            left: left,
            right: right
        });
    else
        res.send({
            left: 0,
            right: 0
        });
});

//Websocket server side initialized
wsServer = new WebSocketServer({
    httpServer: server,
    autoAcceptConnections: false
});
var clients = [];
var newFrame = "";
var firstFrame = false;

//Listen function for websocket
wsServer.on('request', function(request) {

    var connection = request.accept('', request.origin);
    console.log((new Date()) + ' Connection accepted.');
    console.log("Origin is: " + request.origin);
    var userAgent = request.httpRequest.headers['user-agent'];
    var ua = parser(userAgent);
    console.log("User Agent : " + ua.browser.name);
    //If-control for identifying different clients for websocket, if it has a browser name that means it's not ESP8266
    if (ua.browser.name) {
        clients.push(connection);
        browserClient = connection;
        //browser-client listen function
        browserClient.on('message', function(message) {
            console.log(message);
            if (message.type === 'utf8') {
                var msg = JSON.parse(message.utf8Data);
                if (msg.cmd == "start") {
                    isWorking = true;
                } else if (msg.cmd == "stop") {
                    isWorking = false;
                } else { //part where engine values set
                    if (msg.type == "engineValue") {
                        left = Math.round(msg.left);
                        right = Math.round(msg.right);
                    }
                }
            } else {
                console.log("Received some data = " + message)
            }


        });
    } else { // else part for ESP8266
        espClient = connection;
        espClient.on('message', function(message) {
            console.log(message);
            if (message.type === 'utf8') { // Part where header for stream data is identified
                if (newFrame) {
                    for (var i = 0; i < clients.length; i++) { // Function where one frame send to clients
                        clients[i].sendBytes(newFrame);
                    }
                    newFrame = "";
                }
                for (var i = 0; i < clients.length; i++) { // Function where header information send to clients
                    clients[i].sendUTF(message.utf8Data);
                }
                firstFrame = true;

            } else if (message.type === 'binary') { // Function for creating frames and control if its completed.

                if (firstFrame) {
                    newFrame = message.binaryData;
                    firstFrame = false;
                } else {
                    newFrame += message.binaryData;
                }
                for (var i = 0; i < clients.length; i++) {
                    clients[i].sendBytes(message.binaryData);
                }

            } else {
                console.log("3-Received some data = " + message)
            }


        });
    }



    connection.on('close', function(reasonCode, description) {
        console.log((new Date()) + ' Peer ' + connection.remoteAddress + ' disconnected.');
    });
});