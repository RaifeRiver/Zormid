import * as base64 from "./base64.js";

let websocket;
let connected = false;
let encoder = new TextEncoder();
let decoder = new TextDecoder();

export const USER_ID_SIZE = 12;

export let connectHandler;
export function setConnectHandler(handler) {
    connectHandler = handler;
}

export let authHandler;
export function setAuthHandler(handler) {
    authHandler = handler;
}

export let friendListRequestHandler;
export function setFriendListRequestHandler(handler) {
    friendListRequestHandler = handler;
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

export let channelInfoHandler;
export function setChannelInfoHandler(handler) {
    channelInfoHandler = handler;
}

export let channelMessagesHandler;
export function setChannelMessagesHandler(handler) {
    channelMessagesHandler = handler;
}

export let connectionLostHandler;
export function setConnectionLostHandler(handler) {
    connectionLostHandler = handler;
}

export let timeoutHandler;
export function setTimeoutHandler(handler) {
    timeoutHandler = handler;
}

function readString(packet, pos) {
    let size = packet.getUint32(pos, true);
    let bytes = new Uint8Array(packet.buffer, pos + 4, size);
    return {text: decoder.decode(bytes), bytes: size};
}

function readID(packet, pos, size) {
    let id = "";
    for (let i = 0; i < size; i++) {
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
                let username = readString(view, 4).text;
                authHandler(username);
            }
            break;
        case 0x1:
            if (friendListRequestHandler) {
                let friendCount = view.getUint32(4, true);
                let friends = [];
                let pos = 8;
                for (let i = 0; i < friendCount; i++) {
                    let id = readID(view, pos, USER_ID_SIZE);
                    let {text: username, bytes: usernameSize} = readString(view, pos + USER_ID_SIZE);
                    pos += 4 + USER_ID_SIZE + usernameSize;
                    friends.push({userID: id, username: username});
                }
                friendListRequestHandler(friends);
            }
            break;
        case 0x2:
            if (usernameRequestHandler) {
                let userID = readID(view, 4, USER_ID_SIZE);
                let username = readString(view, 4 + USER_ID_SIZE).text;
                usernameRequestHandler(username, userID);
            }
            break;
        case 0x3:
            if (channelListRequestHandler) {
                let userID = readID(view, 4, USER_ID_SIZE);
                let channelCount = view.getUint32(4 + USER_ID_SIZE, true);
                let channels = [];
                let pos = 8 + USER_ID_SIZE;
                for (let i = 0; i < channelCount; i++) {
                    let channelID = readID(view, pos, 20);
                    pos += 20;
                    let {text: channelName, bytes: channelSize} = readString(view, pos);
                    pos += channelSize + 4;
                    channels.push({channelID: channelID, channelName: channelName});
                }
                channelListRequestHandler(channels, userID);
            }
            break;
        case 0x4:
            if (messageHandler) {
                let userID = readID(view, 4, USER_ID_SIZE);
                let channelID = readID(view, 4 + USER_ID_SIZE, 20);
                let {text: content, bytes: contentSize} = readString(view, 24 + USER_ID_SIZE);
                let {text: username, bytes: usernameSize} = readString(view, 28 + USER_ID_SIZE + contentSize);
                let index = view.getUint32(32 + USER_ID_SIZE + contentSize + usernameSize, true);
                messageHandler({index: index, content: content, username: username, channelID: channelID, userID: userID});
            }
            break;
        case 0x5:
            if (channelInfoHandler) {
                let channelID = readID(view, 4, 20);
                let {text: channelName, bytes: channelNameSize} = readString(view, 24);
                let messageCount = view.getUint32(28 + channelNameSize, true);
                let userCount = view.getUint32(32 + channelNameSize, true);
                let users = [];
                let pos = 36 + channelNameSize;
                for (let i = 0; i < userCount; i++) {
                    let userID = readID(view, pos, USER_ID_SIZE);
                    let {text: username, bytes: usernameSize} = readString(view, pos + USER_ID_SIZE);
                    users.push({userID: userID, username: username});
                    pos += USER_ID_SIZE + 4 + usernameSize;
                }
                channelInfoHandler({channelID: channelID, name: channelName, messageCount: messageCount, users: users});
            }
            break;
        case 0x6:
            if (channelMessagesHandler) {
                let channelID = readID(view, 4, 20);
                let firstMessage = view.getUint32(24, true);
                let messageCount = view.getUint32(28, true);
                let messages = [];
                let pos = 32;
                for (let i = 0; i < messageCount; i++) {
                    let userID = readID(view, pos, USER_ID_SIZE);
                    pos += USER_ID_SIZE;
                    let {text: content, bytes: contentSize} = readString(view, pos);
                    pos += contentSize + 4;
                    let {text: username, bytes: usernameSize} = readString(view, pos);
                    pos += usernameSize + 4;
                    messages.push({userID: userID, content: content, username: username});
                }
                channelMessagesHandler({channelID: channelID, firstMessage: firstMessage, messages: messages});
            }
            break;
        default:
            console.warn("Unknown packet ID: " + id);
            break;
    }
}

