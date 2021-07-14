var connectionCheckInterval = window.setInterval(function(){ 
    checkConnection();
  }, 2000);



var rainbowEnable = false;
var connection = new WebSocket('ws://'+location.hostname+':81/', ['arduino']);

connection.onopen = function () {
    connection.send('Connect ' + new Date());
};
connection.onerror = function (error) {
    console.log('WebSocket Error ', error);
};
connection.onmessage = function (e) {  
    console.log('Server: ', e.data);
};
connection.onclose = function(){
    checkConnection();
    console.log('WebSocket connection closed');
};


function sendJSON(message){
    // input : message object

    messageString = JSON.stringify(message);
    connection.send(messageString);
}


function checkConnection(){
    // 0	CONNECTING	Socket has been created. The connection is not yet open.
    // 1	OPEN	The connection is open and ready to communicate.
    // 2	CLOSING	The connection is in the process of closing.
    // 3	CLOSED	The connection is closed or couldn't be opened.
    switch(connection.readyState){
        case 0: 
            document.getElementById('connectionStatus').innerHTML = "Status: <b style='color: gray'>LACZENIE</b>";
            break;
        case 1:
            document.getElementById('connectionStatus').innerHTML = "Status: <b style='color: green'>POLACZONO</b>";
            break;
        case 2:
            document.getElementById('connectionStatus').innerHTML = "Status: <b style='color: gray'>ZAMYKANIE</b>";
            break;
        case 3:
            document.getElementById('connectionStatus').innerHTML = "Status: <b style='color: gray'>ROZLACZONO</b>";
            break;
    }

}


function sendSolidColor() {
    var color = document.getElementById('solidColor').value;

    color = color.slice(1,7); //get rid of '#' at beggining
    var colorNumber = parseInt(color, 16);

    var message = 
    {
        type: "SOLID_COLOR",
        color: colorNumber
    }
    sendJSON(message);

}

function sendRainbowEffect(){
    rainbowEnable = ! rainbowEnable;

    var message = 
    {
        type: "RAINBOW",
        value: "rainbow"
    }

    if(rainbowEnable){
        document.getElementById('rainbow').style.backgroundColor = '#00878F';
    } else {
        document.getElementById('rainbow').style.backgroundColor = '#999';
    }
    
    sendJSON(message);
}


function switchModeDropdownHandler(){
    var value = document.getElementById("mode").value;
    switchMode(value);
}

function switchMode(mode){
    switch(mode){
        case "ADALIGHT":
            document.getElementById("settingsAdalight").style.display = "inline";
            document.getElementById("settingsSolidColor").style.display = "none";
            document.getElementById("settingsEffects").style.display = "none";
            break;
        case "SOLID_COLOR":
            document.getElementById("settingsAdalight").style.display = "none";
            document.getElementById("settingsSolidColor").style.display = "inline";
            document.getElementById("settingsEffects").style.display = "none";
            var message = {
                type: "SOLID_COLOR",
                color: "NO_CHANGE"
            }
            sendJSON(message); 
            break;
        case "RAINBOW": //rainbow has no settings
            document.getElementById("settingsAdalight").style.display = "none";
            document.getElementById("settingsSolidColor").style.display = "none";
            document.getElementById("settingsEffects").style.display = "none";
            var message = {
                type: "RAINBOW"
            }
            sendJSON(message); 
            break;
    }

}

