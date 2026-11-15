import * as api from "./api.js";
import * as cookies from "./cookies.js";
import * as emojis from "./emojis.js";

let appContainer = document.getElementById("app-container");
let loadingScreen = document.getElementById("loading-screen");

let channelContainer = document.getElementById("channel-container");
let messageInput = document.getElementById("message-input");
let messageTemplate = document.getElementById("message-template");
let continuationMessageTemplate = document.getElementById("continuation-message-template");
let messageContainer = document.getElementById("message-container");

let sendButton = document.getElementById("send-button");

let addFriendMenu = document.getElementById("add-friend-menu");
let addFriendURL = document.getElementById("add-friend-url");
let addFriendClose = document.getElementById("add-friend-close");
let addFriendCopyURL;

let acceptFriendMenu = document.getElementById("accept-friend-menu");
let acceptFriendText = document.getElementById("accept-friend-text");
let acceptFriendAccept = document.getElementById("accept-friend-accept");
let acceptFriendDecline = document.getElementById("accept-friend-decline");
let acceptFriendClose = document.getElementById("accept-friend-close");
let openingAcceptFriendMenu = false;
let acceptFriendUserID;

let channelList = document.getElementById("channel-list");
let addChannelTemplate = document.getElementById("add-channel-template");
let channelTemplate = document.getElementById("channel-template");

let channelName = document.getElementById("channel-name");

let friendListContainer = document.getElementById("friend-list-container");
let friendContainer = document.getElementById("friend-container");
let addFriendTemplate = document.getElementById("add-friend-template");
let friendTemplate = document.getElementById("friend-template");

let addChannelMenu = document.getElementById("add-channel-menu");
let addChannelClose = document.getElementById("add-channel-close");
let addChannelFriends = document.getElementById("add-channel-friends");
let addChannelFriendTemplate = document.getElementById("add-channel-friend-template");
let addChannelAdd = document.getElementById("add-channel-add");
let addChannelName = document.getElementById("add-channel-name");

let openFriendsListButton = document.getElementById("open-friends-list");

let userNameDisplay = document.getElementById("user-name");

let channelSettingsContainer = document.getElementById("channel-settings-container");
let channelUserListButton = document.getElementById("channel-user-list-button");

let userListContainer = document.getElementById("user-list-container");
let userListUserTemplate = document.getElementById("user-list-user-template");

let emojiList = document.getElementById("emoji-list");
let emojiListList = document.getElementById("emoji-list-list");

let userID;
let currentChannelID;
let channelMessageCount;
let friends = [];
let waitingForChannelMessages = false;
let connectionAttempts = 0;

let messageScrollObserver;

document.documentElement.setAttribute("data-theme", "sync");

