if(!Monitor) { var Monitor = {}; }

Monitor.Graph = function(node, options) {
    if ( !(this instanceof Monitor.Graph) ) {
        return new Monitor.Graph(node, options);
    }
    
    var defaults = {
        showLogDate: false,
        showMarkers: true,
        ratePerSecond: 1,
        showBackgroundBars: true,
        tickLineColor: '#666',
        bgLineColor: '#555',
        barColor: null
    }
    Object.extend(defaults, options);
    this.options = defaults;
    
    this.node = node;
    this.scale = 50;
    this.el = $('graph_'+node);
    this.canvas = this.el.down('canvas');
    this.trafficLog = [];
    
    this.init();
};

Monitor.Graph.prototype = {
    init: function() {
        this.setupContext();
        
        this.lineColors = {
            1600: "#FFF",
            800: "#983839",
            400: "#C44939",
            200: "#F1E29F",
            100: "#7BE4D6",
            50: "#65B88A",
            25: "#5BC4B6",
            12.5: "#3BA496",
            6.25: "#1B8476",
            3.125: "#006456",
            def: "#7BF4D6"
        };
        this.canvasHeight = this.canvas.getHeight();
        this.canvasWidth = this.canvas.getWidth();
        
        this.numMarkers = Math.floor(this.canvasHeight / 23);
        this.resetMarkers();
        
        this.lineWidth = 3;
        
        this.drawEmptyGraph();
        this.tick = 0;
    },
    
    setupContext: function() {
        if(this.canvas.getContext) {
            this.context = this.canvas.getContext('2d');
        } else {
            alert("Sorry, this browser doesn't support canvas");
        }
        
        //this.context.font = "bold 10px Andale Mono, sans-serif";
        //this.context.textAlign = "right";
    },
    
    resetMarkers: function() {
        var leftMarkerContainer = this.el.down('div.axis_left');
        var rightMarkerContainer = this.el.down('div.axis_right');
        
        if(leftMarkerContainer.length == 0) { return; }
        
        var resetMarkerContainer = function(container, numMarkers, scale) {
            var incr = scale / numMarkers;
            for(var i = 0; i <= numMarkers; i++) {
                var markerValue = Math.floor(scale - (i * incr));
                container.insert('<p>' + markerValue + '</p>');
            }
        };
        
        var numMarkers = this.numMarkers;
        var scale = this.scale;
        
        rightMarkerContainer.update('');
        resetMarkerContainer(rightMarkerContainer, numMarkers, scale);
        
        var millisecsBeforeUpdating = 0;
        if (this.lineWidth != null && this.options.ratePerSecond != null) {
            var millisecsPerTick = 1000 / this.options.ratePerSecond;
            var ticksPerFrame = this.canvasWidth / (this.lineWidth * 2.0);
            millisecsBeforeUpdating = millisecsPerTick * ticksPerFrame;
        }
        
        setTimeout(function() {
            leftMarkerContainer.update('');
            resetMarkerContainer(leftMarkerContainer, numMarkers, scale);
        }, millisecsBeforeUpdating);
    },
    
    update_node_table: function (data)
    {
        var elm = $('table_'+this.node).down('tbody');
        var html = '';
        data.each(function(v){
            var speed = v.value.elasped/v.value.count;
            html += '<tr>';
            html += '<th>' + v._id.low_ + '</th>';
            html += '<td>' + v.value.count + '</td>';
            html += '<td>' + speed.toFixed(2) + '</td>';
            html += '<td>' + v.value.bytes + '</td>';
            html += '</tr>';
        });
        elm.update(html);
    }, 
    
    node_summary: function (data) {
        var tpage = tspeed = tbytes = 0;
        var apage = aspeed = abytes = 0;
        var count = data.length;
        if (count > 0)
        {
            data.each(function(v){
                var speed = v.value.elasped/v.value.count;
                tpage += v.value.count;
                tspeed += speed;
                tbytes += v.value.bytes;
            });
            apage = tpage/count;
            aspeed = tspeed/count;
            abytes = tbytes/count;
        }
        var summary = $('summary_'+this.node);
        summary.down('.page').update(apage.toFixed(2));
        summary.down('.speed').update(aspeed.toFixed(2));
        summary.down('.bytes').update(abytes.toFixed(2));
        return {page:apage, speed:aspeed, bytes:abytes};
    },
    
    addValue: function(value) {
        this.trafficLog.push(value);
        if(this.trafficLog.length > this.options.ratePerSecond) {
            this.trafficLog.shift();
        }
    },
    
    shiftCanvas: function(x, y) {
        this.context.putImageData(this.context.getImageData(x, y, this.canvas.width, this.canvas.height), 0, 0);
    },
    
    sum: function(arr) {
        var sum = 0;
        for(var i=0; i<arr.length; i++){
            sum += arr[i];
        }
        return sum;
    },
    
    runningAverage: function() {
        return this.sum(this.trafficLog) * 1.0 / this.trafficLog.length;
    },
    
    drawEmptyGraph: function() {
        var dataPoints = Math.ceil(this.canvasWidth / (this.lineWidth * 2));
        while(dataPoints--) {
            this.drawLogPath(0);
        }
    },
    
    logDate: function() {
        // Draw text background
        this.context.lineCap = "round";
        this.context.lineWidth = 12;
        this.context.beginPath();
        this.context.moveTo(this.canvasWidth - 16, this.canvasHeight + 10);
        this.context.strokeStyle = "#000";
        this.context.lineTo(this.canvasWidth - 70, this.canvasHeight + 10);
        this.context.stroke();
        this.context.closePath();
        this.context.lineCap = "square";
        
        // Draw date text
        this.context.fillStyle = "#FFF";
        var date = new Date();
        
        this.context.fillText(date.formattedTime(), this.canvasWidth - 16, this.canvasHeight + 14);
    },
    
    rescale: function(percent) {
        var oldScale = this.scale;
        
        if(percent === 0) { return; }
        if(percent > 0.9) {
            this.scale = this.scale * 2;
        } else if(percent < 0.08) {
            this.scale = this.scale / 4;
        } else if(percent < 0.16) {
            this.scale = this.scale / 2;
        } else {
            return;
        }
        
        // Set lower bound
        if (this.scale < 10) {
            this.scale = 10;
            // Return if no change in scale
            if (oldScale === this.scale) {
                return;
            }
        }
        
        this.drawSeparator(percent, oldScale, this.scale);
        this.resetMarkers();
    },
    
    drawSeparator: function(percent, oldScale, newScale) {
        this.shiftCanvas(this.lineWidth * 2, 0);
        var newHeight = percent * this.canvasHeight;
        var oldHeight = percent * (oldScale/newScale) * this.canvasHeight;

        this.context.lineWidth = this.lineWidth;
        this.context.beginPath();
        this.context.strokeStyle = "#CCC";
        this.context.moveTo(this.canvasWidth - 10, this.canvasHeight - oldHeight);
        this.context.lineTo(this.canvasWidth - 10, this.canvasHeight - newHeight);
        this.context.stroke();
        this.context.closePath();
    },
    
    drawLogPath: function(value) {
        this.tick++;
        
        if(this.options.showLogDate) {
            if(this.tick % 100 == 0) {
                this.logDate();
            }
        }
        
        this.addValue(value);
        
        var average = this.runningAverage() * this.options.ratePerSecond;
        var percent = average / this.scale;
        var height = Math.max(percent * this.canvasHeight, 1);
        var color = this.options.barColor || this.lineColors[this.scale] || this.lineColors.def;
        var endingPoint = this.canvasHeight - height;
        if(this.tick % (this.options.ratePerSecond * 2) == 0) { // Every 2 seconds
            this.rescale(percent);
            if(this.tick % 1000 == 0) { this.tick = 0; }
            return;
        }
        
        this.shiftCanvas(this.lineWidth * 2, 0);
        this.context.lineWidth = this.lineWidth;
        this.context.beginPath();
        this.context.strokeStyle = color;
        this.context.moveTo(this.canvasWidth - 10, this.canvasHeight);
        this.context.lineTo(this.canvasWidth - 10, endingPoint);
        this.context.stroke();
        this.context.closePath();
        
        if(this.options.showBackgroundBars) {
            this.context.beginPath();
            
            if(this.tick % this.options.ratePerSecond == 0) {
                this.context.strokeStyle = this.options.tickLineColor;
            } else {
                this.context.strokeStyle = this.options.bgLineColor;
            }
            
            this.context.moveTo(this.canvasWidth - 10, endingPoint);
            this.context.lineTo(this.canvasWidth - 10, 0);
            this.context.stroke();
            this.context.closePath();
        }
    }
};

