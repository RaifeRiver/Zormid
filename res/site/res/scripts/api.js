let websocket;

export let connectHandler;
export function setConnectHandler(handler) {
    connectHandler = handler;
}

export let authHandler;
export function setAuthHandler(handler) {
    authHandler = handler;
}

export let messageHandler;
export function setMessageHandler(handler) {
    messageHandler = handler;
}

function readString(packet, pos) {
    let str = "";
    let size = packet.getUint32(pos, true);
    for (let i = 0; i < size; i++) {
        str += String.fromCharCode(packet.getUint8(pos + 4 + i));
    }
    return str;
}

function processPacket(packet) {
    let view = new DataView(packet);
    let id = view.getUint32(0, true);
    switch (id) {
        case 0x0:
            if (authHandler) {
                authHandler();
            }
            break;
        case 0x1:
            if (messageHandler) {
                let content = readString(view, 4);
                let username = readString(view, 8 + content.length);
                messageHandler({content: content, username: username});
            }
            break;
        default:
            console.warn("Unknown packet ID: " + id);
            break;
    }
}

export function connect() {
    websocket = new WebSocket("ws://localhost:8082");
    websocket.binaryType = "arraybuffer";
    websocket.addEventListener("message", (event) => {
        processPacket(event.data);
    });
    websocket.addEventListener("open", () => {
        if (connectHandler) {
            connectHandler();
        }
    });
}

export function authenticate(userID, sessionID) {
    let buffer = new ArrayBuffer(40);
    let view = new DataView(buffer);
    view.setUint32(0, 0x0, true);
    for (let i = 0; i < 20; i++) {
        view.setUint8(i + 4, userID.charCodeAt(i));
    }
    for (let i = 0; i < 16; i++) {
        view.setUint8(i + 24, sessionID.charCodeAt(i));
    }
    websocket.send(buffer);
}

export function sendMessage(message) {
    let buffer = new ArrayBuffer(message.length + 8);
    let view = new DataView(buffer);
    view.setUint32(0, 0x1, true);
    view.setUint32(4, message.length, true);
    for (let i = 0; i < message.length; i++) {
        view.setUint8(i + 8, message.charCodeAt(i));
    }
    websocket.send(buffer);
}