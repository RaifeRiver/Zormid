import * as api from "./api.js";
import * as cookies from "./cookies.js";

const messageInput = document.getElementById("message-input");
const messageTemplate = document.getElementById("message-template");
const messageContainer = document.getElementById("message-container");

let sendButton = document.getElementById("send-button");

let addFriendButton = document.getElementById("add-friend");
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

let friendList = document.getElementById("friend-list");
let friendTemplate = document.getElementById("friend-template");

let userID;
let currentFriendID;

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
    api.requestFriendList(atob(userID));
});
api.setGlobalMessageHandler((message) => {
    if (currentFriendID) {
        return;
    }
    const messageElement = document.importNode(messageTemplate.content, true);
    const usernameElement = messageElement.querySelector(".message-username");
    const contentElement = messageElement.querySelector(".message-content");
    usernameElement.textContent = message.username;
    contentElement.textContent = message.content;
    messageContainer.appendChild(messageElement);
});
api.setUsernameRequestHandler((username, userID) => {
    if (openingAcceptFriendMenu && btoa(userID) === acceptFriendUserID) {
        acceptFriendText.innerHTML = "Accept friend request from " + username + "?";
        acceptFriendMenu.showModal();
    }
});
api.setFriendListRequestHandler((friends, id) => {
    if (btoa(id) !== userID) {
        return;
    }
    friendList.replaceChildren();
    for (let friend of friends) {
        let friendElement = document.importNode(friendTemplate.content, true);
        let friendButton = friendElement.firstElementChild;
        friendButton.textContent = friend.username;
        friendButton.addEventListener("click", () => changeFriend(friend.userID));
        friendList.appendChild(friendElement);
    }
});
api.setMessageHandler((message) => {
    if (message.userID !== currentFriendID && message.userID !== atob(userID)) {
        return;
    }
    const messageElement = document.importNode(messageTemplate.content, true);
    const usernameElement = messageElement.querySelector(".message-username");
    const contentElement = messageElement.querySelector(".message-content");
    usernameElement.textContent = message.username;
    contentElement.textContent = message.content;
    messageContainer.appendChild(messageElement);
});

api.connect();

function sendMessage() {
    if (messageInput.value.length === 0) {
        return;
    }
    if (currentFriendID) {
        api.sendMessage(messageInput.value, currentFriendID);
    }
    else {
        api.sendGlobalMessage(messageInput.value);
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

function changeFriend(friendID) {
    if (friendID === currentFriendID) {
        return;
    }
    currentFriendID = friendID;
    messageContainer.replaceChildren();
}

messageInput.addEventListener("keydown", (event) => {
    if (event.key === "Enter" && !event.shiftKey) {
        event.preventDefault();
        sendMessage();
    }
});

sendButton.addEventListener("click", sendMessage);

addFriendButton.addEventListener("click", openAddFriendMenu);
addFriendClose.addEventListener("click", closeAddFriendMenu);

acceptFriendClose.addEventListener("click", closeAcceptFriendMenu);
acceptFriendAccept.addEventListener("click", acceptFriend);
acceptFriendDecline.addEventListener("click", declineFriend);