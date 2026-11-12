import * as api from "./api.js";
import * as cookies from "./cookies.js";

let appContainer = document.getElementById("app-container");
let loadingScreen = document.getElementById("loading-screen");

let channelContainer = document.getElementById("channel-container");
const messageInput = document.getElementById("message-input");
const messageTemplate = document.getElementById("message-template");
const messageContainer = document.getElementById("message-container");

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

let userID;
let currentChannelID;
let friends = [];

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
api.setAuthHandler(() => {
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
    const messageElement = document.importNode(messageTemplate.content, true);
    const usernameElement = messageElement.querySelector(".message-username");
    const contentElement = messageElement.querySelector(".message-content");
    usernameElement.textContent = message.username;
    contentElement.textContent = message.content;
    messageContainer.appendChild(messageElement);
});
api.setChannelContentsHandler((channelContents) => {
    if (channelContents.channelID !== currentChannelID) {
        return;
    }
    channelName.textContent = channelContents.name;
    messageContainer.replaceChildren();
    for (let message of channelContents.messages) {
        const messageElement = document.importNode(messageTemplate.content, true);
        const usernameElement = messageElement.querySelector(".message-username");
        const contentElement = messageElement.querySelector(".message-content");
        usernameElement.textContent = message.username;
        contentElement.textContent = message.content;
        messageContainer.appendChild(messageElement);
    }
});

api.connect();

function sendMessage() {
    if (messageInput.value.length === 0) {
        return;
    }
    if (currentChannelID) {
        api.sendMessage(messageInput.value, currentChannelID);
    }
    messageInput.value = "";
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
    api.requestChannelContents(channelID);
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
    friendListContainer.style.display = "block";
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