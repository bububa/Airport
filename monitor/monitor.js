require.paths.unshift(__dirname + '/lib');
require.paths.unshift(__dirname + '/lib/express/lib');
require.paths.unshift(__dirname + '/lib/node-mongodb-native/lib');
require.paths.unshift(__dirname + '/lib/Socket.IO-node/lib');
require('express');
require('express/plugins');

var sys = require('sys'), 
    fs = require('fs'), 
    mongo = require('mongodb'), 
    url = require('url'), 
    http = require('http'), 
    io = require('socket.io'), 
    dlog = require('dlog');

var host = process.env['MONGO_NODE_DRIVER_HOST'] != null ? process.env['MONGO_NODE_DRIVER_HOST'] : 'localhost';
var port = process.env['MONGO_NODE_DRIVER_PORT'] != null ? process.env['MONGO_NODE_DRIVER_PORT'] : mongo.Connection.DEFAULT_PORT;

sys.puts("Connecting to " + host + ":" + port);
var json = JSON.stringify;
var db = new mongo.Db('airport', new mongo.Server(host, port, {}), {});
db.open(function(err, p_db) {
    
    if (err) {
        sys.log(json(err, null, 2));
    }
    configure(function(){
        use(MethodOverride), 
        use(ContentLength), 
        use(Static), 
        set('root', __dirname);
        set('db', db);
    });
    
    get('/', function(){
        var self = this;
        dlog.nodes(Express.settings['db'], function(data) {
            self.render('index.html.ejs', {
                locals: {
                    title: 'AIRPORT MONITOR',
                    nodes: data
                }
            });
        });
    });

    run();
    
    server = http.createServer(function(req, res){
        // your normal server code
        var path = url.parse(req.url).pathname;
        switch (path){
            case '/':
                res.writeHead(200, {'Content-Type': 'text/html'});
                res.write('<h1>Welcome. Try the <a href="/chat.html">chat</a> example.</h1>');
                res.end();
                break;
        }
    });
    
    server.listen(8082);
    
    //var cdb = new Array();
    var agint = new Array();
    
    var aggregate = function (client, node, domain) {
        var interval = setInterval(function(){
            dlog.last10Min(Express.settings['db'], node, domain, function(data) {
                client.send({'client':client.sessionId, 'node':node, 'data':data});
            });
        }, 2000);
        agint[client.sessionId][node] = interval;
    };
    
    var aggregatePerMin = function (client, node, domain) {
        dlog.last10Min(Express.settings['db'], node, domain, function(data) {
            client.send({'client':client.sessionId, 'node':node, 'data':data});
        });
    }
    
    io.listen(server, {
        
        onClientConnect: function(client){
            agint[client.sessionId] = new Array();
            //cdb[client.sessionId] = new mongo.Db('airport', new mongo.Server(host, port, {}), {});
            //var conn = cdb[client.sessionId];
            /*aggregate(client, "_system");
            dlog.nodes(Express.settings['db'], function(nodes) {
                for (var i = 0; i < nodes.length; ++i) {
                    aggregate(client, nodes[i]);
                }
            });*/
            client.broadcast(json({ announcement: client.sessionId + ' connected' }));
        },

        onClientDisconnect: function(client){
            client.broadcast(json({ announcement: client.sessionId + ' disconnected' }));
            for (var i in agint[client.sessionId])
            {
                clearInterval(agint[client.sessionId][i]);
            }
            //cdb[client.sessionId].close();
            //delete cdb[client.sessionId];
        },

        onClientMessage: function(message, client){
            var msg = JSON.parse(message);
            aggregatePerMin(client, msg.node, msg.domain);
            //client.broadcast(json(msg));
        }

    });
});