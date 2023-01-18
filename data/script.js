var gateway = `ws://${window.location.hostname}/ws`;
var websocket;
var isLoaded = false;

window.addEventListener('load', onload);
function onload(event) {
 initWebSocket();
 isLoaded = true;
}

function initWebSocket() {
 websocket = new WebSocket(gateway);
 websocket.onopen = onOpen;
 websocket.onclose = onClose;
 websocket.onmessage = onMessage;
}

function onOpen(event) {
 websocket.send(isLoaded);
}

function onClose(event) {
 setTimeout(initWebSocket, 2000);
}

function onMessage(event) {
 console.log(event.data);
}