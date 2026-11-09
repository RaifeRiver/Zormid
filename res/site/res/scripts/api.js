let websocket;

export let connectHandler;
export function setConnectHandler(handler) {
    connectHandler = handler;
}

export let authHandler;
export function setAuthHandler(handler) {
    authHandler = handler;
}

export let globalMessageHandler;
export function setGlobalMessageHandler(handler) {
    globalMessageHandler = handler;
}

export let usernameRequestHandler;
export function setUsernameRequestHandler(handler) {
    usernameRequestHandler = handler;
}

export let channelListRequestHandler;
export function setChannelListRequestHandler(handler) {
    channelListRequestHandler = handler;
}

export let messageHandler;
export function setMessageHandler(handler) {
    messageHandler = handler;
}

export let channelContentsHandler;
export function setChannelContentsHandler(handler) {
    channelContentsHandler = handler;
}

function readString(packet, pos) {
    let str = "";
    let size = packet.getUint32(pos, true);
    for (let i = 0; i < size; i++) {
        str += String.fromCharCode(packet.getUint8(pos + 4 + i));
    }
    return str;
}

function readID(packet, pos) {
    let id = "";
    for (let i = 0; i < 20; i++) {
        id += String.fromCharCode(packet.getUint8(pos + i));
    }
    return id;
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
            if (globalMessageHandler) {
                let userID = readID(view, 4);
                let content = readString(view, 24);
                let username = readString(view, 28 + content.length);
                globalMessageHandler({content: content, username: username, userID: userID});
            }
            break;
        case 0x2:
            if (usernameRequestHandler) {
                let userID = readID(view, 4);
                let username = readString(view, 24);
                usernameRequestHandler(username, userID);
            }
            break;
        case 0x3:
            if (channelListRequestHandler) {
                let userID = readID(view, 4);
                let channelCount = view.getUint32(24, true);
                let channels = [];
                let pos = 28;
                for (let i = 0; i < channelCount; i++) {
                    let channel = {};
                    channel.channelID = readID(view, pos);
                    pos += 20;
                    channel.channelName = readString(view, pos);
                    pos += channel.channelName.length + 4;
                    channels.push(channel);
                }
                channelListRequestHandler(channels, userID);
            }
            break;
        case 0x4:
            if (messageHandler) {
                let userID = readID(view, 4);
                let channelID = readID(view, 24);
                let content = readString(view, 44);
                let username = readString(view, 48 + content.length);
                messageHandler({content: content, username: username, channelID: channelID, userID: userID});
            }
            break;
        case 0x5:
            if (channelContentsHandler) {
                let channelID = readID(view, 4);
                let channelName = readString(view, 24);
                let messageCount = view.getUint32(28 + channelName.length, true);
                let messages = [];
                let pos = 32 + channelName.length;
                for (let i = 0; i < messageCount; i++) {
                    let message = {};
                    message.userID = readID(view, pos);
                    pos += 20;
                    message.content = readString(view, pos);
                    pos += message.content.length + 4;
                    message.username = readString(view, pos);
                    pos += message.username.length + 4;
                    messages.push(message);
                }
                channelContentsHandler({channelID: channelID, name: channelName, messages: messages});
            }
            break;
        default:
            console.warn("Unknown packet ID: " + id);
            break;
    }
}

export function connect() {
    websocket = new WebSocket("ws://" + window.location.host);
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

export function sendGlobalMessage(message) {
    let buffer = new ArrayBuffer(message.length + 8);
    let view = new DataView(buffer);
    view.setUint32(0, 0x1, true);
    view.setUint32(4, message.length, true);
    for (let i = 0; i < message.length; i++) {
        view.setUint8(i + 8, message.charCodeAt(i));
    }
    websocket.send(buffer);
}

export function requestUsername(id) {
    let buffer = new ArrayBuffer(24);
    let view = new DataView(buffer);
    let rawID = atob(id);
    view.setUint32(0, 0x2, true);
    for (let i = 0; i < 20; i++) {
        view.setUint8(i + 4, rawID.charCodeAt(i));
    }
    websocket.send(buffer);
}

export function acceptFriendRequest(userID) {
    let buffer = new ArrayBuffer(24);
    let view = new DataView(buffer);
    view.setUint32(0, 0x3, true);
    for (let i = 0; i < 20; i++) {
        view.setUint8(i + 4, userID.charCodeAt(i));
    }
    websocket.send(buffer);
}

export function requestChannelList(userID) {
    let buffer = new ArrayBuffer(24);
    let view = new DataView(buffer);
    view.setUint32(0, 0x4, true);
    for (let i = 0; i < 20; i++) {
        view.setUint8(i + 4, userID.charCodeAt(i));
    }
    websocket.send(buffer);
}

export function sendMessage(message, channelID) {
    let buffer = new ArrayBuffer(message.length + 28);
    let view = new DataView(buffer);
    view.setUint32(0, 0x5, true);
    for (let i = 0; i < 20; i++) {
        view.setUint8(i + 4, channelID.charCodeAt(i));
    }
    view.setUint32(24, message.length, true);
    for (let i = 0; i < message.length; i++) {
        view.setUint8(i + 28, message.charCodeAt(i));
    }
    websocket.send(buffer);
}

export function requestChannelContents(channelID) {
    let buffer = new ArrayBuffer(24);
    let view = new DataView(buffer);
    view.setUint32(0, 0x6, true);
    for (let i = 0; i < 20; i++) {
        view.setUint8(i + 4, channelID.charCodeAt(i));
    }
    websocket.send(buffer);
}