$$('h3.datagrid').invoke("observe", "click", function(e) {
    var elm = Event.element(e);
    elm.next('table.datagrid').toggle();
});
$$('button.playbtn').invoke("observe", "click", function(e) {
    var elm = Event.element(e);
    var section = elm.up('section')
    section.toggleClassName('stop');
    if (section.hasClassName('stop')) {
        elm.update("Play");
    }else{
        elm.update("Stop");
    }
});

var graph = new Array();
var lastStamp = new Array();
var domain = undefined;
$$("section.node").each(function(e){
    var name = e.id.sub('node_', '');
    graph[name] = Monitor.Graph(name, {});
    lastStamp[name] = 0;
});

delete Array.prototype.toJSON;
delete String.prototype.toJSON;
delete Number.prototype.toJSON;

io.setPath('/public/Socket.IO/');
socket = new io.Socket('localhost', {rememberTransport: true, port: 8082});
socket.connect();

socket.addEvent('message', function(data){
    graph[data.node].update_node_table(data.data);
    graph[data.node].node_summary(data.data);
    var newData = false;
    data.data.each(function(v){
        if (v._id.low_<=lastStamp[data.node]) return;
        lastStamp[data.node] = v._id.low_;
        graph[data.node].drawLogPath(v.value.count);
        newData = true;
    });
    if (!newData) {
        graph[data.node].drawLogPath(0);
    }
});


new PeriodicalExecuter(function(pe) {
    $$("section.node").each(function(e){
        if (e.hasClassName('stop')) return;
        var name = e.id.sub('node_', '');
        socket.send(JSON.stringify({node:name, domain:domain}, null, 2));
    });
}, 2);