export function connect() {
    connected = false;
    let protocol = window.location.protocol === "https:" ? "wss" : "ws";
    websocket = new WebSocket(protocol + "://" + window.location.host);
    websocket.binaryType = "arraybuffer";
    websocket.addEventListener("message", (event) => {
        processPacket(event.data);
    });
    websocket.addEventListener("open", () => {
        connected = true;
        if (connectHandler) {
            connectHandler();
        }
    });
    websocket.addEventListener("close", (event) => {
        if (connected) {
            if (connectionLostHandler) {
                connectionLostHandler();
            }
        }
        else if (!event.wasClean && timeoutHandler) {
            timeoutHandler();
        }
    })
    setTimeout(() => {
        if (websocket.readyState === WebSocket.CONNECTING && timeoutHandler) {
            timeoutHandler();
        }
    }, 5000);
}

export function authenticate(userID, sessionID) {
    let buffer = new ArrayBuffer(20 + USER_ID_SIZE);
    let view = new DataView(buffer);
    view.setUint32(0, 0x0, true);
    for (let i = 0; i < USER_ID_SIZE; i++) {
        view.setUint8(i + 4, userID.charCodeAt(i));
    }
    for (let i = 0; i < 16; i++) {
        view.setUint8(i + 4 + USER_ID_SIZE, sessionID.charCodeAt(i));
    }
    websocket.send(buffer);
}

export function requestFriendList() {
    let buffer = new ArrayBuffer(4);
    let view = new DataView(buffer);
    view.setUint32(0, 0x1, true);
    websocket.send(buffer);
}

export function requestUsername(id) {
    let buffer = new ArrayBuffer(4 + USER_ID_SIZE);
    let view = new DataView(buffer);
    let rawID = atob(id);
    view.setUint32(0, 0x2, true);
    for (let i = 0; i < USER_ID_SIZE; i++) {
        view.setUint8(i + 4, rawID.charCodeAt(i));
    }
    websocket.send(buffer);
}

export function acceptFriendRequest(userID) {
    let buffer = new ArrayBuffer(4 + USER_ID_SIZE);
    let view = new DataView(buffer);
    view.setUint32(0, 0x3, true);
    for (let i = 0; i < USER_ID_SIZE; i++) {
        view.setUint8(i + 4, userID.charCodeAt(i));
    }
    websocket.send(buffer);
}

export function requestChannelList(userID) {
    let buffer = new ArrayBuffer(4 + USER_ID_SIZE);
    let view = new DataView(buffer);
    view.setUint32(0, 0x4, true);
    for (let i = 0; i < USER_ID_SIZE; i++) {
        view.setUint8(i + 4, userID.charCodeAt(i));
    }
    websocket.send(buffer);
}

