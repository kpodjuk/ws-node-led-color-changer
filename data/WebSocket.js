
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
    connectionState = connection.readyState;
    switch(connectionState){
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
        value: colorNumber
    }
    sendJSON(message);

    // var r = (colorNumber & 0xFF0000) >> 16;
    // var g = (colorNumber & 0x00FF00) >> 8; 
    // var b = (colorNumber & 0x0000FF);

    // console.log("R:"+r+"G:"+g+"B:"+b);


    // // weird format accepted by C code
    // var rgb = r << 20 | g << 10 | b;
    // var rgbstr = '#'+ rgb.toString(16);    
    // console.log('RGB: ' + rgbstr); 
    // connection.send(rgbstr);


}

function rainbowEffect(){
    rainbowEnable = ! rainbowEnable;


    var message = 
    {
        type: "effect",
        value: "rainbow"
    }

    if(rainbowEnable){
        connection.send("R");
        // sendJSON(message);
        document.getElementById('rainbow').style.backgroundColor = '#00878F';

    } else {
        connection.send("N");
        document.getElementById('rainbow').style.backgroundColor = '#999';

        // sendRGB();
    }  
}


function switchMode(){
    var value = document.getElementById("mode").value;

    switch(value){
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
                value: "NO_CHANGE"
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
            break;
    }


    // send new mode to arduino


}