api.setConnectHandler(() => {
    const userCookie = cookies.getCookie("user");
    if (!userCookie) {
        return;
    }
    const sessionCookie = cookies.getCookie("sessionID");
    if (!sessionCookie) {
        return;
    }
    userID = userCookie;
    api.authenticate(atob(userID), atob(sessionCookie));
});
api.setAuthHandler((username) => {
    userNameDisplay.textContent = username;

    const addFriendCookie = cookies.getCookie("add-friend");
    if (addFriendCookie) {
        openAcceptFriendMenu(addFriendCookie);
    }
    api.requestChannelList(atob(userID));
    api.requestFriendList();

    loadingScreen.style.display = "none";
    appContainer.style.display = "flex";
});
api.setFriendListRequestHandler((f) => {
    friends = f;
    friendContainer.replaceChildren();
    let addFriendElement = document.importNode(addFriendTemplate.content, true);
    friendContainer.appendChild(addFriendElement);
    document.getElementById("add-friend").addEventListener("click", openAddFriendMenu);
    for (let friend of friends) {
        let friendElement = document.importNode(friendTemplate.content, true);
        let friendButton = friendElement.firstElementChild;
        friendButton.textContent = friend.username;
        friendContainer.appendChild(friendElement);
    }
});
api.setUsernameRequestHandler((username, userID) => {
    if (openingAcceptFriendMenu && btoa(userID) === acceptFriendUserID) {
        acceptFriendText.innerHTML = "Accept friend request from " + username + "?";
        acceptFriendMenu.showModal();
    }
});
api.setChannelListRequestHandler((channels, id) => {
    if (btoa(id) !== userID) {
        return;
    }
    channelList.replaceChildren();
    let addChannelElement = document.importNode(addChannelTemplate.content, true);
    channelList.appendChild(addChannelElement);
    document.getElementById("add-channel").addEventListener("click", openAddChannelMenu);
    for (let channel of channels) {
        let channelElement = document.importNode(channelTemplate.content, true);
        let channelButton = channelElement.firstElementChild;
        channelButton.textContent = channel.channelName;
        channelButton.addEventListener("click", () => changeChannel(channel.channelID));
        channelList.appendChild(channelElement);
    }
});
api.setMessageHandler((message) => {
    if (message.channelID !== currentChannelID && message.userID !== atob(userID)) {
        return;
    }
    insertMessage(message.index, message.username, btoa(message.userID), emojis.toEmojis(message.content));
});
api.setChannelInfoHandler((info) => {
    if (info.channelID !== currentChannelID) {
        return;
    }
    channelName.textContent = info.name;
    channelMessageCount = info.messageCount;
    channelSettingsContainer.style.display = "flex";
    messageContainer.replaceChildren();
    if (channelMessageCount > 0) {
        api.requestChannelMessages(currentChannelID, Math.max(channelMessageCount - 50, 0));
        waitingForChannelMessages = true;
    }
    userListContainer.replaceChildren();
    for (let user of info.users) {
        let userElement = userListUserTemplate.content.cloneNode(true);
        userElement.querySelector(".user-list-username").textContent = user.username;
        userListContainer.appendChild(userElement);
    }
});
api.setChannelMessagesHandler((messageInfo) => {
    if (messageInfo.channelID !== currentChannelID) {
        return;
    }
    for (let i = 0; i < messageInfo.messages.length; i++) {
        insertMessage(messageInfo.firstMessage + i, messageInfo.messages[i].username, btoa(messageInfo.messages[i].userID), emojis.toEmojis(messageInfo.messages[i].content));
    }
    if (waitingForChannelMessages) {
        messageContainer.scrollTop = messageContainer.scrollHeight;
        waitingForChannelMessages = false;
    }
});
api.setConnectionLostHandler(() => {
    appContainer.style.display = "none";
    loadingScreen.querySelector("#loading-text").innerHTML = "Connection lost. Reconnecting...";
    loadingScreen.style.display = "flex";
    connectionAttempts = 1;
    api.connect();
});

api.setTimeoutHandler(() => {
    if (connectionAttempts === 5) {
        loadingScreen.querySelector("#loading-text").innerHTML = "Failed to connect. Please try again later.";
        return;
    }
    loadingScreen.querySelector("#loading-text").innerHTML = "Error connecting. Retrying...";
    setTimeout(() => {
        connectionAttempts++;
        api.connect();
    }, 5000);
});

connectionAttempts++;
api.connect();

emojis.setLoadHandler(() => {
    let emojiList = Object.entries(emojis.emojis);
    let emojiHTML = "";
    for (let i = 0; i < emojiList.length; i++) {
        emojiHTML += `<li id="emoji-list-${i}"><button>${emojiList[i][1]}</button></li>`;
    }
    emojiListList.innerHTML = emojiHTML;
    for (let i = 0; i < emojiList.length; i++) {
        document.getElementById("emoji-list-" + i).addEventListener("click", () => {
            messageInput.value += emojiList[i][1];
        });
    }
});
emojis.loadEmojis();

function sendMessage() {
    if (messageInput.value.length === 0) {
        return;
    }
    if (currentChannelID) {
        api.sendMessage(emojis.toPlainText(messageInput.value), currentChannelID);
    }
    messageInput.value = "";
    messageContainer.scrollTop = messageContainer.scrollHeight;
}

function openAddFriendMenu() {
    addFriendURL.innerHTML = window.location.origin + "/friend/" + userID + "<button id=\"copy-url-button\" class=\"copy-button\">Copy</button>";
    addFriendMenu.showModal();
    addFriendCopyURL = document.getElementById("copy-url-button");
    addFriendCopyURL.addEventListener("click", () => {
        navigator.clipboard.writeText(addFriendURL.innerText.substring(0, addFriendURL.innerText.length - 4)).catch(() => {
            console.error("Failed to copy URL to clipboard");
        });
    });
}