export function sendMessage(message, channelID) {
    let bytes = encoder.encode(message);
    let buffer = new ArrayBuffer(bytes.length + 28);
    let view = new DataView(buffer);
    view.setUint32(0, 0x5, true);
    for (let i = 0; i < 20; i++) {
        view.setUint8(i + 4, channelID.charCodeAt(i));
    }
    view.setUint32(24, bytes.length, true);
    for (let i = 0; i < bytes.length; i++) {
        view.setUint8(i + 28, bytes[i]);
    }
    websocket.send(buffer);
}

export function requestChannelInfo(channelID) {
    let buffer = new ArrayBuffer(24);
    let view = new DataView(buffer);
    view.setUint32(0, 0x6, true);
    for (let i = 0; i < 20; i++) {
        view.setUint8(i + 4, channelID.charCodeAt(i));
    }
    websocket.send(buffer);
}

export function requestChannelMessages(channelID, firstMessage) {
    let buffer = new ArrayBuffer(28);
    let view = new DataView(buffer);
    view.setUint32(0, 0x8, true);
    for (let i = 0; i < 20; i++) {
        view.setUint8(i + 4, channelID.charCodeAt(i));
    }
    view.setUint32(24, firstMessage, true);
    websocket.send(buffer);
}

export function createChannel(name, users) {
    let bytes = encoder.encode(name);
    let buffer = new ArrayBuffer(12 + bytes.length + users.length * USER_ID_SIZE);
    let view = new DataView(buffer);
    view.setUint32(0, 0x7, true);
    view.setUint32(4, bytes.length, true);
    for (let i = 0; i < bytes.length; i++) {
        view.setUint8(i + 8, bytes[i]);
    }
    view.setUint32(8 + bytes.length, users.length, true);
    for (let i = 0; i < users.length; i++) {
        for (let j = 0; j < USER_ID_SIZE; j++) {
            view.setUint8(i * USER_ID_SIZE + j + 12 + bytes.length, users[i].charCodeAt(j));
        }
    }
    websocket.send(buffer);
}

export function changeUsername(name) {
    let bytes = encoder.encode(name);
    let buffer = new ArrayBuffer(8 + bytes.length);
    let view = new DataView(buffer);
    view.setUint32(0, 0x9, true);
    view.setUint32(4, bytes.length, true);
    for (let i = 0; i < bytes.length; i++) {
        view.setUint8(i + 8, bytes[i]);
    }
    websocket.send(buffer);
}

export function changeChannelUsers(channel, users) {
    let buffer = new ArrayBuffer(28 + users.length * USER_ID_SIZE);
    let view = new DataView(buffer);
    view.setUint32(0, 0xA, true);
    for (let i = 0; i < 20; i++) {
        view.setUint8(i + 4, channel.charCodeAt(i));
    }
    view.setUint32(24, users.length, true);
    for (let i = 0; i < users.length; i++) {
        for (let j = 0; j < USER_ID_SIZE; j++) {
            view.setUint8(i * USER_ID_SIZE + j + 28, users[i].charCodeAt(j));
        }
    }
    websocket.send(buffer);
}

export async function uploadFile(file) {
    let fileBuffer = await file.arrayBuffer();
    let hashBuffer = await crypto.subtle.digest("SHA-1", fileBuffer);
    let hash = new Uint8Array(hashBuffer);
    let array = new Uint8Array(fileBuffer);
    let nameBytes = encoder.encode(file.name);
    let buffer = new ArrayBuffer(32 + nameBytes.length + array.length);
    let view = new DataView(buffer);
    view.setUint32(0, 0xB, true);
    let rawID = "";
    for (let i = 0; i < 20; i++) {
        rawID += String.fromCharCode(hash[i]);
        view.setUint8(i + 4, hash[i]);
    }
    let id = base64.encodeURL(rawID);
    view.setUint32(24, nameBytes.length, true);
    for (let i = 0; i < nameBytes.length; i++) {
        view.setUint8(i + 28, nameBytes[i]);
    }
    view.setUint32(28 + nameBytes.length, array.length, true);
    for (let i = 0; i < array.length; i++) {
        view.setUint8(i + 32 + nameBytes.length, array[i]);
    }
    websocket.send(buffer);
    return id;
}