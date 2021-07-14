// message object
var message = 
{
    type: "solid",
    value: "#FFFFFF"
}

messageJson = JSON.stringify(message);

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
    console.log('WebSocket connection closed');
};

function checkConnection(){
    // 0	CONNECTING	Socket has been created. The connection is not yet open.
    // 1	OPEN	The connection is open and ready to communicate.
    // 2	CLOSING	The connection is in the process of closing.
    // 3	CLOSED	The connection is closed or couldn't be opened.

    connectionState = connection.readyState;

    switch(connectionState){
        case 0: 
            document.getElementById('connectionStatus').innerHTML = 'Status: LACZENIE';
            break;
        case 1:
            document.getElementById('connectionStatus').innerHTML = 'Status: POLACZONO';
            break;
        case 2:
            document.getElementById('connectionStatus').innerHTML = 'Status: ZAMYKANIE';
            break;
        case 3:
            document.getElementById('connectionStatus').innerHTML = 'Status: ROZLACZONO';
            break;
    }



}


function sendRGB() {
    var color = document.getElementById('solidColor').value;


    color = color.slice(1,7); //get rid of '#' at beggining

    var colorNumber = parseInt(color, 16);


    var r = (colorNumber & 0xFF0000) >> 16;
    var g = (colorNumber & 0x00FF00) >> 8; 
    var b = (colorNumber & 0x0000FF);

    // console.log("R:"+r+"G:"+g+"B:"+b);


    // weird format accepted by C code
    var rgb = r << 20 | g << 10 | b;
    var rgbstr = '#'+ rgb.toString(16);    
    console.log('RGB: ' + rgbstr); 
    connection.send(rgbstr);
}

function rainbowEffect(){
    rainbowEnable = ! rainbowEnable;
    if(rainbowEnable){
        connection.send("R");
        document.getElementById('rainbow').style.backgroundColor = '#00878F';

    } else {
        connection.send("N");
        document.getElementById('rainbow').style.backgroundColor = '#999';

        sendRGB();
    }  
}


function switchMode(){
    var value = document.getElementById("mode").value;

    switch(value){
        case "adalight":
            document.getElementById("settingsAdalight").style.display = "inline";
            document.getElementById("settingsSolidColor").style.display = "none";
            document.getElementById("settingsEffects").style.display = "none";
            break;
        case "solid":
            document.getElementById("settingsAdalight").style.display = "none";
            document.getElementById("settingsSolidColor").style.display = "inline";
            document.getElementById("settingsEffects").style.display = "none";
            break;
        case "effects":
            document.getElementById("settingsAdalight").style.display = "none";
            document.getElementById("settingsSolidColor").style.display = "none";
            document.getElementById("settingsEffects").style.display = "inline";
            break;
    }


}