function closeAddFriendMenu() {
    addFriendMenu.close();
}

function openAcceptFriendMenu(id) {
    if (userID === id) {
        cookies.deleteCookie("add-friend");
        return;
    }
    openingAcceptFriendMenu = true;
    acceptFriendUserID = id;
    api.requestUsername(id);
}

function closeAcceptFriendMenu() {
    acceptFriendMenu.close();
}

function acceptFriend() {
    api.acceptFriendRequest(atob(acceptFriendUserID));
    closeAcceptFriendMenu();
    cookies.deleteCookie("add-friend");
}

function declineFriend() {
    closeAcceptFriendMenu();
    cookies.deleteCookie("add-friend");
}

function changeChannel(channelID) {
    if (channelID === currentChannelID) {
        return;
    }
    if (!currentChannelID) {
        friendListContainer.style.display = "none";
        channelContainer.style.display = "flex";
    }
    currentChannelID = channelID;
    messageContainer.replaceChildren();
    api.requestChannelInfo(channelID);
}

function openAddChannelMenu() {
    addChannelFriends.replaceChildren();
    for (let friend of friends) {
        let friendElement = document.importNode(addChannelFriendTemplate.content, true);
        friendElement.querySelector(".add-channel-friend-name").textContent = friend.username;
        friendElement.querySelector(".add-channel-friend-button").addEventListener("click", (e) => {
            e.target.textContent = e.target.textContent === "Add" ? "Remove" : "Add";
        })
        addChannelFriends.appendChild(friendElement);
    }
    addChannelMenu.showModal();
}

function closeAddChannelMenu() {
    addChannelMenu.close();
}

function addChannelFromMenu() {
    let name = addChannelName.value;
    if (name === "") {
        addChannelName.placeholder = "Name is required";
        return;
    }
    let selectedFriends = [];
    selectedFriends.push(atob(userID));
    for (let friend of addChannelFriends.children) {
        let selected = friend.querySelector(".add-channel-friend-button").textContent === "Remove";
        if (selected) {
            let username = friend.querySelector(".add-channel-friend-name").textContent;
            for (let f of friends) {
                if (f.username === username) {
                    selectedFriends.push(f.userID);
                }
            }
        }
    }
    api.createChannel(name, selectedFriends);
    addChannelMenu.close();
}

function openFriendsList() {
    if (!currentChannelID) {
        return;
    }
    currentChannelID = undefined;
    channelContainer.style.display = "none";
    channelName.textContent = "Friends";
    channelSettingsContainer.style.display = "none";
    friendListContainer.style.display = "block";
}

