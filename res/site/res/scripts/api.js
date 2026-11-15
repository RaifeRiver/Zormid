let websocket;
let connected = false;

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
    let str = "";
    let size = packet.getUint32(pos, true);
    for (let i = 0; i < size; i++) {
        str += String.fromCharCode(packet.getUint8(pos + 4 + i));
    }
    return str;
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
                let username = readString(view, 4);
                authHandler(username);
            }
            break;
        case 0x1:
            if (friendListRequestHandler) {
                let friendCount = view.getUint32(4, true);
                let friends = [];
                let pos = 8;
                for (let i = 0; i < friendCount; i++) {
                    let friend = {};
                    friend.userID = readID(view, pos, USER_ID_SIZE);
                    friend.username = readString(view, pos + USER_ID_SIZE);
                    pos += 4 + USER_ID_SIZE + friend.username.length;
                    friends.push(friend);
                }
                friendListRequestHandler(friends);
            }
            break;
        case 0x2:
            if (usernameRequestHandler) {
                let userID = readID(view, 4, USER_ID_SIZE);
                let username = readString(view, 4 + USER_ID_SIZE);
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
                    let channel = {};
                    channel.channelID = readID(view, pos, 20);
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
                let userID = readID(view, 4, USER_ID_SIZE);
                let channelID = readID(view, 4 + USER_ID_SIZE, 20);
                let content = readString(view, 24 + USER_ID_SIZE);
                let username = readString(view, 28 + USER_ID_SIZE + content.length);
                let index = view.getUint32(32 + USER_ID_SIZE + content.length + username.length, true);
                messageHandler({index: index, content: content, username: username, channelID: channelID, userID: userID});
            }
            break;
        case 0x5:
            if (channelInfoHandler) {
                let channelID = readID(view, 4, 20);
                let channelName = readString(view, 24);
                let messageCount = view.getUint32(28 + channelName.length, true);
                let userCount = view.getUint32(32 + channelName.length, true);
                let users = [];
                let pos = 36 + channelName.length;
                for (let i = 0; i < userCount; i++) {
                    let userID = readID(view, pos, USER_ID_SIZE);
                    let username = readString(view, pos + USER_ID_SIZE);
                    users.push({userID: userID, username: username});
                    pos += USER_ID_SIZE + 4 + username.length;
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
                    let message = {};
                    message.userID = readID(view, pos, USER_ID_SIZE);
                    pos += USER_ID_SIZE;
                    message.content = readString(view, pos);
                    pos += message.content.length + 4;
                    message.username = readString(view, pos);
                    pos += message.username.length + 4;
                    messages.push(message);
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
    websocket = new WebSocket("ws://" + window.location.host);
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
    let buffer = new ArrayBuffer(12 + name.length + users.length * USER_ID_SIZE);
    let view = new DataView(buffer);
    view.setUint32(0, 0x7, true);
    view.setUint32(4, name.length, true);
    for (let i = 0; i < name.length; i++) {
        view.setUint8(i + 8, name.charCodeAt(i));
    }
    view.setUint32(8 + name.length, users.length, true);
    for (let i = 0; i < users.length; i++) {
        for (let j = 0; j < USER_ID_SIZE; j++) {
            view.setUint8(i * USER_ID_SIZE + j + 12 + name.length, users[i].charCodeAt(j));
        }
    }
    websocket.send(buffer);
}