function processMessageContents(message) {
    message = message.replaceAll("&", "&amp;");
    message = message.replaceAll("<", "&lt;");
    message = message.replaceAll(">", "&gt;");
    message = message.replace(/`(.*)`/g, "<code class=\"message-code\">$1</code>");
    message = message.replace(/\*\*(.*)\*\*/g, "<strong>$1</strong>");
    message = message.replace(/__(.*)__/g, "<strong>$1</strong>");
    message = message.replace(/\*(.*)\*/g, "<em>$1</em>");
    message = message.replace(/_(.*)_/g, "<em>$1</em>");
    message = message.replace(/~~(.*)~~/g, "<s>$1</s>");
    message = message.replace(/(?<!\()(https?:\/\/.*)/g, "<a class=\"message-link\" href=\"$1\">$1</a>");
    message = message.replace(/\[(.*)]\((.*)\)/g, "<a class=\"message-link\" href=\"$2\">$1</a>");
    message = message.replace(/^###### (.*)/gm, "<h6 class=\"message-heading\">$1</h6>");
    message = message.replace(/^##### (.*)/gm, "<h5 class=\"message-heading\">$1</h5>");
    message = message.replace(/^#### (.*)/gm, "<h4 class=\"message-heading\">$1</h4>");
    message = message.replace(/^### (.*)/gm, "<h3 class=\"message-heading\">$1</h3>");
    message = message.replace(/^## (.*)/gm, "<h2 class=\"message-heading\">$1</h2>");
    message = message.replace(/^# (.*)/gm, "<h1 class=\"message-heading\">$1</h1>");
    message = message.replace(/^-# (.*)/gm, "<span class=\"message-mini-text\">$1</span>");
    return message;
}

function insertMessage(index, from, fromID, content) {
    let shouldScroll = messageContainer.scrollTop + messageContainer.clientHeight >= messageContainer.scrollHeight - 2;
    let previousMessageElement;
    let previousIndex;
    let nextMessageElement;
    for (let i = 0; i < messageContainer.children.length; i++) {
        let child = messageContainer.children[i];
        let childIndex = parseInt(child.dataset.index);
        if (childIndex === index) {
            return;
        }
        if (childIndex > index - 1) {
            if (child.dataset.username === from) {
                nextMessageElement = child;
            }
            break;
        }
        if (child.dataset.username === from) {
            previousMessageElement = child;
        }
        else {
            previousMessageElement = null;
        }
        previousIndex = i;
    }
    if (previousMessageElement) {
        let messageElement = document.importNode(continuationMessageTemplate.content, true);
        let contentElement = messageElement.querySelector(".message-content");
        contentElement.innerHTML = processMessageContents(content);
        messageElement.firstElementChild.dataset.username = from;
        messageElement.firstElementChild.dataset.index = index;
        messageContainer.insertBefore(messageElement, messageContainer.children[previousIndex + 1]);
    }
    else {
        let messageElement = document.importNode(messageTemplate.content, true);
        let usernameElement = messageElement.querySelector(".message-username");
        let contentElement = messageElement.querySelector(".message-content");
        usernameElement.textContent = from;
        contentElement.innerHTML = processMessageContents(content);
        messageElement.firstElementChild.dataset.username = from;
        messageElement.firstElementChild.dataset.index = index;
        if (previousIndex !== undefined) {
            messageContainer.insertBefore(messageElement, messageContainer.children[previousIndex + 1]);
        }
        else {
            messageContainer.prepend(messageElement);
        }
    }
    if (nextMessageElement) {
        let modifiedNextElement = document.importNode(continuationMessageTemplate.content, true);
        let contentElement = modifiedNextElement.querySelector(".message-content");
        contentElement.innerHTML = nextMessageElement.querySelector(".message-content").innerHTML;
        modifiedNextElement.firstElementChild.dataset.username = nextMessageElement.dataset.username;
        modifiedNextElement.firstElementChild.dataset.index = nextMessageElement.dataset.index;
        messageContainer.replaceChild(modifiedNextElement, nextMessageElement);
    }
    if (shouldScroll) {
        messageContainer.scrollTop = messageContainer.scrollHeight;
    }
    setupMessageScrollObserver();
}

function setupMessageScrollObserver() {
    if (messageScrollObserver) {
        messageScrollObserver.disconnect();
    }
    messageScrollObserver = new IntersectionObserver((entries, _) => {
        for (let entry of entries) {
            if (!entry.isIntersecting) {
                continue;
            }
            let currentFirstMessage = parseInt(entry.target.dataset.index);
            api.requestChannelMessages(currentChannelID, currentFirstMessage <= 50 ? 0 : currentFirstMessage - 50);
        }
    }, {
        threshold: 1
    });
    messageScrollObserver.observe(messageContainer.firstElementChild);
}

function toggleUserList() {
    userListContainer.style.display = userListContainer.style.display === "none" ? "flex" : "none";
}

function documentClickHandler(event) {
    if (!emojiList.contains(event.target)) {
        emojiList.open = false;
    }
}

messageInput.addEventListener("keydown", (event) => {
    if (event.key === "Enter" && !event.shiftKey) {
        event.preventDefault();
        sendMessage();
    }
});

sendButton.addEventListener("click", sendMessage);

addFriendClose.addEventListener("click", closeAddFriendMenu);

acceptFriendClose.addEventListener("click", closeAcceptFriendMenu);
acceptFriendAccept.addEventListener("click", acceptFriend);
acceptFriendDecline.addEventListener("click", declineFriend);

addChannelClose.addEventListener("click", closeAddChannelMenu);
addChannelAdd.addEventListener("click", addChannelFromMenu);

openFriendsListButton.addEventListener("click", openFriendsList);

channelUserListButton.addEventListener("click", toggleUserList);

document.addEventListener("click", documentClickHandler);